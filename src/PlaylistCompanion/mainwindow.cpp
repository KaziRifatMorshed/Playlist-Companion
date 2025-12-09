#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QFileDialog>
#include <QMessageBox>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow) {
  ui->setupUi(this);
  MainWindow::dbInstance = SQliteDB::instance();
  initGeneralSettings();
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
  playlistWindow = new AddNewPlaylistWindow();
  playlistWindow->show();
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
    // playlistWindow->setAttribute(Qt::WA_DeleteOnClose);
    playlistWindow->show();
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
