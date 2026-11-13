#include "robot/EgmFlowController.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QMessageBox>
#include <QRandomGenerator>
#include <QStringList>
#include <QThread>

#include <algorithm>
#include <cmath>

#include "services/robot/RobotPrintInterfaces.h"
#include "ui/PressureChartWindow.h"

namespace {

// Path txt default folder: prefer src/path inside the source tree.
// The executable normally runs from build/ (or build/Debug, build/Release),
// so walk up from the executable directory until src/path is found. A deployed
// layout may also place src/path beside the executable. Falls back to the
// executable directory when nothing matches.
QString defaultPathTxtDir() {
    const QString appDir = QCoreApplication::applicationDirPath();
    const QStringList candidates = {
        appDir + QStringLiteral("/src/path"),
        appDir + QStringLiteral("/../src/path"),
        appDir + QStringLiteral("/../../src/path"),
        appDir + QStringLiteral("/../../../src/path"),
    };
    for (const QString& candidate : candidates) {
        if (QDir(candidate).exists()) {
            return QDir(candidate).absolutePath();
        }
    }
    return appDir;
}

// data_save 目录：项目根下、与 src 文件夹同级。exe 通常在 build/ 下运行，
// 因此从可执行文件目录逐级向上找到含 src 的目录作为项目根，再在其下建 data_save。
// 部署环境（没有 src）时退回可执行文件同级的 data_save。
QString dataSaveDir() {
    const QString appDir = QCoreApplication::applicationDirPath();
    const QStringList candidateRoots = {
        appDir,
        appDir + QStringLiteral("/.."),
        appDir + QStringLiteral("/../.."),
        appDir + QStringLiteral("/../../.."),
    };
    for (const QString& candidate : candidateRoots) {
        const QString rootPath = QDir(candidate).absolutePath();
        if (!QDir(rootPath + QStringLiteral("/src")).exists()) {
            continue;
        }
        const QString target = rootPath + QStringLiteral("/data_save");
        QDir().mkpath(target);
        return target;
    }
    const QString fallback = QDir(appDir).absoluteFilePath(QStringLiteral("data_save"));
    QDir().mkpath(fallback);
    return fallback;
}

// 位姿读数诊断：把每次启动 EGM 时 CASE 113 的原始应答与本地缓存的反馈位姿
// 追加到 data_save/pose_read.log。用于复盘「上一轮 EGM 结束后 113 返回错误
// 姿态」这类问题（正常时两者应只差几毫米、几十分之一度）。
void appendPoseReadLog(const QString& line) {
    QFile file(QDir(dataSaveDir()).absoluteFilePath(QStringLiteral("pose_read.log")));
    if (!file.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
        return;
    }
    QTextStream stream(&file);
    stream << QDateTime::currentDateTime().toString(Qt::ISODate) << ' ' << line << '\n';
}

// Quaternion helpers (ABB order: q1 = scalar / w). Used only to align the
// path's attitude onto the robot's real attitude at session start.
PathQuaternion QuatNormalize(const PathQuaternion& q) {
    const float len = std::sqrt(q.q1 * q.q1 + q.q2 * q.q2 + q.q3 * q.q3 + q.q4 * q.q4);
    if (len < 1e-6f) return PathQuaternion{1, 0, 0, 0};
    return PathQuaternion{q.q1 / len, q.q2 / len, q.q3 / len, q.q4 / len};
}

PathQuaternion QuatMultiply(const PathQuaternion& a, const PathQuaternion& b) {
    return PathQuaternion{
        a.q1 * b.q1 - a.q2 * b.q2 - a.q3 * b.q3 - a.q4 * b.q4,
        a.q1 * b.q2 + a.q2 * b.q1 + a.q3 * b.q4 - a.q4 * b.q3,
        a.q1 * b.q3 - a.q2 * b.q4 + a.q3 * b.q1 + a.q4 * b.q2,
        a.q1 * b.q4 + a.q2 * b.q3 - a.q3 * b.q2 + a.q4 * b.q1};
}

PathQuaternion QuatConjugate(const PathQuaternion& q) {
    return PathQuaternion{q.q1, -q.q2, -q.q3, -q.q4};
}

// 向 SERVER.mod 发一条无参数指令，并解析应答第三个 token 的整数值
// （CASE 112 = EGM 会话状态，CASE 114 = 上一次 EGM 会话的错误码）。
// 返回 -1 = 无应答；-2 = 应答了但状态位不是 OK（多半是模块还没重载，
// 不认识这条查询）；>=0 = 查询得到的值。超时故意设短，避免把界面卡住。
int queryRobotInt(IRobotCommandSocket* socket, const QString& frame, int timeoutMs) {
    if (socket == nullptr) {
        return -1;
    }
    QString reply;
    if (!socket->sendRawMessageWithReply(frame, reply, timeoutMs).ok) {
        return -1;
    }
    const QStringList tokens = reply.split(' ', Qt::SkipEmptyParts);
    if (tokens.size() >= 2 && tokens.at(1).toInt() == 0) {
        return -2;
    }
    if (tokens.size() < 3) {
        return -1;
    }
    bool ok = false;
    const int value = tokens.at(2).toInt(&ok);
    return ok ? value : -1;
}

// 把 queryRobotInt 的返回码转成给用户看的文字。
QString describeQueryResult(int value) {
    if (value == -1) {
        return QString::fromUtf8(u8"无应答");
    }
    if (value == -2) {
        return QString::fromUtf8(u8"命令未实现（模块可能未重载）");
    }
    return QString::number(value);
}

}  // namespace

