#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QDebug>
#include <QFileDialog>
#include <QFileInfo>
#include <QMessageBox>
#include <QRandomGenerator>
#include <QTimer>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow) {
  ui->setupUi(this);
  MainWindow::dbInstance = SQliteDB::instance();
  initGeneralSettings();
  MainWindow::updatePlaylistListCombo();
  MainWindow::populateVideoTable(MainWindow::lastWatchedPlId);

  // Initialize currentPlayingVideoId from lastWatchedVdoId for the currently
  // loaded playlist
  currentPlayingVideoId = lastWatchedVdoId;

  // The on_allVideosTableWidget_cellDoubleClicked slot is auto-connected by the
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

void MainWindow::on_pushButton_3_clicked() {
  settingsWidgt = new Settings();
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
      // Delete videos associated with the playlist
      dbInstance->execQuery(
          QString("DELETE FROM Video WHERE playlistID = %1").arg(playlistId));
      // Delete the playlist itself
      dbInstance->execQuery(
          QString("DELETE FROM Playlist WHERE playlistId = %1")
              .arg(playlistId));
      updatePlaylistListCombo();
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

    qDebug() << "[MainWindow] Settings Loaded: " << defaultMediaPlayer
             << " | Last Playlist ID:" << lastWatchedPlId;

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

  // 2. Clear previous data to avoid duplicates
  listOfPlaylists.clear();
  combo->clear();

  // 3. Execute Query to fetch all playlists
  // We select all columns to populate the full struct
  QString q = "SELECT * FROM Playlist ORDER BY playlistId ASC";
  QSqlQuery query = dbInstance->execQuery(q);

  // 4. Iterate through results
  while (query.next()) {
    Playlist pl;

    // --- MAP DB COLUMNS TO STRUCT ---
    // (Ensure these variable names match your structures.h definition)
    pl.playlistId = query.value("playlistId").toInt();
    pl.playlistTitle = query.value("playlistTitle").toString();
    pl.playlistPath = query.value("playlistPath").toString();
    pl.status = query.value("status").toString();

    pl.totalVideoCount = query.value("totalVideoCount").toInt();
    pl.watchedCount = query.value("watchedCount").toInt();
    pl.totalTimeHour = query.value("totalTimeHour").toInt();

    // Retrieve Dates
    pl.creationDateTime = query.value("creationDateTime").toString();
    pl.lastWatchedDateTime = query.value("lastWatchedDateTime").toString();
    // -------------------------------

    // 5. Add to the member vector
    listOfPlaylists.append(pl);

    // 6. Add to the UI ComboBox
    // Argument 1: Text to display (Title)
    // Argument 2: UserData (The ID, hidden) - useful for retrieving the
    // specific playlist later
    combo->addItem(pl.playlistTitle, pl.playlistId);
  }

  // 7. (Optional) Auto-select the last watched playlist
  // 'lastWatchedPlId' was loaded in initGeneralSettings()
  if (lastWatchedPlId != -1) {
    int index = combo->findData(lastWatchedPlId);
    if (index != -1) {
      combo->setCurrentIndex(index);
    }
  }

  qDebug() << "[MainWindow] Playlist combo refreshed. Count:"
           << listOfPlaylists.size();
}

