#include "settings.h"
#include "ui_settings.h"

#include <QBrush> // REQUIRED for setting the background brush
#include <QColor> // REQUIRED for setting the background color
#include <QCoreApplication>
#include <QFile> // REQUIRED for checking file existence
#include <QFileDialog>
#include <QMessageBox>
#include <QString>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <utility>
#include <vector>

// --- Global Data ---

std::vector<std::pair<QString, QString>> mediaPlayerEntries = {
    // NOTE: Windows paths must use double backslashes (\\) or forward slashes
    // (/) for proper string escaping.

    {"VLC",
#ifdef __linux__
     // Common default path for VLC executable on many Linux distros
     "/usr/bin/vlc"
#elif _WIN32
// Default path for 64-bit VLC. Note the use of \\
        "C:\\Program Files\\VideoLAN\\VLC\\vlc.exe"
#endif
    },
    {"MPV",
#ifdef __linux__
     // Common default path for MPV executable
     "/usr/bin/mpv"
#elif _WIN32
        // General path, often within a subfolder or C:\Program Files\mpv\mpv.exe
        "C:\\Program Files\\mpv\\mpv.exe"
#endif
    },
    {"Windows Media Player (WMP)",
#ifdef __linux__
     // WMP is not available on Linux. Path is left empty.
     ""
#elif _WIN32
        // The executable is typically in the Windows system directory
        "C:\\Program Files\\Windows Media Player\\wmplayer.exe"
#endif
    },
    {"PotPlayer",
#ifdef __linux__
     ""
#elif _WIN32
        "C:\\Program Files\\DAUM\\PotPlayer\\PotPlayer.exe"
#endif
    },
    // The rest of the entries are left with placeholder paths for brevity,
    // but they require the same double-backslash fix for Windows.
    {"KMPlayer",
#ifdef __linux__
     ""
#elif _WIN32
        "C:\\Program Files\\KMPlayer\\"
#endif
    },
    {"MPlayer",
#ifdef __linux__
     "/usr/bin/mplayer"
#elif _WIN32
        "C:\\Program Files\\MPlayer\\"
#endif
    },
    {"SM Player",
#ifdef __linux__
     "/usr/bin/smplayer"
#elif _WIN32
        "C:\\Program Files\\SMPlayer\\"
#endif
    },
    {"Media Player Classic",
#ifdef __linux__
     ""
#elif _WIN32
        "C:\\Program Files\\MPC-HC\\"
#endif
    },
    {"GOM Player",
#ifdef __linux__
     ""
#elif _WIN32
        "C:\\Program Files\\GRETECH\\GOM Player\\"
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
  ui->listPlayersTableWidget->setRowCount(mediaPlayerEntries.size());
  ui->listPlayersTableWidget->setColumnCount(2);

  // Set headers (Added from previous fix, necessary for a good table)
  QStringList labels;
  labels << "Video Player Name" << "Default Path";
  ui->listPlayersTableWidget->setHorizontalHeaderLabels(labels);

  // clear all media player paths from DB to add new entries
  Settings::dbInstance->execQuery("DELETE FROM MediaPlayerPath");

  int row = 0;

  // 2. Populate the table
  for (const auto &entry : mediaPlayerEntries) {
    const QString &name = entry.first;
    const QString &path = entry.second;

    // Check if the file exists on the current system
    bool fileExists = QFile::exists(path);

    // --- Core Logic for Item Flags and Appearance ---

    // --- DB INSERTION LOGIC ---
    if (fileExists) {
      // Manually escape single quotes (' -> '') to prevent SQL injection/errors
      QString safeName = name;
      safeName.replace("'", "''");

      QString safePath = path;
      safePath.replace("'", "''");

      // Construct the raw query string
      // We use INSERT OR REPLACE to update the path if the player name already
      // exists
      QString q = QString("INSERT OR REPLACE INTO MediaPlayerPath "
                          "(mediaPlayerName, mediaPlayerPath) "
                          "VALUES ('%1', '%2')")
                      .arg(safeName, safePath);

      // Execute using the requested helper function from SQliteDB
      SQliteDB::instance()->execQuery(q);
    }

    // Create QTableWidgetItem for the name and path
    QTableWidgetItem *nameItem = new QTableWidgetItem(name);
    QTableWidgetItem *pathItem = new QTableWidgetItem(path);

    if (!fileExists) {
      // 1. Visually disable: Set text and background color to silver/gray
      // QBrush disabledBrush(Qt::red); // Use a light color for the background

      // Set the color for both cells
      // nameItem->setBackground(disabledBrush);
      // pathItem->setBackground(disabledBrush);

      // 2. Interactively disable: Remove the ItemIsEnabled flag
      Qt::ItemFlags flags = nameItem->flags();
      flags &= ~Qt::ItemIsEnabled;

      nameItem->setFlags(flags);
      pathItem->setFlags(flags);
    }

    // Add the items to the current row
    ui->listPlayersTableWidget->setItem(row, 0, nameItem);
    ui->listPlayersTableWidget->setItem(row, 1, pathItem);

    row++;
  }

  // Optional: Resize the columns to fit content
  ui->listPlayersTableWidget->resizeColumnsToContents();
}

void updateDfltCombo(Ui::Settings *ui) {
  QComboBox *comboBox = ui->dfltMediaPlayerComboBox;

  // 1. Clear any existing items in the combo box
  comboBox->clear();

  // 2. Populate the combo box with media player names
  for (const auto &entry : mediaPlayerEntries) {
    const QString &name = entry.first;
    const QString &path = entry.second;

    // Check if the file exists on the current system
    bool fileExists = QFile::exists(path);

    // Add the name to the combo box.
    // NOTE: We use the *name* as the display text, and optionally,
    // we could store the *path* (entry.second) as the user data.
    if (fileExists)
      comboBox->addItem(name);
  }

  // 3. Select the first item by default
  // Check if the vector was not empty before trying to select
  if (!mediaPlayerEntries.empty()) {
    comboBox->setCurrentIndex(0);
  }
}

// This function is automatically called when the user changes the selection
// private slots:
void Settings::on_dfltMediaPlayerComboBox_currentTextChanged(
    const QString &arg1) {
  // 1. Basic validation
  if (arg1.isEmpty()) {
    return;
  }

  // 2. Prepare the Name for the SELECT query
  // We must escape quotes HERE first to safely find the path
  QString safeLookupName = arg1;
  safeLookupName.replace("'", "''");

  // 3. Fetch the Path from DB
  // Notice the added single quotes around '%1'
  QString selectQ = QString("SELECT mediaPlayerPath FROM MediaPlayerPath WHERE "
                            "mediaPlayerName = '%1'")
                        .arg(safeLookupName);

  QSqlQuery temp = dbInstance->execQuery(selectQ);

  QString pathFound = "";

  // CRITICAL FIX: You must call next() to get the record
  if (temp.next()) {
    pathFound = temp.value("mediaPlayerPath").toString();
  } else {
    qWarning() << "[Settings] Could not find path for player:" << arg1;
    return; // Stop if no path found
  }

  // 4. Escape the Path for the UPDATE query
  // The path (e.g., C:\Program Files\...) might contain special chars
  QString safePath = pathFound;
  safePath.replace("'", "''");

  // 5. Update the General table
  QString updateQ =
      QString("UPDATE General SET defaultMediaPlayer = '%1' WHERE id = 1")
          .arg(safePath);

  dbInstance->execQuery(updateQ);

  qDebug() << "[Settings] Default media player set to path:" << pathFound;
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

Settings::Settings(QWidget *parent) : QWidget(parent), ui(new Ui::Settings) {
  ui->setupUi(this);
  dbInstance = SQliteDB::instance();
  ui->appPath->setText(QCoreApplication::applicationFilePath());
  ui->version->setText("0.1");

  // Call the updated function
  updatePlayerList(ui);
  updateDfltCombo(ui);
}

Settings::~Settings() { delete ui; }

void Settings::on_restoreBackup_clicked() {

  // get which file to restore
  QString filter = "SQLite (*.sqlite)";
  QString backupFileName = QFileDialog::getOpenFileName(
      this, "Select a SQLite file that stored previous backup",
      Settings::dbInstance->getDbDirPath(), filter);
  // check whether the file exists
  QFile instructionFile(backupFileName);
  if (!instructionFile.open(QFile::ReadOnly)) {
    QMessageBox::warning(this, "File failed to select !!!",
                         "File failed to select!");
  }
  // perform backup & replacement
  dbInstance->restoreDBfile(backupFileName);

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
    QMessageBox::information(this, "Success",
                             "Succcessfully backup created at location: \n\n" +
                                 newlyCreatedBackup);
  }
}