EgmFlowController::EgmFlowController(const AppConfig& config,
                                     IRobotCommandSocket* commandSocket,
                                     QWidget* dialogParent,
                                     QObject* parent)
    : QObject(parent),
      config_(config),
      commandSocket_(commandSocket),
      dialogParent_(dialogParent),
      egmClient_(config_.egm, this),
      pathLoader_(this),
      tickTimer_(this),
      startWatchdog_(this) {
    startWatchdog_.setSingleShot(true);

    connect(&tickTimer_, &QTimer::timeout, this, &EgmFlowController::tick);
    connect(&egmClient_, &EgmClient::packetCounterTick,
            this, [this](quint64 packetsSent) {
                emit statusToolTipChanged(QString("EGM: %1 packets sent").arg(packetsSent));
            });
    connect(&egmClient_, &EgmClient::feedbackPoseReceived,
            this, &EgmFlowController::onFeedbackPose);

    // 模拟压力源（10ms tick）→ EGM 压力闭环控制器的测量值注入。
    // PressureSimulator 由 startCsvRecording 启动、stopCsvRecording 停止，
    // 正好与 EGM 会话生命周期对齐。将来换成真实压力传感器时，把这条 connect
    // 换成传感器的 dataReady 信号即可。
    connect(&pressureSim_, &PressureSimulator::ticked,
            &egmClient_, &EgmClient::setPressureMeasured);

    // ---- 实时曲线窗口数据链路 ----
    // 全部带空指针保护：没有 EGM 会话时 chartWindow_ 为空。
    connect(&pressureSim_, &PressureSimulator::ticked,
            this, [this](double pMeas) {
                if (chartWindow_) {
                    // 目标/实测/误差同源同频，误差曲线不再混入 0 值
                    chartWindow_->appendPressureSample(
                        chartPressureTargetN_, pMeas,
                        chartPressureTargetN_ - pMeas);
                }
            });
    connect(&egmClient_, &EgmClient::feedbackPoseReceived,
            this, [this](const EgmTcpPose& pose) {
                if (chartWindow_) {
                    chartWindow_->appendZSample(chartTargetZmm_, pose.z);
                }
            });

    // 启动看门狗：EGM 启动后若一直收不到控制器反馈帧，说明会话根本没跑起来
    // （T_ROB1 停机 / EGM 状态未复位 / 6510 被占）。超时后自动停流并给出诊断，
    // 否则表现为「点了开始、机器人不动、也没有任何报错」。
    connect(&startWatchdog_, &QTimer::timeout,
            this, &EgmFlowController::onStartWatchdogTimeout);
}

EgmFlowController::~EgmFlowController() {
    stopCsvRecording();
    // 曲线窗口是独立顶级窗口，不归 MainWindow 的 ui 管理，这里显式释放。
    delete chartWindow_;
    chartWindow_ = nullptr;
}

void EgmFlowController::pause() {
    if (active_) {
        tickTimer_.stop();
    }
}

void EgmFlowController::resume() {
    if (active_) {
        tickTimer_.start(config_.egm.periodMs);
    }
}

void EgmFlowController::loadPathFromDialog() {
    if (!config_.egm.enabled) {
        QMessageBox::information(dialogParent_, QString::fromUtf8(u8"路径加载"),
            QString::fromUtf8(u8"EGM 未启用，当前只是预加载路径。安装 689-1 并启用 egm.enabled 后点 \"开始打印\" 生效。"));
    }

    const QString defaultDir = defaultPathTxtDir();

    const QString filePath = QFileDialog::getOpenFileName(
        dialogParent_,
        QString::fromUtf8(u8"选择 7-10 列 txt 路径文件"),
        defaultDir,
        QStringLiteral("Path files (*.txt)"));
    if (filePath.isEmpty()) return;

    if (!pathLoader_.loadFromTxt(filePath)) {
        QMessageBox::critical(dialogParent_, QString::fromUtf8(u8"路径加载失败"),
            QString::fromUtf8(u8"%1 解析失败。笛卡尔文件需要 ≥7 列 space-separated；\n"
                              u8"关节文件需要一行 \"# MODE=JOINT\" 注释 + 7 列 j1..j6(deg) v(deg/s)。").arg(filePath));
        return;
    }
    const bool jointPath = pathLoader_.isJointPath();
    const int segCount = jointPath ? pathLoader_.jointSegmentCount() : pathLoader_.segmentCount();
    if (segCount <= 0) {
        QMessageBox::critical(dialogParent_, QString::fromUtf8(u8"路径文件为空"),
            QString::fromUtf8(u8"文件里没有有效行（至少 2 行）。"));
        return;
    }

    loadedPathFile_ = filePath;

    if (jointPath) {
        // 关节模式：v 列是 deg/s
        const float firstV = pathLoader_.jointPoints().isEmpty()
            ? 0.0f : pathLoader_.jointPoints().first().v;
        if (firstV > 0.0f) {
            jointSpeedDegPerSec_ = static_cast<double>(firstV);
        }
        emit statusTextChanged(
            QString::fromUtf8(u8"关节路径已加载：%1  [%2 段, 首段速度 %3 °/s, 关节模式]")
                .arg(QFileInfo(filePath).fileName())
                .arg(segCount)
                .arg(jointSpeedDegPerSec_, 0, 'f', 1));
        return;
    }

    // Use the v column of the first raw point as a hint for traverse speed
    // (legacy Python convention: column 7 is mm/s). Fall back to the default
    // we read from env_config.json / hard-coded otherwise.
    const float firstV = pathLoader_.rawPoints().isEmpty()
        ? 0.0f : pathLoader_.rawPoints().first().v;
    if (firstV > 1.0f) {
        traverseSpeedMmPerSec_ = static_cast<double>(firstV);
    }

    emit statusTextChanged(
        QString::fromUtf8(u8"路径已加载：%1  [%2 段, 首段速度 %3 mm/s]")
            .arg(QFileInfo(filePath).fileName())
            .arg(pathLoader_.segmentCount())
            .arg(traverseSpeedMmPerSec_, 0, 'f', 1));
}

