#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QDebug>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QMessageBox>
#include <QRandomGenerator>
#include <QTime>
#include <QTimer>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow) {
  ui->setupUi(this);
  ui->currentVideoNote_textEdit->installEventFilter(this);
  MainWindow::dbInstance = SQliteDB::instance();
  initGeneralSettings();
  MainWindow::updatePlaylistListCombo();
  MainWindow::populateVideoTable(MainWindow::lastWatchedPlId);

  // Initialize currentPlayingVideoId from lastWatchedVdoId for the currently
  // loaded playlist
  currentPlayingVideoId = lastWatchedVdoId;

  // The on_allVideosTableWidget_cellClicked slot is auto-connected by the
  // uic. A manual connect call is not needed and would cause the slot to fire
  // twice.

  // Connect the buttons in the video group box
  connect(ui->playThisVdo, &QPushButton::clicked, this,
          &MainWindow::playThisVdo_clicked);
  connect(ui->showNextVideo, &QPushButton::clicked, this,
          &MainWindow::showNextVideo_clicked);
  connect(ui->showPrevVideo, &QPushButton::clicked, this,
          &MainWindow::showPrevVideo_clicked);
  connect(ui->vdoNotWatched, &QPushButton::clicked, this,
          &MainWindow::vdoNotWatched_clicked);
    connect(ui->watchedThisVdo, &QPushButton::clicked, this, &MainWindow::watchedThisVdo_clicked);
  
    // --- Thumbnail Generation Setup ---
    m_mediaPlayer = new QMediaPlayer(this);
    m_videoSink = new QVideoSink(this);
    m_mediaPlayer->setVideoSink(m_videoSink);
  
    // Connect the sink's frameChanged signal to a slot
    connect(m_videoSink, &QVideoSink::videoFrameChanged, this, &MainWindow::onFrameChanged);
  
    // Update the video group box with the last watched video on startup
    updateVideoGroupBox(lastWatchedVdoId);
  }

MainWindow::~MainWindow() { delete ui; }

void MainWindow::on_pushButton_3_clicked() // settings
{
    settingsWidgt = new Settings();
    // Connect the signal before showing
    connect(settingsWidgt, &Settings::settingsChanged, this, &MainWindow::initGeneralSettings);

    // 2. Set Modality: This disables the MainWindow while Settings is open
    settingsWidgt->setWindowModality(Qt::ApplicationModal);
    // 3. (Optional) Make sure it deletes itself from memory when closed
    // preventing memory leaks since you use 'new' every time.
    settingsWidgt->setAttribute(Qt::WA_DeleteOnClose);
    settingsWidgt->show();
}

void MainWindow::on_editPlaylistButton_clicked() {
  int playlistId = ui->playlistList->currentData().toInt();
  if (playlistId > 0) {
    playlistWindow = new AddNewPlaylistWindow(nullptr, playlistId);
    playlistWindow->setAttribute(Qt::WA_DeleteOnClose);
    connect(playlistWindow, &AddNewPlaylistWindow::destroyed, this,
            &MainWindow::updatePlaylistListCombo);
    playlistWindow->show();
  } else {
    QMessageBox::warning(this, "No playlist selected",
                         "Please select a playlist to edit.");
  }
}

void MainWindow::on_createNewPlaylist_clicked() {
  // get which directory
  QString plpath = QFileDialog::getExistingDirectory(
      this, "Select a folder that contains your desired videos",
      QDir::homePath(), QFileDialog::ShowDirsOnly);
  // check whether the directory exists
  if (plpath.isEmpty()) {
    QMessageBox::warning(this, "Directory failed to select !!!",
                         "Directory failed to select!");
  } else {
    playlistWindow = new AddNewPlaylistWindow(
        nullptr, -1, plpath); // this does not open new window, rather overrides
                              // current window
    playlistWindow->setAttribute(Qt::WA_DeleteOnClose);
    connect(playlistWindow, &AddNewPlaylistWindow::destroyed, this,
            &MainWindow::updatePlaylistListCombo);
    playlistWindow->show();
  }
}

void MainWindow::on_removePlaylist_clicked() {
  int playlistId = ui->playlistList->currentData().toInt();
  if (playlistId > 0) {
    QMessageBox::StandardButton reply;
    reply = QMessageBox::question(
        this, "Delete Playlist",
        "Are you sure you want to delete this playlist and all its videos?",
        QMessageBox::Yes | QMessageBox::No);
    if (reply == QMessageBox::Yes) {
      // 1. Delete videos associated with the playlist
      dbInstance->execQuery(
          QString("DELETE FROM Video WHERE playlistID = %1").arg(playlistId));
      // 2. Delete the playlist itself
      dbInstance->execQuery(
          QString("DELETE FROM Playlist WHERE playlistId = %1")
              .arg(playlistId));

      // 3. Update 'General' if this was the last watched playlist
      if (playlistId == lastWatchedPlId) {
          lastWatchedPlId = -1;
          currentPlayingVideoId = -1;
          dbInstance->execQuery("UPDATE General SET lastWatchedPlId = -1, lastWatchedVdoId = -1 WHERE id = 1");
      }

      // 4. Refresh UI
      updatePlaylistListCombo();

      // If no playlists left, the combo's currentData will be invalid,
      // which will trigger currentIndexChanged( -1 ) if it was cleared.
      // But updatePlaylistListCombo might select another one.
      // If count is 0, we must clear the table manually if signal doesn't.
      if (ui->playlistList->count() == 0) {
          currentVideoList.clear();
          ui->allVideosTableWidget->setRowCount(0);
          updateVideoGroupBox(-1);
          // Clear labels
          ui->playlistCreationDate->setText("");
          ui->lastWatched->setText("");
          ui->totalTime->setText("");
          ui->progressBar->setValue(0);
          ui->playlistProgressCount->setText("0/0");
      }
    }
  } else {
    QMessageBox::warning(this, "No playlist selected",
                         "Please select a playlist to remove.");
  }
}

