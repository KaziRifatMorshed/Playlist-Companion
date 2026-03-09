#include "settings.h"
#include "ui_settings.h"

#include <QBrush> // REQUIRED for setting the background brush
#include <QColor> // REQUIRED for setting the background color
#include <QCoreApplication>
#include <QDir>
#include <QFile> // REQUIRED for checking file existence
#include <QFileDialog>
#include <QFileInfoList>
#include <QMessageBox>
#include <QString>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <utility>
#include <vector>

// --- Global Data ---

std::vector<std::pair<QString, QString>> mediaPlayerEntries = {
    // NOTE: Windows paths must use double backslashes (/) or forward slashes
    // (/) for proper string escaping.

    {"VLC",
#ifdef __linux__
     // Common default path for VLC executable on many Linux distros
     "/usr/bin/vlc"
#elif _WIN32
// Default path for 64-bit VLC. Note the use of /
        "C:/Program Files/VideoLAN/VLC/vlc.exe"
#endif
    },
    {"MPV",
#ifdef __linux__
     // Common default path for MPV executable
     "/usr/bin/mpv"
#elif _WIN32
        // General path, often within a subfolder or C:\Program Files\mpv\mpv.exe
        "C:/Program Files/mpv/mpv.exe"
#endif
    },
    {"Windows Media Player (WMP)",
#ifdef __linux__
     // WMP is not available on Linux. Path is left empty.
     ""
#elif _WIN32
        // The executable is typically in the Windows system directory
        "C:/Program Files/Windows Media Player/wmplayer.exe"
#endif
    },
    {"PotPlayer",
#ifdef __linux__
     ""
#elif _WIN32
        "C:/Program Files/DAUM/PotPlayer/PotPlayer.exe"
#endif
    },
    // The rest of the entries are left with placeholder paths for brevity,
    // but they require the same double-backslash fix for Windows.
    {"KMPlayer",
#ifdef __linux__
     ""
#elif _WIN32
        "C:/Program Files/KMPlayer/"
#endif
    },
    {"MPlayer",
#ifdef __linux__
     "/usr/bin/mplayer"
#elif _WIN32
        "C:/Program Files/MPlayer/"
#endif
    },
    {"SM Player",
#ifdef __linux__
     "/usr/bin/smplayer"
#elif _WIN32
        "C:/Program Files/SMPlayer/"
#endif
    },
    {"Media Player Classic",
#ifdef __linux__
     ""
#elif _WIN32
        "C:/Program Files/MPC-HC/"
#endif
    },
    {"GOM Player",
#ifdef __linux__
     ""
#elif _WIN32
        "C:/Program Files/GRETECH/GOM Player/"
#endif
    },
    {"GNOME Videos",
#ifdef __linux__
     // Common executable name for GNOME Videos (Totem)
     "/usr/bin/totem"
#elif _WIN32
        "" // Not applicable on Windows
#endif
    }
    //
};

void Settings::updatePlayerList(Ui::Settings *ui) {
  // 1. Setup the table
  ui->listPlayersTableWidget->setColumnCount(2);

  // Set headers
  QStringList labels;
  labels << "Video Player Name" << "Default Path";
  ui->listPlayersTableWidget->setHorizontalHeaderLabels(labels);

  // 2. Try to load from DB first
  QSqlQuery query =
      dbInstance->execQuery("SELECT mediaPlayerName, mediaPlayerPath FROM "
                            "MediaPlayerPath ORDER BY mediaPlayerName ASC");

  std::vector<std::pair<QString, QString>> currentEntries;
  while (query.next()) {
    currentEntries.push_back({query.value(0).toString(), query.value(1).toString()});
  }

  // If DB is empty, use defaults and save them
  if (currentEntries.empty()) {
    for (const auto &entry : mediaPlayerEntries) {
      if (QFile::exists(entry.second)) {
        QString safeName = entry.first;
        safeName.replace("'", "''");
        QString safePath = entry.second;
        safePath.replace("'", "''");

        QString q = QString("INSERT INTO MediaPlayerPath "
                            "(mediaPlayerName, mediaPlayerPath) "
                            "VALUES ('%1', '%2')")
                        .arg(safeName, safePath);
        dbInstance->execQuery(q);
        currentEntries.push_back(entry);
      }
    }
  }

  ui->listPlayersTableWidget->setRowCount(currentEntries.size());

  int row = 0;
  for (const auto &entry : currentEntries) {
    const QString &name = entry.first;
    const QString &path = entry.second;

    bool fileExists = QFile::exists(path);

    QTableWidgetItem *nameItem = new QTableWidgetItem(name);
    QTableWidgetItem *pathItem = new QTableWidgetItem(path);

    if (!fileExists) {
      Qt::ItemFlags flags = nameItem->flags();
      flags &= ~Qt::ItemIsEnabled;
      nameItem->setFlags(flags);
      pathItem->setFlags(flags);
    }

    ui->listPlayersTableWidget->setItem(row, 0, nameItem);
    ui->listPlayersTableWidget->setItem(row, 1, pathItem);
    row++;
  }

  ui->listPlayersTableWidget->resizeColumnsToContents();
}

