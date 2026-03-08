#ifndef SETTINGS_H
#define SETTINGS_H

#include <QWidget>
#include <include/db_sqlite.h>
#include <QVector>

namespace Ui {
class Settings;
}

class Settings : public QWidget {
  Q_OBJECT

public:
  explicit Settings(QWidget *parent = nullptr);
  ~Settings();

signals:
  void settingsChanged();

private slots:
  void on_restoreBackup_clicked();
  void on_createBackup_clicked();
  void on_dfltMediaPlayerComboBox_currentTextChanged(const QString &arg1);
  void on_ExitPushButton_clicked();

private:
  Ui::Settings *ui;
  SQliteDB *dbInstance;

  void updatePlayerList(Ui::Settings *ui);
  void updateDfltCombo(Ui::Settings *ui);
  void updateBackupLabels();
};

#endif // SETTINGS_H