void MainWindow::initGeneralSettings() {
  // 1. Determine the current Operating System
  // The DB schema 'General' table has a CHECK constraint: OS IN ('Windows',
  // 'Linux', 'Mac')
#ifdef Q_OS_WIN
  currentOS = "Windows";
#elif defined(Q_OS_LINUX)
  currentOS = "Linux";
#elif defined(Q_OS_MAC)
  currentOS = "Mac";
#else
  currentOS = "Windows"; // Fallback, though likely unnecessary
#endif

  // 2. Attempt to fetch existing data
  // We only select the columns we need. ID is always 1.
  QString q = "SELECT defaultMediaPlayer, lastWatchedPlId, lastWatchedVdoId "
              "FROM General WHERE id = 1";
  QSqlQuery query = dbInstance->execQuery(q);

  if (query.next()) {
    // --- DATA FOUND: Load into variables ---
    defaultMediaPlayer = query.value("defaultMediaPlayer").toString();

    // toInt() returns 0 if the value is NULL or invalid.
    // Since IDs are AUTOINCREMENT (starting at 1), 0 is a safe "empty" state.
    lastWatchedPlId = query.value("lastWatchedPlId").toInt();
    lastWatchedVdoId = query.value("lastWatchedVdoId").toInt();

  } else {
    // --- TABLE EMPTY: Insert default row ---
    // The table has a constraint CHECK(id = 1), so we explicitly set id=1.
    // We use the determined currentOS.

    QString insertQ =
        QString("INSERT INTO General (id, OS, defaultMediaPlayer) "
                "VALUES (1, '%1', '')")
            .arg(currentOS);

    dbInstance->execQuery(insertQ);

    // Initialize local variables to defaults
    defaultMediaPlayer = "";
    lastWatchedPlId = -1;
    lastWatchedVdoId = -1;

    qDebug()
        << "[MainWindow] General info was empty. Initialized defaults for OS:"
        << currentOS;
    MainWindow::on_pushButton_3_clicked();
  }
}

void MainWindow::updatePlaylistListCombo() {
  QComboBox *combo = ui->playlistList;

  // Store the currently selected playlist ID before clearing
  int currentId = combo->currentData().toInt();
  if (currentId <= 0) {
      currentId = lastWatchedPlId;
  }

  // Block signals so that clearing/adding doesn't trigger on_playlistList_currentIndexChanged
  combo->blockSignals(true);

  // 2. Clear previous data to avoid duplicates
  listOfPlaylists.clear();
  combo->clear();

  // 3. Execute Query to fetch all playlists
  QString q = "SELECT * FROM Playlist ORDER BY playlistId ASC";
  QSqlQuery query = dbInstance->execQuery(q);

  // 4. Iterate through results
  while (query.next()) {
    Playlist pl;
    pl.playlistId = query.value("playlistId").toInt();
    pl.playlistTitle = query.value("playlistTitle").toString();
    pl.playlistPath = query.value("playlistPath").toString();
    pl.status = query.value("status").toString();
    pl.totalVideoCount = query.value("totalVideoCount").toInt();
    pl.watchedCount = query.value("watchedCount").toInt();
    pl.totalTimeHour = query.value("totalTimeHour").toInt();
    pl.creationDateTime = query.value("creationDateTime").toString();
    pl.lastWatchedDateTime = query.value("lastWatchedDateTime").toString();

    listOfPlaylists.append(pl);
    combo->addItem(pl.playlistTitle, pl.playlistId);
  }

  // Restore the previous selection or auto-select the last watched playlist
  int indexToSelect = -1;
  if (currentId > 0) {
      indexToSelect = combo->findData(currentId);
  }
  
  if (indexToSelect == -1 && lastWatchedPlId != -1) {
      indexToSelect = combo->findData(lastWatchedPlId);
  }

  if (indexToSelect != -1) {
      combo->setCurrentIndex(indexToSelect);
  } else if (combo->count() > 0) {
      combo->setCurrentIndex(0);
  }

  // Unblock signals
  combo->blockSignals(false);

  // If the playlist ID is still the same, we only need to update the labels/stats.
  // If it changed, the combo box signal (if not blocked) or our manual call should handle it.
  int newId = combo->currentData().toInt();
  if (newId == currentId && newId > 0) {
      updatePlaylistInfoLabels(newId);
  } else {
      on_playlistList_currentIndexChanged(combo->currentIndex());
  }
}

