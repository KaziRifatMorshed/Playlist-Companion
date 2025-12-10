#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QVector>
#include <addnewplaylistwindow.h>
#include <include/db_sqlite.h>
#include <include/structures.h>
#include <settings.h>

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

private:
  Ui::MainWindow *ui;
  int lastWatchedPlId = -1; // -1 or 0 indicates no playlist selected
  int lastWatchedVdoId = -1;
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
};
#endif // MAINWINDOW_H
