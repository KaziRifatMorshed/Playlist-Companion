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

        // Use a writable location for the database (e.g., AppData on Windows)
        QString baseDataPath = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
        if (baseDataPath.isEmpty()) {
            baseDataPath = SQliteDB::appDirPath;
        }

        QDir baseDir(baseDataPath);
        if (!baseDir.exists()) {
            baseDir.mkpath(".");
        }

        // Add the subdirectory
        SQliteDB::dbDirPath = baseDir.absoluteFilePath("dbPlaylistCompanion");

#ifdef _WIN32
        SQliteDB::dbDirPath += "\\";
#else
        SQliteDB::dbDirPath += "/";
#endif

        SQliteDB::dbPath = SQliteDB::dbDirPath + "db_PL.sqlite";

        // Create dbDirPath if it doesn't exist
        QDir dbDir(SQliteDB::dbDirPath);
        if (!dbDir.exists()) {
            if (!dbDir.mkpath(".")) {
                dbdebug << "Error: Could not create dbDirPath:" << SQliteDB::dbDirPath;
            } else {
                dbdebug << "dbDirPath created successfully:" << SQliteDB::dbDirPath;
            }
        }

        // open db
        dbInstance->openDB(dbInstance->dbPath);
    }
    return SQliteDB::dbInstance;
}

// Open the database
bool SQliteDB::openDB(const QString &dbPath) {
    if (db.isOpen())
        return true;

    // Check if file exists just for logging/debugging
    if (!QFile::exists(dbPath)) {
        dbdebug << "Database file not found. A new one will be created at:" << dbPath;
    }

    dbdebug << "opening db... (" << dbPath << ")";

    const QString connectionName = "db_connection";
    if (QSqlDatabase::contains(connectionName)) {
        db = QSqlDatabase::database(connectionName);
        if (db.databaseName() != dbPath) {
            db.close();
            db.setDatabaseName(dbPath);
        }
    } else {
        db = QSqlDatabase::addDatabase("QSQLITE", connectionName);
        db.setDatabaseName(dbPath);
    }

    if (!db.open()) {
        dbcritical << "Failed to open DB: " << db.lastError().text();
        return false;
    }

    // ALWAYS enable foreign keys when opening a connection in SQLite
    {
        QSqlQuery pragma(db);
        if (!pragma.exec("PRAGMA foreign_keys = ON;")) {
            dbdebug << "Failed to enable foreign keys:" << pragma.lastError().text();
        }
    }

    // Check if the database is newly created (no tables)
    // Qt's tables() returns an empty list if the database has no user tables.
    if (db.tables().isEmpty()) {
        dbdebug << "Database is empty, initializing schema from db_schema.h...";
        QStringList schemaStatements = getInitialSchemaStatements();
        for (const QString &statement : schemaStatements) {
            // Skip the PRAGMA if it's already been run, though running it again is harmless
            if (statement.trimmed().startsWith("PRAGMA", Qt::CaseInsensitive)) continue;

            QSqlQuery query(db);
            if (!query.exec(statement)) {
                dbcritical << "Schema initialization failed for statement:" << statement
                           << "; Error:" << query.lastError().text();
            }
        }
        dbdebug << "Schema initialized successfully.";
    }

    return true;
}

// Execute a query and return QSqlQuery object
QSqlQuery SQliteDB::execQuery(const QString &queryStr) {
    QMutexLocker locker(&queryMutex);
    QSqlQuery query(db);
    if (!query.exec(queryStr)) {
        dbcritical << "Query failed:" << queryStr << "; Error:" << query.lastError().text();
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
    QString newlyCreatedBackup = dbDirPath + "backup_"
                                 + QDateTime::currentDateTime().toString("yyyy-MM-dd_HH-mm-ss")
                                 + ".sqlite";
    if (copyFile(dbPath, newlyCreatedBackup)) {
        dbdebug << "db backup created at :" << newlyCreatedBackup;
        return newlyCreatedBackup;
    }
    return "";
}

void SQliteDB::restoreDBfile(QString targetFilePath) {
    backupDBfile();
    
    // Close and clear the database handle to release file locks on Windows
    QString connectionName = db.connectionName();
    db.close();
    db = QSqlDatabase(); 
    QSqlDatabase::removeDatabase(connectionName);
    
    if (copyFile(targetFilePath, dbPath)) {
        dbdebug << "Restored successfully:" << dbPath;
    }
    
    // Re-open the database
    openDB(dbPath);
}

SQliteDB::SQliteDB() {}
SQliteDB::~SQliteDB() { closeDB(); }


bool SQliteDB::copyFile(QString src, QString dest) {
    // 1. Check if source exists
    if (!QFile::exists(src)) {
        dbdebug << "Error: Source file does not exist.";
        return false;
    }

    // 1.5 Ensure destination directory exists
    QFileInfo destInfo(dest);
    QDir destDir = destInfo.dir();
    if (!destDir.exists()) {
        if (!destDir.mkpath(".")) {
            dbdebug << "Error: Could not create destination directory:" << destDir.path();
            return false;
        }
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
