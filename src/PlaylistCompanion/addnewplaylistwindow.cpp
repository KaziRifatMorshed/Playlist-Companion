#include "addnewplaylistwindow.h"
#include "ui_addnewplaylistwindow.h"
#include <QCollator>
#include <QDateTime>
#include <QDebug>
#include <QDir>
#include <QDirIterator>
#include <QMessageBox>
#include <QSet>
#include <QStringList>
#include <QVector>
#include <algorithm>

AddNewPlaylistWindow::AddNewPlaylistWindow(QWidget *parent, int plListId,
                                           QString plpath)
    : QWidget(parent), ui(new Ui::AddNewPlaylistWindow),
      playlistID(plListId) { // POPULATE UI
  ui->setupUi(this);
  dbInstance = SQliteDB::instance();
  ui->folderPath->setText(plpath);

  m_measurePlayer = new QMediaPlayer(this);
  m_currentIndex = 0;
  m_totalDurationMs = 0;

  // Use mediaStatusChanged to detect when media is loaded or failed
  connect(m_measurePlayer, &QMediaPlayer::mediaStatusChanged, this, [this](QMediaPlayer::MediaStatus status) {
      if (status == QMediaPlayer::LoadedMedia) {
          qint64 duration = m_measurePlayer->duration();
          if (duration > 0) {
              m_totalDurationMs += duration;
              if (m_currentIndex < vdos.fileList.size()) {
                  QString path = vdos.fileList[m_currentIndex];
                  int durationSec = (int)(duration / 1000);
                  vdos.fileDurations.insert(path, durationSec);

                  // Update duration in DB if it's an existing playlist
                  if (playlistID >= 0) {
                      QString safePath = path;
                      safePath.replace("'", "''");
                      dbInstance->execQuery(QString("UPDATE Video SET duration = %1 WHERE playlistID = %2 AND videoPath = '%3'")
                                            .arg(durationSec).arg(playlistID).arg(safePath));
                  }
              }
          }
          // Move to next video
          m_currentIndex++;
          processNextVideo();
      } else if (status == QMediaPlayer::InvalidMedia) {
          plwarn << "Failed to load media for duration calculation:"
                 << vdos.fileList.value(m_currentIndex);
          m_currentIndex++;
          processNextVideo();
      }
  });

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

    startDurationCalculation();

    /* ---- CASE 2 : edit existing playlist ---- */
  } else if (playlistID >= 0) {
    ui->updateVideoListOfThisPlaylist_pushButton->setEnabled(true);
    ui->label->setText(
        "<html><head/><body><p align=\"center\"><span style=\" "
        "font-size:16pt;\">Edit Playlist</span></p></body></html>");

    // NOTE: fetch info from database
    vdos = AddNewPlaylistWindow::getAllVideosFromDB();

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

void AddNewPlaylistWindow::startDurationCalculation() {
    m_currentIndex = 0;
    m_totalDurationMs = 0;
    if (!vdos.fileList.isEmpty()) {
        m_measurePlayer->setSource(QUrl::fromLocalFile(vdos.fileList[m_currentIndex]));
    }
}

void AddNewPlaylistWindow::processNextVideo() {
    if (m_currentIndex < vdos.fileList.size()) {
        m_measurePlayer->setSource(QUrl::fromLocalFile(vdos.fileList[m_currentIndex]));
    } else {
        // All done
        int totalHours = qRound(m_totalDurationMs / 3600000.0);
        ui->totalHourWatched->setText(QString::number(totalHours));
        pldebug << "Total duration calculation finished:" << totalHours << "hours";

        // Update the Playlist record with new totals if editing
        if (playlistID >= 0) {
            int totalCount = ui->totalVideoCount->text().toInt();
            dbInstance->execQuery(QString("UPDATE Playlist SET totalVideoCount = %1, totalTimeHour = %2 WHERE playlistId = %3")
                                  .arg(totalCount).arg(totalHours).arg(playlistID));
            emit playlistDataChanged();
        }
    }
}

void AddNewPlaylistWindow::on_updateVideoListOfThisPlaylist_pushButton_clicked() {
    if (playlistID < 0) return;

    QString folderPath = ui->folderPath->text();
    if (folderPath.isEmpty()) return;

    pldebug << "Updating video list for playlist ID:" << playlistID;

    // 1. Scan folder for current files
    VideoCollection currentDiskVdos = getAllVideosFromDir(folderPath);

    // 2. Fetch current DB videos
    VideoCollection currentDbVdos = getAllVideosFromDB();

    // Convert to sets for easier comparison
    QSet<QString> diskPaths = QSet<QString>(currentDiskVdos.fileList.begin(), currentDiskVdos.fileList.end());
    QSet<QString> dbPaths = QSet<QString>(currentDbVdos.fileList.begin(), currentDbVdos.fileList.end());

    QSet<QString> toAdd = diskPaths - dbPaths;
    QSet<QString> toRemove = dbPaths - diskPaths;

    if (toAdd.isEmpty() && toRemove.isEmpty()) {
        QMessageBox::information(this, "Update Video List", "No changes detected in the folder.");
        return;
    }

    // 3. Update Database
    dbInstance->execQuery("BEGIN TRANSACTION;");

    // Remove missing videos
    for (const QString &path : toRemove) {
        QString safePath = path;
        safePath.replace("'", "''");
        dbInstance->execQuery(QString("DELETE FROM Video WHERE playlistID = %1 AND videoPath = '%2'")
                              .arg(playlistID).arg(safePath));
    }

    // Add new videos (durations will be 0 for now)
    for (const QString &path : toAdd) {
        QString safePath = path;
        safePath.replace("'", "''");
        QFileInfo info(path);
        QString safeTitle = info.fileName().replace("'", "''");
        dbInstance->execQuery(QString("INSERT INTO Video (playlistID, videoPath, videoTitle, duration) VALUES (%1, '%2', '%3', 0)")
                              .arg(playlistID).arg(safePath).arg(safeTitle));
    }

    dbInstance->execQuery("COMMIT;");

    // 4. Update memory and UI
    vdos = getAllVideosFromDB();
    ui->totalVideoCount->setText(QString::number(vdos.count));
    emit playlistDataChanged();

    // 5. Recalculate durations (this will also update the totalHourWatched label when finished)
    startDurationCalculation();

    QMessageBox::information(this, "Update Video List",
                             QString("Updated successfully!\nAdded: %1\nRemoved: %2")
                             .arg(toAdd.size()).arg(toRemove.size()));
}

void AddNewPlaylistWindow::on_pushButton_clicked() {
    emit playlistDataChanged();
    close();
}

void AddNewPlaylistWindow::on_pushButton_2_clicked() { // SAVE TO DB
    pldebug << "Started saving to DB";
    // 1. Fetch data from UI
    QString title = ui->playlistTitle->text();
    QString path = ui->folderPath->text();        // this value is sensitive
    QString status = ui->comboBox->currentText(); // Status: Planned, Watching, Completed

    // Note: Converting UI text to Int. ensuring defaults if empty.
    int totalCount = ui->totalVideoCount->text().toInt();
    int watchedCount = ui->watchedVideoCount->text().toInt();

    // Assuming you have a widget for hours, if not change this to 0 or specific
    // widget name Based on your read logic, you seemed to imply a field for this.
    int totalHours = ui->totalHourWatched->text().toInt(); // Replace with  if widget exists

    // Simple sanitization for SQL strings (doubling single quotes)
    QString safeTitle = title;
    safeTitle.replace("'", "''");
    QString safePath = path;
    safePath.replace("'", "''");

    /* ---- CASE 1 : New Playlist (Insert) ---- */
    if (playlistID == -1) {
        // A. Insert the Playlist Record
        QString sql = QString("INSERT INTO Playlist (playlistTitle, playlistPath, status, "
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
                    "INSERT INTO Video (playlistID, videoPath) VALUES (%1,
       '%2');") .arg(newPlaylistID) .arg(safeVideoPath);
            dbInstance->execQuery(videoSql);
          }    }
    */
        if (newPlaylistID != -1 && !vdos.fileList.isEmpty()) {
            // 1. Start Transaction
            dbInstance->execQuery("BEGIN TRANSACTION;");

            for (const QString &videoPath : vdos.fileList) {
                QString safeVideoPath = videoPath;
                pldebug << safeVideoPath;

                QFileInfo videoInfo(videoPath);
                QString videoTitle = videoInfo.fileName();
                QString safeVideoTitle = videoTitle;
                safeVideoTitle.replace("'", "''");

                safeVideoPath.replace("'", "''");

                int duration = vdos.fileDurations.value(videoPath, 0);

                QString videoSql = QString("INSERT INTO Video (playlistID, videoPath, videoTitle, "
                                           "duration) VALUES (%1, '%2', '%3', %4);")
                                       .arg(newPlaylistID)
                                       .arg(safeVideoPath)
                                       .arg(safeVideoTitle)
                                       .arg(duration);
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

  emit playlistDataChanged();

  // Close the window after saving

  pldebug << "End saving data to DB";
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

  QCollator collator;
  collator.setNumericMode(true);
  collator.setCaseSensitivity(Qt::CaseInsensitive);
  std::sort(result.fileList.begin(), result.fileList.end(), collator);
  return result;
}

VideoCollection AddNewPlaylistWindow::getAllVideosFromDB() {
  QSqlQuery allVdosFromDb = dbInstance->execQuery(
      "SELECT * FROM Video WHERE playlistID = " + QString::number(playlistID) +
      ";");

  VideoCollection vdos;
  while (allVdosFromDb.next()) {
    QString path = allVdosFromDb.value("videoPath").toString();
    int duration = allVdosFromDb.value("duration").toInt();
    vdos.fileList.append(path);
    vdos.fileDurations.insert(path, duration);
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
