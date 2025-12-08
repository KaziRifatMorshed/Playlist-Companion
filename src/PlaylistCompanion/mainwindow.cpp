#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QFileDialog>
#include <QMessageBox>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    dbInstance = SQliteDB::instance();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_pushButton_3_clicked()
{
    settingsWidgt = new Settings();
    settingsWidgt->show();
}


void MainWindow::on_editPlaylistButton_clicked()
{
    playlistWindow = new AddNewPlaylistWindow();
    playlistWindow->show();
}


void MainWindow::on_createNewPlaylist_clicked()
{
    // get which directory
    QString plpath = QFileDialog::getExistingDirectory(
        this, "Select a folder that contains your desired videos",
        QDir::homePath(), QFileDialog::ShowDirsOnly);
    // check whether the directory exists
    if (plpath.isEmpty()) {
        QMessageBox::warning(this, "Directory failed to select !!!",
                             "Directory failed to select!");
    } else {
        playlistWindow = new AddNewPlaylistWindow(nullptr, -1, plpath); // this does not open new window, rather overrides current window
        // playlistWindow->setAttribute(Qt::WA_DeleteOnClose);
        playlistWindow->show();
    }
}

