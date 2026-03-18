# ScanTailor — Claude Code Project Instructions

## What is this project?
ScanTailor is a desktop application (C++/Qt5) for post-processing scanned book pages:
fix orientation, split pages, deskew, select content, adjust margins, and produce
clean output images. It has both a GUI (`scantailor`) and a CLI (`scantailor-cli`).

Version: **0.9.12** — forked from scantailor-advanced, migrated from Qt4 to Qt5.

## Build (local, Windows/MSYS2)

```bash
# Configure
C:\msys64\usr\bin\bash.exe -lc '
  cmake -S /c/Users/Pass/Documents/SCANTAILOR/scantailor \
        -B /c/Users/Pass/Documents/SCANTAILOR/scantailor/build-qt5 \
        -G Ninja -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_PREFIX_PATH=/mingw64 -DENABLE_POPPLER=OFF'

# Build
C:\msys64\usr\bin\bash.exe -lc '
  cmake --build /c/Users/Pass/Documents/SCANTAILOR/scantailor/build-qt5 --parallel'

# Run
C:\Users\Pass\Documents\SCANTAILOR\scantailor\build-qt5\scantailor.exe
```

Build directory: `build-qt5/` (gitignored).

## CI/CD — GitHub Actions

Three workflows in `.github/workflows/`:

| Platform | Workflow              | Runner         | Artifact                         |
|----------|-----------------------|----------------|----------------------------------|
| Windows  | `build-windows.yml`   | windows-latest | `ScanTailor-Windows-x64.zip`     |
| Linux    | `build-linux.yml`     | ubuntu-22.04   | `ScanTailor-Linux-x86_64.AppImage` |
| macOS    | `build-macos.yml`     | macos-latest   | `ScanTailor-macOS` (.dmg)        |

GitHub repo: `https://github.com/pascal-baobab/scantailor`

### Known CI constraints
- **macOS**: Uses `-DCMAKE_CXX_STANDARD=17` — `std::auto_ptr` fully removed.
- **Linux**: Must use `-DCMAKE_POSITION_INDEPENDENT_CODE=ON` — Ubuntu Qt5 is built with `-reduce-relocations`.
- **Windows**: DLL collection uses iterative `objdump` loop (not `windeployqt` which is unreliable on MSYS2). Must iterate until convergence — 2 passes are NOT enough (need 4-5).
- **All platforms**: `concurrency: cancel-in-progress: true` to avoid stale CI runs.

## Architecture

```
filters/               # 6 processing stages (pipeline order):
  fix_orientation/     #   1. Fix page orientation
  page_split/          #   2. Split multi-page scans
  deskew/              #   3. Correct skew angle
  select_content/      #   4. Detect content boundaries
  page_layout/         #   5. Set margins
  output/              #   6. Generate final output
imageproc/             # Low-level image processing algorithms
dewarping/             # Cylindrical surface dewarping
foundation/            # Core utilities (Grid, IntrusivePtr, etc.)
math/                  # Math primitives (polynomials, splines)
interaction/           # UI interaction handlers
zones/                 # Zone editing tools
packaging/             # Platform packaging (osx/, windows/)
translations/          # Qt Linguist .ts files
```

Each filter has: `Filter.cpp/h`, `Task.cpp/h`, `CacheDrivenTask.cpp/h`, `Settings.cpp/h`, `OptionsWidget.cpp/h`, `Params.cpp/h`.

## Code conventions
- **C++ standard**: C++17 (`std::auto_ptr` fully removed, `std::unique_ptr` used throughout)
- **Qt**: Qt5 with AUTOMOC, manual AUTOUIC (OFF)
- **Pointers**: `std::unique_ptr` for ownership, `IntrusivePtr` for ref-counted objects
- **Naming**: CamelCase for classes, camelCase for methods, `m_` prefix for members, `m_ptr` prefix for smart pointer members
- **Build system**: CMake with Ninja generator

## Important notes
- `QLineF::intersect()` is deprecated in Qt 5.14+ (use `intersects()`), but still compiles with warnings. Not worth bulk-fixing unless doing a Qt6 migration.
- All `Q_WS_*` macros (Qt4) replaced with `Q_OS_*` (Qt5). Dead X11/XRender code removed — Qt5 uses raster engine.
- X11/XRender dependency removed (was only needed for Qt4's native X11 paint engine).
