#include <QtTest/QtTest>

#include "app/AppConfig.h"
#include "ftp/FtpClient.h"
#include "services/ftp/FtpService.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QSignalSpy>
#include <QTemporaryDir>

class FakeFtpUploader final : public IFtpUploader {
public:
    FtpUploadResult result{true, "fake upload success"};
    int callCount = 0;
    FtpUploadRequest lastRequest;

    FtpUploadResult uploadFile(const FtpUploadRequest& request) override {
        ++callCount;
        lastRequest = request;
        return result;
    }
};

class TestFtpService final : public QObject {
    Q_OBJECT

private slots:
    void successfulUploadStoresStateAndRemoteBasename();
    void failedUploadClearsState();
    void missingFileFailsBeforeNetwork();
    void nonModFileFailsBeforeNetwork();
    void defaultDirectoryUsesModFilesBesideApplicationWhenPresent();
};

namespace {

QString createFile(QTemporaryDir& dir, const QString& name) {
    const QString path = dir.filePath(name);
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly)) {
        return {};
    }
    file.write("MODULE TestPath\nENDMODULE\n");
    file.close();
    return path;
}

FtpConfig testConfig() {
    FtpConfig config;
    config.ip = "192.168.0.104";
    config.port = 21;
    config.user = "Default User";
    config.password = "robotics";
    return config;
}

} // namespace

void TestFtpService::successfulUploadStoresStateAndRemoteBasename() {
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString modPath = createFile(dir, "path_a.mod");
    QVERIFY(!modPath.isEmpty());

    FakeFtpUploader uploader;
    FtpService service(testConfig(), &uploader);
    QSignalSpy messages(&service, &FtpService::messageRequested);

    QVERIFY(service.uploadSelectedFile(modPath));

    QCOMPARE(uploader.callCount, 1);
    QCOMPARE(uploader.lastRequest.localFilePath, QFileInfo(modPath).absoluteFilePath());
    QCOMPARE(uploader.lastRequest.remoteFilename, QString("path_a.mod"));
    QCOMPARE(service.selectedLocalPath(), QFileInfo(modPath).absoluteFilePath());
    QCOMPARE(service.uploadedModFilename(), QString("path_a.mod"));
    QVERIFY(service.uploadSucceeded());
    QVERIFY(messages.count() >= 1);
}

void TestFtpService::failedUploadClearsState() {
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString modPath = createFile(dir, "path_b.mod");
    QVERIFY(!modPath.isEmpty());

    FakeFtpUploader uploader;
    uploader.result = {false, "upload rejected"};
    FtpService service(testConfig(), &uploader);

    QVERIFY(!service.uploadSelectedFile(modPath));

    QCOMPARE(uploader.callCount, 1);
    QVERIFY(service.selectedLocalPath().isEmpty());
    QVERIFY(service.uploadedModFilename().isEmpty());
    QVERIFY(!service.uploadSucceeded());
}

void TestFtpService::missingFileFailsBeforeNetwork() {
    FakeFtpUploader uploader;
    FtpService service(testConfig(), &uploader);

    QVERIFY(!service.uploadSelectedFile("D:/missing/path/not_here.mod"));

    QCOMPARE(uploader.callCount, 0);
    QVERIFY(!service.uploadSucceeded());
}

void TestFtpService::nonModFileFailsBeforeNetwork() {
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString txtPath = createFile(dir, "path.txt");
    QVERIFY(!txtPath.isEmpty());

    FakeFtpUploader uploader;
    FtpService service(testConfig(), &uploader);

    QVERIFY(!service.uploadSelectedFile(txtPath));

    QCOMPARE(uploader.callCount, 0);
    QVERIFY(!service.uploadSucceeded());
}

void TestFtpService::defaultDirectoryUsesModFilesBesideApplicationWhenPresent() {
    const QDir appDir(QCoreApplication::applicationDirPath());
    const QString modDir = appDir.filePath("mod_files");
    const bool existedBefore = QDir(modDir).exists();
    QVERIFY(QDir().mkpath(modDir));

    QCOMPARE(QDir::cleanPath(FtpService::defaultModDirectory()), QDir::cleanPath(modDir));

    if (!existedBefore) {
        QDir(modDir).removeRecursively();
    }
}

QTEST_GUILESS_MAIN(TestFtpService)
#include "test_ftp_service.moc"