void MainWindow::populateVideoTable(int playlistId) {
  isPopulatingTable = true;
  // 1. Clear existing data
  currentVideoList.clear();
  ui->allVideosTableWidget->setRowCount(0);

  // 2. Setup Table Headers (if not done in UI designer)
  // Column 0: Watched Status, Column 1: Video Name, Column 2: Video Length
  ui->allVideosTableWidget->setColumnCount(3);
  ui->allVideosTableWidget->setHorizontalHeaderLabels(QStringList()
                                                      << "Watched?"
                                                      << "Video Name"
                                                      << "Video Length");

  // Adjust column widths (Status column small, Name column stretches)
  ui->allVideosTableWidget->setColumnWidth(0, 60);
  ui->allVideosTableWidget->horizontalHeader()->setSectionResizeMode(
      1, QHeaderView::Stretch);
  ui->allVideosTableWidget->setColumnWidth(2, 100);

  // 3. Prepare Query
  // We fetch videos only for the selected playlist
  QString q =
      QString("SELECT * FROM Video WHERE playlistID = %1 ORDER BY videoID ASC")
          .arg(playlistId);
  QSqlQuery query = dbInstance->execQuery(q);

  int row = 0;
  while (query.next()) {
    Video vdo;
    vdo.videoID = query.value("videoID").toInt();
    vdo.playlistID = query.value("playlistID").toInt();
    vdo.videoPath = query.value("videoPath").toString();
    vdo.isWatched = query.value("isWatched").toInt();
    vdo.duration = query.value("duration").toInt();

    // Add to local memory vector
    currentVideoList.append(vdo);

    // --- UI POPULATION ---
    ui->allVideosTableWidget->insertRow(row);

    // Col 0: Watched Status (Checkbox)
    QTableWidgetItem *statusItem = new QTableWidgetItem();
    statusItem->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled |
                         Qt::ItemIsSelectable);

    statusItem->setCheckState(
        vdo.isWatched ? Qt::Checked
                      : Qt::Unchecked); // Set Checkbox state based on DB value
    statusItem->setData(Qt::UserRole,
                        vdo.videoID); // Store the videoID in the item so we can
                                      // identify it later if clicked

    ui->allVideosTableWidget->setItem(row, 0, statusItem);

    // Col 1: Video Name (Clean display)
    QFileInfo fileInfo(vdo.videoPath);
    QTableWidgetItem *nameItem = new QTableWidgetItem(fileInfo.fileName());
    // Make it read-only (user can't rename file here)
    nameItem->setFlags(nameItem->flags() ^ Qt::ItemIsEditable);

    ui->allVideosTableWidget->setItem(row, 1, nameItem);

    // Col 2: Video Length
    QString timeStr =
        QTime(0, 0)
            .addSecs(vdo.duration)
            .toString(vdo.duration >= 3600 ? "hh:mm:ss" : "mm:ss");
    QTableWidgetItem *durationItem = new QTableWidgetItem(timeStr);
    durationItem->setFlags(durationItem->flags() ^ Qt::ItemIsEditable);
    durationItem->setTextAlignment(Qt::AlignCenter);
    ui->allVideosTableWidget->setItem(row, 2, durationItem);

    // Highlight the current playing video
    if (vdo.videoID == currentPlayingVideoId) {
      for (int col = 0; col < ui->allVideosTableWidget->columnCount(); ++col) {
        QTableWidgetItem *itemToHighlight =
            ui->allVideosTableWidget->item(row, col);
        if (itemToHighlight) {
          QColor highlightColor(Qt::yellow);
          highlightColor.setAlpha(40); // 15% opacity
          itemToHighlight->setBackground(highlightColor);
        }
      }
      // Ensure the row is selected when rebuilding the table
      ui->allVideosTableWidget->selectRow(row);
    }

    row++;
  }
  isPopulatingTable = false;
}

