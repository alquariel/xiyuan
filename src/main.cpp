#include <QApplication>
#include <QTimer>
#include "app/StatusLogger.h"
#include "ui/MainWindow.h"

int main(int argc, char* argv[]) {
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
#endif
    QApplication app(argc, argv);

    StatusLogger::install();

    MainWindow window;
    window.show();
    QTimer::singleShot(500, &window, &MainWindow::startBackgroundServices);

    return app.exec();
}