void Settings::updateDfltCombo(Ui::Settings *ui) {
  QComboBox *comboBox = ui->dfltMediaPlayerComboBox;
  comboBox->clear();

  // 1. Fetch current default path from General table
  QString currentDefaultPath = "";
  QSqlQuery genQuery = dbInstance->execQuery("SELECT defaultMediaPlayer FROM General WHERE id = 1");
  if (genQuery.next()) {
      currentDefaultPath = genQuery.value(0).toString();
  }

  // 2. Fetch all players from MediaPlayerPath table
  QSqlQuery pathQuery = dbInstance->execQuery("SELECT mediaPlayerName, mediaPlayerPath FROM MediaPlayerPath ORDER BY mediaPlayerName ASC");
  
  int defaultIndex = -1;
  int currentIndex = 0;
  while (pathQuery.next()) {
      QString name = pathQuery.value(0).toString();
      QString path = pathQuery.value(1).toString();
      
      // Only add to combo if file exists
      if (QFile::exists(path)) {
          comboBox->addItem(name, path); // Store path as userData
          if (!currentDefaultPath.isEmpty() && path == currentDefaultPath) {
              defaultIndex = currentIndex;
          }
          currentIndex++;
      }
  }

  // 3. Set current selection
  if (defaultIndex != -1) {
      comboBox->setCurrentIndex(defaultIndex);
  } else if (comboBox->count() > 0) {
      comboBox->setCurrentIndex(0);
  }
}

// This function is automatically called when the user changes the selection
// private slots:
void Settings::on_dfltMediaPlayerComboBox_currentTextChanged(
    const QString &arg1) {
  if (arg1.isEmpty()) return;

  QString pathFound = ui->dfltMediaPlayerComboBox->currentData().toString();
  if (pathFound.isEmpty()) return;

  QString safePath = pathFound;
  safePath.replace("'", "''");

  QString updateQ =
      QString("UPDATE General SET defaultMediaPlayer = '%1' WHERE id = 1")
          .arg(safePath);

  dbInstance->execQuery(updateQ);
}

/*
void setDefaultPlayer(Ui::Settings *ui) {
  QSqlQuery dflt =
      dbInstance->execQuery("FROM General SELECT defaultMediaPlayer");
  QString defaultMediaPlayer;
  while (dflt.next()) {
    defaultMediaPlayer = dflt.value("defaultMediaPlayer").toString();
  }

  if (defaultMediaPlayer.length() == 0) { // no media player set default

      // not implemented, need to also check whether all players exist or not
      // and go through the pre defined list basis
  } else if (defaultMediaPlayer.length() > 0) {

  }
}
*/

// --- Settings Constructor and Destructor ---

