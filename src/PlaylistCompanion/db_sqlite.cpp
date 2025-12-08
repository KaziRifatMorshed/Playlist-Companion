#include "include/db_sqlite.h"

SQliteDB *SQliteDB::dbInstance = nullptr;
QString SQliteDB::appPath = "";
QString SQliteDB::appDirPath = "";
QString SQliteDB::dbPath = "";
QString SQliteDB::dbDirPath = "";
