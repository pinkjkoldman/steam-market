#include <QApplication>
#include <QCommandLineParser>
#include <QFont>
#include <QIcon>
#include <QMessageBox>
#include <QSize>
#include <QStringList>
#include <QLocalServer>
#include <QLocalSocket>

#include "app/AppController.h"
#include "utils/AppInfo.h"
#include "utils/Logger.h"

namespace {
// 单实例保护：已运行时新进程会通知旧进程唤起窗口后退出。
const char kInstanceKey[] = "SteamMarketTerminal.SingleInstance";
}  // namespace

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    QApplication::setApplicationName(QStringLiteral("SteamMarketTerminal"));
    QApplication::setApplicationDisplayName(QStringLiteral("Steam 行情终端"));
    QApplication::setApplicationVersion(QStringLiteral(APP_VERSION));
    QApplication::setOrganizationName(QStringLiteral("Personal"));
    QApplication::setWindowIcon(QIcon(QStringLiteral(":/ui/app.png")));
    // 界面可读性：默认字体上调（中文环境小字号显示模糊）
    QFont baseFont = app.font();
    baseFont.setPointSizeF(9.8);
    app.setFont(baseFont);

    QCommandLineParser parser;
    parser.setApplicationDescription(QStringLiteral("Steam 行情终端：Steam 市场数据统计小软件"));
    parser.addHelpOption();
    parser.addVersionOption();
    const QCommandLineOption smokeOpt(QStringLiteral("smoke-test"),
                                      QStringLiteral("运行冒烟自测；可传截图输出路径"),
                                      QStringLiteral("pngPath"));
    parser.addOption(smokeOpt);
    const QCommandLineOption smokeSizeOpt(
        QStringLiteral("smoke-size"), QStringLiteral("Smoke window size, for example 1280x800"),
        QStringLiteral("widthxheight"), QStringLiteral("1280x800"));
    parser.addOption(smokeSizeOpt);
    const QCommandLineOption smokeSceneOpt(
        QStringLiteral("smoke-scene"),
        QStringLiteral("Smoke scene: welcome, overview, inventory, or detail"),
        QStringLiteral("scene"),
        QStringLiteral("trading"));
    parser.addOption(smokeSceneOpt);
    parser.process(app);

    const bool smokeMode = parser.isSet(smokeOpt);

    // 单实例保护（冒烟自测跳过，允许与正常实例并行）
    QLocalServer instanceServer;
    if (!smokeMode) {
        QLocalSocket probe;
        probe.connectToServer(QLatin1String(kInstanceKey));
        if (probe.waitForConnected(300)) {
            probe.write("show\n");
            probe.waitForBytesWritten(300);
            QMessageBox::information(
                nullptr, QStringLiteral("Steam 行情终端"),
                QStringLiteral("程序已在运行，已为您唤起已有窗口。"));
            return 0;
        }
        QLocalServer::removeServer(QLatin1String(kInstanceKey));
        instanceServer.listen(QLatin1String(kInstanceKey));
    }

    Logger::init();
    AppController controller;

    if (smokeMode) {
        QSize smokeSize(1280, 800);
        const QStringList dimensions = parser.value(smokeSizeOpt).toLower().split(QLatin1Char('x'));
        if (dimensions.size() == 2) {
            bool widthOk = false;
            bool heightOk = false;
            const int width = dimensions.at(0).toInt(&widthOk);
            const int height = dimensions.at(1).toInt(&heightOk);
            if (widthOk && heightOk && width >= 960 && height >= 640) {
                smokeSize = QSize(width, height);
            }
        }
        const int exitCode = controller.runSmokeTest(parser.value(smokeOpt), smokeSize,
                                                     parser.value(smokeSceneOpt));
        return exitCode;
    }
    if (!controller.initialize()) {
        return 1;
    }
    controller.show();
    if (!smokeMode) {
        QObject::connect(&instanceServer, &QLocalServer::newConnection, &controller,
                         [&instanceServer, &controller]() {
                             while (QLocalSocket *conn = instanceServer.nextPendingConnection()) {
                                 conn->deleteLater();
                             }
                             controller.bringToFront();
                         });
    }
    return app.exec();
}
