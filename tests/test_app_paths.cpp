#include <QtTest/QtTest>
#include <QCoreApplication>
#include "app/AppPaths.h"

class TestAppPaths : public QObject {
    Q_OBJECT

private slots:
    void resolvesPathsFromApplicationDirectory() {
        const AppPaths paths(QCoreApplication::applicationDirPath());

        QVERIFY(paths.rootDir().isAbsolute());
        QCOMPARE(paths.configFilePath().fileName(), QString("env_config.json"));
        QVERIFY(paths.configFilePath().path().endsWith("config"));
        QCOMPARE(paths.loggingConfigFilePath().fileName(), QString("logging_config.json"));
        QCOMPARE(paths.iconFilePath().fileName(), QString("icon.png"));
    }

    void acceptsExplicitRootForTests() {
        const AppPaths paths("C:/portable/AFP_ABB_CPP");

        QCOMPARE(paths.rootDir().absolutePath(), QString("C:/portable/AFP_ABB_CPP"));
        QCOMPARE(paths.configDir().absolutePath(), QString("C:/portable/AFP_ABB_CPP/config"));
        QCOMPARE(paths.assetsDir().absolutePath(), QString("C:/portable/AFP_ABB_CPP/assets"));
        QCOMPARE(paths.configFilePath().absoluteFilePath(), QString("C:/portable/AFP_ABB_CPP/config/env_config.json"));
        QCOMPARE(paths.loggingConfigFilePath().absoluteFilePath(), QString("C:/portable/AFP_ABB_CPP/config/logging_config.json"));
        QCOMPARE(paths.iconFilePath().absoluteFilePath(), QString("C:/portable/AFP_ABB_CPP/assets/icons/icon.png"));
    }
};

QTEST_MAIN(TestAppPaths)
#include "test_app_paths.moc"
