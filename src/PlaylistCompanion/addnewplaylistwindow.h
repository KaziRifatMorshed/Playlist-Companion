#ifndef ADDNEWPLAYLISTWINDOW_H
#define ADDNEWPLAYLISTWINDOW_H

#include <QWidget>
#include <include/db_sqlite.h>

namespace Ui {
class AddNewPlaylistWindow;
}

struct VideoCollection {
    QVector<QString> fileList; // Contains full absolute path + filename
    int count;
};


class AddNewPlaylistWindow : public QWidget
{
    Q_OBJECT

public:
    explicit AddNewPlaylistWindow(QWidget *parent = nullptr, int plListId = -1, QString plpath = "");
    ~AddNewPlaylistWindow();

private slots:
    void on_pushButton_2_clicked();

private:
    Ui::AddNewPlaylistWindow *ui;
    SQliteDB *dbInstance;
    VideoCollection vdos;
    int playlistID;

    VideoCollection getAllVideosFromDir(QString rootPath);
    VideoCollection getAllVideosFromDB();
};

#endif // ADDNEWPLAYLISTWINDOW_H