void EgmFlowController::startSession() {
    if (!config_.egm.enabled) {
        QMessageBox::warning(dialogParent_, QString::fromUtf8(u8"EGM 不可用"),
            QString::fromUtf8(u8"EGM 未启用。RobotStudio 需先安装 689-1 External Guided Motion，\n"
                               u8"然后在 env_config.json 里把 egm.enabled 改为 true。"));
        return;
    }

    const bool jointPath = pathLoader_.isJointPath();
    const int segCount = jointPath ? pathLoader_.jointSegmentCount() : pathLoader_.segmentCount();
    if (loadedPathFile_.isEmpty() || segCount <= 0) {
        QMessageBox::information(dialogParent_, QString::fromUtf8(u8"请先加载路径"),
            QString::fromUtf8(u8"先点 \"获取路径\" 选一个 .txt，再点 \"开始打印\" 启动 EGM 流。"));
        return;
    }

    if (commandSocket_ == nullptr || !commandSocket_->isConnected()) {
        QMessageBox::information(dialogParent_, QString::fromUtf8(u8"请先连接机器人"),
            QString::fromUtf8(u8"EGM 使用与打印相同的 socket (5000) 发送 CASE 110 启动信号。\n"
                               u8"请先点 \"连接机器人\" 让 SERVER.mod 接收命令。"));
        return;
    }

    // Reset iterator state
    segmentIndex_ = 0;
    segmentProgressMm_ = 0.0f;
    jointProgressDeg_ = 0.0f;
    active_ = false;

    // 启动前先确认 SERVER.mod 真的在应答。T_ROB1 停机/报警后 TCP 仍然"连着"，
    // 写入的指令会静默丢失 → 表现为「点了开始、机器人不动、也没有报错」。
    // 顺带把缓冲区里残留的 ACK 读掉，避免后续查询拿到过期应答。
    {
        bool rapidAlive = false;
        for (int attempt = 0; attempt < 3 && !rapidAlive; ++attempt) {
            QString reply;
            if (!commandSocket_
                     ->sendRawMessageWithReply(QStringLiteral("0 #"), reply, 800)
                     .ok) {
                continue;
            }
            const QStringList tokens = reply.split(' ', Qt::SkipEmptyParts);
            if (tokens.size() >= 2 && tokens.at(0).toInt() == 0 && tokens.at(1).toInt() == 1) {
                rapidAlive = true;
            }
        }
        if (!rapidAlive) {
            QMessageBox::critical(dialogParent_, QString::fromUtf8(u8"SERVER.mod 无响应"),
                QString::fromUtf8(u8"TCP 连接还在，但 RAPID 侧的 SERVER.mod 没有应答心跳"
                                   u8"（CASE 0）。\n\n"
                                   u8"这通常表示 T_ROB1 已经停止（报警未清除或程序指针不在 "
                                   u8"main），此时发出去的 EGM 指令会被静默丢弃，机器人不会"
                                   u8"运动。\n\n"
                                   u8"请到示教器：清除报警 → PP to main → 启动 T_ROB1，再重试。"));
            emit statusTextChanged(
                QString::fromUtf8(u8"EGM 未启动：SERVER.mod 无响应（T_ROB1 可能已停止）。"));
            return;
        }
    }

    // 关节模式下 EGM 发送的是绝对关节角（单位：度），首帧必须等于机器人当前
    // 关节角，否则 EGMActJoint 的限幅会立刻判定超差、会话秒退。
    // 注意：txt 与 EGM 关节报文都用度，这里不做角度/弧度换算。
    QString poseLiveNote;
    QString poseOffsetNote;
    if (jointPath) {
        float startJoints[6] = {0, 0, 0, 0, 0, 0};
        pathLoader_.jointPositionAt(0, 0.0f, startJoints);
        double jointsDeg[6];
        for (int axis = 0; axis < 6; ++axis) {
            jointsDeg[axis] = static_cast<double>(startJoints[axis]);
        }
        egmClient_.setMode(EgmClient::Mode::Joint);
        egmClient_.setTargetJoints(jointsDeg);
        startContext_ = QStringLiteral("file=%1 mode=joint")
                            .arg(QFileInfo(loadedPathFile_).fileName());
    } else {
        egmClient_.setMode(EgmClient::Mode::Pose);

        const PathVector3 pathStart = pathLoader_.positionAt(0, 0.0f);
        const PathQuaternion pathStartQuat = pathLoader_.orientationAt(0, 0.0f);

        // 先读机器人真实位姿（SERVER.mod CASE 113，currentTool + currentWobj）。
        // 路径里的姿态列可能已经过期（例如跑过 j6 关节测试后姿态变了 5°），
        // 直接下发会让腕部为了"进入路径姿态"而转动。这里把整条路径的姿态
        // 旋转对齐到当前姿态：
        //     q_cmd(t) = q_live ⊗ q_pathStart⁻¹ ⊗ q_path(t)
        // t=0 时 q_cmd 就是机器人当前姿态，入场零转动；路径内部的相对姿态
        // 变化（若有）仍然保留。
        alignValid_ = false;
        alignQuat_ = PathQuaternion{1, 0, 0, 0};
        pathOffsetMm_ = PathVector3{};

        // 当前真实位姿有两个来源：
        //   ① 本轮已经跑过 EGM —— 控制器反馈的真实 TCP 位姿（本地缓存）；
        //   ② SERVER.mod CASE 113 查询。
        // 实测：上一轮 EGM 会话结束后，113 会返回「位置相近、姿态偏 30°~40°」的
        // 错误读数；而查询本身还要等 TCP 超时（默认 3 s），连点几次就会把界面
        // 卡住十几秒。因此有缓存时直接用缓存，完全不查 113。
        QString liveReply;
        double liveX = 0.0;
        double liveY = 0.0;
        double liveZ = 0.0;
        PathQuaternion liveQuat{1, 0, 0, 0};
        bool haveLive = false;
        QString liveSource;

        if (hasLastFeedbackPose_) {
            liveX = lastFeedbackPose_.x;
            liveY = lastFeedbackPose_.y;
            liveZ = lastFeedbackPose_.z;
            liveQuat = PathQuaternion{static_cast<float>(lastFeedbackPose_.q1),
                                      static_cast<float>(lastFeedbackPose_.q2),
                                      static_cast<float>(lastFeedbackPose_.q3),
                                      static_cast<float>(lastFeedbackPose_.q4)};
            haveLive = true;
            liveSource = QString::fromUtf8(u8"EGM 反馈缓存");
        } else {
            // 仅软件刚启动、还没有任何反馈帧时需要查询。最多 2 次、每次 800 ms，
            // 失败就放弃，绝不用路径文件里的旧姿态顶替。
            emit statusTextChanged(QString::fromUtf8(u8"正在读取机器人当前位姿…"));
            QCoreApplication::processEvents();
            for (int attempt = 0; attempt < 2 && !haveLive; ++attempt) {
                if (attempt > 0) {
                    QThread::msleep(300);
                }
                QString reply;
                if (!commandSocket_
                         ->sendRawMessageWithReply(QStringLiteral("113 #"), reply, 800)
                         .ok) {
                    continue;
                }
                const QStringList tokens = reply.split(' ', Qt::SkipEmptyParts);
                if (tokens.size() < 9) {
                    continue;  // 残留 ACK 之类的短应答，不是位姿
                }
                liveX = tokens.at(2).toDouble();
                liveY = tokens.at(3).toDouble();
                liveZ = tokens.at(4).toDouble();
                liveQuat = PathQuaternion{tokens.at(5).toFloat(), tokens.at(6).toFloat(),
                                          tokens.at(7).toFloat(), tokens.at(8).toFloat()};
                haveLive = true;
                liveSource = QStringLiteral("CASE113");
                liveReply = reply;
            }
        }

        appendPoseReadLog(QStringLiteral("CASE113 '%1' | cache '%2'")
                              .arg(liveReply.isEmpty() ? QString::fromUtf8(u8"<未查询>") : liveReply,
                                   hasLastFeedbackPose_
                                       ? QStringLiteral("%1 %2 %3 %4 %5 %6 %7")
                                             .arg(lastFeedbackPose_.x, 0, 'f', 2)
                                             .arg(lastFeedbackPose_.y, 0, 'f', 2)
                                             .arg(lastFeedbackPose_.z, 0, 'f', 2)
                                             .arg(lastFeedbackPose_.q1, 0, 'f', 4)
                                             .arg(lastFeedbackPose_.q2, 0, 'f', 4)
                                             .arg(lastFeedbackPose_.q3, 0, 'f', 4)
                                             .arg(lastFeedbackPose_.q4, 0, 'f', 4)
                                       : QStringLiteral("<none>")));

        // 位姿来源全都不可用就绝不启动：拿 txt 里的旧姿态或错误读数开流，会让
        // 机器人冲到一个完全错误的位置/姿态，撞关节限位（50027）。
        if (!haveLive) {
            QMessageBox::critical(dialogParent_, QString::fromUtf8(u8"无法读取机器人当前位姿"),
                QString::fromUtf8(u8"SERVER.mod 未返回可信的 113 位姿读数（已重试 2 次），"
                                   u8"本地也没有上一轮 EGM 的反馈缓存。\n\n"
                                   u8"常见原因：T_ROB1 已停机、报警未清除，或程序指针不在 main。\n\n"
                                   u8"为避免在姿态未确认的情况下启动 EGM，本次不执行。"));
            return;
        }

        {
            const double dx = liveX - static_cast<double>(pathStart.x);
            const double dy = liveY - static_cast<double>(pathStart.y);
            const double dz = liveZ - static_cast<double>(pathStart.z);
            const double dist = std::sqrt(dx * dx + dy * dy + dz * dz);

            // 偏差超过 EGM 的 ±500 mm 补偿范围：这不是"起点微调"，多半是
            // 路径文件坐标系不对、或位姿读数异常。贸然补偿会让机器人猛冲，
            // 直接拒绝。
            if (dist > 500.0) {
                QMessageBox::critical(dialogParent_,
                    QString::fromUtf8(u8"路径与机器人位置相差过大"),
                    QString::fromUtf8(u8"当前 TCP 与路径首点相差 %1 mm，超出 EGM 允许的 "
                                      u8"±500 mm 补偿范围，本次不执行。\n\n"
                                      u8"请依次确认：\n"
                                      u8"① 选的路径文件与此机器人坐标系（currentTool / "
                                      u8"currentWobj）匹配；\n"
                                      u8"② 机器人确实停在路径起点附近；\n"
                                      u8"③ 示教器上 T_ROB1 正在运行、无未清除报警。")
                        .arg(dist, 0, 'f', 1));
                return;
            }

            // 起始位姿与 txt 首点不一致时，征求确认后再把整条路径整体平移
            // 到机器人当前位置，避免 EGM 一启动就强制机器人“跳”到路径首点。
            if (dist > 0.5) {
                QString questionText = QString::fromUtf8(
                    u8"机器人当前 TCP 与 txt 路径首点不一致：\n\n"
                    u8"路径首点：X %1   Y %2   Z %3\n"
                    u8"当前 TCP：X %4   Y %5   Z %6\n"
                    u8"偏移量：ΔX %7   ΔY %8   ΔZ %9\n"
                    u8"位置偏差：%10 mm\n"
                    u8"位姿来源：%11\n\n"
                    u8"是否将整条路径整体平移该偏移量，"
                    u8"使路径从机器人当前位置开始？")
                    .arg(pathStart.x, 0, 'f', 2)
                    .arg(pathStart.y, 0, 'f', 2)
                    .arg(pathStart.z, 0, 'f', 2)
                    .arg(liveX, 0, 'f', 2)
                    .arg(liveY, 0, 'f', 2)
                    .arg(liveZ, 0, 'f', 2)
                    .arg(dx, 0, 'f', 2)
                    .arg(dy, 0, 'f', 2)
                    .arg(dz, 0, 'f', 2)
                    .arg(dist, 0, 'f', 2)
                    .arg(liveSource);

                // 偏差超过 50 mm 时给出额外安全提示，并改用警告图标。
                const bool largeOffset = dist > 50.0;
                if (largeOffset) {
                    questionText += QString::fromUtf8(
                        u8"\n\n【注意】位置偏差已超过 50 mm。补偿后整条路径会被"
                        u8"平移到机器人当前位置，请先确认机器人周边空间、"
                        u8"工具姿态和工件位置均安全，再继续执行。");
                }

                QMessageBox confirmDialog(dialogParent_);
                confirmDialog.setWindowTitle(QString::fromUtf8(u8"路径起始位姿不一致"));
                confirmDialog.setIcon(largeOffset ? QMessageBox::Warning
                                                  : QMessageBox::Question);
                confirmDialog.setText(questionText);
                confirmDialog.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
                // 默认落在「No」：误按回车不会把机器人送出去。
                confirmDialog.setDefaultButton(QMessageBox::No);
                if (confirmDialog.exec() != QMessageBox::Yes) {
                    emit statusTextChanged(
                        QString::fromUtf8(u8"已取消：未对路径做偏移补偿，EGM 未启动。"));
                    return;
                }
                pathOffsetMm_ = PathVector3{static_cast<float>(dx),
                                            static_cast<float>(dy),
                                            static_cast<float>(dz)};
                poseOffsetNote = QString::fromUtf8(
                    u8"路径已整体偏移补偿：ΔX %1  ΔY %2  ΔZ %3 mm")
                    .arg(dx, 0, 'f', 2).arg(dy, 0, 'f', 2).arg(dz, 0, 'f', 2);
            }
        }

        // 姿态对齐：把整条路径的姿态旋转到机器人当前姿态。
        // 对齐角超过 10° 时说明路径文件与机器人姿态不匹配（或读数异常），此时
        // 强行对齐会让腕部大角度旋转、TCP 随长工具甩出去，宁可按文件姿态执行。
        startContext_ = QStringLiteral("file=%1 offset=(%2,%3,%4) source=%5")
                            .arg(QFileInfo(loadedPathFile_).fileName())
                            .arg(pathOffsetMm_.x, 0, 'f', 2)
                            .arg(pathOffsetMm_.y, 0, 'f', 2)
                            .arg(pathOffsetMm_.z, 0, 'f', 2)
                            .arg(liveSource);
        const PathQuaternion alignQuat =
            QuatNormalize(QuatMultiply(liveQuat, QuatConjugate(pathStartQuat)));
        const double alignHalfW = std::min(1.0, std::fabs(static_cast<double>(alignQuat.q1)));
        const double alignAngleDeg = 2.0 * std::acos(alignHalfW) * 180.0 / 3.141592653589793;
        if (alignAngleDeg > 10.0) {
            alignValid_ = false;
            alignQuat_ = PathQuaternion{1, 0, 0, 0};
            poseLiveNote = QString::fromUtf8(
                u8"机器人姿态与路径首点相差 %1°（>10°），已忽略姿态对齐，"
                u8"按路径文件姿态执行（来源 %2）")
                .arg(alignAngleDeg, 0, 'f', 1)
                .arg(liveSource);
            appendPoseReadLog(QStringLiteral("skip attitude align: %1 deg, source %2")
                                  .arg(alignAngleDeg, 0, 'f', 2)
                                  .arg(liveSource));
        } else {
            alignQuat_ = alignQuat;
            alignValid_ = true;
            poseLiveNote = QString::fromUtf8(u8"已按真实姿态对齐（%1）：[%2, %3, %4, %5]，偏差 %6°")
                               .arg(liveSource)
                               .arg(liveQuat.q1, 0, 'f', 4)
                               .arg(liveQuat.q2, 0, 'f', 4)
                               .arg(liveQuat.q3, 0, 'f', 4)
                               .arg(liveQuat.q4, 0, 'f', 4)
                               .arg(alignAngleDeg, 0, 'f', 2);
        }

        // 位姿模式首帧不能是 EgmClient 内部的 (0,0,0)+单位四元数，否则与当前
        // TCP 相差上千米，EGM 偏移立刻超限 → 会话终止并急停。
        const PathQuaternion startQuat = alignValid_
            ? QuatNormalize(QuatMultiply(alignQuat_, pathStartQuat))
            : pathStartQuat;
        egmClient_.setTargetPose(
            EgmTcpPose{pathStart.x + pathOffsetMm_.x,
                       pathStart.y + pathOffsetMm_.y,
                       pathStart.z + pathOffsetMm_.z,
                       startQuat.q1, startQuat.q2,
                       startQuat.q3, startQuat.q4});
    }

    // 翻转 T_ROB1 的 EGM 启动标志（SERVER.mod CASE 110 / 120）；ACK 返回后
    // RAPID 才真正进入阻塞的 EGM 会话。这里要求拿到应答后再开流，否则指令
    // 可能根本没被处理。
    const QString startCommand = jointPath ? QStringLiteral("120 #") : QStringLiteral("110 #");
    const int startCode = jointPath ? 120 : 110;
    bool startAckOk = false;
    QString startAck;
    for (int attempt = 0; attempt < 3 && !startAckOk; ++attempt) {
        QString reply;
        if (!commandSocket_
                 ->sendRawMessageWithReply(startCommand, reply, 1500)
                 .ok) {
            continue;  // 完全没应答，可以安全重发
        }
        // 收到应答后不再重发：RAPID 的 ACK 之后立刻就进了阻塞的 EGM 会话，
        // 会话期间再排一条 110 会在会话结束后被误当成新指令。
        startAck = reply;
        const QStringList tokens = reply.split(' ', Qt::SkipEmptyParts);
        if (tokens.size() >= 2 && tokens.at(0).toInt() == startCode && tokens.at(1).toInt() == 1) {
            startAckOk = true;
        }
        break;
    }
    if (!startAckOk) {
        QMessageBox::critical(dialogParent_, QString::fromUtf8(u8"发送 EGM 启动指令失败"),
            QString::fromUtf8(u8"SERVER.mod 没有确认 EGM 启动指令（%1 %2），"
                               u8"机器人不会运动。%3")
                .arg(startCode)
                .arg(jointPath ? QStringLiteral("120") : QStringLiteral("110"))
                .arg(startAck.isEmpty() ? QString::fromUtf8(u8"（无应答）") : startAck));
        return;
    }

    // EGM 会话开始前开一个新 CSV，记录控制器实时反馈的 x/y/z + 模拟压力。
    startCsvRecording();

    // ---- 导纳式压力闭环 ----
    // 仅 Pose 模式生效；enable 时内部自动 reset() PID 状态，避免上轮会话的 h_offset 串入。
    // 想临时关掉压力闭环，把这两行注释掉即可。
    egmClient_.setPressureTarget(80.0);        // 目标法向压力 N
    egmClient_.enablePressureControl(true);

    if (!egmClient_.start()) {
        QMessageBox::critical(dialogParent_, QString::fromUtf8(u8"EGM 客户端启动失败"),
            QString::fromUtf8(u8"本地 UDP 端口没有绑定成功（boundPort=%1）。\n\n"
                               u8"6510 可能被占用：上一次残留的上位机进程、Python 旧版程序、"
                               u8"或其它占用该端口的程序需要先关掉。")
                .arg(egmClient_.boundPort()));
        stopCsvRecording();
        QString ack;
        commandSocket_->sendRawMessageWithReply(QStringLiteral("111 #"), ack, 1000);
        return;
    }

    active_ = true;
    // 看门狗：真实环境 2.5s 足够收到反馈；仿真环境（无机器人）超时会触发
    // stop。这里调大到 15s 给仿真环境足够时间看曲线变化。
    startWatchdog_.start(15000);
    tickTimer_.start(config_.egm.periodMs);

    // ---- 实时曲线窗口 ----
    // 独立顶级窗口（nullptr parent），不受主窗口最小化影响。
    // EGM 会话结束后不 hide，曲线停在最后的状态供用户查看；
    // 下次 start 时 clearAll 重新开始。
    chartPressureTargetN_ = 80.0;
    if (!chartWindow_) {
        chartWindow_ = new PressureChartWindow(nullptr);   // 独立窗口，不传 parent
    }
    chartWindow_->clearAll();
    chartWindow_->show();

    if (jointPath) {
        emit statusTextChanged(
            QString::fromUtf8(u8"EGM 关节流启动：%1  [%2 段, %3 °/s, %4 ms 周期]")
                .arg(QFileInfo(loadedPathFile_).fileName())
                .arg(segCount)
                .arg(jointSpeedDegPerSec_, 0, 'f', 1)
                .arg(config_.egm.periodMs));
    } else {
        QString status = QString::fromUtf8(u8"EGM 流启动：%1  [%2 段, %3 mm/s, %4 ms 周期]")
                .arg(QFileInfo(loadedPathFile_).fileName())
                .arg(segCount)
                .arg(traverseSpeedMmPerSec_, 0, 'f', 1)
                .arg(config_.egm.periodMs);
        if (!poseLiveNote.isEmpty()) {
            status += QString::fromUtf8(u8"\n已按真实姿态对齐 —— %1").arg(poseLiveNote);
        }
        if (!poseOffsetNote.isEmpty()) {
            status += QString::fromUtf8(u8"\n%1").arg(poseOffsetNote);
        }
        emit statusTextChanged(status);
    }
}

