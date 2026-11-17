/********************************************************************************
** Form generated from reading UI file 'Interface4600_window.ui'
**
** Created by: Qt User Interface Compiler version 6.8.3
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_INTERFACE4600_WINDOW_H
#define UI_INTERFACE4600_WINDOW_H

#include <QtCore/QVariant>
#include <QtGui/QIcon>
#include <QtWidgets/QApplication>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QFrame>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLCDNumber>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QProgressBar>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QRadioButton>
#include <QtWidgets/QSlider>
#include <QtWidgets/QSpinBox>
#include <QtWidgets/QTabWidget>
#include <QtWidgets/QTextBrowser>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_ContinuosfiberInterface
{
public:
    QGridLayout *gridLayout;
    QTabWidget *Other_tabWidget_Robotplatform;
    QWidget *tab;
    QFrame *frame_6;
    QGroupBox *groupBox_2;
    QPushButton *ConmunicationConnectionButton;
    QPushButton *CloseSerialpushButton;
    QLabel *nozzletemperature;
    QLineEdit *nozzletemperaturelineEdit;
    QGroupBox *groupBox_3;
    QPushButton *open;
    QPushButton *stop;
    QGroupBox *groupBox_4;
    QLabel *BaudRatelabel;
    QComboBox *BaudRateBox;
    QLabel *label_4;
    QComboBox *ComPortBox;
    QLabel *SerialStatelabel;
    QPushButton *ChoosepushButton;
    QGroupBox *groupBox;
    QGroupBox *groupBox_5;
    QGridLayout *gridLayout_2;
    QLabel *label_5;
    QLabel *label_8;
    QLineEdit *RobotXlineEdit;
    QLineEdit *RobotYlineEdit;
    QLabel *label_9;
    QLineEdit *RobotRXlineEdit;
    QLineEdit *RobotZlineEdit;
    QLabel *label_19;
    QLabel *label_21;
    QLabel *label_20;
    QLineEdit *RobotRZlineEdit;
    QLineEdit *RobotRYlineEdit;
    QGroupBox *groupBox_8;
    QGridLayout *gridLayout_4;
    QLabel *label_33;
    QLineEdit *axis1lineEdit;
    QLabel *label_40;
    QLineEdit *axis2lineEdit;
    QLabel *label_41;
    QLineEdit *axis3lineEdit;
    QLabel *label_42;
    QLineEdit *axis4lineEdit;
    QLabel *label_43;
    QLineEdit *axis5lineEdit;
    QLabel *label_44;
    QLineEdit *axis6lineEdit;
    QLabel *label_6;
    QGroupBox *groupBox_20;
    QHBoxLayout *horizontalLayout_11;
    QLabel *label_32;
    QLineEdit *zero_position_x;
    QLabel *label_37;
    QLineEdit *zero_position_y;
    QLabel *label_38;
    QLineEdit *zero_position_z;
    QFrame *frame_21;
    QHBoxLayout *horizontalLayout_10;
    QPushButton *ConnectionRobotButton;
    QPushButton *Robot_movebutton;
    QPushButton *RobotGohomeButton;
    QPushButton *RobotaxismoveButton;
    QLabel *robot_state_show;
    QGroupBox *groupBox_6;
    QLabel *label_34;
    QLineEdit *Positioner1lineEdit;
    QLabel *label_45;
    QLineEdit *Positioner2lineEdit;
    QPushButton *PositionermoveButton;
    QPushButton *PositionercomebackButton;
    QLabel *label_46;
    QLineEdit *Positioner3lineEdit;
    QGroupBox *groupBox_7;
    QGridLayout *gridLayout_5;
    QLabel *label_47;
    QLineEdit *RobotlinespeedlineEdit;
    QLabel *label_64;
    QLineEdit *RobotangularspeedlineEdit;
    QLabel *label_65;
    QLineEdit *AxislinespeedlineEdit;
    QLabel *label_66;
    QLineEdit *AxisangularspeedlineEdit;
    QWidget *tab_3;
    QWidget *widget_2;
    QGroupBox *groupBox_9;
    QPushButton *ConnectionPLCButton;
    QPushButton *DisconnectPLCButton;
    QGroupBox *groupBox_12;
    QGridLayout *gridLayout_6;
    QPushButton *cylinder3_button;
    QPushButton *cylinder2_button;
    QPushButton *cylinder1_button;
    QLabel *label_28;
    QLabel *label_29;
    QLabel *label_31;
    QCheckBox *cylinder1_state;
    QCheckBox *cylinder2_state;
    QCheckBox *cylinder3_state;
    QGroupBox *groupBox_13;
    QHBoxLayout *horizontalLayout_6;
    QLabel *label_81;
    QSpinBox *spinBox;
    QLabel *label_82;
    QProgressBar *progressBar;
    QLabel *label_83;
    QLCDNumber *lcdNumber;
    QGroupBox *groupBox_14;
    QVBoxLayout *verticalLayout_13;
    QPushButton *locusselectsetButton;
    QPushButton *locusseselectButton;
    QPushButton *ParametersuitedtButton;
    QGroupBox *groupBox_15;
    QLabel *labelpic;
    QLabel *HC_RD_2;
    QLabel *TIC_RD_2;
    QLabel *FC_VV_2;
    QGroupBox *groupBox_16;
    QHBoxLayout *horizontalLayout_8;
    QPushButton *Robot_movebutton_Gcode;
    QPushButton *PauseRobotButton;
    QPushButton *ContinueRobotButton;
    QPushButton *StopRobotButton;
    QGroupBox *groupBox_17;
    QCheckBox *force_test;
    QGroupBox *groupBox_11;
    QLineEdit *Paving_distance;
    QLineEdit *Wire_feed_rate;
    QPushButton *parameter_input;
    QPushButton *motor_enable;
    QPushButton *motor_actuation;
    QCheckBox *interruption;
    QCheckBox *continuous;
    QLabel *motor_state;
    QLabel *state_show;
    QPushButton *motor_reset;
    QGroupBox *groupBox_10;
    QLineEdit *target_temperature_input;
    QPushButton *laser_connect;
    QPushButton *laser_open_button;
    QPushButton *laser_close_button;
    QTextBrowser *laser_state_display;
    QLabel *redlight_show;
    QLabel *target_temperature_label;
    QLabel *label_39;
    QPushButton *redlight_open;
    QPushButton *redlight_close;
    QPushButton *set_target_temperature;
    QPushButton *temperature_control;
    QLabel *laser_power_label;
    QLineEdit *laser_power_input;
    QPushButton *set_laser_power;
    QPushButton *set_laser_power_control;
    QGroupBox *groupBox_18;
    QHBoxLayout *horizontalLayout_9;
    QLabel *Hysteresis_ratio;
    QLineEdit *Hysteresis_ratio_text;
    QPushButton *Hysteresis_ratio_input;
    QWidget *tab_2;
    QTabWidget *tabWidget;
    QWidget *HC;
    QWidget *widget_3;
    QVBoxLayout *verticalLayout;
    QLabel *label_27;
    QFrame *line;
    QPushButton *enum_cam;
    QComboBox *camera_list;
    QPushButton *HC_machine;
    QPushButton *HC_data;
    QPushButton *HC_start;
    QPushButton *HC_stop;
    QLabel *HC_RD;
    QLabel *HC_PData;
    QLabel *HC_Abrasion;
    QLabel *HC_Misalignment;
    QLabel *label_10;
    QLabel *label_11;
    QLabel *label_12;
    QLabel *label_13;
    QFrame *frame_3;
    QHBoxLayout *horizontalLayout;
    QFrame *frame_5;
    QVBoxLayout *verticalLayout_5;
    QLabel *label_3;
    QLabel *label;
    QLabel *label_2;
    QFrame *frame_7;
    QVBoxLayout *verticalLayout_6;
    QLineEdit *FrameRate;
    QLineEdit *ExposureTime;
    QLineEdit *Gain;
    QFrame *frame_8;
    QVBoxLayout *verticalLayout_7;
    QPushButton *get_parameter;
    QPushButton *set_parameter;
    QWidget *TIC;
    QLabel *TIC_RD;
    QLabel *TIC_PD;
    QFrame *frame;
    QSlider *horizontalSlider;
    QRadioButton *radioButton;
    QLabel *label_7;
    QRadioButton *radioButton_2;
    QRadioButton *radioButton_3;
    QLabel *label_14;
    QLabel *label_15;
    QTextBrowser *textBrowser;
    QFrame *TIC_control;
    QVBoxLayout *verticalLayout_10;
    QLabel *label_17;
    QHBoxLayout *horizontalLayout_3;
    QFrame *frame_9;
    QVBoxLayout *verticalLayout_2;
    QPushButton *TIC_machine;
    QPushButton *TIC_data;
    QFrame *frame_10;
    QVBoxLayout *verticalLayout_8;
    QPushButton *TIC_start;
    QPushButton *TIC_stop;
    QLabel *label_logbrowser;
    QFrame *closeloop_speed;
    QVBoxLayout *verticalLayout_11;
    QHBoxLayout *horizontalLayout_5;
    QLabel *label_16;
    QFrame *frame_11;
    QHBoxLayout *horizontalLayout_4;
    QRadioButton *radioButton_7;
    QRadioButton *radioButton_8;
    QRadioButton *radioButton_9;
    QSlider *horizontalSlider_3;
    QWidget *FC;
    QWidget *widget_5;
    QVBoxLayout *verticalLayout_3;
    QLabel *label_30;
    QFrame *line_2;
    QPushButton *FC_machine;
    QPushButton *FC_data;
    QPushButton *FC_start;
    QPushButton *FC_stop;
    QLabel *FC_RD;
    QLabel *FC_VV;
    QLabel *FC_VAR;
    QLabel *FC_SD;
    QLabel *FC_PEAK;
    QLabel *label_18;
    QLabel *label_22;
    QLabel *label_23;
    QLabel *label_24;
    QLabel *label_25;
    QFrame *frame_2;
    QHBoxLayout *horizontalLayout_2;
    QLabel *label_26;
    QRadioButton *radioButton_4;
    QRadioButton *radioButton_5;
    QRadioButton *radioButton_6;
    QSlider *horizontalSlider_2;
    QWidget *LL;
    QWidget *widget_6;
    QVBoxLayout *verticalLayout_4;
    QLabel *label_35;
    QFrame *line_3;
    QPushButton *LL_machine;
    QPushButton *LL_data;
    QPushButton *LL_start;
    QPushButton *LL_stop;
    QLabel *LL_RD_1_background;
    QWidget *LL_RD_1;
    QGridLayout *gridLayout_3;
    QGridLayout *grid_display;
    QTextBrowser *textBrowser_ll;
    QLabel *LL_path;
    QLabel *label_36;
    QLabel *label_48;
    QLabel *label_49;
    QWidget *tab_6;
    QFrame *frame_4;

    void setupUi(QWidget *ContinuosfiberInterface)
    {
        if (ContinuosfiberInterface->objectName().isEmpty())
            ContinuosfiberInterface->setObjectName("ContinuosfiberInterface");
        ContinuosfiberInterface->resize(1197, 741);
        QSizePolicy sizePolicy(QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Fixed);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(ContinuosfiberInterface->sizePolicy().hasHeightForWidth());
        ContinuosfiberInterface->setSizePolicy(sizePolicy);
        QIcon icon;
        icon.addFile(QString::fromUtf8("C:/Administrator/Pycharm/LuLu/.designer/backup/\345\233\276\347\211\2071.png"), QSize(), QIcon::Mode::Normal, QIcon::State::Off);
        ContinuosfiberInterface->setWindowIcon(icon);
        ContinuosfiberInterface->setStyleSheet(QString::fromUtf8("background-color: rgb(242, 242,242);\n"
""));
        gridLayout = new QGridLayout(ContinuosfiberInterface);
        gridLayout->setObjectName("gridLayout");
        Other_tabWidget_Robotplatform = new QTabWidget(ContinuosfiberInterface);
        Other_tabWidget_Robotplatform->setObjectName("Other_tabWidget_Robotplatform");
        Other_tabWidget_Robotplatform->setEnabled(true);
        Other_tabWidget_Robotplatform->setMinimumSize(QSize(60, 0));
        QPalette palette;
        QBrush brush(QColor(242, 242, 242, 255));
        brush.setStyle(Qt::SolidPattern);
        palette.setBrush(QPalette::Active, QPalette::Button, brush);
        palette.setBrush(QPalette::Active, QPalette::Base, brush);
        palette.setBrush(QPalette::Active, QPalette::Window, brush);
        palette.setBrush(QPalette::Inactive, QPalette::Button, brush);
        palette.setBrush(QPalette::Inactive, QPalette::Base, brush);
        palette.setBrush(QPalette::Inactive, QPalette::Window, brush);
        palette.setBrush(QPalette::Disabled, QPalette::Button, brush);
        palette.setBrush(QPalette::Disabled, QPalette::Base, brush);
        palette.setBrush(QPalette::Disabled, QPalette::Window, brush);
        Other_tabWidget_Robotplatform->setPalette(palette);
        QFont font;
        font.setFamilies({QString::fromUtf8("\345\276\256\350\275\257\351\233\205\351\273\221")});
        font.setPointSize(14);
        font.setBold(false);
        font.setItalic(false);
        Other_tabWidget_Robotplatform->setFont(font);
        Other_tabWidget_Robotplatform->setCursor(QCursor(Qt::CursorShape::ArrowCursor));
        Other_tabWidget_Robotplatform->setFocusPolicy(Qt::TabFocus);
        Other_tabWidget_Robotplatform->setAutoFillBackground(false);
        Other_tabWidget_Robotplatform->setStyleSheet(QString::fromUtf8("background-color: rgb(242,242, 242);\n"
"border-color: rgb(0,0, 0);\n"
"font: 14pt \"\345\276\256\350\275\257\351\233\205\351\273\221\";\n"
""));
        Other_tabWidget_Robotplatform->setTabShape(QTabWidget::Rounded);
        Other_tabWidget_Robotplatform->setTabBarAutoHide(false);
        tab = new QWidget();
        tab->setObjectName("tab");
        frame_6 = new QFrame(tab);
        frame_6->setObjectName("frame_6");
        frame_6->setGeometry(QRect(0, 5, 1170, 660));
        QPalette palette1;
        palette1.setBrush(QPalette::Active, QPalette::Button, brush);
        palette1.setBrush(QPalette::Active, QPalette::Base, brush);
        palette1.setBrush(QPalette::Active, QPalette::Window, brush);
        palette1.setBrush(QPalette::Inactive, QPalette::Button, brush);
        palette1.setBrush(QPalette::Inactive, QPalette::Base, brush);
        palette1.setBrush(QPalette::Inactive, QPalette::Window, brush);
        palette1.setBrush(QPalette::Disabled, QPalette::Button, brush);
        palette1.setBrush(QPalette::Disabled, QPalette::Base, brush);
        palette1.setBrush(QPalette::Disabled, QPalette::Window, brush);
        frame_6->setPalette(palette1);
        frame_6->setAutoFillBackground(false);
        frame_6->setFrameShape(QFrame::StyledPanel);
        frame_6->setFrameShadow(QFrame::Raised);
        groupBox_2 = new QGroupBox(frame_6);
        groupBox_2->setObjectName("groupBox_2");
        groupBox_2->setGeometry(QRect(20, 20, 310, 210));
        ConmunicationConnectionButton = new QPushButton(groupBox_2);
        ConmunicationConnectionButton->setObjectName("ConmunicationConnectionButton");
        ConmunicationConnectionButton->setGeometry(QRect(20, 60, 120, 45));
        sizePolicy.setHeightForWidth(ConmunicationConnectionButton->sizePolicy().hasHeightForWidth());
        ConmunicationConnectionButton->setSizePolicy(sizePolicy);
        ConmunicationConnectionButton->setMaximumSize(QSize(200, 200));
        ConmunicationConnectionButton->setCursor(QCursor(Qt::CursorShape::PointingHandCursor));
        ConmunicationConnectionButton->setStyleSheet(QString::fromUtf8("QPushButton{\n"
"background-color: rgb(217, 217, 217);\n"
"color: rgb(0,0,0);  \n"
"border-radius:8px;\n"
"font:12pt \"\345\276\256\350\275\257\351\233\205\351\273\221\";\n"
"border:5px outset gray;\n"
"}\n"
"QPushButton::hover{\n"
"background-color: rgb(200, 200, 200);\n"
"color: rgb(0,0,0);  \n"
"border-radius:8px;\n"
"font:12pt \"\345\276\256\350\275\257\351\233\205\351\273\221\";\n"
"border:5px groove gray;\n"
"}"));
        CloseSerialpushButton = new QPushButton(groupBox_2);
        CloseSerialpushButton->setObjectName("CloseSerialpushButton");
        CloseSerialpushButton->setGeometry(QRect(170, 60, 120, 45));
        sizePolicy.setHeightForWidth(CloseSerialpushButton->sizePolicy().hasHeightForWidth());
        CloseSerialpushButton->setSizePolicy(sizePolicy);
        CloseSerialpushButton->setMaximumSize(QSize(200, 200));
        CloseSerialpushButton->setCursor(QCursor(Qt::CursorShape::PointingHandCursor));
        CloseSerialpushButton->setStyleSheet(QString::fromUtf8("QPushButton{\n"
"background-color: rgb(217, 217, 217);\n"
"color: rgb(0,0,0);  \n"
"border-radius:8px;\n"
"font:12pt \"\345\276\256\350\275\257\351\233\205\351\273\221\";\n"
"border:5px outset gray;\n"
"}\n"
"QPushButton::hover{\n"
"background-color: rgb(200, 200, 200);\n"
"color: rgb(0,0,0);  \n"
"border-radius:8px;\n"
"font:12pt \"\345\276\256\350\275\257\351\233\205\351\273\221\";\n"
"border:5px groove gray;\n"
"}"));
        nozzletemperature = new QLabel(groupBox_2);
        nozzletemperature->setObjectName("nozzletemperature");
        nozzletemperature->setGeometry(QRect(30, 140, 111, 27));
        nozzletemperature->setStyleSheet(QString::fromUtf8("font:12pt \"\345\276\256\350\275\257\351\233\205\351\273\221\";"));
        nozzletemperaturelineEdit = new QLineEdit(groupBox_2);
        nozzletemperaturelineEdit->setObjectName("nozzletemperaturelineEdit");
        nozzletemperaturelineEdit->setGeometry(QRect(170, 140, 121, 31));
        nozzletemperaturelineEdit->setStyleSheet(QString::fromUtf8("background-color: rgb(255, 255, 255);"));
        groupBox_3 = new QGroupBox(frame_6);
        groupBox_3->setObjectName("groupBox_3");
        groupBox_3->setGeometry(QRect(20, 235, 310, 130));
        open = new QPushButton(groupBox_3);
        open->setObjectName("open");
        open->setGeometry(QRect(20, 50, 120, 45));
        sizePolicy.setHeightForWidth(open->sizePolicy().hasHeightForWidth());
        open->setSizePolicy(sizePolicy);
        open->setMaximumSize(QSize(200, 200));
        open->setCursor(QCursor(Qt::CursorShape::PointingHandCursor));
        open->setStyleSheet(QString::fromUtf8("QPushButton{\n"
"background-color: rgb(217, 217, 217);\n"
"color: rgb(0,0,0);  \n"
"border-radius:8px;\n"
"font:12pt \"\345\276\256\350\275\257\351\233\205\351\273\221\";\n"
"border:5px outset gray;\n"
"}\n"
"QPushButton::hover{\n"
"background-color: rgb(200, 200, 200);\n"
"color: rgb(0,0,0);  \n"
"border-radius:8px;\n"
"font:12pt \"\345\276\256\350\275\257\351\233\205\351\273\221\";\n"
"border:5px groove gray;\n"
"}"));
        stop = new QPushButton(groupBox_3);
        stop->setObjectName("stop");
        stop->setGeometry(QRect(170, 50, 120, 45));
        sizePolicy.setHeightForWidth(stop->sizePolicy().hasHeightForWidth());
        stop->setSizePolicy(sizePolicy);
        stop->setMaximumSize(QSize(200, 200));
        stop->setCursor(QCursor(Qt::CursorShape::PointingHandCursor));
        stop->setStyleSheet(QString::fromUtf8("QPushButton{\n"
"background-color: rgb(217, 217, 217);\n"
"color: rgb(0,0,0);  \n"
"border-radius:8px;\n"
"font:12pt \"\345\276\256\350\275\257\351\233\205\351\273\221\";\n"
"border:5px outset gray;\n"
"}\n"
"QPushButton::hover{\n"
"background-color: rgb(200, 200, 200);\n"
"color: rgb(0,0,0);  \n"
"border-radius:8px;\n"
"font:12pt \"\345\276\256\350\275\257\351\233\205\351\273\221\";\n"
"border:5px groove gray;\n"
"}"));
        groupBox_4 = new QGroupBox(frame_6);
        groupBox_4->setObjectName("groupBox_4");
        groupBox_4->setGeometry(QRect(20, 370, 310, 220));
        BaudRatelabel = new QLabel(groupBox_4);
        BaudRatelabel->setObjectName("BaudRatelabel");
        BaudRatelabel->setGeometry(QRect(30, 48, 91, 27));
        BaudRatelabel->setStyleSheet(QString::fromUtf8("font:12pt \"\345\276\256\350\275\257\351\233\205\351\273\221\";"));
        BaudRateBox = new QComboBox(groupBox_4);
        BaudRateBox->addItem(QString());
        BaudRateBox->addItem(QString());
        BaudRateBox->setObjectName("BaudRateBox");
        BaudRateBox->setGeometry(QRect(150, 41, 140, 41));
        BaudRateBox->setCursor(QCursor(Qt::CursorShape::PointingHandCursor));
        BaudRateBox->setStyleSheet(QString::fromUtf8("background-color: rgb(255, 255, 255);\n"
"font: 75 12pt \"\345\276\256\350\275\257\351\233\205\351\273\221\";"));
        label_4 = new QLabel(groupBox_4);
        label_4->setObjectName("label_4");
        label_4->setGeometry(QRect(30, 99, 101, 27));
        label_4->setStyleSheet(QString::fromUtf8("font:12pt \"\345\276\256\350\275\257\351\233\205\351\273\221\";"));
        ComPortBox = new QComboBox(groupBox_4);
        ComPortBox->addItem(QString());
        ComPortBox->addItem(QString());
        ComPortBox->addItem(QString());
        ComPortBox->setObjectName("ComPortBox");
        ComPortBox->setGeometry(QRect(150, 92, 140, 41));
        QSizePolicy sizePolicy1(QSizePolicy::Policy::Fixed, QSizePolicy::Policy::Fixed);
        sizePolicy1.setHorizontalStretch(0);
        sizePolicy1.setVerticalStretch(0);
        sizePolicy1.setHeightForWidth(ComPortBox->sizePolicy().hasHeightForWidth());
        ComPortBox->setSizePolicy(sizePolicy1);
        ComPortBox->setCursor(QCursor(Qt::CursorShape::PointingHandCursor));
        ComPortBox->setStyleSheet(QString::fromUtf8("background-color: rgb(255, 255, 255);\n"
"font: 75 12pt \"\345\276\256\350\275\257\351\233\205\351\273\221\";"));
        SerialStatelabel = new QLabel(groupBox_4);
        SerialStatelabel->setObjectName("SerialStatelabel");
        SerialStatelabel->setGeometry(QRect(30, 166, 71, 31));
        SerialStatelabel->setStyleSheet(QString::fromUtf8("font: 75 12pt \"\345\276\256\350\275\257\351\233\205\351\273\221\";"));
        ChoosepushButton = new QPushButton(groupBox_4);
        ChoosepushButton->setObjectName("ChoosepushButton");
        ChoosepushButton->setGeometry(QRect(150, 160, 140, 41));
        sizePolicy.setHeightForWidth(ChoosepushButton->sizePolicy().hasHeightForWidth());
        ChoosepushButton->setSizePolicy(sizePolicy);
        ChoosepushButton->setMaximumSize(QSize(200, 200));
        ChoosepushButton->setCursor(QCursor(Qt::CursorShape::PointingHandCursor));
        ChoosepushButton->setStyleSheet(QString::fromUtf8("QPushButton{\n"
"background-color: rgb(217, 217, 217);\n"
"color: rgb(0,0,0);  \n"
"border-radius:8px;\n"
"font:12pt \"\345\276\256\350\275\257\351\233\205\351\273\221\";\n"
"border:5px outset gray;\n"
"}\n"
"QPushButton::hover{\n"
"background-color: rgb(200, 200, 200);\n"
"color: rgb(0,0,0);  \n"
"border-radius:8px;\n"
"font:12pt \"\345\276\256\350\275\257\351\233\205\351\273\221\";\n"
"border:5px groove gray;\n"
"}"));
        groupBox = new QGroupBox(frame_6);
        groupBox->setObjectName("groupBox");
        groupBox->setGeometry(QRect(340, 20, 490, 570));
        groupBox_5 = new QGroupBox(groupBox);
        groupBox_5->setObjectName("groupBox_5");
        groupBox_5->setGeometry(QRect(10, 190, 220, 370));
        gridLayout_2 = new QGridLayout(groupBox_5);
        gridLayout_2->setObjectName("gridLayout_2");
        label_5 = new QLabel(groupBox_5);
        label_5->setObjectName("label_5");
        label_5->setStyleSheet(QString::fromUtf8("font: 75 12pt \"\345\276\256\350\275\257\351\233\205\351\273\221\";"));
        label_5->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);

        gridLayout_2->addWidget(label_5, 0, 0, 1, 1);

        label_8 = new QLabel(groupBox_5);
        label_8->setObjectName("label_8");
        label_8->setStyleSheet(QString::fromUtf8("font: 75 12pt \"\345\276\256\350\275\257\351\233\205\351\273\221\";"));
        label_8->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);

        gridLayout_2->addWidget(label_8, 1, 0, 1, 1);

        RobotXlineEdit = new QLineEdit(groupBox_5);
        RobotXlineEdit->setObjectName("RobotXlineEdit");
        RobotXlineEdit->setStyleSheet(QString::fromUtf8("background-color: rgb(255, 255, 255);"));

        gridLayout_2->addWidget(RobotXlineEdit, 0, 1, 1, 1);

        RobotYlineEdit = new QLineEdit(groupBox_5);
        RobotYlineEdit->setObjectName("RobotYlineEdit");
        RobotYlineEdit->setStyleSheet(QString::fromUtf8("background-color: rgb(255, 255, 255);"));

        gridLayout_2->addWidget(RobotYlineEdit, 1, 1, 1, 1);

        label_9 = new QLabel(groupBox_5);
        label_9->setObjectName("label_9");
        label_9->setStyleSheet(QString::fromUtf8("font: 75 12pt \"\345\276\256\350\275\257\351\233\205\351\273\221\";"));
        label_9->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);

        gridLayout_2->addWidget(label_9, 2, 0, 1, 1);

        RobotRXlineEdit = new QLineEdit(groupBox_5);
        RobotRXlineEdit->setObjectName("RobotRXlineEdit");
        RobotRXlineEdit->setStyleSheet(QString::fromUtf8("background-color: rgb(255, 255, 255);"));

        gridLayout_2->addWidget(RobotRXlineEdit, 3, 1, 1, 1);

        RobotZlineEdit = new QLineEdit(groupBox_5);
        RobotZlineEdit->setObjectName("RobotZlineEdit");
        RobotZlineEdit->setStyleSheet(QString::fromUtf8("background-color: rgb(255, 255, 255);"));

        gridLayout_2->addWidget(RobotZlineEdit, 2, 1, 1, 1);

        label_19 = new QLabel(groupBox_5);
        label_19->setObjectName("label_19");
        label_19->setStyleSheet(QString::fromUtf8("font: 75 12pt \"\345\276\256\350\275\257\351\233\205\351\273\221\";"));
        label_19->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);

        gridLayout_2->addWidget(label_19, 3, 0, 1, 1);

        label_21 = new QLabel(groupBox_5);
        label_21->setObjectName("label_21");
        label_21->setStyleSheet(QString::fromUtf8("font: 75 12pt \"\345\276\256\350\275\257\351\233\205\351\273\221\";"));
        label_21->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);

        gridLayout_2->addWidget(label_21, 5, 0, 1, 1);

        label_20 = new QLabel(groupBox_5);
        label_20->setObjectName("label_20");
        label_20->setStyleSheet(QString::fromUtf8("font: 75 12pt \"\345\276\256\350\275\257\351\233\205\351\273\221\";"));
        label_20->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);

        gridLayout_2->addWidget(label_20, 4, 0, 1, 1);

        RobotRZlineEdit = new QLineEdit(groupBox_5);
        RobotRZlineEdit->setObjectName("RobotRZlineEdit");
        RobotRZlineEdit->setStyleSheet(QString::fromUtf8("background-color: rgb(255, 255, 255);"));

        gridLayout_2->addWidget(RobotRZlineEdit, 5, 1, 1, 1);

        RobotRYlineEdit = new QLineEdit(groupBox_5);
        RobotRYlineEdit->setObjectName("RobotRYlineEdit");
        RobotRYlineEdit->setStyleSheet(QString::fromUtf8("background-color: rgb(255, 255, 255);"));

        gridLayout_2->addWidget(RobotRYlineEdit, 4, 1, 1, 1);

        groupBox_8 = new QGroupBox(groupBox);
        groupBox_8->setObjectName("groupBox_8");
        groupBox_8->setGeometry(QRect(240, 190, 240, 370));
        gridLayout_4 = new QGridLayout(groupBox_8);
        gridLayout_4->setObjectName("gridLayout_4");
        label_33 = new QLabel(groupBox_8);
        label_33->setObjectName("label_33");
        label_33->setStyleSheet(QString::fromUtf8("font: 75 12pt \"\345\276\256\350\275\257\351\233\205\351\273\221\";"));

        gridLayout_4->addWidget(label_33, 0, 0, 1, 1);

        axis1lineEdit = new QLineEdit(groupBox_8);
        axis1lineEdit->setObjectName("axis1lineEdit");
        axis1lineEdit->setStyleSheet(QString::fromUtf8("background-color: rgb(255, 255, 255);"));

        gridLayout_4->addWidget(axis1lineEdit, 0, 1, 1, 1);

        label_40 = new QLabel(groupBox_8);
        label_40->setObjectName("label_40");
        label_40->setStyleSheet(QString::fromUtf8("font: 75 12pt \"\345\276\256\350\275\257\351\233\205\351\273\221\";"));

        gridLayout_4->addWidget(label_40, 1, 0, 1, 1);

        axis2lineEdit = new QLineEdit(groupBox_8);
        axis2lineEdit->setObjectName("axis2lineEdit");
        axis2lineEdit->setStyleSheet(QString::fromUtf8("background-color: rgb(255, 255, 255);"));

        gridLayout_4->addWidget(axis2lineEdit, 1, 1, 1, 1);

        label_41 = new QLabel(groupBox_8);
        label_41->setObjectName("label_41");
        label_41->setStyleSheet(QString::fromUtf8("font: 75 12pt \"\345\276\256\350\275\257\351\233\205\351\273\221\";"));

        gridLayout_4->addWidget(label_41, 2, 0, 1, 1);

        axis3lineEdit = new QLineEdit(groupBox_8);
        axis3lineEdit->setObjectName("axis3lineEdit");
        axis3lineEdit->setStyleSheet(QString::fromUtf8("background-color: rgb(255, 255, 255);"));

        gridLayout_4->addWidget(axis3lineEdit, 2, 1, 1, 1);

        label_42 = new QLabel(groupBox_8);
        label_42->setObjectName("label_42");
        label_42->setStyleSheet(QString::fromUtf8("font: 75 12pt \"\345\276\256\350\275\257\351\233\205\351\273\221\";"));

        gridLayout_4->addWidget(label_42, 3, 0, 1, 1);

        axis4lineEdit = new QLineEdit(groupBox_8);
        axis4lineEdit->setObjectName("axis4lineEdit");
        axis4lineEdit->setStyleSheet(QString::fromUtf8("background-color: rgb(255, 255, 255);"));

        gridLayout_4->addWidget(axis4lineEdit, 3, 1, 1, 1);

        label_43 = new QLabel(groupBox_8);
        label_43->setObjectName("label_43");
        label_43->setStyleSheet(QString::fromUtf8("font: 75 12pt \"\345\276\256\350\275\257\351\233\205\351\273\221\";"));

        gridLayout_4->addWidget(label_43, 4, 0, 1, 1);

        axis5lineEdit = new QLineEdit(groupBox_8);
        axis5lineEdit->setObjectName("axis5lineEdit");
        axis5lineEdit->setStyleSheet(QString::fromUtf8("background-color: rgb(255, 255, 255);"));

        gridLayout_4->addWidget(axis5lineEdit, 4, 1, 1, 1);

        label_44 = new QLabel(groupBox_8);
        label_44->setObjectName("label_44");
        label_44->setStyleSheet(QString::fromUtf8("font: 75 12pt \"\345\276\256\350\275\257\351\233\205\351\273\221\";"));

        gridLayout_4->addWidget(label_44, 5, 0, 1, 1);

        axis6lineEdit = new QLineEdit(groupBox_8);
        axis6lineEdit->setObjectName("axis6lineEdit");
        axis6lineEdit->setStyleSheet(QString::fromUtf8("background-color: rgb(255, 255, 255);"));

        gridLayout_4->addWidget(axis6lineEdit, 5, 1, 1, 1);

        gridLayout_4->setColumnStretch(0, 1);
        gridLayout_4->setColumnStretch(1, 5);
        label_6 = new QLabel(groupBox);
        label_6->setObjectName("label_6");
        label_6->setGeometry(QRect(20, 50, 110, 30));
        groupBox_20 = new QGroupBox(groupBox);
        groupBox_20->setObjectName("groupBox_20");
        groupBox_20->setGeometry(QRect(230, 20, 250, 75));
        horizontalLayout_11 = new QHBoxLayout(groupBox_20);
        horizontalLayout_11->setObjectName("horizontalLayout_11");
        horizontalLayout_11->setContentsMargins(-1, 5, -1, -1);
        label_32 = new QLabel(groupBox_20);
        label_32->setObjectName("label_32");

        horizontalLayout_11->addWidget(label_32);

        zero_position_x = new QLineEdit(groupBox_20);
        zero_position_x->setObjectName("zero_position_x");
        zero_position_x->setFont(font);
        zero_position_x->setStyleSheet(QString::fromUtf8("background-color: rgb(255, 255, 255);"));

        horizontalLayout_11->addWidget(zero_position_x);

        label_37 = new QLabel(groupBox_20);
        label_37->setObjectName("label_37");

        horizontalLayout_11->addWidget(label_37);

        zero_position_y = new QLineEdit(groupBox_20);
        zero_position_y->setObjectName("zero_position_y");
        zero_position_y->setStyleSheet(QString::fromUtf8("background-color: rgb(255, 255, 255);"));

        horizontalLayout_11->addWidget(zero_position_y);

        label_38 = new QLabel(groupBox_20);
        label_38->setObjectName("label_38");

        horizontalLayout_11->addWidget(label_38);

        zero_position_z = new QLineEdit(groupBox_20);
        zero_position_z->setObjectName("zero_position_z");
        zero_position_z->setStyleSheet(QString::fromUtf8("background-color: rgb(255, 255, 255);"));

        horizontalLayout_11->addWidget(zero_position_z);

        frame_21 = new QFrame(groupBox);
        frame_21->setObjectName("frame_21");
        frame_21->setGeometry(QRect(10, 110, 470, 71));
        frame_21->setStyleSheet(QString::fromUtf8("border:1px solid rgb(212, 212, 212);"));
        horizontalLayout_10 = new QHBoxLayout(frame_21);
        horizontalLayout_10->setSpacing(10);
        horizontalLayout_10->setObjectName("horizontalLayout_10");
        horizontalLayout_10->setContentsMargins(10, 0, 10, 0);
        ConnectionRobotButton = new QPushButton(frame_21);
        ConnectionRobotButton->setObjectName("ConnectionRobotButton");
        ConnectionRobotButton->setEnabled(true);
        QSizePolicy sizePolicy2(QSizePolicy::Policy::Minimum, QSizePolicy::Policy::Fixed);
        sizePolicy2.setHorizontalStretch(0);
        sizePolicy2.setVerticalStretch(0);
        sizePolicy2.setHeightForWidth(ConnectionRobotButton->sizePolicy().hasHeightForWidth());
        ConnectionRobotButton->setSizePolicy(sizePolicy2);
        ConnectionRobotButton->setMinimumSize(QSize(0, 45));
        ConnectionRobotButton->setMaximumSize(QSize(200, 45));
        QPalette palette2;
        QBrush brush1(QColor(0, 0, 0, 255));
        brush1.setStyle(Qt::SolidPattern);
        palette2.setBrush(QPalette::Active, QPalette::WindowText, brush1);
        QBrush brush2(QColor(217, 217, 217, 255));
        brush2.setStyle(Qt::SolidPattern);
        palette2.setBrush(QPalette::Active, QPalette::Button, brush2);
        palette2.setBrush(QPalette::Active, QPalette::Text, brush1);
        palette2.setBrush(QPalette::Active, QPalette::ButtonText, brush1);
        palette2.setBrush(QPalette::Active, QPalette::Base, brush2);
        palette2.setBrush(QPalette::Active, QPalette::Window, brush2);
        QBrush brush3(QColor(0, 0, 0, 128));
        brush3.setStyle(Qt::NoBrush);
#if QT_VERSION >= QT_VERSION_CHECK(5, 12, 0)
        palette2.setBrush(QPalette::Active, QPalette::PlaceholderText, brush3);
#endif
        palette2.setBrush(QPalette::Inactive, QPalette::WindowText, brush1);
        palette2.setBrush(QPalette::Inactive, QPalette::Button, brush2);
        palette2.setBrush(QPalette::Inactive, QPalette::Text, brush1);
        palette2.setBrush(QPalette::Inactive, QPalette::ButtonText, brush1);
        palette2.setBrush(QPalette::Inactive, QPalette::Base, brush2);
        palette2.setBrush(QPalette::Inactive, QPalette::Window, brush2);
        QBrush brush4(QColor(0, 0, 0, 128));
        brush4.setStyle(Qt::NoBrush);
#if QT_VERSION >= QT_VERSION_CHECK(5, 12, 0)
        palette2.setBrush(QPalette::Inactive, QPalette::PlaceholderText, brush4);
#endif
        palette2.setBrush(QPalette::Disabled, QPalette::WindowText, brush1);
        palette2.setBrush(QPalette::Disabled, QPalette::Button, brush2);
        palette2.setBrush(QPalette::Disabled, QPalette::Text, brush1);
        palette2.setBrush(QPalette::Disabled, QPalette::ButtonText, brush1);
        palette2.setBrush(QPalette::Disabled, QPalette::Base, brush2);
        palette2.setBrush(QPalette::Disabled, QPalette::Window, brush2);
        QBrush brush5(QColor(0, 0, 0, 128));
        brush5.setStyle(Qt::NoBrush);
#if QT_VERSION >= QT_VERSION_CHECK(5, 12, 0)
        palette2.setBrush(QPalette::Disabled, QPalette::PlaceholderText, brush5);
#endif
        ConnectionRobotButton->setPalette(palette2);
        ConnectionRobotButton->setCursor(QCursor(Qt::CursorShape::PointingHandCursor));
        ConnectionRobotButton->setStyleSheet(QString::fromUtf8("QPushButton{\n"
"background-color: rgb(217, 217, 217);\n"
"color: rgb(0,0,0);  \n"
"border-radius:8px;\n"
"font:12pt \"\345\276\256\350\275\257\351\233\205\351\273\221\";\n"
"border:5px outset gray;\n"
"}\n"
"QPushButton::hover{\n"
"background-color: rgb(200, 200, 200);\n"
"color: rgb(0,0,0);  \n"
"border-radius:8px;\n"
"font:12pt \"\345\276\256\350\275\257\351\233\205\351\273\221\";\n"
"border:5px groove gray;\n"
"}"));

        horizontalLayout_10->addWidget(ConnectionRobotButton);

        Robot_movebutton = new QPushButton(frame_21);
        Robot_movebutton->setObjectName("Robot_movebutton");
        Robot_movebutton->setEnabled(true);
        Robot_movebutton->setMinimumSize(QSize(0, 45));
        Robot_movebutton->setMaximumSize(QSize(200, 45));
        Robot_movebutton->setCursor(QCursor(Qt::CursorShape::PointingHandCursor));
        Robot_movebutton->setStyleSheet(QString::fromUtf8("QPushButton{\n"
"background-color: rgb(217, 217, 217);\n"
"color: rgb(0,0,0);  \n"
"border-radius:8px;\n"
"font:12pt \"\345\276\256\350\275\257\351\233\205\351\273\221\";\n"
"border:5px outset gray;\n"
"}\n"
"QPushButton::hover{\n"
"background-color: rgb(200, 200, 200);\n"
"color: rgb(0,0,0);  \n"
"border-radius:8px;\n"
"font:12pt \"\345\276\256\350\275\257\351\233\205\351\273\221\";\n"
"border:5px groove gray;\n"
"}"));

        horizontalLayout_10->addWidget(Robot_movebutton);

        RobotGohomeButton = new QPushButton(frame_21);
        RobotGohomeButton->setObjectName("RobotGohomeButton");
        RobotGohomeButton->setMinimumSize(QSize(0, 45));
        RobotGohomeButton->setMaximumSize(QSize(200, 45));
        RobotGohomeButton->setCursor(QCursor(Qt::CursorShape::PointingHandCursor));
        RobotGohomeButton->setStyleSheet(QString::fromUtf8("QPushButton{\n"
"background-color: rgb(217, 217, 217);\n"
"color: rgb(0,0,0);  \n"
"border-radius:8px;\n"
"font:12pt \"\345\276\256\350\275\257\351\233\205\351\273\221\";\n"
"border:5px outset gray;\n"
"}\n"
"QPushButton::hover{\n"
"background-color: rgb(200, 200, 200);\n"
"color: rgb(0,0,0);  \n"
"border-radius:8px;\n"
"font:12pt \"\345\276\256\350\275\257\351\233\205\351\273\221\";\n"
"border:5px groove gray;\n"
"}"));

        horizontalLayout_10->addWidget(RobotGohomeButton);

        RobotaxismoveButton = new QPushButton(frame_21);
        RobotaxismoveButton->setObjectName("RobotaxismoveButton");
        RobotaxismoveButton->setMinimumSize(QSize(0, 45));
        RobotaxismoveButton->setMaximumSize(QSize(200, 45));
        RobotaxismoveButton->setCursor(QCursor(Qt::CursorShape::PointingHandCursor));
        RobotaxismoveButton->setStyleSheet(QString::fromUtf8("QPushButton{\n"
"background-color: rgb(217, 217, 217);\n"
"color: rgb(0,0,0);  \n"
"border-radius:8px;\n"
"font:12pt \"\345\276\256\350\275\257\351\233\205\351\273\221\";\n"
"border:5px outset gray;\n"
"}\n"
"QPushButton::hover{\n"
"background-color: rgb(200, 200, 200);\n"
"color: rgb(0,0,0);  \n"
"border-radius:8px;\n"
"font:12pt \"\345\276\256\350\275\257\351\233\205\351\273\221\";\n"
"border:5px groove gray;\n"
"}"));

        horizontalLayout_10->addWidget(RobotaxismoveButton);

        robot_state_show = new QLabel(groupBox);
        robot_state_show->setObjectName("robot_state_show");
        robot_state_show->setGeometry(QRect(140, 50, 60, 30));
        groupBox_6 = new QGroupBox(frame_6);
        groupBox_6->setObjectName("groupBox_6");
        groupBox_6->setGeometry(QRect(840, 20, 310, 260));
        label_34 = new QLabel(groupBox_6);
        label_34->setObjectName("label_34");
        label_34->setGeometry(QRect(20, 50, 120, 20));
        label_34->setStyleSheet(QString::fromUtf8("font: 75 12pt \"\345\276\256\350\275\257\351\233\205\351\273\221\";"));
        Positioner1lineEdit = new QLineEdit(groupBox_6);
        Positioner1lineEdit->setObjectName("Positioner1lineEdit");
        Positioner1lineEdit->setGeometry(QRect(160, 45, 120, 30));
        Positioner1lineEdit->setStyleSheet(QString::fromUtf8("background-color: rgb(255, 255, 255);"));
        label_45 = new QLabel(groupBox_6);
        label_45->setObjectName("label_45");
        label_45->setGeometry(QRect(20, 100, 120, 20));
        label_45->setStyleSheet(QString::fromUtf8("font: 75 12pt \"\345\276\256\350\275\257\351\233\205\351\273\221\";"));
        Positioner2lineEdit = new QLineEdit(groupBox_6);
        Positioner2lineEdit->setObjectName("Positioner2lineEdit");
        Positioner2lineEdit->setGeometry(QRect(160, 95, 120, 30));
        Positioner2lineEdit->setStyleSheet(QString::fromUtf8("background-color: rgb(255, 255, 255);"));
        PositionermoveButton = new QPushButton(groupBox_6);
        PositionermoveButton->setObjectName("PositionermoveButton");
        PositionermoveButton->setGeometry(QRect(25, 200, 110, 40));
        PositionermoveButton->setCursor(QCursor(Qt::CursorShape::PointingHandCursor));
        PositionermoveButton->setStyleSheet(QString::fromUtf8("QPushButton{\n"
"background-color: rgb(217, 217, 217);\n"
"color: rgb(0,0,0);  \n"
"font:12pt \"\345\276\256\350\275\257\351\233\205\351\273\221\";\n"
"border:5px outset gray;\n"
"border-radius:6px;\n"
"}\n"
"QPushButton::hover{\n"
"background-color: rgb(200, 200, 200);\n"
"color: rgb(0,0,0);  \n"
"font:12pt \"\345\276\256\350\275\257\351\233\205\351\273\221\";\n"
"border:5px groove gray;\n"
"border-radius:6px;\n"
"}"));
        PositionercomebackButton = new QPushButton(groupBox_6);
        PositionercomebackButton->setObjectName("PositionercomebackButton");
        PositionercomebackButton->setGeometry(QRect(170, 200, 110, 40));
        PositionercomebackButton->setCursor(QCursor(Qt::CursorShape::PointingHandCursor));
        PositionercomebackButton->setStyleSheet(QString::fromUtf8("QPushButton{\n"
"background-color: rgb(217, 217, 217);\n"
"color: rgb(0,0,0);  \n"
"font:12pt \"\345\276\256\350\275\257\351\233\205\351\273\221\";\n"
"border:5px outset gray;\n"
"border-radius:6px;\n"
"}\n"
"QPushButton::hover{\n"
"background-color: rgb(200, 200, 200);\n"
"color: rgb(0,0,0);  \n"
"font:12pt \"\345\276\256\350\275\257\351\233\205\351\273\221\";\n"
"border:5px groove gray;\n"
"border-radius:6px;\n"
"}"));
        label_46 = new QLabel(groupBox_6);
        label_46->setObjectName("label_46");
        label_46->setGeometry(QRect(20, 150, 120, 20));
        label_46->setStyleSheet(QString::fromUtf8("font: 75 12pt \"\345\276\256\350\275\257\351\233\205\351\273\221\";"));
        Positioner3lineEdit = new QLineEdit(groupBox_6);
        Positioner3lineEdit->setObjectName("Positioner3lineEdit");
        Positioner3lineEdit->setGeometry(QRect(160, 145, 120, 30));
        Positioner3lineEdit->setStyleSheet(QString::fromUtf8("background-color: rgb(255, 255, 255);"));
        groupBox_7 = new QGroupBox(frame_6);
        groupBox_7->setObjectName("groupBox_7");
        groupBox_7->setGeometry(QRect(840, 290, 310, 300));
        gridLayout_5 = new QGridLayout(groupBox_7);
        gridLayout_5->setObjectName("gridLayout_5");
        label_47 = new QLabel(groupBox_7);
        label_47->setObjectName("label_47");
        QFont font1;
        font1.setFamilies({QString::fromUtf8("\345\276\256\350\275\257\351\233\205\351\273\221")});
        font1.setPointSize(12);
        font1.setBold(false);
        font1.setItalic(false);
        label_47->setFont(font1);
        label_47->setStyleSheet(QString::fromUtf8("font: 75 12pt \"\345\276\256\350\275\257\351\233\205\351\273\221\";"));

        gridLayout_5->addWidget(label_47, 0, 0, 1, 1);

        RobotlinespeedlineEdit = new QLineEdit(groupBox_7);
        RobotlinespeedlineEdit->setObjectName("RobotlinespeedlineEdit");
        RobotlinespeedlineEdit->setStyleSheet(QString::fromUtf8("background-color: rgb(255, 255, 255);"));

        gridLayout_5->addWidget(RobotlinespeedlineEdit, 0, 1, 1, 1);

        label_64 = new QLabel(groupBox_7);
        label_64->setObjectName("label_64");
        label_64->setFont(font1);
        label_64->setStyleSheet(QString::fromUtf8("font: 75 12pt \"\345\276\256\350\275\257\351\233\205\351\273\221\";"));

        gridLayout_5->addWidget(label_64, 1, 0, 1, 1);

        RobotangularspeedlineEdit = new QLineEdit(groupBox_7);
        RobotangularspeedlineEdit->setObjectName("RobotangularspeedlineEdit");
        RobotangularspeedlineEdit->setStyleSheet(QString::fromUtf8("background-color: rgb(255, 255, 255);"));

        gridLayout_5->addWidget(RobotangularspeedlineEdit, 1, 1, 1, 1);

        label_65 = new QLabel(groupBox_7);
        label_65->setObjectName("label_65");
        label_65->setFont(font1);
        label_65->setStyleSheet(QString::fromUtf8("font: 75 12pt \"\345\276\256\350\275\257\351\233\205\351\273\221\";"));

        gridLayout_5->addWidget(label_65, 2, 0, 1, 1);

        AxislinespeedlineEdit = new QLineEdit(groupBox_7);
        AxislinespeedlineEdit->setObjectName("AxislinespeedlineEdit");
        AxislinespeedlineEdit->setStyleSheet(QString::fromUtf8("background-color: rgb(255, 255, 255);"));

        gridLayout_5->addWidget(AxislinespeedlineEdit, 2, 1, 1, 1);

        label_66 = new QLabel(groupBox_7);
        label_66->setObjectName("label_66");
        label_66->setFont(font1);
        label_66->setStyleSheet(QString::fromUtf8("font: 75 12pt \"\345\276\256\350\275\257\351\233\205\351\273\221\";"));

        gridLayout_5->addWidget(label_66, 3, 0, 1, 1);

        AxisangularspeedlineEdit = new QLineEdit(groupBox_7);
        AxisangularspeedlineEdit->setObjectName("AxisangularspeedlineEdit");
        AxisangularspeedlineEdit->setStyleSheet(QString::fromUtf8("background-color: rgb(255, 255, 255);"));

        gridLayout_5->addWidget(AxisangularspeedlineEdit, 3, 1, 1, 1);

        gridLayout_5->setColumnStretch(0, 5);
        gridLayout_5->setColumnStretch(1, 3);
        Other_tabWidget_Robotplatform->addTab(tab, QString());
        tab_3 = new QWidget();
        tab_3->setObjectName("tab_3");
        widget_2 = new QWidget(tab_3);
        widget_2->setObjectName("widget_2");
        widget_2->setGeometry(QRect(0, 0, 1170, 660));
        QPalette palette3;
        palette3.setBrush(QPalette::Active, QPalette::Button, brush);
        palette3.setBrush(QPalette::Active, QPalette::Base, brush);
        palette3.setBrush(QPalette::Active, QPalette::Window, brush);
        palette3.setBrush(QPalette::Inactive, QPalette::Button, brush);
        palette3.setBrush(QPalette::Inactive, QPalette::Base, brush);
        palette3.setBrush(QPalette::Inactive, QPalette::Window, brush);
        palette3.setBrush(QPalette::Disabled, QPalette::Button, brush);
        palette3.setBrush(QPalette::Disabled, QPalette::Base, brush);
        palette3.setBrush(QPalette::Disabled, QPalette::Window, brush);
        widget_2->setPalette(palette3);
        groupBox_9 = new QGroupBox(widget_2);
        groupBox_9->setObjectName("groupBox_9");
        groupBox_9->setGeometry(QRect(620, 110, 260, 80));
        ConnectionPLCButton = new QPushButton(groupBox_9);
        ConnectionPLCButton->setObjectName("ConnectionPLCButton");
        ConnectionPLCButton->setGeometry(QRect(12, 30, 110, 35));
        sizePolicy2.setHeightForWidth(ConnectionPLCButton->sizePolicy().hasHeightForWidth());
        ConnectionPLCButton->setSizePolicy(sizePolicy2);
        ConnectionPLCButton->setMinimumSize(QSize(110, 35));
        ConnectionPLCButton->setMaximumSize(QSize(110, 35));
        ConnectionPLCButton->setFont(font);
        ConnectionPLCButton->setCursor(QCursor(Qt::CursorShape::PointingHandCursor));
        ConnectionPLCButton->setStyleSheet(QString::fromUtf8("QPushButton{\n"
"background-color: rgb(217, 217, 217);\n"
"color: rgb(0,0,0);  \n"
"border-radius:4px;\n"
"border:4px outset gray;\n"
"}\n"
"QPushButton::hover{\n"
"background-color: rgb(200, 200, 200);\n"
"color: rgb(0,0,0);  \n"
"border-radius:4px;\n"
"border:4px groove gray;\n"
"}"));
        DisconnectPLCButton = new QPushButton(groupBox_9);
        DisconnectPLCButton->setObjectName("DisconnectPLCButton");
        DisconnectPLCButton->setGeometry(QRect(140, 30, 110, 35));
        DisconnectPLCButton->setCursor(QCursor(Qt::CursorShape::PointingHandCursor));
        DisconnectPLCButton->setStyleSheet(QString::fromUtf8("QPushButton{\n"
"background-color: rgb(217, 217, 217);\n"
"color: rgb(0,0,0);  \n"
"border-radius:4px;\n"
"border:4px outset gray;\n"
"}\n"
"QPushButton::hover{\n"
"background-color: rgb(200, 200, 200);\n"
"color: rgb(0,0,0);  \n"
"border-radius:4px;\n"
"border:4px groove gray;\n"
"}"));
        groupBox_12 = new QGroupBox(widget_2);
        groupBox_12->setObjectName("groupBox_12");
        groupBox_12->setGeometry(QRect(620, 195, 260, 135));
        gridLayout_6 = new QGridLayout(groupBox_12);
        gridLayout_6->setObjectName("gridLayout_6");
        gridLayout_6->setHorizontalSpacing(10);
        gridLayout_6->setVerticalSpacing(6);
        gridLayout_6->setContentsMargins(10, 4, 10, 10);
        cylinder3_button = new QPushButton(groupBox_12);
        cylinder3_button->setObjectName("cylinder3_button");
        cylinder3_button->setMinimumSize(QSize(90, 0));
        cylinder3_button->setCursor(QCursor(Qt::CursorShape::PointingHandCursor));
        cylinder3_button->setStyleSheet(QString::fromUtf8("QPushButton{\n"
"background-color: rgb(217, 217, 217);\n"
"color: rgb(0,0,0);  \n"
"border-radius:4px;\n"
"font:12pt \"\345\276\256\350\275\257\351\233\205\351\273\221\";\n"
"border:3px outset gray;\n"
"}\n"
"QPushButton::hover{\n"
"background-color: rgb(200, 200, 200);\n"
"color: rgb(0,0,0);  \n"
"border-radius:4px;\n"
"font:12pt \"\345\276\256\350\275\257\351\233\205\351\273\221\";\n"
"border:3px groove gray;\n"
"}"));

        gridLayout_6->addWidget(cylinder3_button, 3, 2, 1, 1);

        cylinder2_button = new QPushButton(groupBox_12);
        cylinder2_button->setObjectName("cylinder2_button");
        cylinder2_button->setCursor(QCursor(Qt::CursorShape::PointingHandCursor));
        cylinder2_button->setStyleSheet(QString::fromUtf8("QPushButton{\n"
"background-color: rgb(217, 217, 217);\n"
"color: rgb(0,0,0);  \n"
"border-radius:4px;\n"
"font:12pt \"\345\276\256\350\275\257\351\233\205\351\273\221\";\n"
"border:3px outset gray;\n"
"}\n"
"QPushButton::hover{\n"
"background-color: rgb(200, 200, 200);\n"
"color: rgb(0,0,0);  \n"
"border-radius:4px;\n"
"font:12pt \"\345\276\256\350\275\257\351\233\205\351\273\221\";\n"
"border:3px groove gray;\n"
"}"));

        gridLayout_6->addWidget(cylinder2_button, 1, 2, 1, 1);

        cylinder1_button = new QPushButton(groupBox_12);
        cylinder1_button->setObjectName("cylinder1_button");
        cylinder1_button->setCursor(QCursor(Qt::CursorShape::PointingHandCursor));
        cylinder1_button->setStyleSheet(QString::fromUtf8("QPushButton{\n"
"background-color: rgb(217, 217, 217);\n"
"color: rgb(0,0,0);  \n"
"border-radius:4px;\n"
"font:12pt \"\345\276\256\350\275\257\351\233\205\351\273\221\";\n"
"border:3px outset gray;\n"
"}\n"
"QPushButton::hover{\n"
"background-color: rgb(200, 200, 200);\n"
"color: rgb(0,0,0);  \n"
"border-radius:4px;\n"
"font:12pt \"\345\276\256\350\275\257\351\233\205\351\273\221\";\n"
"border:3px groove gray;\n"
"}"));

        gridLayout_6->addWidget(cylinder1_button, 0, 2, 1, 1);

        label_28 = new QLabel(groupBox_12);
        label_28->setObjectName("label_28");
        label_28->setAlignment(Qt::AlignCenter);

        gridLayout_6->addWidget(label_28, 0, 0, 1, 1);

        label_29 = new QLabel(groupBox_12);
        label_29->setObjectName("label_29");
        label_29->setAlignment(Qt::AlignCenter);

        gridLayout_6->addWidget(label_29, 1, 0, 1, 1);

        label_31 = new QLabel(groupBox_12);
        label_31->setObjectName("label_31");
        label_31->setAlignment(Qt::AlignCenter);

        gridLayout_6->addWidget(label_31, 3, 0, 1, 1);

        cylinder1_state = new QCheckBox(groupBox_12);
        cylinder1_state->setObjectName("cylinder1_state");
        cylinder1_state->setMaximumSize(QSize(20, 20));
        cylinder1_state->setCursor(QCursor(Qt::CursorShape::ArrowCursor));

        gridLayout_6->addWidget(cylinder1_state, 0, 1, 1, 1);

        cylinder2_state = new QCheckBox(groupBox_12);
        cylinder2_state->setObjectName("cylinder2_state");
        cylinder2_state->setCursor(QCursor(Qt::CursorShape::ArrowCursor));

        gridLayout_6->addWidget(cylinder2_state, 1, 1, 1, 1);

        cylinder3_state = new QCheckBox(groupBox_12);
        cylinder3_state->setObjectName("cylinder3_state");

        gridLayout_6->addWidget(cylinder3_state, 3, 1, 1, 1);

        groupBox_13 = new QGroupBox(widget_2);
        groupBox_13->setObjectName("groupBox_13");
        groupBox_13->setGeometry(QRect(620, 20, 530, 85));
        horizontalLayout_6 = new QHBoxLayout(groupBox_13);
        horizontalLayout_6->setObjectName("horizontalLayout_6");
        horizontalLayout_6->setContentsMargins(-1, 0, -1, -1);
        label_81 = new QLabel(groupBox_13);
        label_81->setObjectName("label_81");
        label_81->setStyleSheet(QString::fromUtf8("font:12pt \"\345\276\256\350\275\257\351\233\205\351\273\221\";"));
        label_81->setAlignment(Qt::AlignCenter);

        horizontalLayout_6->addWidget(label_81);

        spinBox = new QSpinBox(groupBox_13);
        spinBox->setObjectName("spinBox");
        spinBox->setCursor(QCursor(Qt::CursorShape::PointingHandCursor));
        spinBox->setMaximum(999999999);
        spinBox->setSingleStep(1);
        spinBox->setValue(1);

        horizontalLayout_6->addWidget(spinBox);

        label_82 = new QLabel(groupBox_13);
        label_82->setObjectName("label_82");
        label_82->setStyleSheet(QString::fromUtf8("font:12pt \"\345\276\256\350\275\257\351\233\205\351\273\221\";"));
        label_82->setAlignment(Qt::AlignCenter);

        horizontalLayout_6->addWidget(label_82);

        progressBar = new QProgressBar(groupBox_13);
        progressBar->setObjectName("progressBar");
        progressBar->setMaximumSize(QSize(16777215, 30));
        progressBar->setStyleSheet(QString::fromUtf8("QProgressBar{\n"
"	border:1px solid black;\n"
"}"));
        progressBar->setMinimum(0);
        progressBar->setValue(50);
        progressBar->setAlignment(Qt::AlignCenter);
        progressBar->setOrientation(Qt::Horizontal);
        progressBar->setTextDirection(QProgressBar::TopToBottom);

        horizontalLayout_6->addWidget(progressBar);

        label_83 = new QLabel(groupBox_13);
        label_83->setObjectName("label_83");
        label_83->setStyleSheet(QString::fromUtf8("font:12pt \"\345\276\256\350\275\257\351\233\205\351\273\221\";"));

        horizontalLayout_6->addWidget(label_83);

        lcdNumber = new QLCDNumber(groupBox_13);
        lcdNumber->setObjectName("lcdNumber");
        lcdNumber->setMaximumSize(QSize(16777215, 30));
        lcdNumber->setCursor(QCursor(Qt::CursorShape::ArrowCursor));
        lcdNumber->setFrameShadow(QFrame::Plain);
        lcdNumber->setSmallDecimalPoint(true);
        lcdNumber->setDigitCount(6);
        lcdNumber->setMode(QLCDNumber::Dec);
        lcdNumber->setSegmentStyle(QLCDNumber::Flat);
        lcdNumber->setProperty("value", QVariant(386.120000000000005));

        horizontalLayout_6->addWidget(lcdNumber);

        horizontalLayout_6->setStretch(0, 1);
        horizontalLayout_6->setStretch(1, 2);
        horizontalLayout_6->setStretch(2, 1);
        horizontalLayout_6->setStretch(3, 2);
        horizontalLayout_6->setStretch(4, 1);
        horizontalLayout_6->setStretch(5, 2);
        groupBox_14 = new QGroupBox(widget_2);
        groupBox_14->setObjectName("groupBox_14");
        groupBox_14->setGeometry(QRect(20, 110, 140, 220));
        verticalLayout_13 = new QVBoxLayout(groupBox_14);
        verticalLayout_13->setObjectName("verticalLayout_13");
        verticalLayout_13->setContentsMargins(-1, 0, -1, -1);
        locusselectsetButton = new QPushButton(groupBox_14);
        locusselectsetButton->setObjectName("locusselectsetButton");
        locusselectsetButton->setMinimumSize(QSize(0, 40));
        locusselectsetButton->setCursor(QCursor(Qt::CursorShape::PointingHandCursor));
        locusselectsetButton->setStyleSheet(QString::fromUtf8("QPushButton{\n"
"background-color: rgb(217, 217, 217);\n"
"color: rgb(0,0,0);  \n"
"border-radius:8px;\n"
"font:12pt \"\345\276\256\350\275\257\351\233\205\351\273\221\";\n"
"border:5px outset gray;\n"
"}\n"
"QPushButton::hover{\n"
"background-color: rgb(200, 200, 200);\n"
"color: rgb(0,0,0);  \n"
"border-radius:8px;\n"
"font:12pt \"\345\276\256\350\275\257\351\233\205\351\273\221\";\n"
"border:5px groove gray;\n"
"}"));

        verticalLayout_13->addWidget(locusselectsetButton);

        locusseselectButton = new QPushButton(groupBox_14);
        locusseselectButton->setObjectName("locusseselectButton");
        locusseselectButton->setMinimumSize(QSize(0, 40));
        locusseselectButton->setCursor(QCursor(Qt::CursorShape::PointingHandCursor));
        locusseselectButton->setStyleSheet(QString::fromUtf8("QPushButton{\n"
"background-color: rgb(217, 217, 217);\n"
"color: rgb(0,0,0);  \n"
"border-radius:8px;\n"
"font:12pt \"\345\276\256\350\275\257\351\233\205\351\273\221\";\n"
"border:5px outset gray;\n"
"}\n"
"QPushButton::hover{\n"
"background-color: rgb(200, 200, 200);\n"
"color: rgb(0,0,0);  \n"
"border-radius:8px;\n"
"font:12pt \"\345\276\256\350\275\257\351\233\205\351\273\221\";\n"
"border:5px groove gray;\n"
"}"));

        verticalLayout_13->addWidget(locusseselectButton);

        ParametersuitedtButton = new QPushButton(groupBox_14);
        ParametersuitedtButton->setObjectName("ParametersuitedtButton");
        ParametersuitedtButton->setMinimumSize(QSize(0, 40));
        ParametersuitedtButton->setCursor(QCursor(Qt::CursorShape::PointingHandCursor));
        ParametersuitedtButton->setStyleSheet(QString::fromUtf8("QPushButton{\n"
"background-color: rgb(217, 217, 217);\n"
"color: rgb(0,0,0);  \n"
"border-radius:8px;\n"
"font:12pt \"\345\276\256\350\275\257\351\233\205\351\273\221\";\n"
"border:5px outset gray;\n"
"}\n"
"QPushButton::hover{\n"
"background-color: rgb(200, 200, 200);\n"
"color: rgb(0,0,0);  \n"
"border-radius:8px;\n"
"font:12pt \"\345\276\256\350\275\257\351\233\205\351\273\221\";\n"
"border:5px groove gray;\n"
"}"));

        verticalLayout_13->addWidget(ParametersuitedtButton);

        groupBox_15 = new QGroupBox(widget_2);
        groupBox_15->setObjectName("groupBox_15");
        groupBox_15->setGeometry(QRect(170, 110, 440, 480));
        labelpic = new QLabel(groupBox_15);
        labelpic->setObjectName("labelpic");
        labelpic->setGeometry(QRect(1, 26, 16, 25));
        HC_RD_2 = new QLabel(groupBox_15);
        HC_RD_2->setObjectName("HC_RD_2");
        HC_RD_2->setGeometry(QRect(10, 30, 211, 201));
        HC_RD_2->setStyleSheet(QString::fromUtf8("QLabel{\n"
"	border:1px solid gray;\n"
"}"));
        TIC_RD_2 = new QLabel(groupBox_15);
        TIC_RD_2->setObjectName("TIC_RD_2");
        TIC_RD_2->setGeometry(QRect(220, 30, 211, 201));
        TIC_RD_2->setStyleSheet(QString::fromUtf8("QLabel{\n"
"	border:1px solid gray;\n"
"}"));
        FC_VV_2 = new QLabel(groupBox_15);
        FC_VV_2->setObjectName("FC_VV_2");
        FC_VV_2->setGeometry(QRect(10, 240, 421, 231));
        FC_VV_2->setStyleSheet(QString::fromUtf8("QLabel{\n"
"	border:1px solid gray;\n"
"}"));
        groupBox_16 = new QGroupBox(widget_2);
        groupBox_16->setObjectName("groupBox_16");
        groupBox_16->setGeometry(QRect(20, 20, 590, 85));
        horizontalLayout_8 = new QHBoxLayout(groupBox_16);
        horizontalLayout_8->setSpacing(10);
        horizontalLayout_8->setObjectName("horizontalLayout_8");
        horizontalLayout_8->setContentsMargins(10, 0, 10, 10);
        Robot_movebutton_Gcode = new QPushButton(groupBox_16);
        Robot_movebutton_Gcode->setObjectName("Robot_movebutton_Gcode");
        Robot_movebutton_Gcode->setMinimumSize(QSize(0, 40));
        Robot_movebutton_Gcode->setCursor(QCursor(Qt::CursorShape::PointingHandCursor));
        Robot_movebutton_Gcode->setContextMenuPolicy(Qt::ActionsContextMenu);
        Robot_movebutton_Gcode->setStyleSheet(QString::fromUtf8("QPushButton{\n"
"background-color: rgb(217, 217, 217);\n"
"color: rgb(0,0,0);  \n"
"border-radius:8px;\n"
"font:12pt \"\345\276\256\350\275\257\351\233\205\351\273\221\";\n"
"border:5px outset gray;\n"
"}\n"
"QPushButton::hover{\n"
"background-color: rgb(200, 200, 200);\n"
"color: rgb(0,0,0);  \n"
"border-radius:8px;\n"
"font:12pt \"\345\276\256\350\275\257\351\233\205\351\273\221\";\n"
"border:5px groove gray;\n"
"}"));

        horizontalLayout_8->addWidget(Robot_movebutton_Gcode);

        PauseRobotButton = new QPushButton(groupBox_16);
        PauseRobotButton->setObjectName("PauseRobotButton");
        PauseRobotButton->setMinimumSize(QSize(0, 40));
        PauseRobotButton->setCursor(QCursor(Qt::CursorShape::PointingHandCursor));
        PauseRobotButton->setStyleSheet(QString::fromUtf8("QPushButton{\n"
"background-color: rgb(217, 217, 217);\n"
"color: rgb(0,0,0);  \n"
"border-radius:8px;\n"
"font:12pt \"\345\276\256\350\275\257\351\233\205\351\273\221\";\n"
"border:5px outset gray;\n"
"}\n"
"QPushButton::hover{\n"
"background-color: rgb(200, 200, 200);\n"
"color: rgb(0,0,0);  \n"
"border-radius:8px;\n"
"font:12pt \"\345\276\256\350\275\257\351\233\205\351\273\221\";\n"
"border:5px groove gray;\n"
"}"));

        horizontalLayout_8->addWidget(PauseRobotButton);

        ContinueRobotButton = new QPushButton(groupBox_16);
        ContinueRobotButton->setObjectName("ContinueRobotButton");
        ContinueRobotButton->setMinimumSize(QSize(0, 40));
        ContinueRobotButton->setCursor(QCursor(Qt::CursorShape::PointingHandCursor));
        ContinueRobotButton->setStyleSheet(QString::fromUtf8("QPushButton{\n"
"background-color: rgb(217, 217, 217);\n"
"color: rgb(0,0,0);  \n"
"border-radius:8px;\n"
"font:12pt \"\345\276\256\350\275\257\351\233\205\351\273\221\";\n"
"border:5px outset gray;\n"
"}\n"
"QPushButton::hover{\n"
"background-color: rgb(200, 200, 200);\n"
"color: rgb(0,0,0);  \n"
"border-radius:8px;\n"
"font:12pt \"\345\276\256\350\275\257\351\233\205\351\273\221\";\n"
"border:5px groove gray;\n"
"}"));

        horizontalLayout_8->addWidget(ContinueRobotButton);

        StopRobotButton = new QPushButton(groupBox_16);
        StopRobotButton->setObjectName("StopRobotButton");
        StopRobotButton->setMinimumSize(QSize(0, 40));
        StopRobotButton->setCursor(QCursor(Qt::CursorShape::PointingHandCursor));
        StopRobotButton->setStyleSheet(QString::fromUtf8("QPushButton{\n"
"background-color: rgb(217, 217, 217);\n"
"color: rgb(0,0,0);  \n"
"border-radius:8px;\n"
"font:12pt \"\345\276\256\350\275\257\351\233\205\351\273\221\";\n"
"border:5px outset gray;\n"
"}\n"
"QPushButton::hover{\n"
"background-color: rgb(200, 200, 200);\n"
"color: rgb(0,0,0);  \n"
"border-radius:8px;\n"
"font:12pt \"\345\276\256\350\275\257\351\233\205\351\273\221\";\n"
"border:5px groove gray;\n"
"}"));

        horizontalLayout_8->addWidget(StopRobotButton);

        groupBox_17 = new QGroupBox(widget_2);
        groupBox_17->setObjectName("groupBox_17");
        groupBox_17->setGeometry(QRect(20, 335, 140, 255));
        force_test = new QCheckBox(groupBox_17);
        force_test->setObjectName("force_test");
        force_test->setGeometry(QRect(16, 37, 111, 31));
        force_test->setStyleSheet(QString::fromUtf8("QCheckButton{\n"
"background-color: rgb(217, 217, 217);\n"
"color: rgb(0,0,0);  \n"
"border-radius:8px;\n"
"font:12pt \"\345\276\256\350\275\257\351\233\205\351\273\221\";\n"
"}"));
        groupBox_11 = new QGroupBox(widget_2);
        groupBox_11->setObjectName("groupBox_11");
        groupBox_11->setGeometry(QRect(620, 335, 260, 170));
        Paving_distance = new QLineEdit(groupBox_11);
        Paving_distance->setObjectName("Paving_distance");
        Paving_distance->setGeometry(QRect(10, 30, 140, 30));
        Paving_distance->setStyleSheet(QString::fromUtf8("background-color: rgb(255, 255, 255);\n"
"font:12pt \"\345\276\256\350\275\257\351\233\205\351\273\221\";"));
        Wire_feed_rate = new QLineEdit(groupBox_11);
        Wire_feed_rate->setObjectName("Wire_feed_rate");
        Wire_feed_rate->setGeometry(QRect(10, 66, 140, 30));
        Wire_feed_rate->setFont(font1);
        Wire_feed_rate->setStyleSheet(QString::fromUtf8("background-color: rgb(255, 255, 255);\n"
"font:12pt \"\345\276\256\350\275\257\351\233\205\351\273\221\";"));
        parameter_input = new QPushButton(groupBox_11);
        parameter_input->setObjectName("parameter_input");
        parameter_input->setGeometry(QRect(160, 66, 90, 30));
        parameter_input->setCursor(QCursor(Qt::CursorShape::PointingHandCursor));
        parameter_input->setStyleSheet(QString::fromUtf8("QPushButton{\n"
"background-color: rgb(217, 217, 217);\n"
"color: rgb(0,0,0);  \n"
"border-radius:4px;\n"
"font:10pt \"\345\276\256\350\275\257\351\233\205\351\273\221\";\n"
"border:3px outset gray;\n"
"}\n"
"QPushButton::hover{\n"
"background-color: rgb(200, 200, 200);\n"
"color: rgb(0,0,0);  \n"
"border-radius:4px;\n"
"font:10pt \"\345\276\256\350\275\257\351\233\205\351\273\221\";\n"
"border:3px groove gray;\n"
"}"));
        motor_enable = new QPushButton(groupBox_11);
        motor_enable->setObjectName("motor_enable");
        motor_enable->setGeometry(QRect(80, 105, 80, 30));
        motor_enable->setCursor(QCursor(Qt::CursorShape::PointingHandCursor));
        motor_enable->setStyleSheet(QString::fromUtf8("QPushButton{\n"
"background-color: rgb(217, 217, 217);\n"
"color: rgb(0,0,0);\n"
"border-radius:4px;\n"
"font:10pt \"\345\276\256\350\275\257\351\233\205\351\273\221\";\n"
"border:3px outset gray;\n"
"}\n"
"QPushButton::hover{\n"
"background-color: rgb(200, 200, 200);\n"
"color: rgb(0,0,0);\n"
"border-radius:4px;\n"
"font:10pt \"\345\276\256\350\275\257\351\233\205\351\273\221\";\n"
"border:3px groove gray;\n"
"}"));
        motor_actuation = new QPushButton(groupBox_11);
        motor_actuation->setObjectName("motor_actuation");
        motor_actuation->setGeometry(QRect(170, 105, 80, 30));
        motor_actuation->setCursor(QCursor(Qt::CursorShape::PointingHandCursor));
        motor_actuation->setStyleSheet(QString::fromUtf8("QPushButton{\n"
"background-color: rgb(217, 217, 217);\n"
"color: rgb(0,0,0);\n"
"border-radius:4px;\n"
"font:10pt \"\345\276\256\350\275\257\351\233\205\351\273\221\";\n"
"border:3px outset gray;\n"
"}\n"
"QPushButton::hover{\n"
"background-color: rgb(200, 200, 200);\n"
"color: rgb(0,0,0);\n"
"border-radius:4px;\n"
"font:10pt \"\345\276\256\350\275\257\351\233\205\351\273\221\";\n"
"border:3px groove gray;\n"
"}"));
        interruption = new QCheckBox(groupBox_11);
        interruption->setObjectName("interruption");
        interruption->setGeometry(QRect(93, 142, 70, 20));
        interruption->setCursor(QCursor(Qt::CursorShape::PointingHandCursor));
        interruption->setStyleSheet(QString::fromUtf8("font:10pt \"\345\276\256\350\275\257\351\233\205\351\273\221\";"));
        continuous = new QCheckBox(groupBox_11);
        continuous->setObjectName("continuous");
        continuous->setGeometry(QRect(160, 142, 70, 20));
        continuous->setCursor(QCursor(Qt::CursorShape::PointingHandCursor));
        continuous->setStyleSheet(QString::fromUtf8("font:10pt \"\345\276\256\350\275\257\351\233\205\351\273\221\";"));
        motor_state = new QLabel(groupBox_11);
        motor_state->setObjectName("motor_state");
        motor_state->setGeometry(QRect(12, 141, 81, 21));
        motor_state->setStyleSheet(QString::fromUtf8("font:10pt \"\345\276\256\350\275\257\351\233\205\351\273\221\";"));
        state_show = new QLabel(groupBox_11);
        state_show->setObjectName("state_show");
        state_show->setGeometry(QRect(160, 30, 90, 30));
        state_show->setStyleSheet(QString::fromUtf8("background-color: rgb(217, 217, 217);\n"
"color: rgb(0,0,0);  \n"
"font:10pt \"\345\276\256\350\275\257\351\233\205\351\273\221\";\n"
"border:3px outset rgb(200, 200, 200);"));
        state_show->setAlignment(Qt::AlignCenter);
        motor_reset = new QPushButton(groupBox_11);
        motor_reset->setObjectName("motor_reset");
        motor_reset->setGeometry(QRect(10, 105, 60, 30));
        motor_reset->setCursor(QCursor(Qt::CursorShape::PointingHandCursor));
        motor_reset->setStyleSheet(QString::fromUtf8("QPushButton{\n"
"background-color: rgb(217, 217, 217);\n"
"color: rgb(0,0,0);\n"
"border-radius:4px;\n"
"font:10pt \"\345\276\256\350\275\257\351\233\205\351\273\221\";\n"
"border:3px outset gray;\n"
"}\n"
"QPushButton::hover{\n"
"background-color: rgb(200, 200, 200);\n"
"color: rgb(0,0,0);\n"
"border-radius:4px;\n"
"font:10pt \"\345\276\256\350\275\257\351\233\205\351\273\221\";\n"
"border:3px groove gray;\n"
"}"));
        groupBox_10 = new QGroupBox(widget_2);
        groupBox_10->setObjectName("groupBox_10");
        groupBox_10->setGeometry(QRect(890, 110, 260, 481));
        target_temperature_input = new QLineEdit(groupBox_10);
        target_temperature_input->setObjectName("target_temperature_input");
        target_temperature_input->setGeometry(QRect(125, 170, 120, 30));
        target_temperature_input->setStyleSheet(QString::fromUtf8("QLineEdit{\n"
"background-color: rgb(255, 255, 255);\n"
"font:12pt \"\345\276\256\350\275\257\351\233\205\351\273\221\";\n"
"}"));
        laser_connect = new QPushButton(groupBox_10);
        laser_connect->setObjectName("laser_connect");
        laser_connect->setGeometry(QRect(145, 130, 100, 30));
        sizePolicy2.setHeightForWidth(laser_connect->sizePolicy().hasHeightForWidth());
        laser_connect->setSizePolicy(sizePolicy2);
        laser_connect->setCursor(QCursor(Qt::CursorShape::PointingHandCursor));
        laser_connect->setStyleSheet(QString::fromUtf8("QPushButton{\n"
"background-color: rgb(217, 217, 217);\n"
"color: rgb(0,0,0);  \n"
"border-radius:4px;\n"
"font:12pt \"\345\276\256\350\275\257\351\233\205\351\273\221\";\n"
"border:3px outset gray;\n"
"}\n"
"QPushButton::hover{\n"
"background-color: rgb(200, 200, 200);\n"
"color: rgb(0,0,0);  \n"
"border-radius:4px;\n"
"font:12pt \"\345\276\256\350\275\257\351\233\205\351\273\221\";\n"
"border:3px groove gray;\n"
"}"));
        laser_open_button = new QPushButton(groupBox_10);
        laser_open_button->setObjectName("laser_open_button");
        laser_open_button->setGeometry(QRect(135, 330, 110, 30));
        laser_open_button->setCursor(QCursor(Qt::CursorShape::PointingHandCursor));
        laser_open_button->setStyleSheet(QString::fromUtf8("QPushButton{\n"
"background-color: rgb(217, 217, 217);\n"
"color: rgb(0,0,0);  \n"
"border-radius:4px;\n"
"font:12pt \"\345\276\256\350\275\257\351\233\205\351\273\221\";\n"
"border:3px outset gray;\n"
"}\n"
"QPushButton::hover{\n"
"background-color: rgb(200, 200, 200);\n"
"color: rgb(0,0,0);  \n"
"border-radius:4px;\n"
"font:12pt \"\345\276\256\350\275\257\351\233\205\351\273\221\";\n"
"border:3px groove gray;\n"
"}"));
        laser_close_button = new QPushButton(groupBox_10);
        laser_close_button->setObjectName("laser_close_button");
        laser_close_button->setGeometry(QRect(135, 370, 110, 30));
        laser_close_button->setCursor(QCursor(Qt::CursorShape::PointingHandCursor));
        laser_close_button->setStyleSheet(QString::fromUtf8("QPushButton{\n"
"background-color: rgb(217, 217, 217);\n"
"color: rgb(0,0,0);  \n"
"border-radius:4px;\n"
"font:12pt \"\345\276\256\350\275\257\351\233\205\351\273\221\";\n"
"border:3px outset gray;\n"
"}\n"
"QPushButton::hover{\n"
"background-color: rgb(200, 200, 200);\n"
"color: rgb(0,0,0);  \n"
"border-radius:4px;\n"
"font:12pt \"\345\276\256\350\275\257\351\233\205\351\273\221\";\n"
"border:3px groove gray;\n"
"}"));
        laser_state_display = new QTextBrowser(groupBox_10);
        laser_state_display->setObjectName("laser_state_display");
        laser_state_display->setGeometry(QRect(15, 35, 230, 85));
        laser_state_display->setStyleSheet(QString::fromUtf8("QTextBrowser{\n"
"font:8pt \"\345\276\256\350\275\257\351\233\205\351\273\221\";\n"
"}"));
        redlight_show = new QLabel(groupBox_10);
        redlight_show->setObjectName("redlight_show");
        redlight_show->setGeometry(QRect(15, 130, 105, 30));
        redlight_show->setStyleSheet(QString::fromUtf8("QLabel{\n"
"background-color: rgb(217, 217, 217);\n"
"color: rgb(0,0,0);  \n"
"font:10pt \"\345\276\256\350\275\257\351\233\205\351\273\221\";\n"
"border:3px outset rgb(200, 200, 200);\n"
"}"));
        redlight_show->setAlignment(Qt::AlignCenter);
        target_temperature_label = new QLabel(groupBox_10);
        target_temperature_label->setObjectName("target_temperature_label");
        target_temperature_label->setGeometry(QRect(15, 170, 105, 30));
        target_temperature_label->setStyleSheet(QString::fromUtf8("QLabel{\n"
"font:12pt \"\345\276\256\350\275\257\351\233\205\351\273\221\";\n"
"}"));
        label_39 = new QLabel(groupBox_10);
        label_39->setObjectName("label_39");
        label_39->setGeometry(QRect(194, 35, 51, 16));
        label_39->setStyleSheet(QString::fromUtf8("QLabel{\n"
"font:8pt\"\345\276\256\350\275\257\351\233\205\351\273\221\";\n"
"border:1px solid black;\n"
"}"));
        label_39->setAlignment(Qt::AlignCenter);
        redlight_open = new QPushButton(groupBox_10);
        redlight_open->setObjectName("redlight_open");
        redlight_open->setGeometry(QRect(15, 330, 110, 30));
        redlight_open->setCursor(QCursor(Qt::CursorShape::PointingHandCursor));
        redlight_open->setStyleSheet(QString::fromUtf8("QPushButton{\n"
"background-color: rgb(217, 217, 217);\n"
"color: rgb(0,0,0);  \n"
"border-radius:4px;\n"
"font:12pt \"\345\276\256\350\275\257\351\233\205\351\273\221\";\n"
"border:3px outset gray;\n"
"}\n"
"QPushButton::hover{\n"
"background-color: rgb(200, 200, 200);\n"
"color: rgb(0,0,0);  \n"
"border-radius:4px;\n"
"font:12pt \"\345\276\256\350\275\257\351\233\205\351\273\221\";\n"
"border:3px groove gray;\n"
"}"));
        redlight_close = new QPushButton(groupBox_10);
        redlight_close->setObjectName("redlight_close");
        redlight_close->setGeometry(QRect(15, 370, 110, 30));
        redlight_close->setCursor(QCursor(Qt::CursorShape::PointingHandCursor));
        redlight_close->setStyleSheet(QString::fromUtf8("QPushButton{\n"
"background-color: rgb(217, 217, 217);\n"
"color: rgb(0,0,0);  \n"
"border-radius:4px;\n"
"font:12pt \"\345\276\256\350\275\257\351\233\205\351\273\221\";\n"
"border:3px outset gray;\n"
"}\n"
"QPushButton::hover{\n"
"background-color: rgb(200, 200, 200);\n"
"color: rgb(0,0,0);  \n"
"border-radius:4px;\n"
"font:12pt \"\345\276\256\350\275\257\351\233\205\351\273\221\";\n"
"border:3px groove gray;\n"
"}"));
        set_target_temperature = new QPushButton(groupBox_10);
        set_target_temperature->setObjectName("set_target_temperature");
        set_target_temperature->setGeometry(QRect(15, 210, 110, 30));
        set_target_temperature->setCursor(QCursor(Qt::CursorShape::PointingHandCursor));
        set_target_temperature->setStyleSheet(QString::fromUtf8("QPushButton{\n"
"background-color: rgb(217, 217, 217);\n"
"color: rgb(0,0,0);  \n"
"border-radius:4px;\n"
"font:12pt \"\345\276\256\350\275\257\351\233\205\351\273\221\";\n"
"border:3px outset gray;\n"
"}\n"
"QPushButton::hover{\n"
"background-color: rgb(200, 200, 200);\n"
"color: rgb(0,0,0);  \n"
"border-radius:4px;\n"
"font:12pt \"\345\276\256\350\275\257\351\233\205\351\273\221\";\n"
"border:3px groove gray;\n"
"}"));
        temperature_control = new QPushButton(groupBox_10);
        temperature_control->setObjectName("temperature_control");
        temperature_control->setGeometry(QRect(135, 210, 110, 30));
        temperature_control->setCursor(QCursor(Qt::CursorShape::PointingHandCursor));
        temperature_control->setStyleSheet(QString::fromUtf8("QPushButton{\n"
"background-color: rgb(217, 217, 217);\n"
"color: rgb(0,0,0);  \n"
"border-radius:4px;\n"
"font:12pt \"\345\276\256\350\275\257\351\233\205\351\273\221\";\n"
"border:3px outset gray;\n"
"}\n"
"QPushButton::hover{\n"
"background-color: rgb(200, 200, 200);\n"
"color: rgb(0,0,0);  \n"
"border-radius:4px;\n"
"font:12pt \"\345\276\256\350\275\257\351\233\205\351\273\221\";\n"
"border:3px groove gray;\n"
"}"));
        laser_power_label = new QLabel(groupBox_10);
        laser_power_label->setObjectName("laser_power_label");
        laser_power_label->setGeometry(QRect(15, 250, 105, 30));
        laser_power_label->setStyleSheet(QString::fromUtf8("QLabel{\n"
"font:12pt \"\345\276\256\350\275\257\351\233\205\351\273\221\";\n"
"}"));
        laser_power_input = new QLineEdit(groupBox_10);
        laser_power_input->setObjectName("laser_power_input");
        laser_power_input->setGeometry(QRect(125, 250, 120, 30));
        laser_power_input->setStyleSheet(QString::fromUtf8("QLineEdit{\n"
"background-color: rgb(255, 255, 255);\n"
"font:12pt \"\345\276\256\350\275\257\351\233\205\351\273\221\";\n"
"}"));
        set_laser_power = new QPushButton(groupBox_10);
        set_laser_power->setObjectName("set_laser_power");
        set_laser_power->setGeometry(QRect(15, 290, 110, 30));
        set_laser_power->setCursor(QCursor(Qt::CursorShape::PointingHandCursor));
        set_laser_power->setStyleSheet(QString::fromUtf8("QPushButton{\n"
"background-color: rgb(217, 217, 217);\n"
"color: rgb(0,0,0);  \n"
"border-radius:4px;\n"
"font:12pt \"\345\276\256\350\275\257\351\233\205\351\273\221\";\n"
"border:3px outset gray;\n"
"}\n"
"QPushButton::hover{\n"
"background-color: rgb(200, 200, 200);\n"
"color: rgb(0,0,0);  \n"
"border-radius:4px;\n"
"font:12pt \"\345\276\256\350\275\257\351\233\205\351\273\221\";\n"
"border:3px groove gray;\n"
"}"));
        set_laser_power_control = new QPushButton(groupBox_10);
        set_laser_power_control->setObjectName("set_laser_power_control");
        set_laser_power_control->setGeometry(QRect(135, 290, 110, 30));
        set_laser_power_control->setCursor(QCursor(Qt::CursorShape::PointingHandCursor));
        set_laser_power_control->setStyleSheet(QString::fromUtf8("QPushButton{\n"
"background-color: rgb(217, 217, 217);\n"
"color: rgb(0,0,0);  \n"
"border-radius:4px;\n"
"font:12pt \"\345\276\256\350\275\257\351\233\205\351\273\221\";\n"
"border:3px outset gray;\n"
"}\n"
"QPushButton::hover{\n"
"background-color: rgb(200, 200, 200);\n"
"color: rgb(0,0,0);  \n"
"border-radius:4px;\n"
"font:12pt \"\345\276\256\350\275\257\351\233\205\351\273\221\";\n"
"border:3px groove gray;\n"
"}"));
        groupBox_18 = new QGroupBox(widget_2);
        groupBox_18->setObjectName("groupBox_18");
        groupBox_18->setGeometry(QRect(620, 510, 260, 80));
        horizontalLayout_9 = new QHBoxLayout(groupBox_18);
        horizontalLayout_9->setSpacing(5);
        horizontalLayout_9->setObjectName("horizontalLayout_9");
        horizontalLayout_9->setContentsMargins(-1, 4, -1, -1);
        Hysteresis_ratio = new QLabel(groupBox_18);
        Hysteresis_ratio->setObjectName("Hysteresis_ratio");
        Hysteresis_ratio->setMaximumSize(QSize(50, 30));

        horizontalLayout_9->addWidget(Hysteresis_ratio);

        Hysteresis_ratio_text = new QLineEdit(groupBox_18);
        Hysteresis_ratio_text->setObjectName("Hysteresis_ratio_text");
        Hysteresis_ratio_text->setEnabled(true);
        Hysteresis_ratio_text->setMaximumSize(QSize(16777215, 30));
        Hysteresis_ratio_text->setStyleSheet(QString::fromUtf8("background-color: rgb(255, 255, 255);  \n"
"font:12pt\"\345\276\256\350\275\257\351\233\205\351\273\221\";"));

        horizontalLayout_9->addWidget(Hysteresis_ratio_text);

        Hysteresis_ratio_input = new QPushButton(groupBox_18);
        Hysteresis_ratio_input->setObjectName("Hysteresis_ratio_input");
        Hysteresis_ratio_input->setMinimumSize(QSize(65, 30));
        Hysteresis_ratio_input->setMaximumSize(QSize(65, 30));
        Hysteresis_ratio_input->setCursor(QCursor(Qt::CursorShape::PointingHandCursor));
        Hysteresis_ratio_input->setStyleSheet(QString::fromUtf8("QPushButton{\n"
"background-color: rgb(217, 217, 217);\n"
"color: rgb(0,0,0);  \n"
"font:12pt\"\345\276\256\350\275\257\351\233\205\351\273\221\";\n"
"border-radius:4px;\n"
"border:3px outset gray;\n"
"}\n"
"QPushButton::hover{\n"
"background-color: rgb(200, 200, 200);\n"
"color: rgb(0,0,0);  \n"
"font:12pt\"\345\276\256\350\275\257\351\233\205\351\273\221\";  \n"
"border-radius:4px;\n"
"border:3px groove gray;\n"
"}"));

        horizontalLayout_9->addWidget(Hysteresis_ratio_input);

        Other_tabWidget_Robotplatform->addTab(tab_3, QString());
        tab_2 = new QWidget();
        tab_2->setObjectName("tab_2");
        tabWidget = new QTabWidget(tab_2);
        tabWidget->setObjectName("tabWidget");
        tabWidget->setGeometry(QRect(20, 10, 1165, 655));
        tabWidget->setStyleSheet(QString::fromUtf8("QTabWidget{\n"
"	border:1px outset gray;\n"
"}\n"
"QTabWidget::tab-bar { \n"
"	alignment: center; \n"
"} \n"
""));
        HC = new QWidget();
        HC->setObjectName("HC");
        widget_3 = new QWidget(HC);
        widget_3->setObjectName("widget_3");
        widget_3->setGeometry(QRect(5, 5, 120, 602));
        widget_3->setStyleSheet(QString::fromUtf8("QWidget{\n"
"	border:1px outset gray;\n"
"}"));
        verticalLayout = new QVBoxLayout(widget_3);
        verticalLayout->setSpacing(20);
        verticalLayout->setObjectName("verticalLayout");
        verticalLayout->setContentsMargins(10, 20, 10, 10);
        label_27 = new QLabel(widget_3);
        label_27->setObjectName("label_27");
        label_27->setMaximumSize(QSize(16777215, 30));
        label_27->setStyleSheet(QString::fromUtf8("QLabel{\n"
"    font:12pt\"\345\276\256\350\275\257\351\233\205\351\273\221\";\n"
"	border:none;\n"
"}"));
        label_27->setAlignment(Qt::AlignCenter);

        verticalLayout->addWidget(label_27);

        line = new QFrame(widget_3);
        line->setObjectName("line");
        line->setFrameShape(QFrame::Shape::HLine);
        line->setFrameShadow(QFrame::Shadow::Sunken);

        verticalLayout->addWidget(line);

        enum_cam = new QPushButton(widget_3);
        enum_cam->setObjectName("enum_cam");
        enum_cam->setMinimumSize(QSize(0, 40));
        enum_cam->setCursor(QCursor(Qt::CursorShape::PointingHandCursor));
        enum_cam->setStyleSheet(QString::fromUtf8("QPushButton{\n"
"	border:4px outset gray;\n"
"	background-color:rgb(217, 217, 217);\n"
"	border-radius:6px;\n"
"	font:12pt \"\345\276\256\350\275\257\351\233\205\351\273\221\";\n"
"}\n"
"QPushButton::hover{\n"
"	border:4px groove gray;\n"
"	background-color:rgb(200, 200, 200);\n"
"	border-radius:6px;\n"
"	font:12pt \"\345\276\256\350\275\257\351\233\205\351\273\221\";\n"
"}"));

        verticalLayout->addWidget(enum_cam);

        camera_list = new QComboBox(widget_3);
        camera_list->setObjectName("camera_list");
        camera_list->setMinimumSize(QSize(0, 35));
        camera_list->setCursor(QCursor(Qt::CursorShape::PointingHandCursor));
        camera_list->setStyleSheet(QString::fromUtf8("QComboBox{ \n"
"	font:12pt \"\345\276\256\350\275\257\351\233\205\351\273\221\";\n"
"}"));

        verticalLayout->addWidget(camera_list);

        HC_machine = new QPushButton(widget_3);
        HC_machine->setObjectName("HC_machine");
        sizePolicy.setHeightForWidth(HC_machine->sizePolicy().hasHeightForWidth());
        HC_machine->setSizePolicy(sizePolicy);
        HC_machine->setMaximumSize(QSize(100, 200));
        HC_machine->setCursor(QCursor(Qt::CursorShape::PointingHandCursor));
        HC_machine->setStyleSheet(QString::fromUtf8("QPushButton{\n"
"background-color: rgb(217, 217, 217);\n"
"color: rgb(0,0,0);  \n"
"border-radius:8px;\n"
"font:12pt \"\345\276\256\350\275\257\351\233\205\351\273\221\";\n"
"border:5px outset gray;\n"
"}\n"
"QPushButton::hover{\n"
"background-color: rgb(200, 200, 200);\n"
"color: rgb(0,0,0);  \n"
"border-radius:8px;\n"
"font:12pt \"\345\276\256\350\275\257\351\233\205\351\273\221\";\n"
"border:5px groove gray;\n"
"}"));

        verticalLayout->addWidget(HC_machine);

        HC_data = new QPushButton(widget_3);
        HC_data->setObjectName("HC_data");
        HC_data->setMaximumSize(QSize(200, 200));
        HC_data->setCursor(QCursor(Qt::CursorShape::PointingHandCursor));
        HC_data->setStyleSheet(QString::fromUtf8("QPushButton{\n"
"background-color: rgb(217, 217, 217);\n"
"color: rgb(0,0,0);  \n"
"border-radius:8px;\n"
"font:12pt \"\345\276\256\350\275\257\351\233\205\351\273\221\";\n"
"border:5px outset gray;\n"
"}\n"
"QPushButton::hover{\n"
"background-color: rgb(200, 200, 200);\n"
"color: rgb(0,0,0);  \n"
"border-radius:8px;\n"
"font:12pt \"\345\276\256\350\275\257\351\233\205\351\273\221\";\n"
"border:5px groove gray;\n"
"}"));

        verticalLayout->addWidget(HC_data);

        HC_start = new QPushButton(widget_3);
        HC_start->setObjectName("HC_start");
        sizePolicy.setHeightForWidth(HC_start->sizePolicy().hasHeightForWidth());
        HC_start->setSizePolicy(sizePolicy);
        HC_start->setMaximumSize(QSize(200, 200));
        HC_start->setCursor(QCursor(Qt::CursorShape::PointingHandCursor));
        HC_start->setStyleSheet(QString::fromUtf8("QPushButton{\n"
"background-color: rgb(217, 217, 217);\n"
"color: rgb(0,0,0);  \n"
"border-radius:8px;\n"
"font:12pt \"\345\276\256\350\275\257\351\233\205\351\273\221\";\n"
"border:5px outset gray;\n"
"}\n"
"QPushButton::hover{\n"
"background-color: rgb(200, 200, 200);\n"
"color: rgb(0,0,0);  \n"
"border-radius:8px;\n"
"font:12pt \"\345\276\256\350\275\257\351\233\205\351\273\221\";\n"
"border:5px groove gray;\n"
"}"));

        verticalLayout->addWidget(HC_start);

        HC_stop = new QPushButton(widget_3);
        HC_stop->setObjectName("HC_stop");
        sizePolicy.setHeightForWidth(HC_stop->sizePolicy().hasHeightForWidth());
        HC_stop->setSizePolicy(sizePolicy);
        HC_stop->setMaximumSize(QSize(200, 200));
        HC_stop->setCursor(QCursor(Qt::CursorShape::PointingHandCursor));
        HC_stop->setStyleSheet(QString::fromUtf8("QPushButton{\n"
"background-color: rgb(217, 217, 217);\n"
"color: rgb(0,0,0);  \n"
"border-radius:8px;\n"
"font:12pt \"\345\276\256\350\275\257\351\233\205\351\273\221\";\n"
"border:5px outset gray;\n"
"}\n"
"QPushButton::hover{\n"
"background-color: rgb(200, 200, 200);\n"
"color: rgb(0,0,0);  \n"
"border-radius:8px;\n"
"font:12pt \"\345\276\256\350\275\257\351\233\205\351\273\221\";\n"
"border:5px groove gray;\n"
"}"));

        verticalLayout->addWidget(HC_stop);

        HC_RD = new QLabel(HC);
        HC_RD->setObjectName("HC_RD");
        HC_RD->setGeometry(QRect(130, 5, 613, 410));
        HC_RD->setStyleSheet(QString::fromUtf8("QLabel{\n"
"	border:1px solid gray;\n"
"}"));
        HC_PData = new QLabel(HC);
        HC_PData->setObjectName("HC_PData");
        HC_PData->setGeometry(QRect(748, 5, 406, 272));
        HC_PData->setStyleSheet(QString::fromUtf8("QLabel{\n"
"	border:1px solid gray;\n"
"}"));
        HC_Abrasion = new QLabel(HC);
        HC_Abrasion->setObjectName("HC_Abrasion");
        HC_Abrasion->setGeometry(QRect(748, 282, 406, 160));
        HC_Abrasion->setStyleSheet(QString::fromUtf8("QLabel{\n"
"	border:1px solid gray;\n"
"}"));
        HC_Misalignment = new QLabel(HC);
        HC_Misalignment->setObjectName("HC_Misalignment");
        HC_Misalignment->setGeometry(QRect(748, 447, 406, 160));
        HC_Misalignment->setStyleSheet(QString::fromUtf8("QLabel{\n"
"	border:1px solid gray;\n"
"}"));
        label_10 = new QLabel(HC);
        label_10->setObjectName("label_10");
        label_10->setGeometry(QRect(130, 5, 80, 20));
        label_10->setStyleSheet(QString::fromUtf8("QLabel{\n"
"	border:1px solid gray;\n"
"	font:10pt \"\345\276\256\350\275\257\351\233\205\351\273\221\";\n"
"	background-color:none;\n"
"}"));
        label_10->setAlignment(Qt::AlignCenter);
        label_11 = new QLabel(HC);
        label_11->setObjectName("label_11");
        label_11->setGeometry(QRect(748, 5, 80, 20));
        label_11->setStyleSheet(QString::fromUtf8("QLabel{\n"
"	border:1px solid gray;\n"
"	font:10pt \"\345\276\256\350\275\257\351\233\205\351\273\221\";\n"
"	background-color:none;\n"
"}"));
        label_11->setAlignment(Qt::AlignCenter);
        label_12 = new QLabel(HC);
        label_12->setObjectName("label_12");
        label_12->setGeometry(QRect(748, 282, 70, 20));
        label_12->setStyleSheet(QString::fromUtf8("QLabel{\n"
"	border:1px solid gray;\n"
"	font:10pt \"\345\276\256\350\275\257\351\233\205\351\273\221\";\n"
"	background-color:none;\n"
"}"));
        label_12->setAlignment(Qt::AlignCenter);
        label_13 = new QLabel(HC);
        label_13->setObjectName("label_13");
        label_13->setGeometry(QRect(748, 447, 100, 20));
        label_13->setStyleSheet(QString::fromUtf8("QLabel{\n"
"	border:1px solid gray;\n"
"	font:10pt \"\345\276\256\350\275\257\351\233\205\351\273\221\";\n"
"	background-color:none;\n"
"}"));
        label_13->setAlignment(Qt::AlignCenter);
        frame_3 = new QFrame(HC);
        frame_3->setObjectName("frame_3");
        frame_3->setGeometry(QRect(130, 420, 613, 187));
        frame_3->setStyleSheet(QString::fromUtf8("QFrame{\n"
"	border:1px solid gray;\n"
"}"));
        frame_3->setFrameShape(QFrame::StyledPanel);
        frame_3->setFrameShadow(QFrame::Raised);
        horizontalLayout = new QHBoxLayout(frame_3);
        horizontalLayout->setObjectName("horizontalLayout");
        frame_5 = new QFrame(frame_3);
        frame_5->setObjectName("frame_5");
        frame_5->setStyleSheet(QString::fromUtf8("QFrame{\n"
"	border:none;\n"
"}"));
        frame_5->setFrameShape(QFrame::StyledPanel);
        frame_5->setFrameShadow(QFrame::Raised);
        verticalLayout_5 = new QVBoxLayout(frame_5);
        verticalLayout_5->setSpacing(4);
        verticalLayout_5->setObjectName("verticalLayout_5");
        verticalLayout_5->setContentsMargins(-1, 15, -1, 15);
        label_3 = new QLabel(frame_5);
        label_3->setObjectName("label_3");
        label_3->setStyleSheet(QString::fromUtf8("QLabel{\n"
"	font:10pt\"\345\276\256\350\275\257\351\233\205\351\273\221\";\n"
"}"));

        verticalLayout_5->addWidget(label_3);

        label = new QLabel(frame_5);
        label->setObjectName("label");
        label->setStyleSheet(QString::fromUtf8("QLabel{\n"
"	font:10pt\"\345\276\256\350\275\257\351\233\205\351\273\221\";\n"
"}"));

        verticalLayout_5->addWidget(label);

        label_2 = new QLabel(frame_5);
        label_2->setObjectName("label_2");
        label_2->setStyleSheet(QString::fromUtf8("QLabel{\n"
"	font:10pt\"\345\276\256\350\275\257\351\233\205\351\273\221\";\n"
"}"));

        verticalLayout_5->addWidget(label_2);


        horizontalLayout->addWidget(frame_5);

        frame_7 = new QFrame(frame_3);
        frame_7->setObjectName("frame_7");
        frame_7->setStyleSheet(QString::fromUtf8("QFrame{\n"
"	border:none;\n"
"}"));
        frame_7->setFrameShape(QFrame::StyledPanel);
        frame_7->setFrameShadow(QFrame::Raised);
        verticalLayout_6 = new QVBoxLayout(frame_7);
        verticalLayout_6->setObjectName("verticalLayout_6");
        FrameRate = new QLineEdit(frame_7);
        FrameRate->setObjectName("FrameRate");
        FrameRate->setStyleSheet(QString::fromUtf8("QLineEdit{\n"
"	font:10pt\"\345\276\256\350\275\257\351\233\205\351\273\221\";\n"
"}"));

        verticalLayout_6->addWidget(FrameRate);

        ExposureTime = new QLineEdit(frame_7);
        ExposureTime->setObjectName("ExposureTime");
        ExposureTime->setStyleSheet(QString::fromUtf8("QLineEdit{\n"
"	font:10pt\"\345\276\256\350\275\257\351\233\205\351\273\221\";\n"
"}"));

        verticalLayout_6->addWidget(ExposureTime);

        Gain = new QLineEdit(frame_7);
        Gain->setObjectName("Gain");
        Gain->setStyleSheet(QString::fromUtf8("QLineEdit{\n"
"	font:10pt\"\345\276\256\350\275\257\351\233\205\351\273\221\";\n"
"}"));

        verticalLayout_6->addWidget(Gain);


        horizontalLayout->addWidget(frame_7);

        frame_8 = new QFrame(frame_3);
        frame_8->setObjectName("frame_8");
        frame_8->setStyleSheet(QString::fromUtf8("QFrame{\n"
"	border:none;\n"
"}"));
        frame_8->setFrameShape(QFrame::StyledPanel);
        frame_8->setFrameShadow(QFrame::Raised);
        verticalLayout_7 = new QVBoxLayout(frame_8);
        verticalLayout_7->setObjectName("verticalLayout_7");
        get_parameter = new QPushButton(frame_8);
        get_parameter->setObjectName("get_parameter");
        get_parameter->setCursor(QCursor(Qt::CursorShape::PointingHandCursor));
        get_parameter->setStyleSheet(QString::fromUtf8("QPushButton{\n"
"background-color: rgb(217, 217, 217);\n"
"color: rgb(0,0,0);  \n"
"font:10pt \"\345\276\256\350\275\257\351\233\205\351\273\221\";\n"
"}\n"
"QPushButton::hover{\n"
"background-color: rgb(200, 200, 200);\n"
"color: rgb(0,0,0);  \n"
"font:10pt \"\345\276\256\350\275\257\351\233\205\351\273\221\";\n"
"}"));

        verticalLayout_7->addWidget(get_parameter);

        set_parameter = new QPushButton(frame_8);
        set_parameter->setObjectName("set_parameter");
        set_parameter->setCursor(QCursor(Qt::CursorShape::PointingHandCursor));
        set_parameter->setStyleSheet(QString::fromUtf8("QPushButton{\n"
"background-color: rgb(217, 217, 217);\n"
"color: rgb(0,0,0);  \n"
"font:10pt \"\345\276\256\350\275\257\351\233\205\351\273\221\";\n"
"}\n"
"QPushButton::hover{\n"
"background-color: rgb(200, 200, 200);\n"
"color: rgb(0,0,0);  \n"
"font:10pt \"\345\276\256\350\275\257\351\233\205\351\273\221\";\n"
"}"));

        verticalLayout_7->addWidget(set_parameter);


        horizontalLayout->addWidget(frame_8);

        tabWidget->addTab(HC, QString());
        HC_Misalignment->raise();
        HC_Abrasion->raise();
        HC_PData->raise();
        HC_RD->raise();
        widget_3->raise();
        label_10->raise();
        label_12->raise();
        label_11->raise();
        label_13->raise();
        frame_3->raise();
        TIC = new QWidget();
        TIC->setObjectName("TIC");
        TIC_RD = new QLabel(TIC);
        TIC_RD->setObjectName("TIC_RD");
        TIC_RD->setGeometry(QRect(5, 90, 691, 518));
        TIC_RD->setStyleSheet(QString::fromUtf8("QLabel{\n"
"	border:1px outset gray;\n"
"}"));
        TIC_PD = new QLabel(TIC);
        TIC_PD->setObjectName("TIC_PD");
        TIC_PD->setGeometry(QRect(701, 158, 453, 450));
        TIC_PD->setStyleSheet(QString::fromUtf8("QLabel{\n"
"	border:1px outset gray;\n"
"}\n"
""));
        frame = new QFrame(TIC);
        frame->setObjectName("frame");
        frame->setGeometry(QRect(130, 635, 560, 40));
        frame->setStyleSheet(QString::fromUtf8("QFrame{\n"
"	border:1px outset gray;\n"
"}"));
        frame->setFrameShape(QFrame::StyledPanel);
        frame->setFrameShadow(QFrame::Raised);
        horizontalSlider = new QSlider(frame);
        horizontalSlider->setObjectName("horizontalSlider");
        horizontalSlider->setGeometry(QRect(315, 6, 235, 30));
        horizontalSlider->setCursor(QCursor(Qt::CursorShape::PointingHandCursor));
        horizontalSlider->setStyleSheet(QString::fromUtf8("QSlider{\n"
"}"));
        horizontalSlider->setMinimum(-50);
        horizontalSlider->setMaximum(50);
        horizontalSlider->setSliderPosition(0);
        horizontalSlider->setTracking(true);
        horizontalSlider->setOrientation(Qt::Horizontal);
        radioButton = new QRadioButton(frame);
        radioButton->setObjectName("radioButton");
        radioButton->setGeometry(QRect(105, 5, 65, 30));
        radioButton->setStyleSheet(QString::fromUtf8("QRadioButton{\n"
"	font:14pt \"\345\276\256\350\275\257\351\233\205\351\273\221\";\n"
"}"));
        radioButton->setChecked(false);
        label_7 = new QLabel(frame);
        label_7->setObjectName("label_7");
        label_7->setGeometry(QRect(5, 5, 100, 30));
        label_7->setStyleSheet(QString::fromUtf8("QLabel{\n"
"	border:none;\n"
"}"));
        radioButton_2 = new QRadioButton(frame);
        radioButton_2->setObjectName("radioButton_2");
        radioButton_2->setGeometry(QRect(175, 5, 70, 30));
        radioButton_2->setStyleSheet(QString::fromUtf8("QRadioButton{\n"
"	font:14pt \"\345\276\256\350\275\257\351\233\205\351\273\221\";\n"
"}"));
        radioButton_2->setChecked(true);
        radioButton_3 = new QRadioButton(frame);
        radioButton_3->setObjectName("radioButton_3");
        radioButton_3->setGeometry(QRect(250, 5, 60, 30));
        radioButton_3->setStyleSheet(QString::fromUtf8("QRadioButton{\n"
"	font:14pt \"\345\276\256\350\275\257\351\233\205\351\273\221\";\n"
"}"));
        label_14 = new QLabel(TIC);
        label_14->setObjectName("label_14");
        label_14->setGeometry(QRect(5, 90, 80, 20));
        label_14->setStyleSheet(QString::fromUtf8("QLabel{\n"
"	border:1px solid gray;\n"
"	font:10pt \"\345\276\256\350\275\257\351\233\205\351\273\221\";\n"
"	background-color:none;\n"
"}"));
        label_14->setAlignment(Qt::AlignCenter);
        label_15 = new QLabel(TIC);
        label_15->setObjectName("label_15");
        label_15->setGeometry(QRect(701, 158, 80, 20));
        label_15->setStyleSheet(QString::fromUtf8("QLabel{\n"
"	border:1px solid gray;\n"
"	font:10pt \"\345\276\256\350\275\257\351\233\205\351\273\221\";\n"
"	background-color:none;\n"
"}"));
        label_15->setAlignment(Qt::AlignCenter);
        textBrowser = new QTextBrowser(TIC);
        textBrowser->setObjectName("textBrowser");
        textBrowser->setGeometry(QRect(5, 5, 390, 80));
        textBrowser->setStyleSheet(QString::fromUtf8("QTextBrowser{\n"
"    font:8pt\"\345\276\256\350\275\257\351\233\205\351\273\221\";\n"
"}"));
        TIC_control = new QFrame(TIC);
        TIC_control->setObjectName("TIC_control");
        TIC_control->setGeometry(QRect(701, 5, 454, 147));
        TIC_control->setStyleSheet(QString::fromUtf8("QFrame{\n"
"	border:1px solid gray;\n"
"}"));
        TIC_control->setFrameShape(QFrame::StyledPanel);
        TIC_control->setFrameShadow(QFrame::Raised);
        verticalLayout_10 = new QVBoxLayout(TIC_control);
        verticalLayout_10->setSpacing(0);
        verticalLayout_10->setObjectName("verticalLayout_10");
        verticalLayout_10->setContentsMargins(0, 0, 0, 0);
        label_17 = new QLabel(TIC_control);
        label_17->setObjectName("label_17");
        label_17->setStyleSheet(QString::fromUtf8("QLabel{\n"
"	font:12pt\"\345\276\256\350\275\257\351\233\205\351\273\221\";\n"
"	border:none;\n"
"	border-bottom:1px solid gray;\n"
"}"));
        label_17->setAlignment(Qt::AlignCenter);

        verticalLayout_10->addWidget(label_17);

        horizontalLayout_3 = new QHBoxLayout();
        horizontalLayout_3->setSpacing(5);
        horizontalLayout_3->setObjectName("horizontalLayout_3");
        horizontalLayout_3->setContentsMargins(10, -1, 10, -1);
        frame_9 = new QFrame(TIC_control);
        frame_9->setObjectName("frame_9");
        frame_9->setStyleSheet(QString::fromUtf8("QFrame{\n"
"	border:none;\n"
"}"));
        frame_9->setFrameShape(QFrame::StyledPanel);
        frame_9->setFrameShadow(QFrame::Raised);
        verticalLayout_2 = new QVBoxLayout(frame_9);
        verticalLayout_2->setSpacing(4);
        verticalLayout_2->setObjectName("verticalLayout_2");
        verticalLayout_2->setContentsMargins(0, 1, 4, 0);
        TIC_machine = new QPushButton(frame_9);
        TIC_machine->setObjectName("TIC_machine");
        sizePolicy.setHeightForWidth(TIC_machine->sizePolicy().hasHeightForWidth());
        TIC_machine->setSizePolicy(sizePolicy);
        TIC_machine->setMinimumSize(QSize(0, 30));
        TIC_machine->setCursor(QCursor(Qt::CursorShape::PointingHandCursor));
        TIC_machine->setStyleSheet(QString::fromUtf8("QPushButton{\n"
"background-color: rgb(217, 217, 217);\n"
"color: rgb(0,0,0);  \n"
"font:10pt \"\345\276\256\350\275\257\351\233\205\351\273\221\";\n"
"border:2px outset gray;\n"
"}\n"
"QPushButton::hover{\n"
"background-color: rgb(200, 200, 200);\n"
"color: rgb(0,0,0);  \n"
"font:10pt \"\345\276\256\350\275\257\351\233\205\351\273\221\";\n"
"border:2px groove gray;\n"
"}"));

        verticalLayout_2->addWidget(TIC_machine);

        TIC_data = new QPushButton(frame_9);
        TIC_data->setObjectName("TIC_data");
        sizePolicy.setHeightForWidth(TIC_data->sizePolicy().hasHeightForWidth());
        TIC_data->setSizePolicy(sizePolicy);
        TIC_data->setMinimumSize(QSize(0, 30));
        TIC_data->setCursor(QCursor(Qt::CursorShape::PointingHandCursor));
        TIC_data->setStyleSheet(QString::fromUtf8("QPushButton{\n"
"background-color: rgb(217, 217, 217);\n"
"color: rgb(0,0,0);  \n"
"font:10pt \"\345\276\256\350\275\257\351\233\205\351\273\221\";\n"
"border:2px outset gray;\n"
"}\n"
"QPushButton::hover{\n"
"background-color: rgb(200, 200, 200);\n"
"color: rgb(0,0,0);  \n"
"font:10pt \"\345\276\256\350\275\257\351\233\205\351\273\221\";\n"
"border:2px groove gray;\n"
"}"));

        verticalLayout_2->addWidget(TIC_data);


        horizontalLayout_3->addWidget(frame_9);

        frame_10 = new QFrame(TIC_control);
        frame_10->setObjectName("frame_10");
        frame_10->setStyleSheet(QString::fromUtf8("QFrame{\n"
"	border:none;\n"
"}"));
        frame_10->setFrameShape(QFrame::StyledPanel);
        frame_10->setFrameShadow(QFrame::Raised);
        verticalLayout_8 = new QVBoxLayout(frame_10);
        verticalLayout_8->setSpacing(4);
        verticalLayout_8->setObjectName("verticalLayout_8");
        verticalLayout_8->setContentsMargins(4, 1, 0, 0);
        TIC_start = new QPushButton(frame_10);
        TIC_start->setObjectName("TIC_start");
        sizePolicy.setHeightForWidth(TIC_start->sizePolicy().hasHeightForWidth());
        TIC_start->setSizePolicy(sizePolicy);
        TIC_start->setMinimumSize(QSize(0, 30));
        TIC_start->setCursor(QCursor(Qt::CursorShape::PointingHandCursor));
        TIC_start->setStyleSheet(QString::fromUtf8("QPushButton{\n"
"background-color: rgb(217, 217, 217);\n"
"color: rgb(0,0,0);  \n"
"font:10pt \"\345\276\256\350\275\257\351\233\205\351\273\221\";\n"
"border:2px outset gray;\n"
"}\n"
"QPushButton::hover{\n"
"background-color: rgb(200, 200, 200);\n"
"color: rgb(0,0,0);  \n"
"font:10pt \"\345\276\256\350\275\257\351\233\205\351\273\221\";\n"
"border:2px groove gray;\n"
"}"));

        verticalLayout_8->addWidget(TIC_start);

        TIC_stop = new QPushButton(frame_10);
        TIC_stop->setObjectName("TIC_stop");
        sizePolicy.setHeightForWidth(TIC_stop->sizePolicy().hasHeightForWidth());
        TIC_stop->setSizePolicy(sizePolicy);
        TIC_stop->setMinimumSize(QSize(0, 30));
        TIC_stop->setCursor(QCursor(Qt::CursorShape::PointingHandCursor));
        TIC_stop->setStyleSheet(QString::fromUtf8("QPushButton{\n"
"background-color: rgb(217, 217, 217);\n"
"color: rgb(0,0,0);  \n"
"font:10pt \"\345\276\256\350\275\257\351\233\205\351\273\221\";\n"
"border:2px outset gray;\n"
"}\n"
"QPushButton::hover{\n"
"background-color: rgb(200, 200, 200);\n"
"color: rgb(0,0,0);  \n"
"font:10pt \"\345\276\256\350\275\257\351\233\205\351\273\221\";\n"
"border:2px groove gray;\n"
"}"));

        verticalLayout_8->addWidget(TIC_stop);


        horizontalLayout_3->addWidget(frame_10);


        verticalLayout_10->addLayout(horizontalLayout_3);

        verticalLayout_10->setStretch(0, 2);
        verticalLayout_10->setStretch(1, 4);
        label_logbrowser = new QLabel(TIC);
        label_logbrowser->setObjectName("label_logbrowser");
        label_logbrowser->setGeometry(QRect(305, 5, 90, 20));
        label_logbrowser->setStyleSheet(QString::fromUtf8("QLabel{\n"
"	border:1px solid gray;\n"
"	font:10pt \"\345\276\256\350\275\257\351\233\205\351\273\221\";\n"
"	background-color:none;\n"
"}"));
        label_logbrowser->setAlignment(Qt::AlignCenter);
        closeloop_speed = new QFrame(TIC);
        closeloop_speed->setObjectName("closeloop_speed");
        closeloop_speed->setGeometry(QRect(400, 5, 296, 80));
        closeloop_speed->setStyleSheet(QString::fromUtf8("QFrame{\n"
"	font:10pt\"\345\276\256\350\275\257\351\233\205\351\273\221\";\n"
"	border:1px solid gray;\n"
"}"));
        closeloop_speed->setFrameShape(QFrame::StyledPanel);
        closeloop_speed->setFrameShadow(QFrame::Raised);
        verticalLayout_11 = new QVBoxLayout(closeloop_speed);
        verticalLayout_11->setSpacing(5);
        verticalLayout_11->setObjectName("verticalLayout_11");
        verticalLayout_11->setContentsMargins(0, 0, 0, 5);
        horizontalLayout_5 = new QHBoxLayout();
        horizontalLayout_5->setSpacing(5);
        horizontalLayout_5->setObjectName("horizontalLayout_5");
        horizontalLayout_5->setContentsMargins(10, -1, -1, -1);
        label_16 = new QLabel(closeloop_speed);
        label_16->setObjectName("label_16");
        label_16->setStyleSheet(QString::fromUtf8("QLabel{\n"
"	border:none;\n"
"}"));

        horizontalLayout_5->addWidget(label_16);

        frame_11 = new QFrame(closeloop_speed);
        frame_11->setObjectName("frame_11");
        frame_11->setStyleSheet(QString::fromUtf8("QFrame{\n"
"	border:none;\n"
"\n"
"}"));
        frame_11->setFrameShape(QFrame::StyledPanel);
        frame_11->setFrameShadow(QFrame::Raised);
        horizontalLayout_4 = new QHBoxLayout(frame_11);
        horizontalLayout_4->setObjectName("horizontalLayout_4");
        radioButton_7 = new QRadioButton(frame_11);
        radioButton_7->setObjectName("radioButton_7");
        radioButton_7->setCursor(QCursor(Qt::CursorShape::PointingHandCursor));
        radioButton_7->setStyleSheet(QString::fromUtf8("font:10pt\"\345\276\256\350\275\257\351\233\205\351\273\221\";"));

        horizontalLayout_4->addWidget(radioButton_7);

        radioButton_8 = new QRadioButton(frame_11);
        radioButton_8->setObjectName("radioButton_8");
        radioButton_8->setCursor(QCursor(Qt::CursorShape::PointingHandCursor));
        radioButton_8->setStyleSheet(QString::fromUtf8("font:10pt\"\345\276\256\350\275\257\351\233\205\351\273\221\";"));

        horizontalLayout_4->addWidget(radioButton_8);

        radioButton_9 = new QRadioButton(frame_11);
        radioButton_9->setObjectName("radioButton_9");
        radioButton_9->setCursor(QCursor(Qt::CursorShape::PointingHandCursor));
        radioButton_9->setStyleSheet(QString::fromUtf8("font:10pt\"\345\276\256\350\275\257\351\233\205\351\273\221\";"));

        horizontalLayout_4->addWidget(radioButton_9);


        horizontalLayout_5->addWidget(frame_11);

        horizontalLayout_5->setStretch(0, 2);
        horizontalLayout_5->setStretch(1, 5);

        verticalLayout_11->addLayout(horizontalLayout_5);

        horizontalSlider_3 = new QSlider(closeloop_speed);
        horizontalSlider_3->setObjectName("horizontalSlider_3");
        horizontalSlider_3->setCursor(QCursor(Qt::CursorShape::ClosedHandCursor));
        horizontalSlider_3->setSliderPosition(50);
        horizontalSlider_3->setOrientation(Qt::Horizontal);

        verticalLayout_11->addWidget(horizontalSlider_3);

        tabWidget->addTab(TIC, QString());
        TIC_RD->raise();
        frame->raise();
        label_14->raise();
        TIC_PD->raise();
        label_15->raise();
        textBrowser->raise();
        TIC_control->raise();
        label_logbrowser->raise();
        closeloop_speed->raise();
        FC = new QWidget();
        FC->setObjectName("FC");
        widget_5 = new QWidget(FC);
        widget_5->setObjectName("widget_5");
        widget_5->setGeometry(QRect(5, 5, 120, 602));
        widget_5->setStyleSheet(QString::fromUtf8("QWidget{\n"
"	border:1px outset gray;\n"
"}\n"
""));
        verticalLayout_3 = new QVBoxLayout(widget_5);
        verticalLayout_3->setSpacing(10);
        verticalLayout_3->setObjectName("verticalLayout_3");
        verticalLayout_3->setContentsMargins(10, 10, 10, 10);
        label_30 = new QLabel(widget_5);
        label_30->setObjectName("label_30");
        label_30->setStyleSheet(QString::fromUtf8("QLabel{\n"
"	font:12pt\"\345\276\256\350\275\257\351\233\205\351\273\221\";\n"
"	border:none;\n"
"}"));
        label_30->setAlignment(Qt::AlignCenter);

        verticalLayout_3->addWidget(label_30);

        line_2 = new QFrame(widget_5);
        line_2->setObjectName("line_2");
        line_2->setFrameShape(QFrame::Shape::HLine);
        line_2->setFrameShadow(QFrame::Shadow::Sunken);

        verticalLayout_3->addWidget(line_2);

        FC_machine = new QPushButton(widget_5);
        FC_machine->setObjectName("FC_machine");
        sizePolicy.setHeightForWidth(FC_machine->sizePolicy().hasHeightForWidth());
        FC_machine->setSizePolicy(sizePolicy);
        FC_machine->setMinimumSize(QSize(0, 50));
        FC_machine->setMaximumSize(QSize(200, 200));
        FC_machine->setCursor(QCursor(Qt::CursorShape::PointingHandCursor));
        FC_machine->setStyleSheet(QString::fromUtf8("QPushButton{\n"
"background-color: rgb(217, 217, 217);\n"
"color: rgb(0,0,0);  \n"
"border-radius:8px;\n"
"font:12pt \"\345\276\256\350\275\257\351\233\205\351\273\221\";\n"
"border:5px outset gray;\n"
"}\n"
"QPushButton::hover{\n"
"background-color: rgb(200, 200, 200);\n"
"color: rgb(0,0,0);  \n"
"border-radius:8px;\n"
"font:12pt \"\345\276\256\350\275\257\351\233\205\351\273\221\";\n"
"border:5px groove gray;\n"
"}"));

        verticalLayout_3->addWidget(FC_machine);

        FC_data = new QPushButton(widget_5);
        FC_data->setObjectName("FC_data");
        sizePolicy.setHeightForWidth(FC_data->sizePolicy().hasHeightForWidth());
        FC_data->setSizePolicy(sizePolicy);
        FC_data->setMaximumSize(QSize(200, 200));
        FC_data->setCursor(QCursor(Qt::CursorShape::PointingHandCursor));
        FC_data->setStyleSheet(QString::fromUtf8("QPushButton{\n"
"background-color: rgb(217, 217, 217);\n"
"color: rgb(0,0,0);  \n"
"border-radius:8px;\n"
"font:12pt \"\345\276\256\350\275\257\351\233\205\351\273\221\";\n"
"border:5px outset gray;\n"
"}\n"
"QPushButton::hover{\n"
"background-color: rgb(200, 200, 200);\n"
"color: rgb(0,0,0);  \n"
"border-radius:8px;\n"
"font:12pt \"\345\276\256\350\275\257\351\233\205\351\273\221\";\n"
"border:5px groove gray;\n"
"}"));

        verticalLayout_3->addWidget(FC_data);

        FC_start = new QPushButton(widget_5);
        FC_start->setObjectName("FC_start");
        sizePolicy.setHeightForWidth(FC_start->sizePolicy().hasHeightForWidth());
        FC_start->setSizePolicy(sizePolicy);
        FC_start->setMaximumSize(QSize(200, 200));
        FC_start->setCursor(QCursor(Qt::CursorShape::PointingHandCursor));
        FC_start->setStyleSheet(QString::fromUtf8("QPushButton{\n"
"background-color: rgb(217, 217, 217);\n"
"color: rgb(0,0,0);  \n"
"border-radius:8px;\n"
"font:12pt \"\345\276\256\350\275\257\351\233\205\351\273\221\";\n"
"border:5px outset gray;\n"
"}\n"
"QPushButton::hover{\n"
"background-color: rgb(200, 200, 200);\n"
"color: rgb(0,0,0);  \n"
"border-radius:8px;\n"
"font:12pt \"\345\276\256\350\275\257\351\233\205\351\273\221\";\n"
"border:5px groove gray;\n"
"}"));

        verticalLayout_3->addWidget(FC_start);

        FC_stop = new QPushButton(widget_5);
        FC_stop->setObjectName("FC_stop");
        sizePolicy.setHeightForWidth(FC_stop->sizePolicy().hasHeightForWidth());
        FC_stop->setSizePolicy(sizePolicy);
        FC_stop->setMaximumSize(QSize(200, 200));
        FC_stop->setCursor(QCursor(Qt::CursorShape::PointingHandCursor));
        FC_stop->setStyleSheet(QString::fromUtf8("QPushButton{\n"
"background-color: rgb(217, 217, 217);\n"
"color: rgb(0,0,0);  \n"
"border-radius:8px;\n"
"font:12pt \"\345\276\256\350\275\257\351\233\205\351\273\221\";\n"
"border:5px outset gray;\n"
"}\n"
"QPushButton::hover{\n"
"background-color: rgb(200, 200, 200);\n"
"color: rgb(0,0,0);  \n"
"border-radius:8px;\n"
"font:12pt \"\345\276\256\350\275\257\351\233\205\351\273\221\";\n"
"border:5px groove gray;\n"
"}"));

        verticalLayout_3->addWidget(FC_stop);

        verticalLayout_3->setStretch(0, 2);
        verticalLayout_3->setStretch(2, 4);
        verticalLayout_3->setStretch(3, 4);
        verticalLayout_3->setStretch(4, 4);
        verticalLayout_3->setStretch(5, 4);
        FC_RD = new QLabel(FC);
        FC_RD->setObjectName("FC_RD");
        FC_RD->setGeometry(QRect(130, 5, 590, 286));
        FC_RD->setStyleSheet(QString::fromUtf8("QLabel{\n"
"	border:1px outset gray;\n"
"}"));
        FC_VV = new QLabel(FC);
        FC_VV->setObjectName("FC_VV");
        FC_VV->setGeometry(QRect(725, 5, 430, 320));
        FC_VV->setStyleSheet(QString::fromUtf8("QLabel{\n"
"	border:1px outset gray;\n"
"}\n"
""));
        FC_VAR = new QLabel(FC);
        FC_VAR->setObjectName("FC_VAR");
        FC_VAR->setGeometry(QRect(130, 330, 338, 277));
        FC_VAR->setStyleSheet(QString::fromUtf8("QLabel{\n"
"	border:1px outset gray;\n"
"}\n"
""));
        FC_SD = new QLabel(FC);
        FC_SD->setObjectName("FC_SD");
        FC_SD->setGeometry(QRect(473, 330, 338, 277));
        FC_SD->setStyleSheet(QString::fromUtf8("QLabel{\n"
"	border:1px outset gray;\n"
"}"));
        FC_PEAK = new QLabel(FC);
        FC_PEAK->setObjectName("FC_PEAK");
        FC_PEAK->setGeometry(QRect(816, 330, 339, 277));
        FC_PEAK->setStyleSheet(QString::fromUtf8("QLabel{\n"
"	border:1px outset gray;\n"
"}"));
        label_18 = new QLabel(FC);
        label_18->setObjectName("label_18");
        label_18->setGeometry(QRect(130, 5, 80, 20));
        label_18->setStyleSheet(QString::fromUtf8("QLabel{\n"
"	border:1px solid gray;\n"
"	font:10pt \"\345\276\256\350\275\257\351\233\205\351\273\221\";\n"
"	background-color:none;\n"
"}"));
        label_18->setAlignment(Qt::AlignCenter);
        label_22 = new QLabel(FC);
        label_22->setObjectName("label_22");
        label_22->setGeometry(QRect(725, 5, 60, 20));
        label_22->setStyleSheet(QString::fromUtf8("QLabel{\n"
"	border:1px solid gray;\n"
"	font:10pt \"\345\276\256\350\275\257\351\233\205\351\273\221\";\n"
"	background-color:none;\n"
"}"));
        label_22->setAlignment(Qt::AlignCenter);
        label_23 = new QLabel(FC);
        label_23->setObjectName("label_23");
        label_23->setGeometry(QRect(130, 330, 60, 20));
        label_23->setStyleSheet(QString::fromUtf8("QLabel{\n"
"	border:1px solid gray;\n"
"	font:10pt \"\345\276\256\350\275\257\351\233\205\351\273\221\";\n"
"	background-color:none;\n"
"	alignment:center;\n"
"}"));
        label_23->setAlignment(Qt::AlignCenter);
        label_24 = new QLabel(FC);
        label_24->setObjectName("label_24");
        label_24->setGeometry(QRect(473, 330, 70, 20));
        label_24->setStyleSheet(QString::fromUtf8("QLabel{\n"
"	border:1px solid gray;\n"
"	font:10pt \"\345\276\256\350\275\257\351\233\205\351\273\221\";\n"
"	background-color:none;\n"
"}"));
        label_24->setAlignment(Qt::AlignCenter);
        label_25 = new QLabel(FC);
        label_25->setObjectName("label_25");
        label_25->setGeometry(QRect(816, 330, 60, 20));
        label_25->setStyleSheet(QString::fromUtf8("QLabel{\n"
"	border:1px solid gray;\n"
"	font:10pt \"\345\276\256\350\275\257\351\233\205\351\273\221\";\n"
"	background-color:none;\n"
"}"));
        label_25->setAlignment(Qt::AlignCenter);
        frame_2 = new QFrame(FC);
        frame_2->setObjectName("frame_2");
        frame_2->setGeometry(QRect(130, 290, 590, 36));
        frame_2->setCursor(QCursor(Qt::CursorShape::ArrowCursor));
        frame_2->setStyleSheet(QString::fromUtf8("QFrame{\n"
"	border:1px outset gray;\n"
"}"));
        frame_2->setFrameShape(QFrame::StyledPanel);
        frame_2->setFrameShadow(QFrame::Raised);
        horizontalLayout_2 = new QHBoxLayout(frame_2);
        horizontalLayout_2->setObjectName("horizontalLayout_2");
        horizontalLayout_2->setContentsMargins(10, 0, 10, 0);
        label_26 = new QLabel(frame_2);
        label_26->setObjectName("label_26");
        QFont font2;
        font2.setFamilies({QString::fromUtf8("\345\276\256\350\275\257\351\233\205\351\273\221")});
        font2.setPointSize(10);
        font2.setBold(false);
        font2.setItalic(false);
        label_26->setFont(font2);
        label_26->setStyleSheet(QString::fromUtf8("QLabel{\n"
"	border:none;\n"
"    font:10pt \"\345\276\256\350\275\257\351\233\205\351\273\221\";\n"
"}"));

        horizontalLayout_2->addWidget(label_26);

        radioButton_4 = new QRadioButton(frame_2);
        radioButton_4->setObjectName("radioButton_4");
        radioButton_4->setMinimumSize(QSize(60, 0));
        radioButton_4->setCursor(QCursor(Qt::CursorShape::PointingHandCursor));
        radioButton_4->setStyleSheet(QString::fromUtf8("QRadioButton{\n"
"	border:none;\n"
"    font:10pt \"\345\276\256\350\275\257\351\233\205\351\273\221\";\n"
"}"));

        horizontalLayout_2->addWidget(radioButton_4);

        radioButton_5 = new QRadioButton(frame_2);
        radioButton_5->setObjectName("radioButton_5");
        radioButton_5->setMinimumSize(QSize(60, 0));
        radioButton_5->setCursor(QCursor(Qt::CursorShape::PointingHandCursor));
        radioButton_5->setStyleSheet(QString::fromUtf8("QRadioButton{\n"
"	border:none;\n"
"    font:10pt \"\345\276\256\350\275\257\351\233\205\351\273\221\";\n"
"}"));

        horizontalLayout_2->addWidget(radioButton_5);

        radioButton_6 = new QRadioButton(frame_2);
        radioButton_6->setObjectName("radioButton_6");
        radioButton_6->setMinimumSize(QSize(60, 0));
        radioButton_6->setCursor(QCursor(Qt::CursorShape::PointingHandCursor));
        radioButton_6->setStyleSheet(QString::fromUtf8("QRadioButton{\n"
"	border:none;\n"
"    font:10pt \"\345\276\256\350\275\257\351\233\205\351\273\221\";\n"
"}"));

        horizontalLayout_2->addWidget(radioButton_6);

        horizontalSlider_2 = new QSlider(frame_2);
        horizontalSlider_2->setObjectName("horizontalSlider_2");
        horizontalSlider_2->setCursor(QCursor(Qt::CursorShape::ClosedHandCursor));
        horizontalSlider_2->setStyleSheet(QString::fromUtf8("QSlider{\n"
"}"));
        horizontalSlider_2->setMinimum(-50);
        horizontalSlider_2->setMaximum(50);
        horizontalSlider_2->setSliderPosition(0);
        horizontalSlider_2->setTracking(true);
        horizontalSlider_2->setOrientation(Qt::Horizontal);

        horizontalLayout_2->addWidget(horizontalSlider_2);

        horizontalLayout_2->setStretch(0, 1);
        horizontalLayout_2->setStretch(1, 1);
        horizontalLayout_2->setStretch(2, 1);
        horizontalLayout_2->setStretch(3, 1);
        horizontalLayout_2->setStretch(4, 9);
        tabWidget->addTab(FC, QString());
        LL = new QWidget();
        LL->setObjectName("LL");
        widget_6 = new QWidget(LL);
        widget_6->setObjectName("widget_6");
        widget_6->setGeometry(QRect(5, 5, 120, 600));
        widget_6->setStyleSheet(QString::fromUtf8("QWidget{\n"
"	border:1px outset gray;\n"
"}"));
        verticalLayout_4 = new QVBoxLayout(widget_6);
        verticalLayout_4->setSpacing(10);
        verticalLayout_4->setObjectName("verticalLayout_4");
        verticalLayout_4->setContentsMargins(10, 10, 10, 10);
        label_35 = new QLabel(widget_6);
        label_35->setObjectName("label_35");
        label_35->setStyleSheet(QString::fromUtf8("QLabel{\n"
"	font:12pt\"\345\276\256\350\275\257\351\233\205\351\273\221\";\n"
"	border:none;\n"
"}"));
        label_35->setAlignment(Qt::AlignCenter);

        verticalLayout_4->addWidget(label_35);

        line_3 = new QFrame(widget_6);
        line_3->setObjectName("line_3");
        line_3->setFrameShape(QFrame::Shape::HLine);
        line_3->setFrameShadow(QFrame::Shadow::Sunken);

        verticalLayout_4->addWidget(line_3);

        LL_machine = new QPushButton(widget_6);
        LL_machine->setObjectName("LL_machine");
        sizePolicy.setHeightForWidth(LL_machine->sizePolicy().hasHeightForWidth());
        LL_machine->setSizePolicy(sizePolicy);
        LL_machine->setMaximumSize(QSize(200, 200));
        LL_machine->setCursor(QCursor(Qt::CursorShape::PointingHandCursor));
        LL_machine->setStyleSheet(QString::fromUtf8("QPushButton{\n"
"background-color: rgb(217, 217, 217);\n"
"color: rgb(0,0,0);  \n"
"border-radius:8px;\n"
"font:12pt \"\345\276\256\350\275\257\351\233\205\351\273\221\";\n"
"border:5px outset gray;\n"
"}\n"
"QPushButton::hover{\n"
"background-color: rgb(200, 200, 200);\n"
"color: rgb(0,0,0);  \n"
"border-radius:8px;\n"
"font:12pt \"\345\276\256\350\275\257\351\233\205\351\273\221\";\n"
"border:5px groove gray;\n"
"}"));

        verticalLayout_4->addWidget(LL_machine);

        LL_data = new QPushButton(widget_6);
        LL_data->setObjectName("LL_data");
        sizePolicy.setHeightForWidth(LL_data->sizePolicy().hasHeightForWidth());
        LL_data->setSizePolicy(sizePolicy);
        LL_data->setMaximumSize(QSize(200, 200));
        LL_data->setCursor(QCursor(Qt::CursorShape::PointingHandCursor));
        LL_data->setStyleSheet(QString::fromUtf8("QPushButton{\n"
"background-color: rgb(217, 217, 217);\n"
"color: rgb(0,0,0);  \n"
"border-radius:8px;\n"
"font:12pt \"\345\276\256\350\275\257\351\233\205\351\273\221\";\n"
"border:5px outset gray;\n"
"}\n"
"QPushButton::hover{\n"
"background-color: rgb(200, 200, 200);\n"
"color: rgb(0,0,0);  \n"
"border-radius:8px;\n"
"font:12pt \"\345\276\256\350\275\257\351\233\205\351\273\221\";\n"
"border:5px groove gray;\n"
"}"));

        verticalLayout_4->addWidget(LL_data);

        LL_start = new QPushButton(widget_6);
        LL_start->setObjectName("LL_start");
        sizePolicy.setHeightForWidth(LL_start->sizePolicy().hasHeightForWidth());
        LL_start->setSizePolicy(sizePolicy);
        LL_start->setMaximumSize(QSize(200, 200));
        LL_start->setCursor(QCursor(Qt::CursorShape::PointingHandCursor));
        LL_start->setStyleSheet(QString::fromUtf8("QPushButton{\n"
"background-color: rgb(217, 217, 217);\n"
"color: rgb(0,0,0);  \n"
"border-radius:8px;\n"
"font:12pt \"\345\276\256\350\275\257\351\233\205\351\273\221\";\n"
"border:5px outset gray;\n"
"}\n"
"QPushButton::hover{\n"
"background-color: rgb(200, 200, 200);\n"
"color: rgb(0,0,0);  \n"
"border-radius:8px;\n"
"font:12pt \"\345\276\256\350\275\257\351\233\205\351\273\221\";\n"
"border:5px groove gray;\n"
"}"));

        verticalLayout_4->addWidget(LL_start);

        LL_stop = new QPushButton(widget_6);
        LL_stop->setObjectName("LL_stop");
        sizePolicy.setHeightForWidth(LL_stop->sizePolicy().hasHeightForWidth());
        LL_stop->setSizePolicy(sizePolicy);
        LL_stop->setMaximumSize(QSize(200, 200));
        LL_stop->setCursor(QCursor(Qt::CursorShape::PointingHandCursor));
        LL_stop->setStyleSheet(QString::fromUtf8("QPushButton{\n"
"background-color: rgb(217, 217, 217);\n"
"color: rgb(0,0,0);  \n"
"border-radius:8px;\n"
"font:12pt \"\345\276\256\350\275\257\351\233\205\351\273\221\";\n"
"border:5px outset gray;\n"
"}\n"
"QPushButton::hover{\n"
"background-color: rgb(200, 200, 200);\n"
"color: rgb(0,0,0);  \n"
"border-radius:8px;\n"
"font:12pt \"\345\276\256\350\275\257\351\233\205\351\273\221\";\n"
"border:5px groove gray;\n"
"}"));

        verticalLayout_4->addWidget(LL_stop);

        verticalLayout_4->setStretch(0, 2);
        verticalLayout_4->setStretch(2, 4);
        verticalLayout_4->setStretch(3, 4);
        verticalLayout_4->setStretch(4, 4);
        verticalLayout_4->setStretch(5, 4);
        LL_RD_1_background = new QLabel(LL);
        LL_RD_1_background->setObjectName("LL_RD_1_background");
        LL_RD_1_background->setGeometry(QRect(550, 5, 600, 600));
        LL_RD_1_background->setStyleSheet(QString::fromUtf8("QLabel{\n"
"	border:1px solid gray;\n"
"}"));
        LL_RD_1 = new QWidget(LL);
        LL_RD_1->setObjectName("LL_RD_1");
        LL_RD_1->setGeometry(QRect(560, 15, 581, 581));
        gridLayout_3 = new QGridLayout(LL_RD_1);
        gridLayout_3->setObjectName("gridLayout_3");
        grid_display = new QGridLayout();
        grid_display->setObjectName("grid_display");

        gridLayout_3->addLayout(grid_display, 0, 0, 1, 1);

        textBrowser_ll = new QTextBrowser(LL);
        textBrowser_ll->setObjectName("textBrowser_ll");
        textBrowser_ll->setGeometry(QRect(130, 5, 415, 95));
        textBrowser_ll->setStyleSheet(QString::fromUtf8("QTextBrowser{\n"
"    font:8pt\"\345\276\256\350\275\257\351\233\205\351\273\221\";\n"
"}"));
        LL_path = new QLabel(LL);
        LL_path->setObjectName("LL_path");
        LL_path->setGeometry(QRect(130, 105, 415, 500));
        LL_path->setStyleSheet(QString::fromUtf8("QLabel{\n"
"	border:1px solid gray;\n"
"	font:10pt\"\345\276\256\350\275\257\351\233\205\351\273\221\";\n"
"}"));
        label_36 = new QLabel(LL);
        label_36->setObjectName("label_36");
        label_36->setGeometry(QRect(475, 5, 70, 20));
        label_36->setStyleSheet(QString::fromUtf8("QLabel{\n"
"	border:1px solid gray;\n"
"	font:10pt \"\345\276\256\350\275\257\351\233\205\351\273\221\";\n"
"	background-color:none;\n"
"}"));
        label_36->setAlignment(Qt::AlignCenter);
        label_48 = new QLabel(LL);
        label_48->setObjectName("label_48");
        label_48->setGeometry(QRect(130, 105, 70, 20));
        label_48->setStyleSheet(QString::fromUtf8("QLabel{\n"
"	border:1px solid gray;\n"
"	font:10pt \"\345\276\256\350\275\257\351\233\205\351\273\221\";\n"
"	background-color:none;\n"
"}"));
        label_48->setAlignment(Qt::AlignCenter);
        label_49 = new QLabel(LL);
        label_49->setObjectName("label_49");
        label_49->setGeometry(QRect(550, 5, 80, 20));
        label_49->setStyleSheet(QString::fromUtf8("QLabel{\n"
"	border:1px solid gray;\n"
"	font:10pt \"\345\276\256\350\275\257\351\233\205\351\273\221\";\n"
"	background-color:none;\n"
"}"));
        label_49->setAlignment(Qt::AlignCenter);
        tabWidget->addTab(LL, QString());
        Other_tabWidget_Robotplatform->addTab(tab_2, QString());
        tab_6 = new QWidget();
        tab_6->setObjectName("tab_6");
        frame_4 = new QFrame(tab_6);
        frame_4->setObjectName("frame_4");
        frame_4->setGeometry(QRect(0, 0, 941, 471));
        frame_4->setFrameShape(QFrame::StyledPanel);
        frame_4->setFrameShadow(QFrame::Raised);
        Other_tabWidget_Robotplatform->addTab(tab_6, QString());

        gridLayout->addWidget(Other_tabWidget_Robotplatform, 0, 1, 1, 1);


        retranslateUi(ContinuosfiberInterface);

        Other_tabWidget_Robotplatform->setCurrentIndex(1);
        tabWidget->setCurrentIndex(2);


        QMetaObject::connectSlotsByName(ContinuosfiberInterface);
    } // setupUi

    void retranslateUi(QWidget *ContinuosfiberInterface)
    {
        ContinuosfiberInterface->setWindowTitle(QCoreApplication::translate("ContinuosfiberInterface", "ContinousfiberInterface", nullptr));
        groupBox_2->setTitle(QCoreApplication::translate("ContinuosfiberInterface", "\345\226\267\345\244\264\346\216\247\345\210\266", nullptr));
        ConmunicationConnectionButton->setText(QCoreApplication::translate("ContinuosfiberInterface", "    \345\274\200\346\234\272    ", nullptr));
        CloseSerialpushButton->setText(QCoreApplication::translate("ContinuosfiberInterface", "\345\205\263\346\234\272", nullptr));
        nozzletemperature->setText(QCoreApplication::translate("ContinuosfiberInterface", "\346\211\223\345\215\260\346\270\251\345\272\246(\342\204\203)", nullptr));
        groupBox_3->setTitle(QCoreApplication::translate("ContinuosfiberInterface", "\347\264\253\345\244\226\346\216\247\345\210\266", nullptr));
        open->setText(QCoreApplication::translate("ContinuosfiberInterface", "    \345\274\200\346\234\272    ", nullptr));
        stop->setText(QCoreApplication::translate("ContinuosfiberInterface", "    \345\205\263\346\234\272    ", nullptr));
        groupBox_4->setTitle(QCoreApplication::translate("ContinuosfiberInterface", "\344\270\262\345\217\243\350\256\276\347\275\256", nullptr));
        BaudRatelabel->setText(QCoreApplication::translate("ContinuosfiberInterface", "\346\263\242\347\211\271\347\216\207", nullptr));
        BaudRateBox->setItemText(0, QCoreApplication::translate("ContinuosfiberInterface", "9600", nullptr));
        BaudRateBox->setItemText(1, QCoreApplication::translate("ContinuosfiberInterface", "11200", nullptr));

        label_4->setText(QCoreApplication::translate("ContinuosfiberInterface", "\344\270\262\345\217\243\351\200\211\346\213\251", nullptr));
        ComPortBox->setItemText(0, QCoreApplication::translate("ContinuosfiberInterface", "COM1", nullptr));
        ComPortBox->setItemText(1, QCoreApplication::translate("ContinuosfiberInterface", "COM2", nullptr));
        ComPortBox->setItemText(2, QCoreApplication::translate("ContinuosfiberInterface", "COM10", nullptr));

        SerialStatelabel->setText(QCoreApplication::translate("ContinuosfiberInterface", "COM1", nullptr));
        ChoosepushButton->setText(QCoreApplication::translate("ContinuosfiberInterface", " \346\243\200\346\265\213\344\270\262\345\217\243 ", nullptr));
        groupBox->setTitle(QCoreApplication::translate("ContinuosfiberInterface", "\350\277\220\345\212\250\346\216\247\345\210\266", nullptr));
        groupBox_5->setTitle(QCoreApplication::translate("ContinuosfiberInterface", "\347\254\233\345\215\241\345\260\224\345\235\220\346\240\207\347\263\273", nullptr));
        label_5->setText(QCoreApplication::translate("ContinuosfiberInterface", "X\357\274\232", nullptr));
        label_8->setText(QCoreApplication::translate("ContinuosfiberInterface", "Y\357\274\232", nullptr));
        label_9->setText(QCoreApplication::translate("ContinuosfiberInterface", "Z\357\274\232", nullptr));
        label_19->setText(QCoreApplication::translate("ContinuosfiberInterface", " I\357\274\232", nullptr));
        label_21->setText(QCoreApplication::translate("ContinuosfiberInterface", "K\357\274\232", nullptr));
        label_20->setText(QCoreApplication::translate("ContinuosfiberInterface", "J\357\274\232", nullptr));
        groupBox_8->setTitle(QCoreApplication::translate("ContinuosfiberInterface", "\346\236\201\345\235\220\346\240\207\347\263\273", nullptr));
        label_33->setText(QCoreApplication::translate("ContinuosfiberInterface", "A1:", nullptr));
        label_40->setText(QCoreApplication::translate("ContinuosfiberInterface", "A2:", nullptr));
        label_41->setText(QCoreApplication::translate("ContinuosfiberInterface", "A3:", nullptr));
        label_42->setText(QCoreApplication::translate("ContinuosfiberInterface", "A4:", nullptr));
        label_43->setText(QCoreApplication::translate("ContinuosfiberInterface", "A5:", nullptr));
        label_44->setText(QCoreApplication::translate("ContinuosfiberInterface", "A6:", nullptr));
        label_6->setText(QCoreApplication::translate("ContinuosfiberInterface", "\346\234\272\346\242\260\350\207\202\347\212\266\346\200\201\357\274\232", nullptr));
        groupBox_20->setTitle(QCoreApplication::translate("ContinuosfiberInterface", "\351\233\266\344\275\215\350\256\276\347\275\256", nullptr));
        label_32->setText(QCoreApplication::translate("ContinuosfiberInterface", "X", nullptr));
        zero_position_x->setText(QCoreApplication::translate("ContinuosfiberInterface", "-800", nullptr));
        label_37->setText(QCoreApplication::translate("ContinuosfiberInterface", "Y", nullptr));
        zero_position_y->setText(QCoreApplication::translate("ContinuosfiberInterface", "100", nullptr));
        label_38->setText(QCoreApplication::translate("ContinuosfiberInterface", "Z", nullptr));
        zero_position_z->setText(QCoreApplication::translate("ContinuosfiberInterface", "500", nullptr));
        ConnectionRobotButton->setText(QCoreApplication::translate("ContinuosfiberInterface", "\351\223\276\346\216\245IP", nullptr));
        Robot_movebutton->setText(QCoreApplication::translate("ContinuosfiberInterface", "\345\256\232\344\275\215", nullptr));
        RobotGohomeButton->setText(QCoreApplication::translate("ContinuosfiberInterface", "\345\244\215\344\275\215", nullptr));
        RobotaxismoveButton->setText(QCoreApplication::translate("ContinuosfiberInterface", "\350\275\254\350\275\264", nullptr));
        robot_state_show->setText(QCoreApplication::translate("ContinuosfiberInterface", "\346\234\252\350\277\236\346\216\245", nullptr));
        groupBox_6->setTitle(QCoreApplication::translate("ContinuosfiberInterface", "\345\244\226\351\203\250\350\275\264\346\216\247\345\210\266", nullptr));
        label_34->setText(QCoreApplication::translate("ContinuosfiberInterface", "\345\200\276\346\226\234\350\275\264(\345\217\230\344\275\215\346\234\272):", nullptr));
        Positioner1lineEdit->setText(QCoreApplication::translate("ContinuosfiberInterface", "0", nullptr));
        label_45->setText(QCoreApplication::translate("ContinuosfiberInterface", "\346\227\213\350\275\254\350\275\264(\345\217\230\344\275\215\346\234\272):", nullptr));
        Positioner2lineEdit->setText(QCoreApplication::translate("ContinuosfiberInterface", "0", nullptr));
        PositionermoveButton->setText(QCoreApplication::translate("ContinuosfiberInterface", "\347\247\273\345\212\250", nullptr));
        PositionercomebackButton->setText(QCoreApplication::translate("ContinuosfiberInterface", "\345\244\215\344\275\215", nullptr));
        label_46->setText(QCoreApplication::translate("ContinuosfiberInterface", "\345\271\263\347\247\273\350\275\264(\345\234\260\350\275\250):", nullptr));
        Positioner3lineEdit->setText(QCoreApplication::translate("ContinuosfiberInterface", "0", nullptr));
        groupBox_7->setTitle(QCoreApplication::translate("ContinuosfiberInterface", "\346\234\272\345\231\250\344\272\272\351\200\237\345\272\246\346\216\247\345\210\266", nullptr));
        label_47->setText(QCoreApplication::translate("ContinuosfiberInterface", "\346\234\272\345\231\250\350\277\220\350\241\214\351\200\237\345\272\246(mm/s)", nullptr));
        label_64->setText(QCoreApplication::translate("ContinuosfiberInterface", "\351\207\215\345\256\232\344\275\215\350\247\222\351\200\237\345\272\246(deg/s)", nullptr));
        label_65->setText(QCoreApplication::translate("ContinuosfiberInterface", "\345\244\226\351\203\250\350\275\264\347\272\277\351\200\237\345\272\246(mm/s)", nullptr));
        label_66->setText(QCoreApplication::translate("ContinuosfiberInterface", "\345\244\226\351\203\250\350\275\264\350\247\222\351\200\237\345\272\246(deg/s)", nullptr));
        Other_tabWidget_Robotplatform->setTabText(Other_tabWidget_Robotplatform->indexOf(tab), QCoreApplication::translate("ContinuosfiberInterface", "  \345\210\235\345\247\213\345\214\226   ", nullptr));
        groupBox_9->setTitle(QCoreApplication::translate("ContinuosfiberInterface", "PLC\346\216\247\345\210\266", nullptr));
        ConnectionPLCButton->setText(QCoreApplication::translate("ContinuosfiberInterface", "\350\277\236\346\216\245PLC", nullptr));
        DisconnectPLCButton->setText(QCoreApplication::translate("ContinuosfiberInterface", "\346\226\255\345\274\200PLC", nullptr));
        groupBox_12->setTitle(QCoreApplication::translate("ContinuosfiberInterface", "\346\260\224\347\274\270\346\216\247\345\210\266", nullptr));
        cylinder3_button->setText(QCoreApplication::translate("ContinuosfiberInterface", "\345\274\200\345\205\263\342\205\242", nullptr));
        cylinder2_button->setText(QCoreApplication::translate("ContinuosfiberInterface", "\345\274\200\345\205\263\342\205\241", nullptr));
        cylinder1_button->setText(QCoreApplication::translate("ContinuosfiberInterface", "\345\274\200\345\205\263\342\205\240", nullptr));
        label_28->setText(QCoreApplication::translate("ContinuosfiberInterface", "\345\211\252\345\210\200\346\216\247\345\210\266\357\274\232", nullptr));
        label_29->setText(QCoreApplication::translate("ContinuosfiberInterface", "\346\212\254\345\215\207\346\216\247\345\210\266\357\274\232", nullptr));
        label_31->setText(QCoreApplication::translate("ContinuosfiberInterface", "\345\216\213\344\270\235\346\216\247\345\210\266\357\274\232", nullptr));
        cylinder1_state->setText(QString());
        cylinder2_state->setText(QString());
        cylinder3_state->setText(QString());
        groupBox_13->setTitle(QCoreApplication::translate("ContinuosfiberInterface", "\350\277\207\347\250\213\345\217\202\346\225\260", nullptr));
        label_81->setText(QCoreApplication::translate("ContinuosfiberInterface", "\350\267\257\345\276\204\350\241\214\346\225\260 :", nullptr));
        label_82->setText(QCoreApplication::translate("ContinuosfiberInterface", "\345\256\214\346\210\220\347\216\207 :", nullptr));
        label_83->setText(QCoreApplication::translate("ContinuosfiberInterface", "\345\211\251\344\275\231\346\227\266\351\227\264 :", nullptr));
        groupBox_14->setTitle(QCoreApplication::translate("ContinuosfiberInterface", "\350\267\257\345\276\204\350\256\276\347\275\256", nullptr));
        locusselectsetButton->setText(QCoreApplication::translate("ContinuosfiberInterface", "\350\216\267\345\217\226\350\267\257\345\276\204", nullptr));
        locusseselectButton->setText(QCoreApplication::translate("ContinuosfiberInterface", "\350\267\257\345\276\204\345\214\271\351\205\215", nullptr));
        ParametersuitedtButton->setText(QCoreApplication::translate("ContinuosfiberInterface", "\345\217\202\346\225\260\351\200\211\346\213\251", nullptr));
        groupBox_15->setTitle(QCoreApplication::translate("ContinuosfiberInterface", "\350\267\257\345\276\204\345\217\257\350\247\206\345\214\226", nullptr));
        labelpic->setText(QString());
        HC_RD_2->setText(QString());
        TIC_RD_2->setText(QString());
        FC_VV_2->setText(QString());
        groupBox_16->setTitle(QCoreApplication::translate("ContinuosfiberInterface", "\350\277\220\345\212\250\346\216\247\345\210\266", nullptr));
        Robot_movebutton_Gcode->setText(QCoreApplication::translate("ContinuosfiberInterface", "\345\274\200\345\247\213\346\211\223\345\215\260", nullptr));
        PauseRobotButton->setText(QCoreApplication::translate("ContinuosfiberInterface", "\346\232\202\345\201\234\346\211\223\345\215\260", nullptr));
        ContinueRobotButton->setText(QCoreApplication::translate("ContinuosfiberInterface", "\347\273\247\347\273\255\346\211\223\345\215\260", nullptr));
        StopRobotButton->setText(QCoreApplication::translate("ContinuosfiberInterface", "\345\201\234\346\255\242\346\211\223\345\215\260", nullptr));
        groupBox_17->setTitle(QCoreApplication::translate("ContinuosfiberInterface", "\350\267\257\345\276\204\346\265\213\350\257\225", nullptr));
        force_test->setText(QCoreApplication::translate("ContinuosfiberInterface", "\345\216\213\345\212\233\346\265\213\350\257\225", nullptr));
        groupBox_11->setTitle(QCoreApplication::translate("ContinuosfiberInterface", "\347\224\265\346\234\272\346\216\247\345\210\266", nullptr));
        Paving_distance->setPlaceholderText(QCoreApplication::translate("ContinuosfiberInterface", "\351\223\272\345\270\246\350\267\235\347\246\273(mm)", nullptr));
        Wire_feed_rate->setPlaceholderText(QCoreApplication::translate("ContinuosfiberInterface", "\351\200\201\344\270\235\351\200\237\345\272\246(mm/s)", nullptr));
        parameter_input->setText(QCoreApplication::translate("ContinuosfiberInterface", "\345\217\202\346\225\260\345\206\231\345\205\245", nullptr));
        motor_enable->setText(QCoreApplication::translate("ContinuosfiberInterface", "\347\224\265\346\234\272\344\275\277\350\203\275", nullptr));
        motor_actuation->setText(QCoreApplication::translate("ContinuosfiberInterface", "\347\224\265\346\234\272\345\220\257\345\212\250", nullptr));
        interruption->setText(QCoreApplication::translate("ContinuosfiberInterface", "\347\202\271\345\212\250", nullptr));
        continuous->setText(QCoreApplication::translate("ContinuosfiberInterface", "\350\277\236\347\273\255", nullptr));
        motor_state->setText(QCoreApplication::translate("ContinuosfiberInterface", "\347\224\265\346\234\272\347\212\266\346\200\201\357\274\232", nullptr));
        state_show->setText(QCoreApplication::translate("ContinuosfiberInterface", "\346\223\215\344\275\234\346\214\211\351\222\256", nullptr));
        motor_reset->setText(QCoreApplication::translate("ContinuosfiberInterface", "\345\244\215\344\275\215", nullptr));
        groupBox_10->setTitle(QCoreApplication::translate("ContinuosfiberInterface", "\347\272\242\345\244\226\346\277\200\345\205\211", nullptr));
        target_temperature_input->setPlaceholderText(QCoreApplication::translate("ContinuosfiberInterface", "\347\233\256\346\240\207\346\270\251\345\272\246(\302\260C)", nullptr));
        laser_connect->setText(QCoreApplication::translate("ContinuosfiberInterface", "\346\277\200\345\205\211\350\277\236\346\216\245", nullptr));
        laser_open_button->setText(QCoreApplication::translate("ContinuosfiberInterface", "\346\277\200\345\205\211\345\274\200", nullptr));
        laser_close_button->setText(QCoreApplication::translate("ContinuosfiberInterface", "\346\277\200\345\205\211\345\205\263", nullptr));
        laser_state_display->setHtml(QCoreApplication::translate("ContinuosfiberInterface", "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.0//EN\" \"http://www.w3.org/TR/REC-html40/strict.dtd\">\n"
"<html><head><meta name=\"qrichtext\" content=\"1\" /><style type=\"text/css\">\n"
"p, li { white-space: pre-wrap; }\n"
"</style></head><body style=\" font-family:'\345\276\256\350\275\257\351\233\205\351\273\221'; font-size:8pt; font-weight:400; font-style:normal;\">\n"
"<p style=\"-qt-paragraph-type:empty; margin-top:0px; margin-bottom:0px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;\"><br /></p></body></html>", nullptr));
        redlight_show->setText(QCoreApplication::translate("ContinuosfiberInterface", "\347\272\242\345\205\211\347\212\266\346\200\201", nullptr));
        target_temperature_label->setText(QCoreApplication::translate("ContinuosfiberInterface", "\347\233\256\346\240\207\346\270\251\345\272\246(\302\260C):", nullptr));
        label_39->setText(QCoreApplication::translate("ContinuosfiberInterface", "\347\212\266\346\200\201\346\227\245\345\277\227", nullptr));
        redlight_open->setText(QCoreApplication::translate("ContinuosfiberInterface", "\347\272\242\345\205\211\345\274\200", nullptr));
        redlight_close->setText(QCoreApplication::translate("ContinuosfiberInterface", "\347\272\242\345\205\211\345\205\263", nullptr));
        set_target_temperature->setText(QCoreApplication::translate("ContinuosfiberInterface", "\350\256\276\347\275\256\346\270\251\345\272\246", nullptr));
        temperature_control->setText(QCoreApplication::translate("ContinuosfiberInterface", "\346\270\251\345\272\246\350\260\203\346\216\247", nullptr));
        laser_power_label->setText(QCoreApplication::translate("ContinuosfiberInterface", "\346\277\200\345\205\211\345\212\237\347\216\207(W):", nullptr));
        laser_power_input->setPlaceholderText(QCoreApplication::translate("ContinuosfiberInterface", "\345\212\237\347\216\207\350\256\276\345\256\232\345\200\274(W)", nullptr));
        set_laser_power->setText(QCoreApplication::translate("ContinuosfiberInterface", "\346\277\200\345\205\211\345\212\237\347\216\207\345\206\231\345\205\245", nullptr));
        set_laser_power_control->setText(QCoreApplication::translate("ContinuosfiberInterface", "\346\277\200\345\205\211\345\212\237\347\216\207\346\216\247\345\210\266", nullptr));
        groupBox_18->setTitle(QCoreApplication::translate("ContinuosfiberInterface", "\347\243\201\346\273\236\346\216\247\345\210\266", nullptr));
        Hysteresis_ratio->setText(QCoreApplication::translate("ContinuosfiberInterface", "\346\257\224\344\276\213\357\274\232", nullptr));
        Hysteresis_ratio_text->setPlaceholderText(QCoreApplication::translate("ContinuosfiberInterface", "\350\214\203\345\233\264(100-300)", nullptr));
        Hysteresis_ratio_input->setText(QCoreApplication::translate("ContinuosfiberInterface", "\345\206\231\345\205\245", nullptr));
        Other_tabWidget_Robotplatform->setTabText(Other_tabWidget_Robotplatform->indexOf(tab_3), QCoreApplication::translate("ContinuosfiberInterface", "  \346\216\247\345\210\266\347\263\273\347\273\237  ", nullptr));
        label_27->setText(QCoreApplication::translate("ContinuosfiberInterface", "<\347\233\270\346\234\272\346\216\247\345\210\266>", nullptr));
        enum_cam->setText(QCoreApplication::translate("ContinuosfiberInterface", "\346\243\200\346\265\213\350\256\276\345\244\207", nullptr));
        HC_machine->setText(QCoreApplication::translate("ContinuosfiberInterface", "\350\277\236\346\216\245\350\256\276\345\244\207", nullptr));
        HC_data->setText(QCoreApplication::translate("ContinuosfiberInterface", "\347\274\272\351\231\267\351\207\207\351\233\206", nullptr));
        HC_start->setText(QCoreApplication::translate("ContinuosfiberInterface", "\345\274\200\345\247\213\351\207\207\351\233\206", nullptr));
        HC_stop->setText(QCoreApplication::translate("ContinuosfiberInterface", "\345\205\263\351\227\255\347\233\270\346\234\272", nullptr));
        HC_RD->setText(QString());
        HC_PData->setText(QString());
        HC_Abrasion->setText(QString());
        HC_Misalignment->setText(QString());
        label_10->setText(QCoreApplication::translate("ContinuosfiberInterface", "\345\216\237\345\247\213\345\233\276\345\203\217", nullptr));
        label_11->setText(QCoreApplication::translate("ContinuosfiberInterface", "\347\274\272\351\231\267\346\230\276\347\244\272", nullptr));
        label_12->setText(QCoreApplication::translate("ContinuosfiberInterface", "Abrasion", nullptr));
        label_13->setText(QCoreApplication::translate("ContinuosfiberInterface", "Misalignment", nullptr));
        label_3->setText(QCoreApplication::translate("ContinuosfiberInterface", "Frame rate:", nullptr));
        label->setText(QCoreApplication::translate("ContinuosfiberInterface", "Exposure time:", nullptr));
        label_2->setText(QCoreApplication::translate("ContinuosfiberInterface", "Gain:", nullptr));
        FrameRate->setPlaceholderText(QCoreApplication::translate("ContinuosfiberInterface", "\350\257\267\350\276\223\345\205\245\345\270\247\347\216\207", nullptr));
        ExposureTime->setPlaceholderText(QCoreApplication::translate("ContinuosfiberInterface", "\350\257\267\350\276\223\345\205\245\346\233\235\345\205\211\346\227\266\351\227\264", nullptr));
        Gain->setPlaceholderText(QCoreApplication::translate("ContinuosfiberInterface", "\350\257\267\350\276\223\345\205\245\345\242\236\347\233\212", nullptr));
        get_parameter->setText(QCoreApplication::translate("ContinuosfiberInterface", "\346\230\276\347\244\272\345\217\202\346\225\260", nullptr));
        set_parameter->setText(QCoreApplication::translate("ContinuosfiberInterface", "\350\256\276\347\275\256\345\217\202\346\225\260", nullptr));
        tabWidget->setTabText(tabWidget->indexOf(HC), QCoreApplication::translate("ContinuosfiberInterface", "\351\253\230\351\200\237\347\233\270\346\234\272", nullptr));
        TIC_RD->setText(QString());
        TIC_PD->setText(QString());
        radioButton->setText(QCoreApplication::translate("ContinuosfiberInterface", "High", nullptr));
        label_7->setText(QCoreApplication::translate("ContinuosfiberInterface", "\346\211\223\345\215\260\351\200\237\345\272\246:", nullptr));
        radioButton_2->setText(QCoreApplication::translate("ContinuosfiberInterface", "Good", nullptr));
        radioButton_3->setText(QCoreApplication::translate("ContinuosfiberInterface", "Low", nullptr));
        label_14->setText(QCoreApplication::translate("ContinuosfiberInterface", "\345\216\237\345\247\213\345\233\276\345\203\217", nullptr));
        label_15->setText(QCoreApplication::translate("ContinuosfiberInterface", "\346\210\252\345\217\226\345\233\276\345\203\217", nullptr));
        label_17->setText(QCoreApplication::translate("ContinuosfiberInterface", "<\347\272\242\345\244\226\347\203\255\345\203\217\344\273\252\346\216\247\345\210\266>", nullptr));
        TIC_machine->setText(QCoreApplication::translate("ContinuosfiberInterface", "\350\277\236\346\216\245\350\256\276\345\244\207", nullptr));
        TIC_data->setText(QCoreApplication::translate("ContinuosfiberInterface", "\346\270\251\345\272\246\345\210\206\346\236\220", nullptr));
        TIC_start->setText(QCoreApplication::translate("ContinuosfiberInterface", "\346\270\205\347\251\272\347\274\223\345\255\230", nullptr));
        TIC_stop->setText(QCoreApplication::translate("ContinuosfiberInterface", "\345\201\234\346\255\242\351\207\207\351\233\206", nullptr));
        label_logbrowser->setText(QCoreApplication::translate("ContinuosfiberInterface", "\346\223\215\344\275\234\346\227\245\345\277\227", nullptr));
        label_16->setText(QCoreApplication::translate("ContinuosfiberInterface", "\346\211\223\345\215\260\351\200\237\345\272\246:", nullptr));
        radioButton_7->setText(QCoreApplication::translate("ContinuosfiberInterface", "high", nullptr));
        radioButton_8->setText(QCoreApplication::translate("ContinuosfiberInterface", "good", nullptr));
        radioButton_9->setText(QCoreApplication::translate("ContinuosfiberInterface", "low", nullptr));
        tabWidget->setTabText(tabWidget->indexOf(TIC), QCoreApplication::translate("ContinuosfiberInterface", "\347\272\242\345\244\226\347\203\255\345\203\217\344\273\252", nullptr));
        label_30->setText(QCoreApplication::translate("ContinuosfiberInterface", "<\345\216\213\345\212\233\346\216\247\345\210\266>", nullptr));
        FC_machine->setText(QCoreApplication::translate("ContinuosfiberInterface", "\350\277\236\346\216\245\350\256\276\345\244\207", nullptr));
        FC_data->setText(QCoreApplication::translate("ContinuosfiberInterface", "\345\216\213\345\212\233\346\225\260\346\215\256", nullptr));
        FC_start->setText(QCoreApplication::translate("ContinuosfiberInterface", "\345\274\200\345\247\213\351\207\207\351\233\206", nullptr));
        FC_stop->setText(QCoreApplication::translate("ContinuosfiberInterface", "\345\201\234\346\255\242\351\207\207\351\233\206", nullptr));
        FC_RD->setText(QString());
        FC_VV->setText(QString());
        FC_VAR->setText(QString());
        FC_SD->setText(QString());
        FC_PEAK->setText(QString());
        label_18->setText(QCoreApplication::translate("ContinuosfiberInterface", "\345\216\237\345\247\213\346\225\260\346\215\256", nullptr));
        label_22->setText(QCoreApplication::translate("ContinuosfiberInterface", "\346\234\211\346\225\210\345\200\274", nullptr));
        label_23->setText(QCoreApplication::translate("ContinuosfiberInterface", "\346\226\271\345\267\256", nullptr));
        label_24->setText(QCoreApplication::translate("ContinuosfiberInterface", "\346\240\207\345\207\206\345\267\256", nullptr));
        label_25->setText(QCoreApplication::translate("ContinuosfiberInterface", "\345\263\260\345\200\274", nullptr));
        label_26->setText(QCoreApplication::translate("ContinuosfiberInterface", "\345\261\202\345\216\232\357\274\232", nullptr));
        radioButton_4->setText(QCoreApplication::translate("ContinuosfiberInterface", "high", nullptr));
        radioButton_5->setText(QCoreApplication::translate("ContinuosfiberInterface", "good", nullptr));
        radioButton_6->setText(QCoreApplication::translate("ContinuosfiberInterface", "low", nullptr));
        tabWidget->setTabText(tabWidget->indexOf(FC), QCoreApplication::translate("ContinuosfiberInterface", "\345\216\213\345\212\233\344\274\240\346\204\237\345\231\250", nullptr));
        label_35->setText(QCoreApplication::translate("ContinuosfiberInterface", "<\347\272\277\346\277\200\345\205\211\346\216\247\345\210\266>", nullptr));
        LL_machine->setText(QCoreApplication::translate("ContinuosfiberInterface", "\347\273\210\345\247\213\345\235\220\346\240\207", nullptr));
        LL_data->setText(QCoreApplication::translate("ContinuosfiberInterface", "\345\220\257\345\212\250\346\211\253\346\217\217", nullptr));
        LL_start->setText(QCoreApplication::translate("ContinuosfiberInterface", "\346\230\276\347\244\272\346\211\253\346\217\217", nullptr));
        LL_stop->setText(QCoreApplication::translate("ContinuosfiberInterface", "\346\211\247\350\241\214\344\277\256\345\244\215", nullptr));
        LL_RD_1_background->setText(QString());
        LL_path->setText(QString());
        label_36->setText(QCoreApplication::translate("ContinuosfiberInterface", "\346\223\215\344\275\234\346\227\245\345\277\227", nullptr));
        label_48->setText(QCoreApplication::translate("ContinuosfiberInterface", "\350\267\257\345\276\204\347\224\237\346\210\220", nullptr));
        label_49->setText(QCoreApplication::translate("ContinuosfiberInterface", "\347\274\272\351\231\267\345\214\272\347\202\271\344\272\221", nullptr));
        tabWidget->setTabText(tabWidget->indexOf(LL), QCoreApplication::translate("ContinuosfiberInterface", "\347\272\277\346\277\200\345\205\211", nullptr));
        Other_tabWidget_Robotplatform->setTabText(Other_tabWidget_Robotplatform->indexOf(tab_2), QCoreApplication::translate("ContinuosfiberInterface", "  \347\233\221\346\265\213\347\263\273\347\273\237 ", nullptr));
        Other_tabWidget_Robotplatform->setTabText(Other_tabWidget_Robotplatform->indexOf(tab_6), QCoreApplication::translate("ContinuosfiberInterface", "\346\277\200\345\205\211\347\263\273\347\273\237", nullptr));
    } // retranslateUi

};

namespace Ui {
    class ContinuosfiberInterface: public Ui_ContinuosfiberInterface {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_INTERFACE4600_WINDOW_H