void MainWindow::populateVideoTable(int playlistId) {
  // 1. Clear existing data
  currentVideoList.clear();
  ui->allVideosTableWidget->setRowCount(0);

  // 2. Setup Table Headers (if not done in UI designer)
  // Column 0: Watched Status, Column 1: Video Name
  ui->allVideosTableWidget->setColumnCount(2);
  ui->allVideosTableWidget->setHorizontalHeaderLabels(
      QStringList() << "Watched" << "Video Name");

  // Adjust column widths (Status column small, Name column stretches)
  ui->allVideosTableWidget->setColumnWidth(0, 80);
  ui->allVideosTableWidget->horizontalHeader()->setSectionResizeMode(
      1, QHeaderView::Stretch);

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

    // Add to local memory vector
    currentVideoList.append(vdo);

    // --- UI POPULATION ---
    ui->allVideosTableWidget->insertRow(row);

    // Col 0: Watched Status (Checkbox)
    QTableWidgetItem *statusItem = new QTableWidgetItem();
    statusItem->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled |
                         Qt::ItemIsSelectable);
    // BUG: user checkbox e check kore data change kortese but database e save
    // hocche na
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

    // Highlight the current playing video
    if (vdo.videoID == currentPlayingVideoId) {
      for (int col = 0; col < ui->allVideosTableWidget->columnCount(); ++col) {
        QTableWidgetItem *itemToHighlight =
            ui->allVideosTableWidget->item(row, col);
        if (itemToHighlight) {
          itemToHighlight->setBackground(
              QColor(Qt::yellow).lighter(15)); // Light yellow
        }
      }
    }

    row++;
  }
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

    populateVideoTable(playlistId);
    lastWatchedPlId = playlistId; // Update the global tracker

    // Find the playlist in our list
    Playlist currentPlaylist;
    for (const auto &pl : listOfPlaylists) {
      if (pl.playlistId == playlistId) {
        currentPlaylist = pl;
        break;
      }
    }

    // Now update the UI elements
    ui->playlistCreationDate->setText(currentPlaylist.creationDateTime);
    ui->lastWatched->setText(currentPlaylist.lastWatchedDateTime);
    ui->totalTime->setText(QString::number(currentPlaylist.totalTimeHour) +
                           " hours");

    // Progress bar and count
    if (currentPlaylist.totalVideoCount > 0) {
      int progress = (currentPlaylist.watchedCount * 100) /
                     currentPlaylist.totalVideoCount;
      ui->progressBar->setValue(progress);
    } else {
      ui->progressBar->setValue(0);
    }
    ui->playlistProgressCount->setText(
        QString("%1/%2")
            .arg(currentPlaylist.watchedCount)
            .arg(currentPlaylist.totalVideoCount));

    // Update 'General' table in DB so app remembers this selection next time
    QString q = QString("UPDATE General SET lastWatchedPlId = %1 WHERE id = 1")
                    .arg(playlistId);
    dbInstance->execQuery(q);
  } else {
    // Clear the labels if no playlist is selected
    ui->playlistCreationDate->setText("");
    ui->lastWatched->setText("");
    ui->totalTime->setText("");
    ui->progressBar->setValue(0);
    ui->playlistProgressCount->setText("0/0");
  }
}

// --- Video Playback & Navigation ---
int MainWindow::currentVideoNumberInPlaylist() {
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
    showNextVideo();
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
    updatePlaylistListCombo(); // Refresh playlist combo to update counts
  }

  // --- Advance to Next Video ---
  showNextVideo();
}

void MainWindow::vdoNotWatched(int videoId) {
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
    updatePlaylistListCombo(); // Refresh playlist combo to update counts
    populateVideoTable(currentPlaylistId); // Refresh video table
  }
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
  // Refresh table to update highlight (handled by populateVideoTable)
  populateVideoTable(targetVideo.playlistID);

  // 4. Update lastWatchedVdoId in General settings
  QString updateGeneralQuery =
      QString("UPDATE General SET lastWatchedVdoId = %1 WHERE id = 1")
          .arg(videoId);
  dbInstance->execQuery(updateGeneralQuery);

  // 5. Launch the video using QProcess
  QString program = defaultMediaPlayer;
  QStringList arguments;
  arguments << targetVideo.videoPath;

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

