#ifndef DB_SCHEMA_H
#define DB_SCHEMA_H

#include <QString>
#include <QStringList>

// The full database schema as a raw string for initializing a new database.
const QString DB_SCHEMA = R"rawsql(
PRAGMA foreign_keys = ON;

CREATE TABLE IF NOT EXISTS MediaPlayerPath (
    mediaPlayerName TEXT PRIMARY KEY,
    mediaPlayerPath TEXT NOT NULL
);

CREATE TABLE IF NOT EXISTS Playlist (
    playlistId INTEGER PRIMARY KEY AUTOINCREMENT,
    playlistTitle TEXT NOT NULL CHECK(length(playlistTitle) > 0),
    playlistPath TEXT NOT NULL CHECK(length(playlistPath) > 0),
    status TEXT DEFAULT 'Planned to Watch' CHECK(status IN ('Planned to Watch', 'Watching', 'Completed')),
    totalVideoCount INTEGER DEFAULT 0 CHECK(totalVideoCount >= 0),
    watchedCount INTEGER DEFAULT 0 CHECK(watchedCount >= 0 AND watchedCount <= totalVideoCount),
    totalTimeHour INTEGER DEFAULT 0 CHECK(totalTimeHour >= 0),
    creationDateTime TEXT DEFAULT CURRENT_TIMESTAMP,
    updatingDateTime TEXT,
    lastWatchedDateTime TEXT
);

CREATE TABLE IF NOT EXISTS Video (
    videoID INTEGER PRIMARY KEY AUTOINCREMENT,
    playlistID INTEGER NOT NULL,
    videoPath TEXT NOT NULL,
    videoTitle TEXT NOT NULL,
    resumeTime INTEGER DEFAULT 0 CHECK(resumeTime >= 0),
    isWatched INTEGER DEFAULT 0 CHECK(isWatched IN (0, 1)),
    UNIQUE(playlistID, videoPath),
    FOREIGN KEY (playlistID) REFERENCES Playlist(playlistId) ON DELETE CASCADE
);

CREATE TABLE IF NOT EXISTS Notes (
    noteID INTEGER PRIMARY KEY AUTOINCREMENT,
    playlistId INTEGER NOT NULL,
    videoID INTEGER,
    noteText TEXT,
    vdoStartTime TEXT CHECK (vdoStartTime IS NULL OR vdoStartTime GLOB '[0-9][0-9]:[0-9][0-9]:[0-9][0-9]'),
    vdoEndTime TEXT CHECK (vdoEndTime IS NULL OR vdoEndTime GLOB '[0-9][0-9]:[0-9][0-9]:[0-9][0-9]'),
    FOREIGN KEY (playlistId) REFERENCES Playlist(playlistId) ON DELETE CASCADE,
    FOREIGN KEY (videoID) REFERENCES Video(videoID) ON DELETE CASCADE
);

CREATE TABLE IF NOT EXISTS General (
    id INTEGER PRIMARY KEY CHECK(id = 1),
    OS TEXT CHECK(OS IN ('Windows', 'Linux', 'Mac')),
    lastUpdated TEXT DEFAULT CURRENT_TIMESTAMP,
    defaultMediaPlayer TEXT DEFAULT '',
    lastWatchedPlId INTEGER,
    lastWatchedVdoId INTEGER,
    FOREIGN KEY (lastWatchedPlId) REFERENCES Playlist(playlistId) ON DELETE SET NULL,
    FOREIGN KEY (lastWatchedVdoId) REFERENCES Video(videoID) ON DELETE SET NULL
);
)rawsql";

// Individual statements for execution in order
inline QStringList getInitialSchemaStatements() {
    return {
        "PRAGMA foreign_keys = ON;",
        "CREATE TABLE IF NOT EXISTS MediaPlayerPath (mediaPlayerName TEXT PRIMARY KEY, mediaPlayerPath TEXT NOT NULL);",
        "CREATE TABLE IF NOT EXISTS Playlist (playlistId INTEGER PRIMARY KEY AUTOINCREMENT, playlistTitle TEXT NOT NULL CHECK(length(playlistTitle) > 0), playlistPath TEXT NOT NULL CHECK(length(playlistPath) > 0), status TEXT DEFAULT 'Planned to Watch' CHECK(status IN ('Planned to Watch', 'Watching', 'Completed')), totalVideoCount INTEGER DEFAULT 0 CHECK(totalVideoCount >= 0), watchedCount INTEGER DEFAULT 0 CHECK(watchedCount >= 0 AND watchedCount <= totalVideoCount), totalTimeHour INTEGER DEFAULT 0 CHECK(totalTimeHour >= 0), creationDateTime TEXT DEFAULT CURRENT_TIMESTAMP, updatingDateTime TEXT, lastWatchedDateTime TEXT);",
        "CREATE TABLE IF NOT EXISTS Video (videoID INTEGER PRIMARY KEY AUTOINCREMENT, playlistID INTEGER NOT NULL, videoPath TEXT NOT NULL, videoTitle TEXT NOT NULL, resumeTime INTEGER DEFAULT 0 CHECK(resumeTime >= 0), isWatched INTEGER DEFAULT 0 CHECK(isWatched IN (0, 1)), UNIQUE(playlistID, videoPath), FOREIGN KEY (playlistID) REFERENCES Playlist(playlistId) ON DELETE CASCADE);",
        "CREATE TABLE IF NOT EXISTS Notes (noteID INTEGER PRIMARY KEY AUTOINCREMENT, playlistId INTEGER NOT NULL, videoID INTEGER, noteText TEXT, vdoStartTime TEXT CHECK (vdoStartTime IS NULL OR vdoStartTime GLOB '[0-9][0-9]:[0-9][0-9]:[0-9][0-9]'), vdoEndTime TEXT CHECK (vdoEndTime IS NULL OR vdoEndTime GLOB '[0-9][0-9]:[0-9][0-9]:[0-9][0-9]'), FOREIGN KEY (playlistId) REFERENCES Playlist(playlistId) ON DELETE CASCADE, FOREIGN KEY (videoID) REFERENCES Video(videoID) ON DELETE CASCADE);",
        "CREATE TABLE IF NOT EXISTS General (id INTEGER PRIMARY KEY CHECK(id = 1), OS TEXT CHECK(OS IN ('Windows', 'Linux', 'Mac')), lastUpdated TEXT DEFAULT CURRENT_TIMESTAMP, defaultMediaPlayer TEXT DEFAULT '', lastWatchedPlId INTEGER, lastWatchedVdoId INTEGER, FOREIGN KEY (lastWatchedPlId) REFERENCES Playlist(playlistId) ON DELETE SET NULL, FOREIGN KEY (lastWatchedVdoId) REFERENCES Video(videoID) ON DELETE SET NULL);"
    };
}

#endif // DB_SCHEMA_H
