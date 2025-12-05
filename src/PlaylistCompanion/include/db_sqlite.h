#ifndef DB_SQLITE_H
#define DB_SQLITE_H

#include <QCoreApplication>
#include <QDateTime>
#include <QDebug>
#include <QFileInfo>
#include <QProcess>
#include <QString>
#include <QVariant>
#include <QtSql/QSqlDatabase>
#include <QtSql/QSqlError>
#include <QtSql/QSqlQuery>

class SQliteDB {

public:
  static SQliteDB *dbInstance;
  static QString dbPath;
  static QString dbDirPath;

  // Get the singleton instance
  static SQliteDB *instance() {
    if (!dbInstance) {
      qDebug() << "[sqLiteDB] db engine started";
      dbInstance = new SQliteDB();
      dbPath = QCoreApplication::applicationFilePath();
      dbDirPath = QCoreApplication::applicationDirPath();
    }
    return dbInstance;
  }

  // Delete copy and assignment
  SQliteDB(const SQliteDB &) = delete;
  SQliteDB &operator=(const SQliteDB &) = delete;

  // Open the database
  bool openDB(const QString &dbPath = "./db/db.sqlite") {
    if (db.isOpen())
      return true;

    qDebug() << "[sqLiteDB] opening db...";

    // QList listOfDrivers = QSqlDatabase::drivers();
    // qDebug() << listOfDrivers;

    const QString connectionName = "db_connection";
    if (QSqlDatabase::contains(connectionName)) {
      db = QSqlDatabase::database(connectionName);
    } else {
      db = QSqlDatabase::addDatabase("QSQLITE", connectionName);
      db.setDatabaseName(dbPath);
    }

    if (!db.open()) {
      qCritical() << "[sqLiteDB] Failed to open DB: " << db.lastError().text();
      return false;
    }

    return true;
  }

  // Execute a query and return QSqlQuery object
  QSqlQuery execQuery(const QString &queryStr) {
    QSqlQuery query(db);
    if (!query.exec(queryStr)) {
      qCritical() << "[sqLiteDB] Query failed:" << queryStr
                  << " ; Error:" << query.lastError().text();
    }
    return query;
  }

  // Check if DB is open
  bool isOpen() const { return db.isOpen(); }

  // Close the database connection
  void closeDB() {
    if (db.isOpen())
      db.close();
  }

  // Get raw QSqlDatabase for advanced operations
  QSqlDatabase &database() { return db; }

  void backupDBfile() {
#ifdef __linux__
    QProcess::execute("/usr/bin/cp",
                      {dbPath, dbDirPath + "backup_" +
                                   QDateTime::currentDateTime().toString(
                                       "yyyy-MM-dd_HH-mm-ss") +
                                   ".sqlite"});
#elif _WIN32

#endif
  } // not tested

private:
  SQliteDB() { openDB(); }
  ~SQliteDB() { closeDB(); }

  QSqlDatabase db;
};

#endif // DB_SQLITE_H