void MainWindow::showNextVideo() {
  int currentIdx = currentVideoNumberInPlaylist();
  if (currentIdx != -1 && currentIdx < currentVideoList.size() - 1) {
    currentPlayingVideoId = currentVideoList[currentIdx + 1].videoID;
    updateVideoGroupBox(currentPlayingVideoId);
    populateVideoTable(lastWatchedPlId); // Repopulate to update highlight
  } else if (currentIdx == currentVideoList.size() - 1) {
    QMessageBox::information(this, "End of Playlist",
                             "This is the last video in the playlist.");
  } else {
    // If no video is playing, select the first one
    if (!currentVideoList.isEmpty()) {
      currentPlayingVideoId = currentVideoList[0].videoID;
      updateVideoGroupBox(currentPlayingVideoId);
      populateVideoTable(lastWatchedPlId);
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
    populateVideoTable(lastWatchedPlId); // Repopulate to update highlight
  } else if (currentIdx == 0) {
    QMessageBox::information(this, "Beginning of Playlist",
                             "This is the first video in the playlist.");
  } else {
    // If no video is playing, select the last one
    if (!currentVideoList.isEmpty()) {
      currentPlayingVideoId = currentVideoList.last().videoID;
      updateVideoGroupBox(currentPlayingVideoId);
      populateVideoTable(lastWatchedPlId);
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

void MainWindow::on_allVideosTableWidget_cellDoubleClicked(int row,
                                                           int column) {
  // Get the video ID from the clicked row (stored in the first column's item
  // data)
  QTableWidgetItem *item = ui->allVideosTableWidget->item(row, 0);
  if (item) {
    int videoId = item->data(Qt::UserRole).toInt();
    if (videoId > 0) {
      // Do NOT play the video, just update the current selection and UI
      currentPlayingVideoId = videoId;
      populateVideoTable(lastWatchedPlId); // Re-populate to update highlight
      updateVideoGroupBox(currentPlayingVideoId); // Update the info box
    }
  }
}

void MainWindow::updateVideoGroupBox(int videoId) {
    ui->currentVideoThumbnail->clear(); // Clear previous thumbnail

    if (videoId == -1) {
        ui->currentVideoTitle->setText("No Video Selected");
        ui->currentVideoNumberInPlaylist->setText("");
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
        } else {
            ui->currentVideoTitle->setText("Video not found");
            ui->currentVideoNumberInPlaylist->setText("");
        }
    }
}

void MainWindow::generateThumbnail(const QString &videoPath) {
    if (videoPath.isEmpty() || !QFile::exists(videoPath)) {
        ui->currentVideoThumbnail->setText("Video path invalid");
        return;
    }

    m_mediaPlayer->setSource(QUrl::fromLocalFile(videoPath));

    // We need to wait for the media to be loaded to get its duration.
    // We connect to the durationChanged signal once.
    QObject::connect(m_mediaPlayer, &QMediaPlayer::durationChanged, m_mediaPlayer, [=](qint64 duration) {
        if (duration > 0) {
            // Disconnect this connection so it doesn't fire again for this player
            QObject::disconnect(m_mediaPlayer, &QMediaPlayer::durationChanged, nullptr, nullptr);

            // Calculate a random position
            qint64 randomPosition = QRandomGenerator::global()->bounded(duration);
            m_mediaPlayer->setPosition(randomPosition);

            // Play briefly to ensure a frame is rendered
            m_mediaPlayer->play();
        }
    }, Qt::SingleShotConnection);

    // If durationChanged doesn't fire (e.g., for an invalid video file),
    // we should have a timeout.
    QTimer::singleShot(3000, this, [=]() {
        if (m_mediaPlayer->duration() == 0) { // If it still hasn't loaded
            QObject::disconnect(m_mediaPlayer, &QMediaPlayer::durationChanged, nullptr, nullptr);
            ui->currentVideoThumbnail->setText("Thumbnail failed");
            m_mediaPlayer->stop();
        }
    });
}

void MainWindow::onFrameChanged(const QVideoFrame &frame) {
    if (!frame.isValid()) {
        return;
    }
    // Stop the player as soon as we have one frame.
    m_mediaPlayer->stop();

    QImage image = frame.toImage();
    if (!image.isNull()) {
        ui->currentVideoThumbnail->setPixmap(QPixmap::fromImage(image).scaled(
            ui->currentVideoThumbnail->size(),
            Qt::KeepAspectRatio,
            Qt::SmoothTransformation
        ));
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
