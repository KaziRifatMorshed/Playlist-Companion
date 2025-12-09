#include "include/db_sqlite.h"

SQliteDB *SQliteDB::dbInstance = nullptr;
QString SQliteDB::appPath = "";
QString SQliteDB::appDirPath = "";
QString SQliteDB::dbPath = "";
QString SQliteDB::dbDirPath = "";

QString SQliteDB::getAppPath() { return appPath; }
QString SQliteDB::getAppDirPath() { return appDirPath; }
QString SQliteDB::getDbPath() { return dbPath; }
QString SQliteDB::getDbDirPath() { return dbDirPath; }

// Get the singleton instance
SQliteDB *SQliteDB::instance() {
    if (!dbInstance) {
        dbdebug << "db engine started";
        SQliteDB::dbInstance = new SQliteDB();

        // generate paths
        SQliteDB::appPath = QCoreApplication::applicationFilePath();
        SQliteDB::appDirPath = QCoreApplication::applicationDirPath();
        SQliteDB::dbDirPath = SQliteDB::appDirPath +
#ifdef __linux__
            "/dbPlaylistCompanion/";
#elif _WIN32
            "\\dbPlaylistCompanion\\";
#endif
        SQliteDB::dbPath = SQliteDB::dbDirPath + "db_PL.sqlite";

        // open db
        dbInstance->openDB(dbInstance->dbPath);
    }
    return SQliteDB::dbInstance;
}

// Open the database
bool SQliteDB::openDB(const QString &dbPath) {
    if (db.isOpen())
        return true;

    dbdebug << "opening db... (" << dbPath << ")";

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
QSqlQuery SQliteDB::execQuery(const QString &queryStr) {
    QMutexLocker locker(&queryMutex);
    QSqlQuery query(db);
    if (!query.exec(queryStr)) {
        qCritical() << "[sqLiteDB] Query failed:" << queryStr
            << "; Error:" << query.lastError().text();
    }
    return query;
}

// Check if DB is open
bool SQliteDB::isOpen() const { return db.isOpen(); }

// Close the database connection
void SQliteDB::closeDB() {
    if (db.isOpen())
        db.close();
}

// Get raw QSqlDatabase for advanced operations
QSqlDatabase &SQliteDB::database() { return db; }

QString SQliteDB::backupDBfile() {
    QString newlyCreatedBackup =
        dbDirPath + "backup_" +
        QDateTime::currentDateTime().toString("yyyy-MM-dd_HH-mm-ss") +
        ".sqlite";
    copyFile(dbPath, newlyCreatedBackup);
    dbdebug << "db backup created at :" << newlyCreatedBackup;
    return newlyCreatedBackup;
}

void SQliteDB::restoreDBfile(QString targetFilePath) {
    backupDBfile();
    copyFile(targetFilePath, dbInstance->dbPath);
}

SQliteDB::SQliteDB() {}
SQliteDB::~SQliteDB() { closeDB(); }


bool SQliteDB::copyFile(QString src, QString dest) {
    // 1. Check if source exists
    if (!QFile::exists(src)) {
        dbdebug << "Error: Source file does not exist.";
        return false;
    }

    // 2. Handle Overwrite: Remove destination if it exists
    if (QFile::exists(dest)) {
        if (!QFile::remove(dest)) {
            dbdebug << "Error: Could not remove existing destination file.";
            return false;
        }
    }

    // 3. Perform the copy
    bool success = QFile::copy(src, dest);

    if (!success) {
        dbdebug << "Error: Copy failed.";
    }
    return success;
}

/*
void execCP(QString src, QString dest) {
// 1. Declare a static mutex.
// 'static' ensures this single mutex instance is shared by ALL threads.
// If it weren't static, every thread would create its own mutex, rendering
// it useless.
static QMutex mutex;

// 2. Use QMutexLocker for RAII-style locking.
// The mutex is locked when 'locker' is created.
// The mutex is automatically unlocked when 'locker' goes out of scope (end
// of function).
QMutexLocker locker(&mutex);

QString cpPath;
QStringList arguments;

#ifdef __linux__
cpPath = "/usr/bin/cp";
arguments = {src, dest};
#elif _WIN32
// Windows does not have a standalone 'cp' executable.
// We must invoke 'cmd.exe' and tell it to run the 'copy' command.
cpPath = "cmd.exe";
// /c tells cmd to run the command and terminate.
// /y tells copy to suppress confirmation prompts (overwrite without
// asking).
arguments = {"/c", "copy", "/y", src, dest};
#endif

// This thread now has exclusive access. Other threads hitting this function
// will wait here until the copy finishes.
QProcess::execute(cpPath, arguments);
}
*/