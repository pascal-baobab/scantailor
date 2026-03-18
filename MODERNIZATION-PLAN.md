# ScanTailor Modernization Strategy (revised)

## Context

ScanTailor is a ~20-year-old C++/Qt5 desktop app for post-processing scanned book pages. The codebase is functionally solid but carries heavy technical debt: legacy C++ (`std::auto_ptr`, `boost::lambda`), Qt4-era signal/slot macros, a pre-modern CMake build, no linting or formatting, and no CI test execution. This plan is an incremental roadmap — each step preserves full functionality and can be applied independently.

**Sequenza concordata con Pascal:**

1. Finire TUTTE le feature in corso su master (Potrace/PDF pipeline, etc.)
2. Push + CI verde su tutte e 3 le piattaforme (Windows, macOS, Linux)
3. Tag release stabile `v0.9.12` su master
4. SOLO DOPO: creare worktree + branch modernization e iniziare questo piano

---

## Phase 0 — Project Isolation (Git Worktree)

*Prerequisito: master e' stabile, CI verde, tag v0.9.12 creato.*

**Goal:** Two side-by-side folders sharing one git repository.

```
Documents/SCANTAILOR/
  ├── scantailor/            ← master branch (v0.9.12, stable, untouched)
  │     └── build-qt5/
  └── scantailor-new26/      ← modernization branch (all new work here)
        └── build-qt5/
```

### Steps

```bash
# 1. Verify master is clean, CI has passed, tag exists
cd /c/Users/Pass/Documents/SCANTAILOR/scantailor
git status                    # should be clean
git tag v0.9.12               # if not already tagged
git push origin v0.9.12       # push tag to GitHub

# 2. Create the worktree + branch in one command
git worktree add ../scantailor-new26 -b modernization

# 3. Verify both exist
ls /c/Users/Pass/Documents/SCANTAILOR/scantailor/         # master
ls /c/Users/Pass/Documents/SCANTAILOR/scantailor-new26/   # modernization branch
```

### How it works

- `scantailor/` stays on `master` at tag `v0.9.12` — never touch it during modernization
- `scantailor-new26/` is on `modernization` branch — all changes go here
- Both share one `.git` — no duplication, full history, one GitHub remote
- Each has its own `build-qt5/` directory (independent builds)
- You can open both in VS Code simultaneously
- When modernization is done: `git merge modernization` into master

### Safety rules

- **Never commit directly to master** during modernization
- If you need a bug fix in both: fix on master, then `git merge master` into modernization
- If something goes wrong: `git worktree remove ../scantailor-new26` removes the worktree cleanly (branch preserved)
- To nuke and restart: `git worktree remove ../scantailor-new26 && git branch -D modernization`
- You can always go back to `v0.9.12`: `git checkout v0.9.12`

### Build in the new worktree

```bash
# Same commands, different directory
cd /c/Users/Pass/Documents/SCANTAILOR/scantailor-new26
PATH="/c/msys64/mingw64/bin:$PATH"
cmake -S . -B build-qt5 -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_PREFIX_PATH="C:/msys64/mingw64" \
  -DENABLE_POPPLER=ON -DENABLE_TESSERACT=ON -DENABLE_POTRACE=ON
cmake --build build-qt5 --parallel
```

Both `scantailor/build-qt5/scantailor.exe` and `scantailor-new26/build-qt5/scantailor.exe` can run at the same time for comparison.

---

## Baseline Metrics

| Metric | Count | Scope |
|--------|-------|-------|
| `std::auto_ptr` | 191 | 65 files |
| Old `SIGNAL()/SLOT()` macros | 421 | 46 files |
| Raw `new`/`delete` | 366 | 119 files |
| `IntrusivePtr` (custom ref-count) | 651 | throughout |
| Boost production includes | 240+ | `boost::foreach`, `boost::function`, `boost::lambda`, `boost::bind`, `boost::multi_index`, `boost::shared_ptr`, `boost::scoped_array`, `boost::intrusive` |
| Test files | 24 | imageproc/ + math/ only |
| `stcore` source files | 147 | monolithic static lib |
| Linting/formatting configs | 0 | none |
| CMake minimum | 3.5 | legacy patterns throughout |
| C++ standard | not declared | compiler default; macOS CI forces C++14 |

---

## Phase 1 — Tooling & Safety Net

*Zero-risk changes that enable everything else.*

### 1.1 Set `CMAKE_CXX_STANDARD 14` explicitly
Currently **not declared** — relies on compiler defaults. This is fragile: different compilers on different CI runners pick different defaults. One line in root CMakeLists.txt.

**File:** `CMakeLists.txt`

### 1.2 Add `.clang-format` + `.git-blame-ignore-revs`
Add formatting config, but **do NOT reformat the whole codebase at once**. Instead:
- Apply only to files you're actively modifying (gradual adoption)
- When you do a bulk reformat later, record the commit SHA in `.git-blame-ignore-revs` so `git blame` stays clean

```yaml
# .clang-format
BasedOnStyle: Google
IndentWidth: 4
TabWidth: 4
ColumnLimit: 100
PointerAlignment: Left
SortIncludes: false
AllowShortFunctionsOnASingleLine: Empty
```

**Files to create:** `.clang-format`, `.git-blame-ignore-revs`

### 1.3 Add `.clang-tidy` (conservative)
Start with only:
- `modernize-use-nullptr`
- `modernize-use-override`
- `readability-redundant-member-init`

Disable everything else until the codebase is ready.

**File to create:** `.clang-tidy`

### 1.4 Add `.editorconfig`
Standardize indentation, line endings, encoding.

**File to create:** `.editorconfig`

### 1.5 Upgrade CMake minimum to 3.16
Ubuntu 22.04 ships 3.22; MSYS2 has 3.28+. Enables `target_compile_features`, modern generator expressions, `cmake_path`.

