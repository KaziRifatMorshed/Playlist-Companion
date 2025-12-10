#include "addnewplaylistwindow.h"
#include "ui_addnewplaylistwindow.h"
#include <QDateTime>
#include <QDebug>
#include <QDir>
#include <QDirIterator>
#include <QStringList>
#include <QVector>

#define printdebug qDebug() << "[AddEditPlaylistWindow] "

AddNewPlaylistWindow::AddNewPlaylistWindow(QWidget *parent, int plListId,
                                           QString plpath)
    : QWidget(parent), ui(new Ui::AddNewPlaylistWindow),
      playlistID(plListId) { // POPULATE UI
  ui->setupUi(this);
  dbInstance = SQliteDB::instance();
  ui->folderPath->setText(plpath);

  /* ---- CASE 1 : new playlist ---- */
  if (playlistID == -1) {

    ui->label->setText(
        "<html><head/><body><p align=\"center\"><span style=\" "
        "font-size:16pt;\">Add New Playlist</span></p></body></html>");

    vdos = getAllVideosFromDir(plpath);
    ui->totalVideoCount->setText(QString::number(vdos.count));
    ui->watchedVideoCount->setText(QString::number(0));

    QDir plPath(plpath);
    ui->playlistTitle->setText(plPath.dirName());

    ui->playlistCreationDate->setText(
        QDateTime::currentDateTime().toString("yyyy-MM-dd") + " (yyyy-MM-dd)");

    /* ---- CASE 2 : edit existing playlist ---- */
  } else if (playlistID >= 0) {

    ui->label->setText(
        "<html><head/><body><p align=\"center\"><span style=\" "
        "font-size:16pt;\">Edit Playlist</span></p></body></html>");

    // NOTE: fetch info from database
    vdos = getAllVideosFromDB();

    QSqlQuery playlistInfo =
        dbInstance->execQuery("SELECT * FROM Playlist WHERE playlistId = " +
                              QString::number(playlistID) + ";");
    while (playlistInfo.next()) {
      ui->folderPath->setText(playlistInfo.value("playlistPath").toString());
      ui->playlistTitle->setText(
          playlistInfo.value("playlistTitle").toString());
      ui->comboBox->setCurrentText(playlistInfo.value("status").toString());
      ui->totalVideoCount->setText(
          playlistInfo.value("totalVideoCount").toString());
      ui->watchedVideoCount->setText(
          playlistInfo.value("watchedCount").toString());
      ui->totalHourWatched->setText(
          playlistInfo.value("totalTimeHour").toString());
      ui->playlistCreationDate->setText(
          playlistInfo.value("creationDateTime").toString());
    }
  }
}

AddNewPlaylistWindow::~AddNewPlaylistWindow() { delete ui; }

void AddNewPlaylistWindow::on_pushButton_2_clicked() { // SAVE TO DB
    printdebug << "Started saving to DB";
  // 1. Fetch data from UI
  QString title = ui->playlistTitle->text();
  QString path = ui->folderPath->text();
  QString status =
      ui->comboBox->currentText(); // Status: Planned, Watching, Completed

  // Note: Converting UI text to Int. ensuring defaults if empty.
  int totalCount = ui->totalVideoCount->text().toInt();
  int watchedCount = ui->watchedVideoCount->text().toInt();

  // Assuming you have a widget for hours, if not change this to 0 or specific
  // widget name Based on your read logic, you seemed to imply a field for this.
  int totalHours =
      ui->totalHourWatched->text().toInt(); // Replace with  if widget exists

  // Simple sanitization for SQL strings (doubling single quotes)
  QString safeTitle = title;
  safeTitle.replace("'", "''");
  QString safePath = path;
  safePath.replace("'", "''");

  /* ---- CASE 1 : New Playlist (Insert) ---- */
  if (playlistID == -1) {
    // A. Insert the Playlist Record
    QString sql =
        QString("INSERT INTO Playlist (playlistTitle, playlistPath, status, "
                "totalVideoCount, watchedCount, totalTimeHour) "
                "VALUES ('%1', '%2', '%3', %4, %5, %6);")
            .arg(safeTitle, safePath, status)
            .arg(totalCount)
            .arg(watchedCount)
            .arg(totalHours);

    QSqlQuery insertQuery = dbInstance->execQuery(sql);

    // B. Get the ID of the playlist we just created
    // We need this ID to link the videos in the Video table
    // 2. Get the ID directly from the query object
    // No need to run "SELECT last_insert_rowid()"
    QVariant lastId = insertQuery.lastInsertId();

    int newPlaylistID = -1;
    if (lastId.isValid()) {
        newPlaylistID = lastId.toInt();
    }

    // C. Insert all Videos found in the directory (from vdos struct)
/*
    if (newPlaylistID != -1 && !vdos.fileList.isEmpty()) {
      // Optimization: In a real app, use a Transaction here for speed
      for (const QString &videoPath : vdos.fileList) {
        QString safeVideoPath = videoPath;
        safeVideoPath.replace("'", "''"); // Escape quotes in filenames

        QString videoSql =
            QString(
                "INSERT INTO Video (playlistID, videoPath) VALUES (%1, '%2');")
                .arg(newPlaylistID)
                .arg(safeVideoPath);
        dbInstance->execQuery(videoSql);
      }    }
*/
    if (newPlaylistID != -1 && !vdos.fileList.isEmpty()) {

        // 1. Start Transaction
        dbInstance->execQuery("BEGIN TRANSACTION;");

        for (const QString &videoPath : vdos.fileList) {
            QString safeVideoPath = videoPath;
            qDebug() << safeVideoPath;
            safeVideoPath.replace("'", "''");

            QString videoSql = QString("INSERT INTO Video (playlistID, videoPath) VALUES (%1, '%2');")
                                   .arg(newPlaylistID).arg(safeVideoPath);
            dbInstance->execQuery(videoSql);
        }

        // 2. Commit Transaction
        dbInstance->execQuery("COMMIT;");
    }
  }

  /* ---- CASE 2 : Edit Existing Playlist (Update) ---- */
  else if (playlistID >= 0) {
    // We update Title, Status, Counts, and set updatingDateTime to NOW
    // We usually do NOT update the Video list here unless you want to re-scan
    // the folder

    QString sql = QString("UPDATE Playlist SET "
                          "playlistTitle = '%1', "
                          "status = '%2', "
                          "totalVideoCount = %3, "
                          "watchedCount = %4, "
                          "totalTimeHour = %5, "
                          "updatingDateTime = CURRENT_TIMESTAMP "
                          "WHERE playlistId = %6;")
                      .arg(safeTitle, status)
                      .arg(totalCount)
                      .arg(watchedCount)
                      .arg(totalHours)
                      .arg(playlistID);

    dbInstance->execQuery(sql);
  }

  // Close the window after saving

  printdebug << "End saving data to DB";
  close();
}

VideoCollection AddNewPlaylistWindow::getAllVideosFromDir(QString rootPath) {
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

VideoCollection AddNewPlaylistWindow::getAllVideosFromDB() {
  QSqlQuery allVdosFromDb = dbInstance->execQuery(
      "SELECT * FROM Video WHERE playlistID = " + QString::number(playlistID) +
      ";");

  VideoCollection vdos;
  while (allVdosFromDb.next()) {
    vdos.fileList.append(allVdosFromDb.value("videoPath").toString());
  }
  vdos.count = vdos.fileList.size();
  return vdos;
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



SELECT
    videoID,
    videoPath,
    isWatched
FROM
    Video
WHERE
    playlistID = 1; -- Replace '1' with the desired Playlist ID (int)
*/