void MainWindow::on_playlistList_currentIndexChanged(int index) {
  // Get the UserData (Playlist ID) we stored earlier in
  // updatePlaylistListCombo
  int playlistId =
      ui->playlistList->currentData().toInt(); // kmne kaj korlo !!!!

  bool isValidPlaylist = playlistId > 0;
  ui->editPlaylistButton->setEnabled(isValidPlaylist);
  ui->removePlaylist->setEnabled(isValidPlaylist);

  if (isValidPlaylist) { // -1 or 0 usually indicates invalid ID or "Select
                         // Playlist..." placeholder
    // Determine currentPlayingVideoId for the newly selected playlist
    QString queryLastVdoId =
        QString("SELECT lastWatchedVdoId FROM General WHERE id = 1");
    QSqlQuery query = dbInstance->execQuery(queryLastVdoId);
    int generalLastVdoId = -1;
    if (query.next()) {
      generalLastVdoId = query.value("lastWatchedVdoId").toInt();
    }

    // Determine if currentPlayingVideoId is already valid for this playlist
    bool alreadyBelongs = false;
    if (currentPlayingVideoId != -1) {
      QString checkVdoPlaylist = QString("SELECT videoID FROM Video WHERE "
                                         "videoID = %1 AND playlistID = %2")
                                     .arg(currentPlayingVideoId)
                                     .arg(playlistId);
      QSqlQuery checkQuery = dbInstance->execQuery(checkVdoPlaylist);
      if (checkQuery.next()) {
        alreadyBelongs = true;
      }
    }

    if (!alreadyBelongs) {
      currentPlayingVideoId = -1; // Reset before checking
      if (generalLastVdoId != -1) {
        // Check if the last watched video from General settings belongs to the
        // currently selected playlist
        QString checkVdoPlaylist = QString("SELECT videoID FROM Video WHERE "
                                           "videoID = %1 AND playlistID = %2")
                                       .arg(generalLastVdoId)
                                       .arg(playlistId);
        QSqlQuery checkQuery = dbInstance->execQuery(checkVdoPlaylist);
        if (checkQuery.next()) {
          currentPlayingVideoId = generalLastVdoId; // It belongs, so set it
        }
      }
    }

    populateVideoTable(playlistId);
    updateVideoGroupBox(currentPlayingVideoId);
    lastWatchedPlId = playlistId; // Update the global tracker

    updatePlaylistInfoLabels(playlistId);

    // Update 'General' table in DB so app remembers this selection next time
    QString q = QString("UPDATE General SET lastWatchedPlId = %1 WHERE id = 1")
                    .arg(playlistId);
    dbInstance->execQuery(q);
  } else {
    // Clear everything if no playlist is selected
    currentVideoList.clear();
    ui->allVideosTableWidget->setRowCount(0);
    updateVideoGroupBox(-1);

    ui->playlistCreationDate->setText("");
    ui->lastWatched->setText("");
    ui->totalTime->setText("");
    ui->progressBar->setValue(0);
    ui->playlistProgressCount->setText("0/0");
  }
}

void MainWindow::updatePlaylistInfoLabels(int playlistId) {
  if (playlistId <= 0) return;

  // Find the playlist in our list or re-fetch from DB for most accurate counts
  QString q = QString("SELECT * FROM Playlist WHERE playlistId = %1").arg(playlistId);
  QSqlQuery query = dbInstance->execQuery(q);
  
  if (query.next()) {
    Playlist pl;
    pl.playlistId = query.value("playlistId").toInt();
    pl.creationDateTime = query.value("creationDateTime").toString();
    pl.lastWatchedDateTime = query.value("lastWatchedDateTime").toString();
    pl.watchedCount = query.value("watchedCount").toInt();
    pl.totalVideoCount = query.value("totalVideoCount").toInt();
    pl.totalTimeHour = query.value("totalTimeHour").toInt();

    // Now update the UI elements
    ui->playlistCreationDate->setText(pl.creationDateTime);
    ui->lastWatched->setText(pl.lastWatchedDateTime);
    
    int remainingHours = sumRemainingTime(playlistId);
    ui->totalTime->setText(QString("%1/%2 hours")
                           .arg(remainingHours)
                           .arg(pl.totalTimeHour));

    // Progress bar and count
    if (pl.totalVideoCount > 0) {
      int progress = (pl.watchedCount * 100) / pl.totalVideoCount;
      ui->progressBar->setValue(progress);
    } else {
      ui->progressBar->setValue(0);
    }
    ui->playlistProgressCount->setText(
        QString("%1/%2")
            .arg(pl.watchedCount)
            .arg(pl.totalVideoCount));
            
    // Update memory list so it stays in sync
    for (int i = 0; i < listOfPlaylists.size(); ++i) {
      if (listOfPlaylists[i].playlistId == playlistId) {
        listOfPlaylists[i].watchedCount = pl.watchedCount;
        listOfPlaylists[i].lastWatchedDateTime = pl.lastWatchedDateTime;
        listOfPlaylists[i].status = query.value("status").toString();
        break;
      }
    }
  }
}

// --- Video Playback & Navigation ---
int MainWindow::currentVideoNumberInPlaylist()
{ // inefficient
    for (int i = 0; i < currentVideoList.size(); ++i) {
        if (currentVideoList[i].videoID == currentPlayingVideoId) {
            return i;
        }
    }
    return -1; // Not found
}

