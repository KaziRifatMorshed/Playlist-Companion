#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QFileDialog>
#include <QMessageBox>
#include <QFileInfo>
#include <QDebug>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow) {
  ui->setupUi(this);
  MainWindow::dbInstance = SQliteDB::instance();
  initGeneralSettings();
  MainWindow::updatePlaylistListCombo();
  MainWindow::populateVideoTable(MainWindow::lastWatchedPlId);
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
          QString("DELETE FROM Playlist WHERE playlistId = %1").arg(playlistId));
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
    QComboBox* combo = ui->playlistList;

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
        // Argument 2: UserData (The ID, hidden) - useful for retrieving the specific playlist later
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

    qDebug() << "[MainWindow] Playlist combo refreshed. Count:" << listOfPlaylists.size();
}

void MainWindow::populateVideoTable(int playlistId) {
    // 1. Clear existing data
    currentVideoList.clear();
    ui->allVideosTableWidget->setRowCount(0);

    // 2. Setup Table Headers (if not done in UI designer)
    // Column 0: Watched Status, Column 1: Video Name
    ui->allVideosTableWidget->setColumnCount(2);
    ui->allVideosTableWidget->setHorizontalHeaderLabels(QStringList() << "Watched" << "Video Name");

    // Adjust column widths (Status column small, Name column stretches)
    ui->allVideosTableWidget->setColumnWidth(0, 80);
    ui->allVideosTableWidget->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);

    // 3. Prepare Query
    // We fetch videos only for the selected playlist
    QString q = QString("SELECT * FROM Video WHERE playlistID = %1 ORDER BY videoID ASC").arg(playlistId);
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
        statusItem->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled | Qt::ItemIsSelectable);
        // BUG: user checkbox e check kore data change kortese but database e save hocche na
        statusItem->setCheckState(vdo.isWatched ? Qt::Checked : Qt::Unchecked); // Set Checkbox state based on DB value
        statusItem->setData(Qt::UserRole, vdo.videoID); // Store the videoID in the item so we can identify it later if clicked

        ui->allVideosTableWidget->setItem(row, 0, statusItem);

        // Col 1: Video Name (Clean display)
        QFileInfo fileInfo(vdo.videoPath);
        QTableWidgetItem *nameItem = new QTableWidgetItem(fileInfo.fileName());
        // Make it read-only (user can't rename file here)
        nameItem->setFlags(nameItem->flags() ^ Qt::ItemIsEditable);

        ui->allVideosTableWidget->setItem(row, 1, nameItem);

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
      int progress =
          (currentPlaylist.watchedCount * 100) / currentPlaylist.totalVideoCount;
      ui->progressBar->setValue(progress);
    } else {
      ui->progressBar->setValue(0);
    }
    ui->playlistProgressCount->setText(QString("%1/%2")
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
  // MainWindow::updatePlaylistListCombo(); // BUG : main window dows not launch
}
