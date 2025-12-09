PRAGMA foreign_keys = ON;

----------------------------------------------------------
-- 1. Table: General  (Single row configuration table)
----------------------------------------------------------
CREATE TABLE IF NOT EXISTS General (
    id INTEGER PRIMARY KEY CHECK(id = 1),   -- Ensures only 1 row
    OS TEXT CHECK(OS IN ('Windows', 'Linux', 'Mac')),
    lastUpdated TEXT DEFAULT CURRENT_TIMESTAMP,
    defaultMediaPlayer TEXT DEFAULT '',
    hasOpenedBefore INTEGER DEFAULT 0 CHECK(hasOpenedBefore IN (0, 1))
);

----------------------------------------------------------
-- 2. Table: MediaPlayerPath
----------------------------------------------------------
CREATE TABLE IF NOT EXISTS MediaPlayerPath (
    mediaPlayerName TEXT PRIMARY KEY,           -- Unique media player name
    mediaPlayerPath TEXT NOT NULL
);

----------------------------------------------------------
-- 3. Table: Playlist
----------------------------------------------------------
CREATE TABLE IF NOT EXISTS Playlist (
    playlistId INTEGER PRIMARY KEY AUTOINCREMENT,
    playlistTitle TEXT NOT NULL CHECK(length(playlistTitle) > 0),
    playlistPath TEXT NOT NULL CHECK(length(playlistPath) > 0),

    status TEXT DEFAULT 'Planned to Watch'
        CHECK(status IN ('Planned to Watch', 'Watching', 'Completed')),

    totalVideoCount INTEGER DEFAULT 0 CHECK(totalVideoCount >= 0),
    watchedCount INTEGER DEFAULT 0
        CHECK(watchedCount >= 0 AND watchedCount <= totalVideoCount),

    totalTimeHour INTEGER DEFAULT 0 CHECK(totalTimeHour >= 0),

    creationDateTime TEXT DEFAULT CURRENT_TIMESTAMP,
    updatingDateTime TEXT,
    lastWatchedDateTime TEXT
);

----------------------------------------------------------
-- 4. Table: Video
----------------------------------------------------------
CREATE TABLE IF NOT EXISTS Video (
    videoID INTEGER PRIMARY KEY AUTOINCREMENT,
    playlistID INTEGER NOT NULL,
    videoPath TEXT NOT NULL,

    isWatched INTEGER DEFAULT 0 CHECK(isWatched IN (0, 1)),

    -- Prevent duplicates inside the same playlist
    UNIQUE(playlistID, videoPath),

    FOREIGN KEY (playlistID)
        REFERENCES Playlist(playlistId)
        ON DELETE CASCADE
);

----------------------------------------------------------
-- 5. Table: Notes
----------------------------------------------------------
CREATE TABLE IF NOT EXISTS Notes (
    noteID INTEGER PRIMARY KEY AUTOINCREMENT,

    playlistId INTEGER NOT NULL,
    videoID INTEGER,   -- NULL = general playlist note

    -- Text times stored as HH:MM:SS
    vdoStartTime TEXT CHECK (
        vdoStartTime IS NULL OR
        vdoStartTime GLOB '[0-9][0-9]:[0-9][0-9]:[0-9][0-9]'
    ),
    vdoEndTime TEXT CHECK (
        vdoEndTime IS NULL OR
        vdoEndTime GLOB '[0-9][0-9]:[0-9][0-9]:[0-9][0-9]'
    ),

    FOREIGN KEY (playlistId)
        REFERENCES Playlist(playlistId)
        ON DELETE CASCADE,

    FOREIGN KEY (videoID)
        REFERENCES Video(videoID)
        ON DELETE CASCADE
);