void MainWindow::watchedThisVdo(int videoId) {
  // First, check if the video is already watched. If so, do nothing.
  Video currentVideo;
  bool found = false;
  for (const auto &vdo : currentVideoList) {
    if (vdo.videoID == videoId) {
      currentVideo = vdo;
      found = true;
      break;
    }
  }
  if (found && currentVideo.isWatched) {
    // If already watched, just move to the next video's info
    showNextVideo(videoId);
    return;
  }

  // --- Mark as Watched ---
  QString updateVideoQuery =
      QString("UPDATE Video SET isWatched = 1 WHERE videoID = %1").arg(videoId);
  dbInstance->execQuery(updateVideoQuery);

  // Update the playlist's watchedCount
  int currentPlaylistId = ui->playlistList->currentData().toInt();
  if (currentPlaylistId > 0) {
    QString updatePlaylistQuery =
        QString("UPDATE Playlist SET watchedCount = watchedCount + 1, "
                "lastWatchedDateTime = '%1' WHERE playlistId = %2")
            .arg(QDateTime::currentDateTime().toString(Qt::ISODate))
            .arg(currentPlaylistId);
    dbInstance->execQuery(updatePlaylistQuery);

    // Update status
    updatePlaylistStatus(currentPlaylistId);

    updatePlaylistInfoLabels(currentPlaylistId);
  }

  // Update memory state of the video
  for (int i = 0; i < currentVideoList.size(); ++i) {
    if (currentVideoList[i].videoID == videoId) {
      currentVideoList[i].isWatched = 1;
      break;
    }
  }

  // Update the checkbox in the table if it exists
  isPopulatingTable = true;
  for (int row = 0; row < ui->allVideosTableWidget->rowCount(); ++row) {
    QTableWidgetItem *item = ui->allVideosTableWidget->item(row, 0);
    if (item && item->data(Qt::UserRole).toInt() == videoId) {
      item->setCheckState(Qt::Checked);
      break;
    }
  }
  isPopulatingTable = false;

  // --- Advance to Next Video ---
  showNextVideo(videoId);
}

void MainWindow::vdoNotWatched(int videoId) {
  // First, check if the video is already not watched. If so, do nothing.
  Video currentVideo;
  bool found = false;
  for (const auto &vdo : currentVideoList) {
    if (vdo.videoID == videoId) {
      currentVideo = vdo;
      found = true;
      break;
    }
  }

  if (found && !currentVideo.isWatched) {
    // If already not watched, just do nothing or maybe just update next info
    // if needed.
    return;
  }

  QString updateVideoQuery =
      QString("UPDATE Video SET isWatched = 0 WHERE videoID = %1").arg(videoId);
  dbInstance->execQuery(updateVideoQuery);

  // Update the playlist's watchedCount
  int currentPlaylistId = ui->playlistList->currentData().toInt();
  if (currentPlaylistId > 0) {
    QString updatePlaylistQuery =
        QString("UPDATE Playlist SET watchedCount = watchedCount - 1 WHERE "
                "playlistId = %1")
            .arg(currentPlaylistId);
    dbInstance->execQuery(updatePlaylistQuery);

    // Update status
    updatePlaylistStatus(currentPlaylistId);

    updatePlaylistInfoLabels(currentPlaylistId);
  }

  // Update memory state of the video
  for (int i = 0; i < currentVideoList.size(); ++i) {
    if (currentVideoList[i].videoID == videoId) {
      currentVideoList[i].isWatched = 0;
      break;
    }
  }

  // Update the checkbox in the table if it exists
  isPopulatingTable = true;
  for (int row = 0; row < ui->allVideosTableWidget->rowCount(); ++row) {
    QTableWidgetItem *item = ui->allVideosTableWidget->item(row, 0);
    if (item && item->data(Qt::UserRole).toInt() == videoId) {
      item->setCheckState(Qt::Unchecked);
      break;
    }
  }
  isPopulatingTable = false;
}

void MainWindow::playThisVdo(int videoId) {
  // 1. Find the video in the currentVideoList
  Video targetVideo;
  bool found = false;
  for (const auto &vdo : currentVideoList) {
    if (vdo.videoID == videoId) {
      targetVideo = vdo;
      found = true;
      break;
    }
  }

  if (!found) {
    QMessageBox::warning(this, "Playback Error",
                         "Video not found in current playlist.");
    return;
  }

  // 2. Update current playing video ID
  currentPlayingVideoId = videoId;
  lastWatchedVdoId = videoId;   // Also update last watched for session
  updateVideoGroupBox(videoId); // Update the UI to reflect the new video

  // 3. Mark as watched if not already (REMOVED as per user request)
  // if (!targetVideo.isWatched) {
  //     watchedThisVdo(videoId);
  // }
  // Refresh table to update highlight
  updateTableHighlight();

  // 4. Update lastWatchedVdoId in General settings
  QString updateGeneralQuery =
      QString("UPDATE General SET lastWatchedVdoId = %1 WHERE id = 1")
          .arg(videoId);
  dbInstance->execQuery(updateGeneralQuery);

  // 5. Launch the video using QProcess
  QString program = QDir::toNativeSeparators(defaultMediaPlayer);
  QStringList arguments;
  arguments << QDir::toNativeSeparators(targetVideo.videoPath);

  // TODO: Add support for resumeTime if the media player supports it
  // For VLC, it might be something like: arguments << "--start-time" <<
  // QString::number(targetVideo.resumeTime);

      QProcess *process = new QProcess(this);
      process->setProcessChannelMode(QProcess::SeparateChannels); // Suppress output from the media player
      process->start(program, arguments);
      if (!process->waitForStarted()) {    QMessageBox::critical(this, "Player Launch Error",
                          "Could not start media player: " + program);
    qDebug() << "Failed to start media player:" << process->errorString();
  } else {
    qDebug() << "Playing video:" << targetVideo.videoPath;
    qDebug() << "With player:" << program;
  }
}

