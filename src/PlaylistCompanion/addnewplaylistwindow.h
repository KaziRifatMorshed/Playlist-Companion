#ifndef ADDNEWPLAYLISTWINDOW_H
#define ADDNEWPLAYLISTWINDOW_H

#include <QMap>
#include <QWidget>
#include <include/db_sqlite.h>

#define pldebug qDebug() << "[playlistWindow] "
#define plcritical qCritical() << "[playlistWindow][CRITICAL] "
#define plwarn qWarning() << "[playlistWindow][WARNING] "

namespace Ui {
class AddNewPlaylistWindow;
}


struct VideoCollection {
    QVector<QString> fileList; // Contains full absolute path + filename
    QMap<QString, int> fileDurations; // Maps path to duration in seconds
    int count;
};


#include <QMediaPlayer>

class AddNewPlaylistWindow : public QWidget
{
    Q_OBJECT

signals:
    void playlistDataChanged();

public:
    explicit AddNewPlaylistWindow(QWidget *parent = nullptr, int plListId = -1, QString plpath = "");
    ~AddNewPlaylistWindow();

private slots:
    void on_pushButton_clicked(); // exit button
    void on_pushButton_2_clicked(); // save button
    void processNextVideo();
    void on_updateVideoListOfThisPlaylist_pushButton_clicked();

private:
    Ui::AddNewPlaylistWindow *ui;
    SQliteDB *dbInstance;
    VideoCollection vdos;
    int playlistID;

    QMediaPlayer *m_measurePlayer;
    int m_currentIndex;
    qint64 m_totalDurationMs;

    VideoCollection getAllVideosFromDir(QString rootPath);
    VideoCollection getAllVideosFromDB();
    void startDurationCalculation();
};

#endif // ADDNEWPLAYLISTWINDOW_H
