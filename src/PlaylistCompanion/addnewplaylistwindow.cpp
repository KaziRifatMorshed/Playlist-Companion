#include "addnewplaylistwindow.h"
#include "ui_addnewplaylistwindow.h"
#include <QDebug>
#include <QDir>
#include <QDirIterator>
#include <QStringList>
#include <QDateTime>
#include <QVector>

struct VideoCollection {
  QVector<QString> fileList; // Contains full absolute path + filename
  int count;
};

VideoCollection getAllVideos(QString rootPath) {
  VideoCollection result;
  result.count = 0;

  // 1. Define what counts as a "Video"
  QStringList filters;
  filters << "*.mp4" << "*.avi" << "*.mkv" << "*.mov"
          << "*.wmv" << "*.flv" << "*.webm" << "*.ts";

  // 2. Setup the Iterator
  // QDir::Files -> Only look for files (not folders)
  // QDir::NoDotAndDotDot -> Skip "." and ".."
  // QDirIterator::Subdirectories -> RECURSIVE (Looks inside nested folders)
  QDirIterator it(rootPath, filters, QDir::Files | QDir::NoDotAndDotDot,
                  QDirIterator::Subdirectories);

  // 3. Iterate through the directory tree
  while (it.hasNext()) {
    QString fullPath = it.next(); // Returns full absolute path (e.g.,
                                  // C:/Movies/Action/Matrix.mp4)
    result.fileList.append(fullPath);
    result.count++;
  }

  return result;
}

AddNewPlaylistWindow::AddNewPlaylistWindow(QWidget *parent, int playlistID,
                                           QString plpath)
    : QWidget(parent), ui(new Ui::AddNewPlaylistWindow) { // POPULATE UI
  ui->setupUi(this);
  dbInstance = SQliteDB::instance();
  ui->folderPath->setText(plpath);

  int totalVdoCnt = 0;
  int watchedCnt = 0;

  if (playlistID == -1) { // new playlist
    ui->label->setText(
        "<html><head/><body><p align=\"center\"><span style=\" "
        "font-size:16pt;\">Add New Playlist</span></p></body></html>");
    VideoCollection vdos = getAllVideos(plpath);
    totalVdoCnt = vdos.count;
    QDir plPath(plpath);
    ui->playlistTitle->setText(plPath.dirName());
    ui->playlistCreationDate->setText(QDateTime::currentDateTime().toString("yyyy-MM-dd") + " (yyyy-MM-dd)");

  } else if (playlistID >= 0) { // existing playlist
    ui->label->setText(
        "<html><head/><body><p align=\"center\"><span style=\" "
        "font-size:16pt;\">Edit Playlist</span></p></body></html>");
    // NOTE: fetch info from database
    QSqlQuery playlistInfo = dbInstance->execQuery("");
  }

  ui->totalVideoCount->setText(QString::number(totalVdoCnt));
  ui->watchedVideoCount->setText(QString::number(watchedCnt));
}

AddNewPlaylistWindow::~AddNewPlaylistWindow() { delete ui; }

void AddNewPlaylistWindow::on_pushButton_2_clicked() { // SAVE TO DB
  // NOTE: save to database
  close();
}

/*
INSERT INTO Playlist (
    playlistTitle,
    playlistPath,
    totalVideoCount,
    watchedCount,
    totalTimeHour
)
VALUES (
    'Learn C++ Advanced',        -- playlistTitle
    'D:/Courses/Cpp_Advanced',   -- playlistPath
    24,                          -- totalVideoCount
    0,                           -- watchedCount
    12                           -- totalTimeHour
);


INSERT INTO Playlist (
    playlistTitle,
    playlistPath,
    status,
    totalVideoCount,
    watchedCount,
    totalTimeHour,
    lastWatchedDateTime
)
VALUES (
    'Qt 6 for Beginners',        -- playlistTitle
    'D:/Courses/Qt_Framework',   -- playlistPath
    'Watching',                  -- status (Must match CHECK constraint)
    50,                          -- totalVideoCount
    10,                          -- watchedCount
    25,                          -- totalTimeHour
    CURRENT_TIMESTAMP            -- Sets lastWatched to "now"
);


UPDATE Playlist
SET
    watchedCount = 11,
    status = 'Watching',
    lastWatchedDateTime = CURRENT_TIMESTAMP,
    updatingDateTime = CURRENT_TIMESTAMP
WHERE
    playlistId = 1;
*/