void MainWindow::showNextVideo(int startFromId) {
  int targetId = (startFromId != -1) ? startFromId : currentPlayingVideoId;
  int currentIdx = -1;
  for (int i = 0; i < currentVideoList.size(); ++i) {
    if (currentVideoList[i].videoID == targetId) {
      currentIdx = i;
      break;
    }
  }

  if (currentIdx != -1 && currentIdx < currentVideoList.size() - 1) {
    currentPlayingVideoId = currentVideoList[currentIdx + 1].videoID;
    updateVideoGroupBox(currentPlayingVideoId);
    // Select the row in the table (triggers currentCellChanged which handles everything if needed,
    // but we already updated currentPlayingVideoId, so we just need highlight)
    isPopulatingTable = true;
    ui->allVideosTableWidget->selectRow(currentIdx + 1);
    isPopulatingTable = false;
    updateTableHighlight();
  } else if (currentIdx == currentVideoList.size() - 1) {
    QMessageBox::information(this, "End of Playlist",
                             "This is the last video in the playlist.");
  } else {
    // If no video is playing or found, select the first one
    if (!currentVideoList.isEmpty()) {
      currentPlayingVideoId = currentVideoList[0].videoID;
      updateVideoGroupBox(currentPlayingVideoId);
      isPopulatingTable = true;
      ui->allVideosTableWidget->selectRow(0);
      isPopulatingTable = false;
      updateTableHighlight();
    } else {
      QMessageBox::warning(this, "Navigation Error", "Playlist is empty.");
    }
  }
}

void MainWindow::showPrevVideo() {
  int currentIdx = currentVideoNumberInPlaylist();
  if (currentIdx > 0) {
    currentPlayingVideoId = currentVideoList[currentIdx - 1].videoID;
    updateVideoGroupBox(currentPlayingVideoId);
    isPopulatingTable = true;
    ui->allVideosTableWidget->selectRow(currentIdx - 1);
    isPopulatingTable = false;
    updateTableHighlight();
  } else if (currentIdx == 0) {
    QMessageBox::information(this, "Beginning of Playlist",
                             "This is the first video in the playlist.");
  } else {
    // If no video is playing, select the last one
    if (!currentVideoList.isEmpty()) {
      currentPlayingVideoId = currentVideoList.last().videoID;
      updateVideoGroupBox(currentPlayingVideoId);
      isPopulatingTable = true;
      ui->allVideosTableWidget->selectRow(currentVideoList.size() - 1);
      isPopulatingTable = false;
      updateTableHighlight();
    } else {
      QMessageBox::warning(this, "Navigation Error", "Playlist is empty.");
    }
  }
}
QString MainWindow::currentVideoTitle() {
  for (const auto &vdo : currentVideoList) {
    if (vdo.videoID == currentPlayingVideoId) {
      QFileInfo fileInfo(vdo.videoPath);
      return fileInfo.fileName();
    }
  }
  return "No Video Playing";
}

void MainWindow::on_allVideosTableWidget_cellClicked(int row,
                                                            int column) {
  // Get the video ID from the clicked row (stored in the first column's item
  // data)
  QTableWidgetItem *item = ui->allVideosTableWidget->item(row, 0);
  if (item) {
    int videoId = item->data(Qt::UserRole).toInt();
    if (videoId > 0) {
      if (currentPlayingVideoId == videoId) {
          return; // Already selected, skip re-generation and re-population
      }
      // Do NOT play the video, just update the current selection and UI
      currentPlayingVideoId = videoId;
      // Update only the highlight without re-populating the whole table
      updateTableHighlight();
      updateVideoGroupBox(currentPlayingVideoId); // Update the info box
    }
  }
}

void MainWindow::updateTableHighlight() {
    isPopulatingTable = true; // Prevent signals from firing during UI update
    for (int row = 0; row < ui->allVideosTableWidget->rowCount(); ++row) {
        QTableWidgetItem *firstColItem = ui->allVideosTableWidget->item(row, 0);
        if (firstColItem) {
            int videoId = firstColItem->data(Qt::UserRole).toInt();
            bool isCurrent = (videoId == currentPlayingVideoId);
            
            for (int col = 0; col < ui->allVideosTableWidget->columnCount(); ++col) {
                QTableWidgetItem *item = ui->allVideosTableWidget->item(row, col);
                if (item) {
                    if (isCurrent) {
                        QColor highlightColor(Qt::yellow);
                        highlightColor.setAlpha(40); // 15% opacity
                        item->setBackground(highlightColor);
                    } else {
                        // Reset background for other rows
                        item->setBackground(QBrush());
                    }
                }
            }
        }
    }
    isPopulatingTable = false;
}

void MainWindow::on_allVideosTableWidget_currentCellChanged(int currentRow, int currentColumn, int previousRow, int previousColumn) {
    if (isPopulatingTable || currentRow < 0) {
        return;
    }

    // Reuse the same logic as cellClicked but only if the row actually changed
    if (currentRow != previousRow) {
        on_allVideosTableWidget_cellClicked(currentRow, currentColumn);
    }
}