void EgmFlowController::tick() {
    if (!active_) return;

    const double dtSec = config_.egm.periodMs / 1000.0;

    // ---- 关节模式：txt 与 EGM 报文都用「度」，不做角度/弧度换算 ----
    if (pathLoader_.isJointPath()) {
        jointProgressDeg_ += static_cast<float>(jointSpeedDegPerSec_ * dtSec);
        const auto& jointSegs = pathLoader_.jointSegments();

        while (segmentIndex_ < jointSegs.size() &&
               jointProgressDeg_ >= jointSegs[segmentIndex_].lengthDeg) {
            jointProgressDeg_ -= jointSegs[segmentIndex_].lengthDeg;
            ++segmentIndex_;
        }

        if (segmentIndex_ >= jointSegs.size()) {
            stop();
            emit statusTextChanged(
                QString::fromUtf8(u8"EGM 关节流结束：已走完 %1 段").arg(jointSegs.size()));
            return;
        }

        float jointsDeg[6] = {0, 0, 0, 0, 0, 0};
        pathLoader_.jointPositionAt(segmentIndex_, jointProgressDeg_, jointsDeg);
        double target[6];
        for (int axis = 0; axis < 6; ++axis) {
            target[axis] = static_cast<double>(jointsDeg[axis]);
        }
        egmClient_.setTargetJoints(target);

        // ---- joint 模式仿真兜底：仅在收不到真实反馈时灌模拟数据 ----
        if (chartWindow_ && !egmClient_.isPeerConnected()) {
            static double s_jointSimZ = -427.0;
            s_jointSimZ += (QRandomGenerator::global()->generateDouble() - 0.5) * 0.05;
            s_jointSimZ = std::clamp(s_jointSimZ, -428.0, -426.0);
            chartWindow_->appendZSample(s_jointSimZ - 0.1, s_jointSimZ);
        }
        return;
    }

    const float advanceMm = static_cast<float>(traverseSpeedMmPerSec_ * dtSec);

    segmentProgressMm_ += advanceMm;

    while (segmentIndex_ < pathLoader_.segmentCount() &&
           segmentProgressMm_ >= pathLoader_.segments()[segmentIndex_].lengthMm) {
        segmentProgressMm_ -= pathLoader_.segments()[segmentIndex_].lengthMm;
        ++segmentIndex_;
    }

    if (segmentIndex_ >= pathLoader_.segmentCount()) {
        stop();
        emit statusTextChanged(
            QString::fromUtf8(u8"EGM 流结束：已走完 %1 段").arg(pathLoader_.segmentCount()));
        return;
    }

    const PathVector3 pos = pathLoader_.positionAt(segmentIndex_, segmentProgressMm_);
    PathQuaternion quat = pathLoader_.orientationAt(segmentIndex_, segmentProgressMm_);
    if (alignValid_) {
        quat = QuatNormalize(QuatMultiply(alignQuat_, quat));
    }
    // EGM 的 planned.cartesian 是绝对位姿（工具/工件坐标系见 RAPID 端 EGMActPose）。
    // 用户确认过的整体偏移补偿在这里叠加：整条路径平移到机器人实际起点。
    egmClient_.setTargetPose(
        EgmTcpPose{pos.x + pathOffsetMm_.x,
                   pos.y + pathOffsetMm_.y,
                   pos.z + pathOffsetMm_.z,
                   quat.q1, quat.q2, quat.q3, quat.q4});

    // 图表窗口需要的目标 z —— 就是我们刚 setTargetPose 写进去的那个值。
    chartTargetZmm_ = pos.z + pathOffsetMm_.z;

    // ---- 仿真模式兜底：没有真实机器人反馈时，用目标 z 自己灌图表 ----
    // 一旦收到真实 EgmRobot 报文就停止灌假数据，避免反馈曲线被硬编码值污染。
    if (chartWindow_ && !egmClient_.isPeerConnected()) {
        static double s_simZJitter = 0.0;
        s_simZJitter += (QRandomGenerator::global()->generateDouble() - 0.5) * 0.05;
        s_simZJitter = std::clamp(s_simZJitter, -0.1, 0.1);
        chartWindow_->appendZSample(chartTargetZmm_,
                                    chartTargetZmm_ + s_simZJitter);
    }
}