void Settings::on_ExitPushButton_clicked() {
  // 1. Save all paths from listPlayersTableWidget to MediaPlayerPath table
  for (int i = 0; i < ui->listPlayersTableWidget->rowCount(); ++i) {
    QTableWidgetItem *nameItem = ui->listPlayersTableWidget->item(i, 0);
    QTableWidgetItem *pathItem = ui->listPlayersTableWidget->item(i, 1);
    if (nameItem && pathItem) {
      QString name = nameItem->text();
      QString path = pathItem->text();

      QString safeName = name;
      safeName.replace("'", "''");
      QString safePath = path;
      safePath.replace("'", "''");

      QString q = QString("INSERT OR REPLACE INTO MediaPlayerPath "
                          "(mediaPlayerName, mediaPlayerPath) "
                          "VALUES ('%1', '%2')")
                      .arg(safeName, safePath);
      dbInstance->execQuery(q);
    }
  }

  // 2. Save the selected default media player's PATH to General table
  QString selectedPlayerName = ui->dfltMediaPlayerComboBox->currentText();
  if (!selectedPlayerName.isEmpty()) {
    // Find the path for this player in the table (it's fresher than DB)
    QString selectedPath = "";
    for (int i = 0; i < ui->listPlayersTableWidget->rowCount(); ++i) {
      QTableWidgetItem *nameItem = ui->listPlayersTableWidget->item(i, 0);
      if (nameItem && nameItem->text() == selectedPlayerName) {
        QTableWidgetItem *pathItem = ui->listPlayersTableWidget->item(i, 1);
        if (pathItem) {
          selectedPath = pathItem->text();
        }
        break;
      }
    }

    if (!selectedPath.isEmpty()) {
      QString safePath = selectedPath;
      safePath.replace("'", "''");
      QString updateQ =
          QString("UPDATE General SET defaultMediaPlayer = '%1' WHERE id = 1")
              .arg(safePath);
      dbInstance->execQuery(updateQ);
    }
  }

  emit settingsChanged();
  this->close();
}

Settings::Settings(QWidget *parent) : QWidget(parent), ui(new Ui::Settings) {
  ui->setupUi(this);
  dbInstance = SQliteDB::instance();
  ui->appPath->setText(QCoreApplication::applicationFilePath());
  ui->version->setText(APP_VERSION_STR);

  // Call the updated function
  updatePlayerList(ui);
  updateDfltCombo(ui);
  updateBackupLabels();
}

Settings::~Settings() { delete ui; }

void Settings::updateBackupLabels() {
  QString backupDir = SQliteDB::getDbDirPath();
  ui->backupLocation_qLabel->setText(backupDir);

  QDir dir(backupDir);
  QStringList filters;
  filters << "backup_*.sqlite";
  dir.setNameFilters(filters);
  dir.setSorting(QDir::Time); // Newest first

  QFileInfoList list = dir.entryInfoList();
  if (!list.isEmpty()) {
    QFileInfo latest = list.first();
    ui->lastBackup_qLabel->setText(latest.fileName());
  } else {
    ui->lastBackup_qLabel->setText("No Backup Found");
  }
}

void Settings::on_restoreBackup_clicked() {

  // get which file to restore
  QString filter = "SQLite (*.sqlite)";
  QString backupFileName = QFileDialog::getOpenFileName(
      this, "Select a SQLite file that stored previous backup",
      SQliteDB::getDbDirPath(), filter);
  // check whether the file exists
  QFile instructionFile(backupFileName);
  if (!instructionFile.open(QFile::ReadOnly)) {
    QMessageBox::warning(this, "File failed to select !!!",
                         "File failed to select!");
  }
  // perform backup & replacement
  dbInstance->restoreDBfile(backupFileName);

  updateBackupLabels();

  // NOTE: upadate UI with new data ; it can be a better approach to close the
  // app and reopen it again

  QMessageBox::information(
      this, "Backup Restoration",
      "For safety measurements, we have made a backup of the current database. "
      "Now, the data will be replaced with the data from the backup/sqlite "
      "file you have just selected.\n\nIf you want to get back your data, you "
      "can restore it again. SQLite backup filename contains timestamp "
      "reffering when backup was performed.");
}

void Settings::on_createBackup_clicked() {
  QString newlyCreatedBackup = dbInstance->backupDBfile();
  QFile newlyCreatedBackupFile(newlyCreatedBackup);
  if (!newlyCreatedBackupFile.open(QFile::ReadOnly)) {
    QMessageBox::warning(this, "Failed !!!",
                         "Failed to create backup! Please make sure .... ");
  } else {
    updateBackupLabels();
    QMessageBox::information(this, "Success",
                             "Succcessfully backup created at location: \n\n" +
                                 newlyCreatedBackup);
  }
}