void MainWindow::updatePlaylistStatus(int playlistId) {
    if (playlistId <= 0) return;

    QString q = QString("SELECT watchedCount, totalVideoCount FROM Playlist WHERE playlistId = %1").arg(playlistId);
    QSqlQuery query = dbInstance->execQuery(q);
    if (query.next()) {
        int watched = query.value(0).toInt();
        int total = query.value(1).toInt();
        QString status = "Planned to Watch";

        if (watched > 0) {
            if (watched >= total && total > 0) {
                status = "Completed";
            } else {
                status = "Watching";
            }
        }

        QString updateQ = QString("UPDATE Playlist SET status = '%1' WHERE playlistId = %2")
                .arg(status)
                .arg(playlistId);
        dbInstance->execQuery(updateQ);
    }
}

int MainWindow::sumRemainingTime(int playlistId) {
    if (playlistId <= 0) return 0;
    QString q = QString("SELECT SUM(duration) FROM Video WHERE playlistID = %1 AND isWatched = 0").arg(playlistId);
    QSqlQuery query = dbInstance->execQuery(q);
    if (query.next()) {
        return qRound(query.value(0).toLongLong() / 3600.0);
    }
    return 0;
}

void MainWindow::on_allVideosTableWidget_cellChanged(int row, int column) {
  if (isPopulatingTable || column != 0) {
    return;
  }

  QTableWidgetItem *item = ui->allVideosTableWidget->item(row, 0);
  if (!item) return;

  int videoId = item->data(Qt::UserRole).toInt();
  bool isChecked = (item->checkState() == Qt::Checked);

  // 1. Update Video table
  QString updateVideoQuery =
      QString("UPDATE Video SET isWatched = %1 WHERE videoID = %2")
          .arg(isChecked ? 1 : 0)
          .arg(videoId);
  dbInstance->execQuery(updateVideoQuery);

  // 2. Update Playlist watchedCount
  int currentPlaylistId = ui->playlistList->currentData().toInt();
  if (currentPlaylistId > 0) {
    QString updatePlaylistQuery;
    if (isChecked) {
      updatePlaylistQuery =
          QString("UPDATE Playlist SET watchedCount = watchedCount + 1, "
                  "lastWatchedDateTime = '%1' WHERE playlistId = %2")
              .arg(QDateTime::currentDateTime().toString(Qt::ISODate))
              .arg(currentPlaylistId);
    } else {
      updatePlaylistQuery =
          QString("UPDATE Playlist SET watchedCount = watchedCount - 1 WHERE "
                  "playlistId = %1")
              .arg(currentPlaylistId);
    }
    dbInstance->execQuery(updatePlaylistQuery);

    // 2.1 Update Playlist status
    updatePlaylistStatus(currentPlaylistId);

    // 3. Refresh UI (Playlist counts, progress bar, etc.)
    updatePlaylistInfoLabels(currentPlaylistId);
  }

  // 4. Update memory state of the video
  for (int i = 0; i < currentVideoList.size(); ++i) {
    if (currentVideoList[i].videoID == videoId) {
      currentVideoList[i].isWatched = isChecked ? 1 : 0;
      break;
    }
  }
}

void MainWindow::updateVideoGroupBox(int videoId) {
    ui->currentVideoThumbnail->clear(); // Clear previous thumbnail

    if (videoId == -1) {
        ui->currentVideoTitle->setText("No Video Selected");
        ui->currentVideoNumberInPlaylist->setText("");
        ui->currentVideoNote_textEdit->clear();
        return;
    }

    // Find the video in the current list
    Video currentVideo;
    bool found = false;
    int videoIndex = -1;
    for (int i = 0; i < currentVideoList.size(); ++i) {
        if (currentVideoList[i].videoID == videoId) {
            currentVideo = currentVideoList[i];
            videoIndex = i;
            found = true;
            break;
        }
    }

    if (found) {
        QFileInfo fileInfo(currentVideo.videoPath);
        ui->currentVideoTitle->setText(fileInfo.fileName());
        ui->currentVideoNumberInPlaylist->setText(QString("%1/%2")
                                                   .arg(videoIndex + 1)
                                                   .arg(currentVideoList.size()));
        // Generate a new thumbnail
        generateThumbnail(currentVideo.videoPath);
        loadCurrentVideoNote(videoId);
    } else {
        // If not in the current list, maybe it's just from initial load.
        // We can query the DB for the title.
        QString q = QString("SELECT videoPath FROM Video WHERE videoID = %1").arg(videoId);
        QSqlQuery query = dbInstance->execQuery(q);
        if (query.next()) {
            QString videoPath = query.value("videoPath").toString();
            QFileInfo fileInfo(videoPath);
            ui->currentVideoTitle->setText(fileInfo.fileName());
            ui->currentVideoNumberInPlaylist->setText(""); // Can't determine number without full list
            // Generate a new thumbnail
            generateThumbnail(videoPath);
            loadCurrentVideoNote(videoId);
        } else {
            ui->currentVideoTitle->setText("Video not found");
            ui->currentVideoNumberInPlaylist->setText("");
            ui->currentVideoNote_textEdit->clear();
        }
    }
}

