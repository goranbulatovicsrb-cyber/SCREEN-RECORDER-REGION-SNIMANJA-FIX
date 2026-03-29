# 📸 ScreenMaster Pro v2.0

> Professional Screenshot & Screen Recording Application built in C++ with Qt6

![Build](https://github.com/YOUR_USERNAME/ScreenMasterPro/actions/workflows/build.yml/badge.svg)

---

## ✨ Features

### Screenshot Modes
| Mode | Shortcut | Description |
|------|----------|-------------|
| 🖥️ Full Screen | `Print Screen` | Captures the entire monitor |
| ✂️ Region Select | `Ctrl+Shift+S` | Draw any area to capture |
| 🪟 Active Window | `Alt+Print Screen` | Captures the foreground window |
| 📜 Scrolling | `Ctrl+Shift+W` | Scrolling/long-page capture |
| 🕐 Timed | `Ctrl+Shift+T` | Countdown timer before capture |
| 🎨 Color Picker | `Ctrl+Shift+C` | Pick any color from screen |

### Screen Recording
- 🎬 Record at **15 / 24 / 30 / 60 FPS**
- ⏸ **Pause & Resume** recording
- 🖱️ Region or full-screen recording
- 💾 Saves as image sequence (FFmpeg-ready)

### Annotation Tools
- **↗ Arrow** — Draw directional arrows
- **▭ Rectangle** — Highlight areas with boxes  
- **◯ Ellipse** — Circle callouts
- **✏ Freehand** — Draw freely
- **T Text** — Add text labels
- **▌ Highlight** — Semi-transparent highlight
- **⬜ Blur** — Redact sensitive info
- **① Step Numbers** — Numbered step markers
- **🎨 Color picker** — 8 presets + custom colors
- ↩ **Undo / Redo** with full history

### Output Options
- 📋 Auto copy to clipboard
- 💾 Save as PNG, JPG, BMP, WebP, TIFF
- ⚙️ Adjustable quality (10–100%)
- 🏷️ Custom filename templates
- 📁 Custom save directory
- ☁️ Cloud upload (configurable)

### UI & UX
- 🌙 **Modern dark theme** with custom stylesheet
- 🖼️ **Splash screen** on startup
- 🔔 **System tray** with quick actions
- 🔍 **Magnifier** in region selector
- 📐 Live dimensions display during selection
- ⌨️ Full **keyboard shortcuts**
- 🖥️ **Multi-monitor** support
- 📜 **Capture history** with thumbnails

---

## 🚀 Building from GitHub Actions

### Step 1 — Fork or Push to GitHub
```
git init
git add .
git commit -m "Initial commit"
git remote add origin https://github.com/YOUR_USERNAME/ScreenMasterPro.git
git push -u origin main
```

### Step 2 — Wait for Build
GitHub Actions will automatically build the project. Go to:
`Actions` tab → `Build ScreenMaster Pro` → Download artifact

### Step 3 — Run
Extract `ScreenMasterPro-Windows-x64.zip` and run `ScreenMasterPro.exe`

---

## 🛠️ Build Locally

### Requirements
- Qt 6.6+
- CMake 3.20+
- MSVC 2019+ (Windows) or GCC/Clang (Linux/macOS)

### Build Steps
```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

---

## 🎬 Converting Recordings to Video

After recording, open the output folder and run:
```bash
ffmpeg -r 30 -i frame_%06d.png -c:v libx264 -pix_fmt yuv420p -crf 18 output.mp4
```

---

## ⌨️ All Keyboard Shortcuts

| Action | Shortcut |
|--------|----------|
| Full Screen Capture | `Print Screen` |
| Region Capture | `Ctrl+Shift+S` |
| Window Capture | `Alt+Print Screen` |
| Scrolling Capture | `Ctrl+Shift+W` |
| Timed Capture | `Ctrl+Shift+T` |
| Color Picker | `Ctrl+Shift+C` |
| Start/Stop Recording | `Ctrl+Shift+R` |
| Copy Last Capture | `Ctrl+C` |
| Save As | `Ctrl+S` |
| Undo (Annotate) | `Ctrl+Z` |
| Redo (Annotate) | `Ctrl+Y` |
| Cancel Selection | `Escape` |

---

## 📂 Project Structure
```
ScreenMasterPro/
├── .github/workflows/build.yml   ← GitHub Actions CI/CD
├── CMakeLists.txt
├── src/
│   ├── main.cpp
│   ├── MainWindow.h/.cpp         ← Main UI window
│   ├── RegionSelector.h/.cpp     ← Region capture overlay
│   ├── AnnotationWidget.h/.cpp   ← Screenshot annotator
│   ├── CaptureManager.h/.cpp     ← Screenshot engine
│   ├── ScreenRecorder.h/.cpp     ← Recording engine
│   ├── HistoryPanel.h/.cpp       ← Capture history
│   ├── TrayManager.h/.cpp        ← System tray
│   ├── CountdownOverlay.h/.cpp   ← Timer countdown UI
│   ├── SettingsDialog.h/.cpp
│   ├── resources.qrc
│   ├── icons/
│   │   └── app_icon.png
│   └── styles/
│       └── dark.qss              ← Full dark theme CSS
└── README.md
```

---

*Made with ❤️ using C++ and Qt6*
