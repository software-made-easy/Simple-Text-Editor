#include "textedit.h"

#include <QApplication>
// #include <QScreen>
#include <QCommandLineParser>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    a.setOrganizationName("software-made-easy");
    a.setApplicationName("Simple Text Editor");
    a.setApplicationDisplayName("Simple Text Editor");
    a.setApplicationVersion(APP_VERSION);
    QCommandLineParser parser;
    parser.setApplicationDescription(a.applicationName());
    parser.addHelpOption();
    parser.addVersionOption();
    parser.addPositionalArgument("File", "The file to open.");
    parser.process(a);

    TextEdit mw;

    if (!mw.load(parser.positionalArguments().value(0, QLatin1String(":/example.html"))))
        mw.fileNew();

    mw.show();
    return a.exec();
}