void MainWindow::generateThumbnail(const QString &videoPath) {
    if (videoPath.isEmpty() || !QFile::exists(videoPath)) {
        ui->currentVideoThumbnail->setText("Video path invalid");
        return;
    }

    currentThumbnailPath = videoPath; // Set active path
    ui->currentVideoThumbnail->setText("Generating...");

    // 1. Stop and reset the player
    m_mediaPlayer->stop();
    m_mediaPlayer->setSource(QUrl()); // Clear current source
    
    // Disconnect any existing durationChanged connections
    QObject::disconnect(m_mediaPlayer, &QMediaPlayer::durationChanged, nullptr, nullptr);

    // 2. Setup the player
    m_mediaPlayer->setSource(QUrl::fromLocalFile(videoPath));

    // 3. Connect to durationChanged (Single Shot)
    QObject::connect(m_mediaPlayer, &QMediaPlayer::durationChanged, this, [this, videoPath](qint64 duration) {
        if (duration > 0 && currentThumbnailPath == videoPath) {
            // Calculate a random position (maybe not the very end)
            qint64 randomPosition = QRandomGenerator::global()->bounded(duration * 0.9);
            m_mediaPlayer->setPosition(randomPosition);
            m_mediaPlayer->play();
        }
    }, Qt::SingleShotConnection);

    // 4. Timeout to handle failures
    QTimer::singleShot(3000, this, [this, videoPath]() {
        if (currentThumbnailPath == videoPath && m_mediaPlayer->playbackState() == QMediaPlayer::StoppedState) {
            // Check if we still haven't gotten a frame
            ui->currentVideoThumbnail->setText("Thumbnail failed");
        }
    });
}

void MainWindow::onFrameChanged(const QVideoFrame &frame) {
    if (!frame.isValid()) {
        return;
    }

    // Capture the frame ONLY if it's for the currently active request
    // Stop the player immediately regardless
    m_mediaPlayer->stop();

    QImage image = frame.toImage();
    if (!image.isNull() && !currentThumbnailPath.isEmpty()) {
        ui->currentVideoThumbnail->setPixmap(QPixmap::fromImage(image).scaled(
            ui->currentVideoThumbnail->size(),
            Qt::KeepAspectRatio,
            Qt::SmoothTransformation
        ));
        currentThumbnailPath = ""; // Success! Clear the path.
    }
}

// --- New Slots for Button Clicks ---
void MainWindow::playThisVdo_clicked() {
  if (currentPlayingVideoId != -1) {
    playThisVdo(currentPlayingVideoId);
  } else {
    QMessageBox::information(this, "No Video", "No video selected to play.");
  }
}

void MainWindow::showNextVideo_clicked() { showNextVideo(); }

void MainWindow::showPrevVideo_clicked() { showPrevVideo(); }

void MainWindow::vdoNotWatched_clicked() {
  if (currentPlayingVideoId != -1) {
    vdoNotWatched(currentPlayingVideoId);
  } else {
    QMessageBox::information(this, "No Video", "No video selected.");
  }
}

void MainWindow::watchedThisVdo_clicked() {
  if (currentPlayingVideoId != -1) {
    watchedThisVdo(currentPlayingVideoId);
  } else {
    QMessageBox::information(this, "No Video", "No video selected.");
  }
}

void MainWindow::loadCurrentVideoNote(int videoId) {
    if (videoId <= 0) {
        ui->currentVideoNote_textEdit->clear();
        return;
    }

    QString queryStr = QString("SELECT noteText FROM Notes WHERE videoID = %1").arg(videoId);
    QSqlQuery query = dbInstance->execQuery(queryStr);

    if (query.next()) {
        ui->currentVideoNote_textEdit->setPlainText(query.value("noteText").toString());
    } else {
        ui->currentVideoNote_textEdit->clear();
    }
}

void MainWindow::saveCurrentVideoNote() {
    if (currentPlayingVideoId <= 0) return;

    QString noteText = ui->currentVideoNote_textEdit->toPlainText();
    int playlistId = ui->playlistList->currentData().toInt();

    if (playlistId <= 0) return;

    // Check if note already exists for this video
    QString checkQuery = QString("SELECT noteID FROM Notes WHERE videoID = %1").arg(currentPlayingVideoId);
    QSqlQuery query = dbInstance->execQuery(checkQuery);

    if (query.next()) {
        // Update
        QSqlQuery q(dbInstance->database());
        q.prepare("UPDATE Notes SET noteText = :noteText WHERE videoID = :videoId");
        q.bindValue(":noteText", noteText);
        q.bindValue(":videoId", currentPlayingVideoId);
        if (!q.exec()) {
            qCritical() << "Failed to update note:" << q.lastError().text();
        }
    } else {
        // Insert
        QSqlQuery q(dbInstance->database());
        q.prepare("INSERT INTO Notes (playlistId, videoID, noteText) VALUES (:playlistId, :videoId, :noteText)");
        q.bindValue(":playlistId", playlistId);
        q.bindValue(":videoId", currentPlayingVideoId);
        q.bindValue(":noteText", noteText);
        if (!q.exec()) {
            qCritical() << "Failed to insert note:" << q.lastError().text();
        }
    }
}

bool MainWindow::eventFilter(QObject *obj, QEvent *event) {
    if (obj == ui->currentVideoNote_textEdit) {
        if (event->type() == QEvent::FocusOut) {
            saveCurrentVideoNote();
        }
    }
    return QMainWindow::eventFilter(obj, event);
}

