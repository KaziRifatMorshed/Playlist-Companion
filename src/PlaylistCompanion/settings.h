#ifndef SETTINGS_H
#define SETTINGS_H

#include <QWidget>
#include <include/db_sqlite.h>

namespace Ui {
class Settings;
}

class Settings : public QWidget {
  Q_OBJECT

public:
  explicit Settings(QWidget *parent = nullptr);
  ~Settings();

private slots:
  void on_restoreBackup_clicked();

private:
  Ui::Settings *ui;
  SQliteDB *dbInstance;
};

#endif // SETTINGS_H
