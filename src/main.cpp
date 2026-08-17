#include <QApplication>
#include <QCommandLineParser>
#include <QFont>
#include <QSize>
#include <QStringList>

#include "app/AppController.h"
#include "utils/Logger.h"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    QApplication::setApplicationName(QStringLiteral("SteamMarketTerminal"));
    QApplication::setApplicationDisplayName(QStringLiteral("Steam 行情终端"));
    QApplication::setOrganizationName(QStringLiteral("Personal"));
    // 界面可读性：默认字体上调（中文环境小字号显示模糊）
    QFont baseFont = app.font();
    baseFont.setPointSizeF(9.8);
    app.setFont(baseFont);

    QCommandLineParser parser;
    parser.setApplicationDescription(QStringLiteral("Steam 行情终端：Steam 市场数据统计小软件"));
    parser.addHelpOption();
    const QCommandLineOption smokeOpt(QStringLiteral("smoke-test"),
                                      QStringLiteral("运行冒烟自测；可传截图输出路径"),
                                      QStringLiteral("pngPath"));
    parser.addOption(smokeOpt);
    const QCommandLineOption smokeSizeOpt(
        QStringLiteral("smoke-size"), QStringLiteral("Smoke window size, for example 1280x800"),
        QStringLiteral("widthxheight"), QStringLiteral("1280x800"));
    parser.addOption(smokeSizeOpt);
    parser.process(app);

    Logger::init();
    AppController controller;

    if (parser.isSet(smokeOpt)) {
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
        const int exitCode = controller.runSmokeTest(parser.value(smokeOpt), smokeSize);
        return exitCode;
    }
    if (!controller.initialize()) {
        return 1;
    }
    controller.show();
    return app.exec();
}