void EgmFlowController::stop() {
    startWatchdog_.stop();
    tickTimer_.stop();
    egmClient_.enablePressureControl(false);
    egmClient_.stop();
    stopCsvRecording();

    // 注意：stop() 完全不碰 chartWindow_。
    // 曲线窗口是独立顶级窗口，EGM 停了也保持显示——用户可以继续看最后的数据、
    // 截图、缩放。下次 startSession 时 clearAll + show 重新开始。

    if (commandSocket_ != nullptr && commandSocket_->isConnected()) {
        QString ack;
        commandSocket_->sendRawMessageWithReply(QStringLiteral("111 #"), ack, 1000);
    }

    active_ = false;
}

// ---- EGM 反馈位姿记录：data_save/data_年月日/data_年月日_时分秒.csv ----
void EgmFlowController::startCsvRecording() {
    stopCsvRecording();

    const QDateTime now = QDateTime::currentDateTime();
    // 每天一个子目录，避免单目录堆积过多文件。
    const QString dayDir = QStringLiteral("data_%1").arg(now.toString(QStringLiteral("yyyyMMdd")));
    const QString dirPath = QDir(dataSaveDir()).absoluteFilePath(dayDir);
    QDir().mkpath(dirPath);

    const QString name = QStringLiteral("data_%1.csv").arg(
        now.toString(QStringLiteral("yyyyMMdd_HHmmss")));
    csvFile_.setFileName(QDir(dirPath).absoluteFilePath(name));
    if (!csvFile_.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
        emit statusTextChanged(
            QString::fromUtf8(u8"EGM 反馈记录无法写入：%1").arg(csvFile_.fileName()));
        return;
    }

    csvStream_.setDevice(&csvFile_);
    csvStream_ << "x,y,z,Fz\n";
    csvStream_.flush();
    csvRecording_ = true;
    csvRowsSinceFlush_ = 0;

    // CSV 录制启动时同步启动模拟压力源，写行时直接取 currentValue()。
    pressureSim_.start();
}

