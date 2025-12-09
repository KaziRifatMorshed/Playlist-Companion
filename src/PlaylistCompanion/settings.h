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
  void on_createBackup_clicked();
  void on_dfltMediaPlayerComboBox_currentTextChanged(const QString &arg1);

private:
  Ui::Settings *ui;
  SQliteDB *dbInstance;
  void updatePlayerList(Ui::Settings *ui);
};

#endif // SETTINGS_H
