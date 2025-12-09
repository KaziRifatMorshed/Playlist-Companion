#ifndef DB_SQLITE_H
#define DB_SQLITE_H

#include <QCoreApplication>
#include <QDateTime>
#include <QDebug>
#include <QFile>
#include <QFileInfo>
#include <QMutex>
#include <QMutexLocker>
#include <QProcess>
#include <QString>
#include <QVariant>
#include <QtSql/QSqlDatabase>
#include <QtSql/QSqlError>
#include <QtSql/QSqlQuery>

#define dbdebug qDebug() << "[sqLiteDB] "

class SQliteDB {

public:
  static SQliteDB *dbInstance;
  static QString getAppPath();
  static QString getAppDirPath();
  static QString getDbPath();
  static QString getDbDirPath();

  // Get the singleton instance
  static SQliteDB *instance();

  // Delete copy and assignment
  SQliteDB(const SQliteDB &) = delete;
  SQliteDB &operator=(const SQliteDB &) = delete;

  // Open the database
  bool openDB(const QString &dbPath);

  // Execute a query and return QSqlQuery object
  QSqlQuery execQuery(const QString &queryStr);

  // Check if DB is open
  bool isOpen() const;

  // Close the database connection
  void closeDB();

  // Get raw QSqlDatabase for advanced operations
  QSqlDatabase &database();

  QString backupDBfile();

  void restoreDBfile(QString targetFilePath);

private:
  SQliteDB();
  ~SQliteDB();

  QSqlDatabase db;
  static QString appPath;
  static QString appDirPath;
  static QString dbPath;
  static QString dbDirPath;

  bool copyFile(QString src, QString dest);
  QMutex queryMutex;
};

#endif // DB_SQLITE_H