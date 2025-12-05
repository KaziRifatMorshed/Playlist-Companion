<div align="center">
  <img src="logo/Banner.png" alt="Playlist Companion Banner">
</div>

<div align="center">

# PlaylistCompanion

**A desktop app to track your progress through local video playlists and tutorials**

[![Build Status](https://img.shields.io/badge/build-passing-brightgreen)](https://github.com/noobcod3r-rtx/Playlist-Companion)
[![License](https://img.shields.io/badge/license-MIT-blue)](https://github.com/noobcod3r-rtx/Playlist-Companion/blob/main/LICENSE)
[![Version](https://img.shields.io/badge/version-0.1.0-orange)](https://github.com/noobcod3r-rtx/Playlist-Companion)

</div>

Playlist Companion is a GUI desktop application built with C++ and the Qt6 framework that helps you track your progress through local video playlists and tutorials. It allows you to easily manage your video files, keep track of watched videos, and take notes to better organize your learning.

## ✨ Features

*   **Track Progress:** Mark videos as watched to keep track of your progress.
*   **Take Notes:** Write and save notes for each video.
*   **Easy Import:** Simply add a folder containing your video files.
*   **Cross-Platform:** Built with Qt, it can be compiled for Windows, macOS, and Linux.

## 🚀 Getting Started

### Prerequisites

*   C++ Compiler (with C++17 support)
*   CMake (version 3.10 or higher)
*   Qt6

### Installation

To build the project, run the following commands from the root directory:

```bash
mkdir -p build
cd build
cmake ..
make
```

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

## 📜 License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## 📂 Project Structure

*   `src/`: Contains the C++ source files.
*   `include/`: Contains the header files.
*   `build/`: Directory for build outputs. It is ignored by git.
*   `tests/`: Contains unit tests.
*   `CMakeLists.txt`: The build script for CMake.