void EgmFlowController::stopCsvRecording() {
    if (!csvRecording_) return;

    pressureSim_.stop();
    csvRecording_ = false;
    csvStream_.flush();
    csvStream_.setDevice(nullptr);
    csvFile_.close();
}

void EgmFlowController::onFeedbackPose(const EgmTcpPose& pose) {
    // 先缓存控制器反馈的真实位姿：即使 CSV 已经不记录（路径走完/手动停止），
    // 下一轮 EGM 启动也要用它做起点补偿与姿态对齐。
    lastFeedbackPose_ = pose;
    hasLastFeedbackPose_ = true;

    if (!csvRecording_) return;

    csvStream_ << QString::number(pose.x, 'f', 3) << ','
               << QString::number(pose.y, 'f', 3) << ','
               << QString::number(pose.z, 'f', 3) << ','
               << QString::number(pressureSim_.currentValue(), 'f', 2) << '\n';

    // 每 250 帧（约 1 秒）落一次盘，兼顾实时性与磁盘写入开销。
    if (++csvRowsSinceFlush_ >= 250) {
        csvStream_.flush();
        csvRowsSinceFlush_ = 0;
    }
}

void EgmFlowController::onStartWatchdogTimeout() {
    if (!active_ || egmClient_.isPeerConnected()) {
        return;
    }
    const quint16 udpPort = egmClient_.boundPort();
    const int egmState = queryRobotInt(commandSocket_,
                                       QStringLiteral("112 #"), 800);
    const int lastEgmErrno = queryRobotInt(commandSocket_,
                                           QStringLiteral("114 #"), 800);
    QString watchdogLine =
        QStringLiteral("EGM watchdog: no feedback frame | %1 | udpPort=%2 egmState=%3 lastErrno=%4");
    watchdogLine = watchdogLine.arg(startContext_)
                                 .arg(static_cast<int>(udpPort))
                                 .arg(egmState)
                                 .arg(lastEgmErrno);
    appendPoseReadLog(watchdogLine);
    stop();
    emit statusTextChanged(
        QString::fromUtf8(u8"EGM 未启动：2.5 s 内未收到控制器反馈帧。"));

    QString detail = QString::fromUtf8(
        u8"EGM 流已经在发，但 2.5 s 内没有收到控制器的任何反馈帧，"
        u8"机器人不会运动。诊断信息：\n\n"
        u8"本地 UDP 端口：%1\n"
        u8"控制器 EGM 状态（CASE 112）：%2\n"
        u8"上次 EGM 会话错误码（CASE 114）：%3\n\n")
        .arg(udpPort)
        .arg(describeQueryResult(egmState))
        .arg(describeQueryResult(lastEgmErrno));

    if (udpPort == 0) {
        detail += QString::fromUtf8(
            u8"→ 6510 端口没有绑定成功：可能有别的程序（上一次残留的上位机进程、"
            u8"Python 旧版程序）还占着这个端口，请先关掉再试。");
    } else if (egmState == -1) {
        detail += QString::fromUtf8(
            u8"→ SERVER.mod 没有应答：T_ROB1 很可能已经停止（报警未清除/程序指针"
            u8"不在 main）。请到示教器清报警 → PP to main → 启动 T_ROB1，再点开始。");
    } else if (lastEgmErrno >= 0 && lastEgmErrno != 0) {
        detail += QString::fromUtf8(
            u8"→ 控制器侧 EGM 会话报错（错误码 %1）。上一次会话可能没有正常收尾，"
            u8"请在 RobotStudio 事件日志里核对这个错误码。")
            .arg(lastEgmErrno);
    } else if (lastEgmErrno == -2) {
        detail += QString::fromUtf8(
            u8"→ 控制器的 SERVER.mod 还是旧版本（不认识 CASE 114）。"
            u8"EGM 相关模块必须执行控制器完整重启（X-start）后才会生效，"
            u8"请先在 RobotStudio 里重启控制器，再重试。");
    } else {
        detail += QString::fromUtf8(
            u8"→ 控制器没有进入 EGM 会话。请检查 RAPID 事件日志里 "
            u8"「SERVER: EGM session start」之后是否有报错。");
    }
    QMessageBox::critical(dialogParent_, QString::fromUtf8(u8"EGM 会话未启动"), detail);
}
