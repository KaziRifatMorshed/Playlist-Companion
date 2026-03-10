QT       += core gui sql

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++23

# Define version and target metadata
VERSION = 1.1.8
APP_VERSION = v$${VERSION}
TARGET = PlaylistCompanion
DISPLAY_NAME = "PlaylistCompanion"
PUBLISHER = "Kazi Rifat Morshed CSEKU230220 Development"
COPYRIGHT = "Copyright (C) 2026 Kazi Rifat Morshed. All rights reserved."
DESCRIPTION = "Local Video Playlist Tracker and Productivity Tool"
CONTACT_EMAIL = "rifat230220@cseku.ac.bd"

# Get Git Hash (if in a git repo)
GIT_HASH = $$system(git rev-parse --short HEAD)
isEmpty(GIT_HASH): GIT_HASH = "unknown"

# Get Build Date & Time
win32 {
    BUILD_DATE_VAL = $$system(powershell -NoProfile -Command "Get-Date -Format 'yyyy-MM-dd_HH-mm-ss'")
} else {
    BUILD_DATE_VAL = $$system(date +"%Y-%m-%d_%H:%M:%S")
}

# Pass to C++ as macros
DEFINES += APP_VERSION_STR=\"\\\"$${APP_VERSION}\\\"\"
DEFINES += APP_NAME_STR=\"\\\"$${DISPLAY_NAME}\\\"\"
DEFINES += APP_PUBLISHER_STR=\"\\\"$${PUBLISHER}\\\"\"
DEFINES += APP_COPYRIGHT_STR=\"\\\"$${COPYRIGHT}\\\"\"
DEFINES += APP_DESC_STR=\"\\\"$${DESCRIPTION}\\\"\"
DEFINES += APP_CONTACT_STR=\"\\\"$${CONTACT_EMAIL}\\\"\"
DEFINES += GIT_HASH_STR=\"\\\"$${GIT_HASH}\\\"\"
DEFINES += BUILD_DATE_TIME=\"\\\"$${BUILD_DATE_VAL}\\\"\"

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    addnewplaylistwindow.cpp \
    db_sqlite.cpp \
    main.cpp \
    mainwindow.cpp \
    settings.cpp

HEADERS += \
    addnewplaylistwindow.h \
    include/db_sqlite.h \
    include/structures.h \
    mainwindow.h \
    settings.h

FORMS += \
    addnewplaylistwindow.ui \
    mainwindow.ui \
    settings.ui

TRANSLATIONS += \
    PlaylistCompanion_bn_BD.ts
CONFIG += lrelease
CONFIG += embed_translations

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

RESOURCES += \
    Playlist-Companion_resources.qrc
win32: RC_ICONS = logo/logo.ico
QT += multimedia
