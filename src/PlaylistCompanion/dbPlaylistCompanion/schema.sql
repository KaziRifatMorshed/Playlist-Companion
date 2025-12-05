-- Enable foreign key constraints (required for SQLite)
PRAGMA foreign_keys = ON;

-- 1. Table: General
CREATE TABLE General (
    OS TEXT CHECK(OS IN ('Windows', 'Linux', 'Mac')),
    lastUpdated TEXT DEFAULT CURRENT_TIMESTAMP
);

-- 2. Table: MediaPlayerPath
CREATE TABLE MediaPlayerPath (
    mediaPlayerName TEXT,
    mediaPlayerPath TEXT,
    -- Converted "string defaultMediaPlayer" to a boolean flag (0 or 1)
    -- Set this to 1 for the player you want to be the default.
    isDefault INTEGER DEFAULT 0 CHECK(isDefault IN (0, 1))
);

-- 3. Table: Playlist
CREATE TABLE Playlist (
    playlistId INTEGER PRIMARY KEY AUTOINCREMENT,
    playlistTitle TEXT NOT NULL CHECK(length(playlistTitle) > 0),
    playlistPath TEXT NOT NULL CHECK(length(playlistPath) > 0),
    status TEXT DEFAULT 'Planned to Watch' CHECK(status IN ('Planned to Watch', 'Watching', 'Completed')),
    totalVideoCount INTEGER,
    watchedCount INTEGER,
    totalTimeHour INTEGER,
    creationDateTime TEXT DEFAULT CURRENT_TIMESTAMP,
    updatingDateTime TEXT,
    lastWatchedDateTime TEXT
);

-- 4. Table: Video
CREATE TABLE Video (
    videoID INTEGER PRIMARY KEY AUTOINCREMENT,
    playlistID INTEGER,
    videoPath TEXT NOT NULL,
    isWatched INTEGER DEFAULT 0 CHECK(isWatched IN (0, 1)), -- 0=False, 1=True
    FOREIGN KEY (playlistID) REFERENCES Playlist(playlistId) ON DELETE CASCADE
);

-- 5. Table: Notes
CREATE TABLE Notes (
    noteID INTEGER PRIMARY KEY AUTOINCREMENT,
    playlistId INTEGER,
    videoID INTEGER NOT NULL DEFAULT -1,
    vdoStartTime TEXT, -- Store as 'HH:MM:SS'
    vdoEndTime TEXT,   -- Store as 'HH:MM:SS'
    FOREIGN KEY (playlistId) REFERENCES Playlist(playlistId) ON DELETE CASCADE
);
