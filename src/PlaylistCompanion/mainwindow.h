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
    void on_pushButton_3_clicked(); // settings
    void on_editPlaylistButton_clicked();
    void on_createNewPlaylist_clicked();
    void on_removePlaylist_clicked();
    void on_playlistList_currentIndexChanged(
        int index); // Slot to handle when user selects a different playlist from
                    // the combo box
    void on_allVideosTableWidget_cellClicked(int row, int column);
    void on_allVideosTableWidget_currentCellChanged(int currentRow, int currentColumn, int previousRow, int previousColumn);
    void on_allVideosTableWidget_cellChanged(int row, int column);
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
  QString currentThumbnailPath; // To track currently being generated thumbnail
  bool isPopulatingTable = false; // To prevent recursion in cellChanged

  // --- Helper Function ---
  void initGeneralSettings();
  void updatePlaylistListCombo();
  void populateVideoTable(
      int playlistId); // Helper function to load videos for a specific playlist
  int currentVideoNumberInPlaylist(); // Returns the 0-based index of the current video
  void playThisVdo(int videoId); // Launches the video player for the given video ID
  void showNextVideo(int startFromId = -1);         // Plays the next video in the playlist
  void showPrevVideo();         // Plays the previous video in the playlist
  void vdoNotWatched(int videoId); // Marks a video as unwatched
  void watchedThisVdo(int videoId); // Marks a video as watched
  void updatePlaylistStatus(int playlistId); // Updates status based on watchedCount
  void updatePlaylistInfoLabels(int playlistId); // Updates only the progress bar and info labels
  int sumRemainingTime(int playlistId); // Sums duration of unwatched videos in hours
  QString currentVideoTitle();      // Returns the title of the current video
  void updateVideoGroupBox(int videoId); // Updates the video group box UI
  void updateTableHighlight(); // Updates only the highlights in the table without re-populating
  void loadCurrentVideoNote(int videoId); // Loads the note for the current video
  void saveCurrentVideoNote();           // Saves the note for the current video

  bool eventFilter(QObject *obj, QEvent *event) override;

  QMediaPlayer *m_mediaPlayer;
  QVideoSink *m_videoSink;
  void generateThumbnail(const QString &videoPath);
};
#endif // MAINWINDOW_H
