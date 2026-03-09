#include "mainwindow.h"

#include <QApplication>
#include <QIcon>
#include <QLocale>
#include <QTranslator>
#include <QtGlobal>

void myMessageOutput(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    // Filter out FFmpeg/QMediaPlayer related output
    if (msg.contains("ffmpeg", Qt::CaseInsensitive) ||
        msg.contains("ffprobe", Qt::CaseInsensitive) ||
        msg.contains("Input #0", Qt::CaseInsensitive) ||
        msg.contains("Metadata:", Qt::CaseInsensitive) ||
        msg.contains("Stream #0", Qt::CaseInsensitive) ||
        msg.contains("Duration:", Qt::CaseInsensitive)) {
        return;
    }

    QByteArray localMsg = msg.toLocal8Bit();
    switch (type) {
    case QtDebugMsg:
        fprintf(stderr, "Debug: %s (%s:%u, %s)\n", localMsg.constData(), context.file, context.line, context.function);
        break;
    case QtInfoMsg:
        fprintf(stderr, "Info: %s (%s:%u, %s)\n", localMsg.constData(), context.file, context.line, context.function);
        break;
    case QtWarningMsg:
        fprintf(stderr, "Warning: %s (%s:%u, %s)\n", localMsg.constData(), context.file, context.line, context.function);
        break;
    case QtCriticalMsg:
        fprintf(stderr, "Critical: %s (%s:%u, %s)\n", localMsg.constData(), context.file, context.line, context.function);
        break;
    case QtFatalMsg:
        fprintf(stderr, "Fatal: %s (%s:%u, %s)\n", localMsg.constData(), context.file, context.line, context.function);
        abort();
    }
}

int main(int argc, char *argv[])
{
    // Silence FFmpeg and Qt Multimedia backend output
    qputenv("QT_LOGGING_RULES", "qt.multimedia.*=false");
    qputenv("FFMPEG_LOG_LEVEL", "quiet");

    qInstallMessageHandler(myMessageOutput);
    QApplication a(argc, argv);
    a.setApplicationName(APP_NAME_STR);
    a.setApplicationVersion(APP_VERSION_STR);
    a.setWindowIcon(QIcon(":/logo/logo.ico"));

    QTranslator translator;
    const QStringList uiLanguages = QLocale::system().uiLanguages();
    for (const QString &locale : uiLanguages) {
        const QString baseName = "PlaylistCompanion_" + QLocale(locale).name();
        if (translator.load(":/i18n/" + baseName)) {
            a.installTranslator(&translator);
            break;
        }
    }
    MainWindow w;
    w.show();
    return a.exec();
}
