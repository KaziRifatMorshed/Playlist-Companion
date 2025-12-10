#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QVector>
#include <addnewplaylistwindow.h>
#include <include/db_sqlite.h>
#include <include/structures.h>
#include <settings.h>
#include <QMediaPlayer>
#include <QVideoSink>
#include <QVideoFrame>
#include <QImage>

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow {
  Q_OBJECT

public:
  MainWindow(QWidget *parent = nullptr);
  ~MainWindow();

private slots:
  void on_pushButton_3_clicked();
  void on_editPlaylistButton_clicked();
  void on_createNewPlaylist_clicked();
  void on_removePlaylist_clicked();
  void on_playlistList_currentIndexChanged(
      int index); // Slot to handle when user selects a different playlist from
                  // the combo box
  void on_allVideosTableWidget_cellDoubleClicked(int row, int column);
  void playThisVdo_clicked();
  void showNextVideo_clicked();
  void showPrevVideo_clicked();
  void vdoNotWatched_clicked();
  void watchedThisVdo_clicked();
  void onFrameChanged(const QVideoFrame &frame);


private:
  Ui::MainWindow *ui;
  int lastWatchedPlId = -1; // -1 or 0 indicates no playlist selected
  int lastWatchedVdoId = -1;
  int currentPlayingVideoId = -1; // New member to track the currently playing video ID

  SQliteDB *dbInstance;
  Settings *settingsWidgt;
  AddNewPlaylistWindow *playlistWindow;
  QVector<Playlist> listOfPlaylists;
  QVector<Video> currentVideoList; // Store videos in memory for easy access
  QString defaultMediaPlayer;
  QString currentOS;

  // --- Helper Function ---
  void initGeneralSettings();
  void updatePlaylistListCombo();
  void populateVideoTable(
      int playlistId); // Helper function to load videos for a specific playlist
  int currentVideoNumberInPlaylist(); // Returns the 0-based index of the current video
  void playThisVdo(int videoId); // Launches the video player for the given video ID
  void showNextVideo();         // Plays the next video in the playlist
  void showPrevVideo();         // Plays the previous video in the playlist
  void vdoNotWatched(int videoId); // Marks a video as unwatched
  void watchedThisVdo(int videoId); // Marks a video as watched
  QString currentVideoTitle();      // Returns the title of the current video
  void updateVideoGroupBox(int videoId); // Updates the video group box UI

  QMediaPlayer *m_mediaPlayer;
  QVideoSink *m_videoSink;
  void generateThumbnail(const QString &videoPath);
};
#endif // MAINWINDOW_H
