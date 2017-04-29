
#include "mainwindow.h"
#include "newfiledialog.h"
#include "optionsdialog.h"

#include <QApplication>
#include <QCommandLineParser>
#include <QTextCodec>
#include <QMessageBox>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    a.setOrganizationName("iaito");
    a.setApplicationName("iaito");
    a.setApplicationVersion(APP_VERSION);

    // Set QString codec to UTF-8
    QTextCodec::setCodecForLocale(QTextCodec::codecForName("UTF-8"));
#if QT_VERSION < QT_VERSION_CHECK(5,0,0)
    QTextCodec::setCodecForCStrings(QTextCodec::codecForName("UTF-8"));
    QTextCodec::setCodecForTr(QTextCodec::codecForName("UTF-8"));
#endif


    QCommandLineParser cmdParser;
    cmdParser.setApplicationDescription("A Qt and C++ GUI for radare2 reverse engineering framework");
    cmdParser.addHelpOption();
    cmdParser.addVersionOption();
    cmdParser.addPositionalArgument("filename", QObject::tr("Filename to open."));

    QCommandLineOption analOption({"A", "anal"},
                                  QObject::tr("Automatically start analysis. Needs filename to be specified. May be a value between 0 and 4. Default is 3."),
                                  QObject::tr("level"),
                                  QObject::tr("3"));
    cmdParser.addOption(analOption);

    cmdParser.process(a);

    QStringList args = cmdParser.positionalArguments();

    // Check r2 version
    QString r2version = r_core_version();
    QString localVersion = "" R2_GITTAP;
    if (r2version != localVersion)
    {
        QMessageBox msg;
        msg.setIcon(QMessageBox::Critical);
        msg.setWindowIcon(QIcon(":/new/prefix1/img/logo-small.png"));
        msg.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
        msg.setWindowTitle("Version mismatch!");
        msg.setText(QString("The version used to compile iaito (%1) does not match the binary version of radare2 (%2). This could result in unexpected behaviour. Are you sure you want to continue?").arg(localVersion, r2version));
        if (msg.exec() == QMessageBox::No)
            return 1;
    }



    bool analLevelSpecified = false;
    int analLevel = 0;

    if (cmdParser.isSet(analOption))
    {
        analLevel = cmdParser.value(analOption).toInt(&analLevelSpecified);

        if (!analLevelSpecified || analLevel < 0 || analLevel > 4)
        {
            printf("%s\n", QObject::tr("Invalid Analysis Level. May be a value between 0 and 4.").toLocal8Bit().constData());
            return 1;
        }
    }


    if (args.empty())
    {
        if (analLevelSpecified)
        {
            printf("%s\n", QObject::tr("Filename must be specified to start analysis automatically.").toLocal8Bit().constData());
            return 1;
        }

        NewFileDialog *n = new NewFileDialog();
        n->setAttribute(Qt::WA_DeleteOnClose);
        n->show();
    }
    else // filename specified as positional argument
    {
        OptionsDialog *o = new OptionsDialog(args[0]);
        o->setAttribute(Qt::WA_DeleteOnClose);
        o->show();

        if (analLevelSpecified)
            o->setupAndStartAnalysis(analLevel);
    }

    return a.exec();
}