**File:** `CMakeLists.txt` line 1

### 1.6 Add `CMakePresets.json`
Named presets for debug/release/CI. Eliminates the long bash incantation in CLAUDE.md.

**File to create:** `CMakePresets.json`

### 1.7 Enable `-Wextra` and MSVC `/W3`
Current flags: only `-Wall -Wno-unused` on GCC, nothing on MSVC. Add:
- GCC/Clang: `-Wextra -Wshadow`
- MSVC: `/W3`
Do NOT add `-Werror` yet — too many existing warnings.

**File:** `cmake/SetDefaultGccFlags.cmake`, `CMakeLists.txt` (MSVC section)

### 1.8 CI improvements
- Add `actions/cache` for MSYS2/Homebrew/apt packages (cuts CI from 8-10 min to 2-3 min)
- Upgrade `actions/upload-artifact` to v4
- **Add `ctest --output-on-failure` step** — CI currently builds but never runs the 24 existing tests

**Files:** `.github/workflows/build-{windows,linux,macos}.yml`

---

## Phase 2 — Security Fixes

*These are concrete vulnerabilities to fix now, not theoretical audits.*

### 2.1 Integer overflow in TiffReader.cpp (REAL vulnerability)
Line 343: `TiffBuffer<uint32>(info.width * info.height)` — multiplies two `int` values with **no overflow check**. A malformed TIFF with huge dimensions can cause buffer underallocation and memory corruption.

**Fix:** Add bounds checking before the multiplication:
```cpp
if (info.width <= 0 || info.height <= 0 ||
    info.width > 100000 || info.height > 100000 ||
    static_cast<int64_t>(info.width) * info.height > MAX_PIXELS) {
    throw std::runtime_error("Image dimensions out of range");
}
```

**File:** `TiffReader.cpp` (line ~343)

### 2.2 XXE in ProjectReader.cpp
`QDomDocument::setContent()` can process external XML entities from untrusted `.ScanTailor` project files.

**Fix:** Use `QXmlStreamReader` (inherently XXE-safe), or configure `QXmlSimpleReader` with external entities disabled.

**File:** `ProjectReader.cpp`

### 2.3 Add project file format version
The project XML has **no version field**. Adding new features (like vectorized PDF settings) risks silently breaking project files opened by older versions. Add a `formatVersion="2"` attribute to the root `<project>` element, with a check on load.

**Files:** `ProjectReader.cpp`, `ProjectWriter.cpp`

### 2.4 Path traversal audit
Verify that output paths in `OutputGenerator.cpp` and `ProjectWriter.cpp` are resolved relative to the project directory and can't escape via `../`.

---

## Phase 3 — Boost Modernization (the hidden dependency)

*The original plan missed this entirely: Boost is used in 240+ production includes, not just tests.*

### 3.1 `BOOST_FOREACH` → range-based `for` (EASY, 100+ files)
`BOOST_FOREACH(Foo const& f, container)` → `for (Foo const& f : container)`. Mechanical, safe, zero risk.

### 3.2 `boost::function` → `std::function` (EASY, 26 files)
Drop-in replacement. Same API.

### 3.3 `boost::bind` → lambdas (EASY, 15 files)
`boost::bind(&Foo::bar, this, _1)` → `[this](auto& x) { bar(x); }`. Each conversion is local.

### 3.4 `boost::lambda` → C++ lambdas (EASY, 38 files)
Same idea. `boost::lambda` was a pre-C++11 workaround.

### 3.5 `boost::shared_ptr`/`weak_ptr` → `std::shared_ptr`/`weak_ptr` (EASY, 13 files)
Drop-in.

### 3.6 `boost::scoped_array` → `std::unique_ptr<T[]>` (EASY, 16 files)
Drop-in.

### 3.7 `boost::array` → `std::array` (EASY, 2 files)
Trivial.

### 3.8 `boost::multi_index_container` — KEEP (7 files)
**No standard C++ replacement exists.** Used in `ProjectWriter`, `ThumbnailSequence`, `ThumbnailPixmapCache`, `FileNameDisambiguator`, `RelinkingModel`, `ProjectPages`, `page_layout/Settings`. These are complex multi-key lookup structures. Keep Boost as a dependency for this.

### 3.9 `boost::intrusive::list` — KEEP (7 files)
No standard equivalent. Keep.

**After 3.1-3.7:** Boost dependency reduces from "heavy production use" to "multi_index + intrusive only". Testing dependency remains (Boost.Test).

---

## Phase 4 — C++ Language Modernization

### 4.1 `NULL`/`0` → `nullptr` (mechanical, safe)
Run `clang-tidy --fix` with `modernize-use-nullptr`.

### 4.2 Add `override` keywords (mechanical, safe)
Run `clang-tidy --fix` with `modernize-use-override`.

### 4.3 `std::auto_ptr` → `std::unique_ptr` (HIGH risk, file-by-file)
**The critical path to C++17 and Qt6.**

- 191 usages, 65 files
- `auto_ptr` copy = implicit move; `unique_ptr` requires explicit `std::move()`
- Each file must be individually migrated and compile-tested

**Order (leaf → core):**
1. Individual filter `Task.cpp` files (6 files)
2. Individual filter `Settings.cpp`, `Params.cpp` files
3. `imageproc/` utilities
4. `ThumbnailSequence.cpp` (29 usages)
5. `MainWindow.cpp` (9 usages) — **last**

**Do NOT batch. One file per commit.**

### 4.4 `SIGNAL()/SLOT()` → new connect syntax (MEDIUM risk, widget-by-widget)
421 usages, 46 files. Old macros are string-based (typos compile but fail silently at runtime).

New syntax catches mismatches at compile time:
```cpp
// Old: connect(btn, SIGNAL(clicked()), this, SLOT(onClicked()));
// New: connect(btn, &QPushButton::clicked, this, &MyWidget::onClicked);
```

