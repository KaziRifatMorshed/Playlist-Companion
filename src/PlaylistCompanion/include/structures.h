#ifndef STRUCTURES_H
#define STRUCTURES_H
#include <QString>

struct Playlist {
  int playlistId;
  QString playlistName;
  QString playlistPath;
  QString playlistTitle;
  QString status;
  int totalVideoCount;
  int watchedCount;
  int totalTimeHour;
  QString creationDateTime;
  QString lastWatchedDateTime;
};

struct Video {
    int videoID;
    int playlistID;
    QString videoPath;
    QString videoTitle;
    int isWatched; // 0 for false, 1 for true
    int resumeTime;
};

#endif // STRUCTURES_H
