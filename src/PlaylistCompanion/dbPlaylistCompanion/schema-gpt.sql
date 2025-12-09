PRAGMA foreign_keys = ON;

----------------------------------------------------------
-- 1. Table: MediaPlayerPath (Independent)
----------------------------------------------------------
CREATE TABLE IF NOT EXISTS MediaPlayerPath (
    mediaPlayerName TEXT PRIMARY KEY,
    mediaPlayerPath TEXT NOT NULL
);

----------------------------------------------------------
-- 2. Table: Playlist (Parent Table)
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
-- 3. Table: Video (Depends on Playlist)
----------------------------------------------------------
CREATE TABLE IF NOT EXISTS Video (
    videoID INTEGER PRIMARY KEY AUTOINCREMENT,
    playlistID INTEGER NOT NULL,
    videoPath TEXT NOT NULL,

    isWatched INTEGER DEFAULT 0 CHECK(isWatched IN (0, 1)),

    -- Prevent duplicates: Cannot have same video path twice in one playlist
    UNIQUE(playlistID, videoPath),

    FOREIGN KEY (playlistID)
        REFERENCES Playlist(playlistId)
        ON DELETE CASCADE
);

----------------------------------------------------------
-- 4. Table: Notes (Depends on Playlist and Video)
----------------------------------------------------------
CREATE TABLE IF NOT EXISTS Notes (
    noteID INTEGER PRIMARY KEY AUTOINCREMENT,

    playlistId INTEGER NOT NULL,
    videoID INTEGER,   -- NULL allowed for general playlist notes

    noteText TEXT,     -- Added this (Missing in your snippet?)

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

----------------------------------------------------------
-- 5. Table: General (Depends on Playlist and Video)
----------------------------------------------------------
-- Moved to the end so it can reference tables created above
CREATE TABLE IF NOT EXISTS General (
    id INTEGER PRIMARY KEY CHECK(id = 1),   -- Ensures only 1 row
    OS TEXT CHECK(OS IN ('Windows', 'Linux', 'Mac')),
    lastUpdated TEXT DEFAULT CURRENT_TIMESTAMP,
    defaultMediaPlayer TEXT DEFAULT '',

    lastWatchedPlId INTEGER,
    lastWatchedVdoId INTEGER,

    -- CHANGED: ON DELETE SET NULL
    -- If the playlist/video is deleted, just clear this field.
    -- Do NOT delete the settings row.
    FOREIGN KEY (lastWatchedPlId) REFERENCES Playlist(playlistId) ON DELETE SET NULL,
    FOREIGN KEY (lastWatchedVdoId) REFERENCES Video(videoID) ON DELETE SET NULL
);