Migrate one widget at a time. `MainWindow.cpp` (98 connections) last.

### 4.5 Upgrade to C++17 (AFTER 4.3 is complete)
**Prerequisite:** All `std::auto_ptr` removed.

Then set `CMAKE_CXX_STANDARD 17` and remove the macOS CI `-DCMAKE_CXX_STANDARD=14` workaround.

Unlocks: `std::optional`, structured bindings, `if constexpr`, `std::string_view`.

### 4.6 IntrusivePtr — DO NOT REPLACE
`IntrusivePtr` embeds the ref count in the object itself (one allocation, one pointer dereference). `std::shared_ptr` uses a separate control block (two allocations or `make_shared`, two dereferences). For 651 usages in image-processing-heavy code, **keep IntrusivePtr**. It's well-implemented with atomic ref counting (`QAtomicInt`).

---

## Phase 5 — Modern CMake Migration

*The build system uses pre-3.0 CMake patterns throughout.*

### 5.1 Replace global `INCLUDE_DIRECTORIES()` with per-target
Currently 10+ global `INCLUDE_DIRECTORIES()` calls apply ALL dependency headers to ALL targets. Migration:
```cmake
# Before (global):
INCLUDE_DIRECTORIES("${JPEG_INCLUDE_DIR}")
# After (per-target):
target_include_directories(stcore PRIVATE "${JPEG_INCLUDE_DIR}")
```

### 5.2 Add `PRIVATE`/`PUBLIC` keywords to `target_link_libraries`
Currently all link dependencies are implicitly PUBLIC. Add explicit keywords to express actual dependency relationships:
```cmake
target_link_libraries(scantailor PRIVATE stcore fix_orientation ...)
target_link_libraries(stcore PUBLIC Qt5::Core Qt5::Gui PRIVATE ${TIFF_LIBRARY})
```

### 5.3 Replace `ADD_DEFINITIONS()` with per-target `target_compile_definitions()`
Global `ADD_DEFINITIONS()` pollutes all targets.

### 5.4 Use `target_compile_features()` instead of raw flags where possible
```cmake
target_compile_features(stcore PRIVATE cxx_std_14)
```

**These changes are internal refactoring — no functional impact, but cleaner builds and faster incremental compilation.**

---

## Phase 6 — Performance

### 6.1 Profile first, optimize second
Add a `RelWithDebInfo` CMake preset. Profile real workloads with:
- Windows: Very Sleepy or Visual Studio profiler
- Linux: `perf` + flamegraph
- macOS: Instruments

**Do NOT add SIMD until profiling identifies actual hotspots.** The existing BinaryImage word-packing is already reasonably efficient. Premature SIMD adds complexity and platform-specific maintenance burden.

### 6.2 Streaming XML for large projects
Replace `QDomDocument` (full DOM in memory) with `QXmlStreamReader`/`QXmlStreamWriter` for `ProjectReader`/`ProjectWriter`. Benefits projects with 1000+ pages.

**Note:** This pairs well with 2.2 (XXE fix) — `QXmlStreamReader` is inherently XXE-safe.

### 6.3 Parallel multi-page processing (investigate only)
`WorkerThreadPool` already uses `QThreadPool` and filter `Settings` classes use `QMutex`. But `WorkerThreadPool` re-reads `QSettings` on every task submission (inefficiency). Worth caching the thread count.

Full parallelization across pages needs careful audit for shared mutable state — investigate before committing.

---

## Phase 7 — Testing

### 7.1 Add CI test execution (immediate, zero effort)
Add `ctest --output-on-failure` to all three CI workflows. The 24 existing tests compile but never run in CI.

### 7.2 Add filter Params round-trip tests
Each filter has `Params::toXml()`/`Params(QDomElement)`. Add unit tests verifying that serialize→deserialize produces identical objects. Uses existing Boost.Test framework.

### 7.3 Add Settings thread-safety tests
Filter `Settings` classes use `QMutex`. Add concurrent set/get tests to verify correctness.

### 7.4 Add regression image tests (later)
Small corpus of reference scans with known-good outputs. Pipeline comparison with structural similarity threshold.

### 7.5 Test framework — stay with Boost.Test
Switching to GoogleTest/Catch2 adds migration effort with minimal benefit. Boost.Test is already integrated and working. Keep it.

---

## Phase 8 — Architecture (long-term)

### 8.1 Split `stcore` (147 files) into sub-libraries
Current separation of concerns is poor — image I/O, UI components, processing, caching, metadata, and PDF export all in one static library.

Proposed split:
- `stcore_imageio` — ImageLoader, TiffReader/Writer, metadata loaders
- `stcore_pipeline` — BackgroundTask, WorkerThreadPool, FilterData
- `stcore_project` — ProjectReader/Writer, XmlMarshaller
- `stcore_ui` — ImageViewBase, ThumbnailSequence, PixmapRenderer, dialogs
- `stcore_export` — VectorPdfExporter, PotraceVectorizer, RagExporter

### 8.2 Decouple filter processing from UI
`Task.cpp` inner `UiUpdater` classes directly manipulate Qt widgets. Extract pure data result objects and a separate presenter layer. This would also clean up the GUI/CLI code split.

### 8.3 Add structured logging
Replace scattered `qDebug()` and `Q_ASSERT` with Qt logging categories or spdlog. Useful for CI diagnostics and user bug reports.

### 8.4 Qt6 migration (LONG TERM)
**Qt5 LTS support ended in 2025.** Qt6 migration is increasingly important but blocked by:
- `std::auto_ptr` removal (Phase 4.3)
- `QRegExp` → `QRegularExpression`
- `QTextCodec` changes
- Signal/slot signature changes
- `QPainter` API updates

**Prerequisite:** Complete Phases 3, 4.3, and 4.4 first.

---

