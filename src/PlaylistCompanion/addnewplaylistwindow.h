#ifndef ADDNEWPLAYLISTWINDOW_H
#define ADDNEWPLAYLISTWINDOW_H

#include <QWidget>
#include <include/db_sqlite.h>

namespace Ui {
class AddNewPlaylistWindow;
}

class AddNewPlaylistWindow : public QWidget
{
    Q_OBJECT

public:
    explicit AddNewPlaylistWindow(QWidget *parent = nullptr, int playlistID = -1, QString plpath = "");
    ~AddNewPlaylistWindow();

private slots:
    void on_pushButton_2_clicked();

private:
    Ui::AddNewPlaylistWindow *ui;
    SQliteDB *dbInstance;
};

#endif // ADDNEWPLAYLISTWINDOW_H
