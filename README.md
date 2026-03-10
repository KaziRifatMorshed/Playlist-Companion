<div align="center">
  <img src="logo/Banner.png" alt="Playlist Companion Banner">
</div>

<div align="center">

# PlaylistCompanion

**A desktop app to track your progress through local video playlists and tutorials**

[![Build Status](https://img.shields.io/badge/build-passing-brightgreen)](https://github.com/noobcod3r-rtx/Playlist-Companion)
[![License](https://img.shields.io/badge/license-Apache--2.0-blue)](https://github.com/noobcod3r-rtx/Playlist-Companion/blob/main/LICENSE)
[![Version](https://img.shields.io/badge/version-1.1.8-orange)](https://github.com/noobcod3r-rtx/Playlist-Companion)
[![Project Website](https://img.shields.io/badge/website-visit-brightgreen)](https://kazirifatmorshed.github.io/projects/PlaylistCompanion.html)

</div>

Playlist Companion is a GUI desktop application built with C++ and the Qt6 framework that helps you track your progress through local video playlists and tutorials. It allows you to easily manage your video files, keep track of watched videos, and take notes to better organize your learning.

## ✨ Features

*   **Playlist Management:**
    *   Create, edit, and remove playlists effortlessly.
    *   **Auto-Import:** Add a folder, and the app automatically scans and imports all video files.
    *   **Sync:** Update existing playlists to include new videos added to the source folder.
*   **Progress Tracking:**
    *   **Watch Status:** Mark videos as watched or unwatched to keep track of your learning journey.
    *   **Navigation:** Quickly jump between videos with "Next" and "Previous" controls.
    *   **Visual Indicators:** Highlighting in the video table clearly shows your current progress.
*   **Detailed Analytics:**
    *   **Progress Bars:** Real-time visual representation of completion for each playlist.
    *   **Time Statistics:** Automatically calculates total watched time and remaining time in hours.
    *   **Completion Stats:** View the number of videos watched versus the total count at a glance.
*   **Integrated Note-Taking:**
    *   Write and save persistent notes for *every* video in your playlist.
    *   Notes are automatically saved and loaded as you navigate through your videos.
*   **Media Integration:**
    *   **External Player:** Launch your videos in your favorite external media player.
    *   **Configurable Settings:** Choose and set your preferred default media player.
    *   **Thumbnails:** Automatic thumbnail generation to help you visually identify your videos.
*   **Data Reliability:**
    *   **SQLite Backend:** All your data is safely stored in a local SQLite database.
    *   **Backup & Restore:** Built-in tools to create database backups and restore them whenever needed.
*   **Cross-Platform:**
    *   Native support for Windows, macOS, and Linux thanks to the Qt6 framework.
*   **Multi-language Support:**
    *   Currently supports English.

## 🚀 Getting Started

### Prerequisites

*   C++ Compiler (with C++23 support)
*   Qt6 (with `multimedia`, `sql`, and `widgets` modules)
*   `qmake` (Qt Build Tool)

### Installation

To build the project from source:

1.  **Clone the repository:**
    ```bash
    git clone https://github.com/noobcod3r-rtx/Playlist-Companion.git
    cd Playlist-Companion
    ```

2.  **Navigate to the source directory:**
    ```bash
    cd src/PlaylistCompanion
    ```

3.  **Build the project:**
    ```bash
    mkdir build
    cd build
    qmake ..
    make
    ```
    *Note: On Windows, use `nmake` or `jom` instead of `make`, or open the `.pro` file in Qt Creator.*

## 🏃‍♀️ Usage

After building the project, you can run the executable from the `build` directory:

```bash
./PlaylistCompanion
```

## 🤝 Contributing

Contributions are welcome! Please feel free to submit a pull request or open an issue.

1.  Fork the Project
2.  Create your Feature Branch (`git checkout -b feature/AmazingFeature`)
3.  Commit your Changes (`git commit -m 'Add some AmazingFeature'`)
4.  Push to the Branch (`git push origin feature/AmazingFeature`)
5.  Open a Pull Request

## 💖 Acknowledgments

*   **Pritom Das (CSEKU250220):** I am really grateful for your contribution and support as an active tester and for quickly finding issues.

## 📜 License

This project is licensed under the Apache License 2.0 - see the [LICENSE](LICENSE) file for details.

## 📂 Project Structure

*   `src/PlaylistCompanion/`: Main source code, UI files, and project configuration (`.pro`).
    *   `include/`: Header files.
*   `logo/`: Application logos and assets.
*   `script/`: Installation and helper scripts.
*   `tests/`: Unit tests for the application.
*   `PlaylistCompanion.pro`: The qmake project file.

# Author

- **Name**: Kazi Rifat Morshed
- **Affiliation**: Computer Science and Engineering Discipline, [Khulna University](https://www.ku.ac.bd)
- **Email**: rifat230220@cseku.ac.bd
- **Website**: [https://kazirifatmorshed.github.io](https://kazirifatmorshed.github.io)