## Phase 9 — Advanced PDF & OCR Pipeline (NEW FEATURES)

*Queste feature distinguono ScanTailor da ogni altro tool open-source di scansione.*

**Prerequisiti:** Phase 4.3 (auto_ptr) completata, C++17 disponibile.

### 9.1 PDF Ibrido Vettoriale (il "Santo Graal")

**Obiettivo:** Un PDF a 3 layer dove il testo e' vettoriale, le immagini sono raster, e tutto e' ricercabile.

```
Layer 3 (top):    Invisible OCR text     ← Tesseract (gia' implementato)
Layer 2 (middle): Vector bezier paths    ← Potrace (testo nitido a qualsiasi zoom)
Layer 1 (bottom): JPEG raster            ← Foto/illustrazioni
```

**Perche' funziona con ScanTailor:** Il filtro Output in modalita' "Mixed" gia' separa foreground (testo B&W) da background (immagini colore). Questa separazione e' la chiave:

```
Scan originale
  │
  ├──► Output "Mixed" mode (gia' esistente)
  │     ├── Foreground (B&W text)  ──► Potrace ──► Bezier paths (vector layer)
  │     └── Background (color)     ──► JPEG compress (raster layer)
  │
  └──► Tesseract OCR ──► Invisible text (search layer)
```

**Implementazione (3 sotto-step):**

**9.1a — Integrare PotraceVectorizer in VectorPdfExporter**
- `PotraceVectorizer.cpp` esiste gia', genera path PDF
- Modificare `VectorPdfExporter::exportPage()` per chiamare `traceToPathStream()`
- Risultato: PDF con testo vettoriale (senza raster background per ora)
- **File:** `VectorPdfExporter.cpp`, `PotraceVectorizer.cpp`

**9.1b — Zone Detection: separare testo da immagini**
- **Approccio 1 (preferito):** Usare l'output Mixed mode di ScanTailor (foreground/background gia' separati)
  - `OutputGenerator::process()` gia' produce `foreground_mask` (B&W) e `background_mask` (color)
  - Foreground → Potrace vectorization
  - Background → JPEG compression (150 DPI)
