#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <settings.h>
#include <addnewplaylistwindow.h>
#include <include/db_sqlite.h>
#include <QVector>

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void on_pushButton_3_clicked();

    void on_editPlaylistButton_clicked();

    void on_createNewPlaylist_clicked();

private:
    Ui::MainWindow *ui;
    SQliteDB *dbInstance;
    Settings *settingsWidgt;
    AddNewPlaylistWindow *playlistWindow;
    QVector<QString> listOfPlaylists;
};
#endif // MAINWINDOW_H