- **Approccio 2 (fallback):** Usare **Leptonica** (gia' dipendenza di Tesseract, zero costi aggiuntivi)
  - `pixClassifyBySkeleton()` — classifica componenti connessi come testo/non-testo
  - `pixGenHalftoneMask()` — separa halftone (foto) da lineart (testo)
  - `pixFindPageForeground()` — identifica regioni di contenuto
  - Vantaggio: funziona anche quando l'utente non usa Mixed mode
- **File:** `filters/output/OutputGenerator.cpp`, `VectorPdfExporter.cpp`

**9.1c — Composizione PDF multi-layer**
- Layer 1: Form XObject con JPEG del background
- Layer 2: Path operators dal Potrace (foreground vettoriale)
- Layer 3: Invisible text dal Tesseract OCR
- Clip e compositing corretti per sovrapporre i layer
- **File:** `VectorPdfExporter.cpp`

**Vantaggi del risultato:**
- Testo nitidissimo a qualsiasi zoom (non pixellato come JPEG)
- File piu' piccoli (le curve bezier pesano meno dei pixel)
- Foto preservate in qualita' JPEG
- Testo ricercabile e selezionabile via OCR
- Stampabile in alta qualita' senza artefatti

### 9.2 OCR Avanzato

**Obiettivo:** Andare oltre il semplice "testo invisibile" per supportare use case accademici e archivistici.

**9.2a — Layout Analysis multi-colonna**
- Tesseract ha gia' Page Segmentation Modes (PSM) per layout complessi
- PSM 1 (auto con OSD) rileva automaticamente colonne, blocchi, reading order
- Attualmente usiamo PSM 6 (singolo blocco uniforme) — limitante per libri a 2 colonne
- **Cambiamento:** Usare `PSM_AUTO_OSD` per default, con override manuale nell'UI
- **File:** `VectorPdfExporter.cpp` (Tesseract init)

**9.2b — Output hOCR**
- hOCR e' HTML con coordinate di ogni parola — standard per digital humanities
- Tesseract ha `GetHOCRText()` gia' nella sua API
- Generare file `.hocr` affiancato al PDF
- **File:** `VectorPdfExporter.cpp` (nuovo metodo `exportHOCR()`)

**9.2c — Output ALTO XML**
- ALTO (Analyzed Layout and Text Object) e' lo standard per biblioteche digitali (Library of Congress)
- **NON** convertire da hOCR — scrivere ALTO direttamente dal Tesseract `ResultIterator`
  - Abbiamo gia' word/line/block coordinates e confidence scores
  - Usare `QXmlStreamWriter` per generare ALTO 4.x valido
  - Schema ref: [github.com/altoxml/schema](https://github.com/altoxml/schema) (CC-BY-SA 4.0)
- Generare file `.alto.xml` affiancato al PDF
- **File:** Nuovo `AltoXmlWriter.cpp/h`

**9.2d — UI per opzioni OCR avanzate**
- Dropdown PSM mode (Auto, Single Block, Single Column, Sparse Text)
- Checkbox "Export hOCR" / "Export ALTO"
- Soglia confidenza OCR (slider, default 10%, range 0-50%)
- **File:** `filters/output/OptionsWidget.cpp`, `ui/OutputOptionsWidget.ui`

### 9.3 Cosa NON implementare (scope control)

| Idea | Perche' NO |
|------|-----------|
| AI Embeddings (RAG/pdf2vectors) | Dominio completamente diverso. Chi vuole embeddings usa Python, non un tool di scansione C++ |
| Font recognition/re-embedding | Complessita' enorme, risultati mediocri. Potrace tracing e' superiore per scansioni |
| PDF editing (stile Inkscape) | ScanTailor e' un pipeline, non un editor. Fuori scope |
| PDF/A compliance | Utile ma bassa priorita'. Richiede ICC profiles, XMP metadata, subset embedding. Fase futura se richiesto |

---

## Phase 10 — JBIG2 Compression (file size revolution)

*Scoperta dalla ricerca: jbig2enc riduce i PDF B&W del 3-5x rispetto a CCITT Group 4.*

### Contesto

OCRmyPDF usa `jbig2enc` per ottenere PDF di scansioni B&W estremamente compressi. La libreria classifica i componenti connessi (lettere simili) come template + offset, ottenendo compressione drammatica senza perdita visibile.

### 10.1 Integrare jbig2enc come codec opzionale

- **Libreria:** [github.com/agl/jbig2enc](https://github.com/agl/jbig2enc) — Apache 2.0 (compatibile GPL)
- **Dipendenza:** Usa Leptonica (gia' presente come dipendenza Tesseract)
- **Integrazione:** Aggiungere `ENABLE_JBIG2` flag in CMakeLists.txt
- **Dove:** `VectorPdfExporter.cpp` — per le pagine B&W, usare JBIG2 invece di JPEG
- **Risultato:** PDF 3-5x piu' piccoli per libri di solo testo

### 10.2 Modalita' lossless e lossy

- **Lossless:** Identica alla sorgente, massima compressione sicura
- **Lossy:** Ancora piu' compresso — raggruppa glifi simili (es. tutte le "e" diventano un template)
- **UI:** Checkbox "JBIG2 compression" + radio "Lossless / Lossy" nel pannello PDF
- **File:** `filters/output/OptionsWidget.cpp`, `VectorPdfExporter.cpp`

### 10.3 Combinazione con PDF ibrido

```
Pagina scansionata
  ├── Foreground (testo B&W) ──► JBIG2 compress ──► Layer raster ultra-compresso
  │                            └──► Potrace ──► Layer vettoriale (alternativa)
  └── Background (foto)       ──► JPEG compress ──► Layer raster colore
```

L'utente sceglie: testo come **vettori Potrace** (nitido, scalabile) o come **JBIG2 raster** (piu' piccolo, piu' veloce). Entrambi migliori del JPEG puro attuale.

---

## Phase 11 — Backend Hardening: ImageLoader + PDF Import + C++17

*Decisione presa 2026-03-18: mantenere il Qt5 GUI intatto, modernizzare solo il backend.
Un frontend React/Tauri e' stato valutato e scartato — la codebase C++ originale e'
troppo ben progettata per giustificare un rewrite.*

### 11.1 Fix ImageLoader Bugs (Critical)

**BUG: Double-free in ImageLoader.cpp:50-56**

Quando Poppler renderizza una pagina ma `image.isNull() == true`, `doc` viene deletato
due volte (linea 50 e linea 56). Undefined behavior.

```cpp
// Percorso che causa double-free:
Poppler::Document *doc = Poppler::Document::load(file_path);  // line 42
if (doc && !doc->isLocked()) {
    if (page_num >= 0 && page_num < doc->numPages()) {
        Poppler::Page *page = doc->page(page_num);
        if (page) {
            QImage image = page->renderToImage(300, 300);
            delete page;
            delete doc;     // ← line 50: PRIMO delete
            if (!image.isNull()) {
                return image;  // ← NON preso se image e' null
            }
            // cade qui...
        }
    }
    delete doc;             // ← line 56: SECONDO delete → UNDEFINED BEHAVIOR
}
```

**Tutti i fix:**

| # | Issue | Fix |
|---|-------|-----|
| 1 | Double-free `Poppler::Document*` | `std::unique_ptr` RAII (segue pattern `TiffReader::TiffHandle`) |
| 2 | Raw `Poppler::Page*` | `std::unique_ptr<Poppler::Page>` |
| 3 | `toLower()` heap allocation | `QString::compare(..., Qt::CaseInsensitive)` |
| 4 | Magic number 300 DPI duplicato | `static constexpr int DefaultPdfDpi = 300` in header |
| 5 | No `[[nodiscard]]` sui metodi `load()` | Aggiungere attributo C++17 |
| 6 | `image.load(&io_dev, 0)` — int come pointer | `nullptr` |
| 7 | Indentazione mista (tabs vs 2-spaces) | Tabs (convenzione progetto) |

**File modificati:**

- `ImageLoader.h` — `constexpr DefaultPdfDpi`, `[[nodiscard]]`
- `ImageLoader.cpp` — RAII, extract `renderPdfPage()`, fix double-free
- `PdfMetadataLoader.cpp` — RAII, usa `DefaultPdfDpi` condiviso

### 11.2 PDF Import Completo

**Stato attuale:**

- `PdfMetadataLoader` ✅ legge page count e dimensioni
- `ImageLoader::load()` ✅ renderizza singole pagine PDF a 300 DPI
- `ProjectFilesDialog` ✅ riconosce file PDF
- **Mancante:** DPI configurabile, selezione page range, gestione password

**Nuovo dialog `PdfImportDialog`:**

- Selezione page range (es. "1-50, 75-120")
- DPI selector (150/200/300/400/600, default 300)
- Campo password (appare se PDF e' protetto)
- Preview page count

**File creati:**

- `PdfImportDialog.h` — dichiarazione classe dialog Qt5
- `PdfImportDialog.cpp` — implementazione dialog

**File modificati:**

- `ImageLoader.h` — overload `load()` con parametro DPI
- `ImageLoader.cpp` — `renderPdfPage()` accetta DPI
- `ImageMetadataLoader.h` — nuovo status `PDF_PASSWORD_REQUIRED`
- `PdfMetadataLoader.cpp` — ritorna status distinto per PDF locked
- `ProjectFilesDialog.cpp` — mostra `PdfImportDialog` quando PDF selezionato
- `CMakeLists.txt` — aggiungere sorgenti `PdfImportDialog`

### 11.3 Modernizzazione C++17 Chirurgica

**Regola:** modernizzare SOLO i file gia' toccati per 11.1 e 11.2, piu' file con bug noti.
NON fare bulk-rewrite cosmetici.

**Feature C++17 usate:**

- `static constexpr int` — elimina magic numbers (zero overhead)
- `[[nodiscard]]` — previene discard di return costosi
- `std::unique_ptr` — RAII per Poppler (segue pattern esistente `TiffHandle`)
- `QImage{}` brace-init — intent piu' chiaro
- `nullptr` — type-safe, sostituisce 0-come-pointer
- Anonymous namespace — linkage interno, preferito a `static` dal C++11

**NON toccare:**

- `IntrusivePtr` (pattern progetto, performante)
- `QMutex` nei Settings (threading Qt, coerente)
- `QString const&` (convenzione Qt)
- `m_` prefix, CamelCase, tabs (stile progetto)
- Nessun file che non sia gia' in modifica

### 11.4 Ordine di Implementazione

| Step | Effort | Descrizione |
|------|--------|-------------|
| 11.1 | 1-2h | Fix ImageLoader + PdfMetadataLoader (RAII, double-free) |
| 11.2a | 1h | Aggiungere overload DPI a ImageLoader |
| 11.2b | 2-3h | Creare PdfImportDialog (Qt5 dialog) |
| 11.2c | 1h | Wire PdfImportDialog in ProjectFilesDialog |
| 11.2d | 1h | Password-protected PDF handling |
| 11.5 | 1h | Build + test completo |
| **Totale** | **~8h** | |

### 11.5 Verifica

1. **ASAN:** caricare PDF dove Poppler ritorna null image → no crash (double-free fix)
2. **PDF import:** aprire PDF 120 pagine, selezionare range 1-50 → 50 pagine nel progetto
3. **DPI:** importare stesso PDF a 150 e 600 DPI → risoluzioni diverse
4. **Password:** aprire PDF criptato → dialog password → password corretta funziona
5. **TIFF regression:** aprire TIFF multi-pagina → comportamento identico
6. **No-Poppler build:** `cmake -DENABLE_POPPLER=OFF` → compila pulito
7. **CI:** tutte e 3 le piattaforme passano

---

## External Resources & GitHub References

*Progetti e librerie scoperti durante la ricerca che sono direttamente utili.*

### ScanTailor Forks (reference per Qt6/C++17)

| Fork | URL | Note chiave |
|------|-----|-------------|
| ScanTailor Advanced | [github.com/ScanTailor-Advanced/scantailor-advanced](https://github.com/ScanTailor-Advanced/scantailor-advanced) | **Ha gia' migrato a Qt6 + C++17.** Studiare il loro approccio per Phase 4.5 e 8.4 |
| ScanTailor Universal | [github.com/trufanov-nok/scantailor-universal](https://github.com/trufanov-nok/scantailor-universal) | Combina Enhanced + Featured + Master |
| UB-Mannheim fork | [github.com/UB-Mannheim/scantailor](https://github.com/UB-Mannheim/scantailor) | Fork accademico (Universita' Mannheim) |

**Azione:** Prima di Phase 4.3 (auto_ptr) e 8.4 (Qt6), studiare i commit di ScanTailor-Advanced per capire come hanno fatto la migrazione. Non reinventare la ruota.

### Librerie PDF

| Libreria | Licenza | Decisione |
|----------|---------|-----------|
| PoDoFo 1.0 | LGPL 2.0 | **Candidato futuro** se serve PDF 2.0/PDF-A. Per ora il PDF hand-written basta |
| QPDF | Apache 2.0 | No — trasformazione, non creazione |
| libHaru | zlib | No — non mantenuta |
| Poppler | GPL | Solo lettura, gia' usata per import |
| MuPDF | **AGPL** | **Evitare** — licenza troppo restrittiva |

**Decisione: Mantenere il PDF hand-written.** E' veloce, controllabile, senza dipendenze aggiuntive. Considerare PoDoFo solo se/quando servira' PDF/A compliance.

### OCR & Layout Analysis

| Tool | URL | Uso |
|------|-----|-----|
| Leptonica | [github.com/DanBloomberg/leptonica](https://github.com/DanBloomberg/leptonica) | **Gia' dipendenza Tesseract.** Usare per zone detection (text vs image) invece di scrivere codice custom. 2700+ funzioni per document analysis |
| jbig2enc | [github.com/agl/jbig2enc](https://github.com/agl/jbig2enc) | Compressione B&W estrema. Apache 2.0. Usa Leptonica |
| Tesseract ResultIterator | (built-in API) | Coordinate precise per word/line/block — meglio di GetHOCRText() per il posizionamento PDF |

### hOCR → ALTO Conversion

| Tool | URL | Tipo |
|------|-----|------|
| filak/hOCR-to-ALTO | [github.com/filak/hOCR-to-ALTO](https://github.com/filak/hOCR-to-ALTO) | XSLT 2.0 stylesheets, supporta ALTO 2.0-4.x |
| UB-Mannheim/ocr-fileformat | [github.com/UB-Mannheim/ocr-fileformat](https://github.com/UB-Mannheim/ocr-fileformat) | CLI/web per conversione e validazione OCR formats |
| ALTO schema | [github.com/altoxml/schema](https://github.com/altoxml/schema) | Schema ufficiale Library of Congress, CC-BY-SA 4.0 |

**Nota per Phase 9.2c:** Invece di usare XSLT (richiede Saxon/processore esterno), scrivere un `AltoXmlWriter` nativo in C++ che usa direttamente i dati del Tesseract `ResultIterator`. Piu' semplice e zero dipendenze aggiuntive.

### Riferimenti per la pipeline OCRmyPDF

OCRmyPDF (Python) usa questa pipeline che possiamo emulare in C++:
1. Tesseract genera text-only PDF (invisible text layer)
2. img2pdf wrappa l'immagine originale in PDF
3. pikepdf (basato su QPDF) compone i due layer
4. jbig2enc comprime le pagine B&W
5. Risultato: sandwich PDF ottimizzato

**Il nostro equivalente C++:** Tesseract API diretto + Potrace + jbig2enc + PDF hand-written = stessa qualita', zero dipendenze Python.

---

## Priority Matrix

| # | Change | Risk | Effort | Impact | Phase |
|---|--------|------|--------|--------|-------|
| 1 | Set `CMAKE_CXX_STANDARD 14` | None | 5 min | High (correctness) | 1 |
| 2 | Fix TiffReader integer overflow | None | 1h | **Critical (security)** | 2 |
| 3 | Add CI test step | None | 30m | High (safety) | 7 |
| 4 | XXE fix in ProjectReader | Low | 2h | High (security) | 2 |
| 5 | Add project format version | Low | 3h | High (data integrity) | 2 |
| 6 | `BOOST_FOREACH` → range-for | None | 4h | Medium (cleanup) | 3 |
| 7 | `boost::function/bind/lambda` → std | None | 4h | Medium (cleanup) | 3 |
| 8 | `nullptr` replacement | None | 2h | Medium | 4 |
| 9 | `override` keywords | None | 2h | Medium | 4 |
| 10 | `.clang-format` + blame-ignore | None | 1h | Medium (consistency) | 1 |
| 11 | CMake presets + cache CI | Low | 3h | Medium (DX) | 1 |
| 12 | Enable `-Wextra` | Low | 1h | Medium | 1 |
| 13 | `std::auto_ptr` → `unique_ptr` | **High** | 2-3w | **Critical (C++17 gate)** | 4 |
| 14 | `SIGNAL/SLOT` → new syntax | Medium | 1-2w | High (type safety) | 4 |
| 15 | Modern CMake (per-target) | Medium | 1w | Medium (build hygiene) | 5 |
| 16 | Filter Params tests | Low | 3d | High (safety) | 7 |
| 17 | C++17 upgrade | Medium | 1d | High (after 4.3) | 4 |
| 18 | Profile + optimize | Low | 1w | Varies | 6 |
| 19 | Split stcore | Medium | 2w | Medium (maintainability) | 8 |
| 20 | Qt6 migration | **High** | 4-8w | High (longevity) | 8 |
| 21 | Potrace → VectorPdfExporter integration | Low | 3d | **High (key feature)** | 9 |
| 22 | Zone detection (Mixed fg/bg split) | Medium | 1w | **High (hybrid PDF)** | 9 |
| 23 | Multi-layer PDF compositing | Medium | 1w | **High (hybrid PDF)** | 9 |
| 24 | OCR layout analysis (PSM auto) | Low | 2h | Medium (accuracy) | 9 |
| 25 | hOCR export | Low | 1d | Medium (academic use) | 9 |
| 26 | ALTO XML export | Low | 2d | Medium (library use) | 9 |
| 27 | Advanced OCR UI options | Low | 1d | Medium (usability) | 9 |
| 28 | JBIG2 compression (jbig2enc) | Low | 3d | **High (file size -80%)** | 10 |
| 29 | JBIG2 UI (lossless/lossy toggle) | Low | 2h | Medium (usability) | 10 |
| 30 | Leptonica zone detection | Low | 3d | **High (enables hybrid PDF)** | 9 |
| 31 | Study ScanTailor-Advanced Qt6 commits | None | 2d | **High (saves weeks)** | 8 |
| 32 | Fix ImageLoader double-free (RAII) | None | 1h | **Critical (bug)** | 11 |
| 33 | PDF import dialog (range + DPI) | Low | 3h | High (feature) | 11 |
| 34 | Password-protected PDF support | Low | 1h | Medium (feature) | 11 |
| 35 | C++17 surgical modernization | None | 1h | Medium (quality) | 11 |

---

## Recommended Execution Order

```
PREREQUISITO: Master stabile, CI verde 3 piattaforme, tag v0.9.12

Day 0:       Phase 0 — git worktree setup (scantailor-new26/ on modernization branch)
             ALL subsequent work happens in scantailor-new26/ only.

Immediate:   Fix TiffReader overflow, set CMAKE_CXX_STANDARD, add CI test step
Week 1:      Phase 1 (tooling) + Phase 2 (remaining security fixes)
Week 2-3:    Phase 3 (Boost cleanup — mechanical, safe, high volume)
Week 4+:     Phase 4.1-4.2 (nullptr, override — mechanical)
Week 5-8:    Phase 4.3 (auto_ptr → unique_ptr — careful, file by file)
Week 9-10:   Phase 4.4 (SIGNAL/SLOT migration)
Week 11:     Phase 4.5 (C++17 upgrade)
Week 12:     Phase 5 (modern CMake)

--- Feature Development (su codebase modernizzato) ---

Week 13:     Phase 9.1a (Potrace integration in VectorPdfExporter)
Week 14:     Phase 9.1b (Leptonica zone detection — gia' dipendenza, zero costi)
Week 15:     Phase 9.1c (multi-layer PDF compositing)
Week 16:     Phase 10 (JBIG2 compression — alternativa ultra-compressa al JPEG)
Week 17:     Phase 9.2a-d (advanced OCR: layout, hOCR, ALTO, UI)

--- Backend Hardening ---

Week 18:     Phase 11.1 (Fix ImageLoader double-free, RAII Poppler)
Week 18:     Phase 11.2 (PDF import dialog: page range, DPI, password)
Week 18:     Phase 11.3 (C++17 chirurgico sui file toccati)

--- Optimization & Architecture ---

Week 19+:    Phase 6 (profile + optimize)
Pre-Qt6:     Studiare commit ScanTailor-Advanced per capire la loro migrazione Qt6
Long term:   Phase 8 (architecture, Qt6)

Final:       git merge modernization into master, remove worktree
```

---

## What the Original Plan Got Wrong

| Issue | Correction |
|-------|------------|
| Boost described as "test-only" | **240+ production includes** across function/lambda/bind/foreach/multi_index/shared_ptr — needs its own migration phase |
| sprintf/strcpy audit suggested | **None exist** — codebase is clean, this wastes effort |
| SIMD as performance priority | **Profile first** — premature without data, existing word-packing is decent |
| IntrusivePtr replaceable with shared_ptr | **Keep IntrusivePtr** — intrusive design is faster (1 alloc vs 2), well-implemented |
| CMake upgrade = "bump version number" | **Entire build uses pre-3.0 patterns** — global includes, no PRIVATE/PUBLIC, no CXX_STANDARD |
| Project XML streaming as "performance" | **Also a security fix** (XXE) and **data integrity issue** (no format version) |
| Qt5 LTS "until ~2026" | **Qt5 LTS ended 2025** — Qt6 is more urgent than stated |
| No mention of `.git-blame-ignore-revs` | Bulk reformatting without this destroys git blame history |
| Security section skipped TiffReader | **Concrete integer overflow vulnerability** found at line 343 |
| No mention of JBIG2 compression | **jbig2enc** (Apache 2.0) riduce B&W del 3-5x. Usato da OCRmyPDF. Dipende da Leptonica (gia' presente) |
| No mention of Leptonica for zone detection | **Gia' dipendenza Tesseract** — ha 2700+ funzioni per document analysis, inclusa separazione text/image |
| No mention of existing ScanTailor forks | **ScanTailor-Advanced ha gia' migrato a Qt6+C++17** — studiare i loro commit risparmia settimane |
| ALTO XML via conversione hOCR | **Meglio scrivere ALTO direttamente** dal Tesseract ResultIterator — zero dipendenze esterne |

---

## Critical Files

| File | Why |
|------|-----|
| `TiffReader.cpp:343` | Integer overflow vulnerability |
| `ProjectReader.cpp` | XXE risk, no format version |
| `ProjectWriter.cpp` | No format version |
| `CMakeLists.txt` | No CXX_STANDARD, legacy patterns throughout |
| `cmake/SetDefaultGccFlags.cmake` | Minimal warnings, no MSVC flags |
| `MainWindow.cpp` | 98 SIGNAL/SLOT, 9 auto_ptr — most complex file |
| `ThumbnailSequence.cpp` | 29 auto_ptr |
| `foundation/IntrusivePtr.h` | Keep as-is — well-designed, performant |
| `WorkerThreadPool.cpp` | QSettings re-read on every submit |
| All `filters/*/Task.cpp` | auto_ptr + UI coupling |
| All `filters/*/OptionsWidget.cpp` | SIGNAL/SLOT migration targets |
| `VectorPdfExporter.cpp` | Phase 9: multi-layer PDF composition, Potrace integration |
| `PotraceVectorizer.cpp` | Phase 9: bezier tracing, gia' skeleton funzionante |
| `filters/output/OutputGenerator.cpp` | Phase 9: fg/bg zone separation per hybrid PDF |
| Nuovo: `AltoXmlWriter.cpp/h` | Phase 9.2c: ALTO XML export per biblioteche |
| Leptonica (dipendenza Tesseract) | Phase 9.1b: zone detection text/image, gia' linkata |
| jbig2enc (nuova dipendenza opzionale) | Phase 10: compressione B&W estrema, Apache 2.0 |
| ScanTailor-Advanced (fork reference) | Phase 8.4: studiare la loro migrazione Qt6+C++17 |

---

## Verification Plan

*Come testare ogni fase prima di procedere alla successiva.*

| Phase | Test | Pass criteria |
|-------|------|---------------|
| 0 | `git worktree list` mostra 2 entries | Entrambi i worktree esistono |
| 1 | `cmake --build build-qt5 --parallel` compila senza errori | Zero errori, warnings accettabili |
| 1 | `ctest --test-dir build-qt5 --output-on-failure` | 24/24 test passano |
| 2 | Aprire un TIFF con dimensioni 99999x99999 | Errore gestito, no crash |
| 2 | Aprire un `.ScanTailor` con XXE payload | Nessuna entita' esterna processata |
| 3 | `grep -r "BOOST_FOREACH\|boost::function\|boost::bind\|boost::lambda" --include="*.cpp" --include="*.h"` | Zero match (solo multi_index e intrusive rimangono) |
| 4.3 | `grep -r "auto_ptr" --include="*.cpp" --include="*.h"` | Zero match |
| 4.5 | Build con `-DCMAKE_CXX_STANDARD=17` su 3 piattaforme | CI verde |
| 9.1 | Esportare PDF da batch Mixed mode, aprire in Adobe/Evince | Testo vettoriale (zoom 1000% nitido), foto JPEG preservate |
| 9.2 | Esportare hOCR + ALTO da batch OCR | File validi, parole con coordinate corrette |
| 10 | Esportare PDF con JBIG2 vs JPEG, confrontare dimensioni | JBIG2 almeno 3x piu' piccolo per pagine B&W |
| Finale | Confrontare scantailor.exe (master) vs scantailor.exe (modernization) su stesso progetto | Output identico pixel-per-pixel (regression test) |

### Dipendenze totali nella versione modernizzata

```
Obbligatorie (gia' presenti):    Qt5/Qt6, Boost (solo multi_index + intrusive), libtiff, libjpeg, libpng, zlib
Opzionali (gia' presenti):       Tesseract + Leptonica, Potrace, Poppler
Nuove opzionali:                 jbig2enc (Apache 2.0, dipende da Leptonica gia' presente)
Rimosse:                         Boost function/bind/lambda/foreach/shared_ptr/scoped_array/array
```
