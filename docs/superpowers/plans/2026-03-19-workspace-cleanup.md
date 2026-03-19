# Workspace & Configuration Cleanup Plan

> **For agentic workers:** REQUIRED: Use superpowers:subagent-driven-development (if subagents available) or superpowers:executing-plans to implement this plan. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Risolvere le 10 inconsistenze trovate nell'audit del workspace SCANTAILOR: memory, CLAUDE.md, file duplicati, file non tracciati, bot Telegram.

**Architecture:** Operazioni su file di configurazione e documentazione — nessuna modifica al codice sorgente C++. Ogni task è indipendente e può essere eseguito in parallelo.

**Tech Stack:** Git, Markdown, Claude Code memory system

**Stato attuale (verificato 2026-03-19):**
- `scantailor/` → branch `claude/fix-scantailor-dll-errors-eKZ1a` (main repo)
- `scantailor-new26/` → worktree branch `modernization`
- `scantailor-phase11/` → worktree branch `phase11`
- `scantailor-advanced/` → fork upstream 4lex4 (riferimento)
- `~/.claude/telegram-bot/` → vecchio bot (obsoleto, sostituito da `claude-telegram-bridge/`)
- `claude-telegram-bridge/` e `docs/` → **UNTRACKED** nel repo

---

## Chunk 1: Memory e CLAUDE.md

### Task 1: Creare memory per la root SCANTAILOR/

**Problema:** Quando Claude si apre da `C:\Users\Pass\Documents\SCANTAILOR\` (root), cerca memory in `C--Users-Pass-Documents-SCANTAILOR/memory/` che è VUOTA. Le 14 memorie esistono in `c--Users-Pass-Documents-SCANTAILOR-scantailor/memory/`.

**Files:**
- Create: `C:\Users\Pass\.claude\projects\C--Users-Pass-Documents-SCANTAILOR\memory\MEMORY.md`
- Create: `C:\Users\Pass\.claude\projects\C--Users-Pass-Documents-SCANTAILOR\memory\user_profile.md`
- Create: `C:\Users\Pass\.claude\projects\C--Users-Pass-Documents-SCANTAILOR\memory\project_workspace.md`

**Approccio:** NON duplicare tutte le 14 memorie. Creare un MEMORY.md minimale per la root che:
- Contiene il profilo utente (necessario ovunque)
- Contiene una memoria "workspace" che spiega la struttura multi-worktree
- Rimanda alle memorie dettagliate in `scantailor/` per chi apre da lì

- [ ] **Step 1: Creare user_profile.md nella root memory**

```markdown
---
name: User profile
description: Pascal — Italian developer, Windows 10 MSYS2, high quality standards, scans Italian academic texts
type: user
---

- **Name**: Pascal (GitHub: pascal-baobab)
- **Language**: Italian (communicates in Italian, code/comments in English)
- **Role**: Developer/creator of ScanTailor fork — manages the project end-to-end
- **Platform**: Windows 10 IoT Enterprise LTSC 2021, MSYS2/MinGW64 toolchain
- **Working style**: Fast, decisive action. Values working artifacts over perfect code. Test locally before CI.
- **Primary use case**: Scanning Italian academic texts (Rudolf Steiner). Needs compact, searchable PDFs.
- **Quality**: Very high standards — no visual quality degradation tolerated.
```

- [ ] **Step 2: Creare project_workspace.md**

```markdown
---
name: project_workspace
description: Struttura workspace SCANTAILOR — 3 worktree git, bot Telegram, relazioni tra directory
type: project
---

## Workspace Structure

La root `C:\Users\Pass\Documents\SCANTAILOR\` contiene un singolo repo git con worktree multipli:

| Directory | Branch | Ruolo |
|-----------|--------|-------|
| `scantailor/` | `master` (+ feature branches) | Main repo, codice originale v0.9.12 |
| `scantailor-new26/` | `modernization` (worktree) | Fasi 1-7 completate, C++17, Qt5 moderno |
| `scantailor-phase11/` | `phase11` (worktree) | Backend hardening (Phase 11) |
| `scantailor-advanced/` | — | Fork upstream 4lex4 (solo riferimento, non toccare) |

**Bot Telegram:** `scantailor/claude-telegram-bridge/` (per-progetto, sostituisce il vecchio `~/.claude/telegram-bot/`)

**Why:** Pascal lavora su più fasi in parallelo usando git worktree. Ogni worktree è una "vista" dello stesso repo su branch diversi.

**How to apply:** Quando si lavora dalla root, identificare prima quale worktree/branch è il target. Le memorie dettagliate (build, CI, feedback) sono in `c--Users-Pass-Documents-SCANTAILOR-scantailor/memory/`.
```

- [ ] **Step 3: Creare MEMORY.md index**

```markdown
# Memory Index

## User
- [[user_profile]] — Pascal: Italian developer, Windows 10/MSYS2, high quality standards

## Project
- [[project_workspace]] — Struttura workspace: 3 worktree git, bot Telegram, relazioni tra directory

> **Note:** Memorie dettagliate (build, CI, feedback, PDF, Telegram) sono in `c--Users-Pass-Documents-SCANTAILOR-scantailor/memory/` — accessibili quando Claude si apre da `scantailor/`.
```

- [ ] **Step 4: Verificare**

Aprire una nuova sessione Claude dalla root `SCANTAILOR/` e verificare che le memorie vengano caricate.

---

### Task 2: Creare CLAUDE.md per la root SCANTAILOR/

**Problema:** Nessun CLAUDE.md nella root — Claude non ha contesto quando si apre da qui.

**Files:**
- Create: `C:\Users\Pass\Documents\SCANTAILOR\CLAUDE.md`

- [ ] **Step 1: Creare CLAUDE.md root**

```markdown
# ScanTailor Workspace — Claude Code Instructions

## Structure

This is a multi-worktree workspace for the ScanTailor project.

| Directory | Branch | Purpose |
|-----------|--------|---------|
| `scantailor/` | master + feature branches | Main repo (v0.9.12, original Qt4→Qt5 migration) |
| `scantailor-new26/` | `modernization` worktree | Phases 1-7 complete: C++17, modern CMake, tests |
| `scantailor-phase11/` | `phase11` worktree | Phase 11: Backend Hardening |
| `scantailor-advanced/` | upstream reference | 4lex4's fork — read-only reference, do NOT modify |

All worktrees share the same git repo (`scantailor/.git`).

## Active work

- **Phase 11** (scantailor-phase11/): Backend hardening — ImageLoader fixes, PDF import dialog
- **Telegram Bot** (scantailor/claude-telegram-bridge/): Bidirectional Claude↔Telegram bridge

## Build

See `scantailor/CLAUDE.md` for full build instructions. Quick reference:

```bash
C:\msys64\usr\bin\bash.exe -lc '
  cmake --build /c/Users/Pass/Documents/SCANTAILOR/scantailor/build-qt5 --parallel'
```

## Important

- **Language**: Communicate in Italian with Pascal.
- **Quality**: Never sacrifice visual quality for file size.
- **Testing**: Always test locally before pushing to CI.
- **Workflow**: Ask clarifying questions before implementing (Socratic approach).
```

- [ ] **Step 2: Verificare** che Claude carichi il CLAUDE.md quando aperto dalla root.

---

### Task 3: Correggere CLAUDE.md nei worktree

**Problema:** `scantailor-new26/CLAUDE.md` e `scantailor-phase11/CLAUDE.md` hanno:
- Versione "2.2" nell'intro ma "current: 3.0" nel versioning → contraddizione
- Build path che puntano a `scantailor/build-qt5` invece dei propri path

**Files:**
- Modify: `C:\Users\Pass\Documents\SCANTAILOR\scantailor-new26\CLAUDE.md`
- Modify: `C:\Users\Pass\Documents\SCANTAILOR\scantailor-phase11\CLAUDE.md`

- [ ] **Step 1: Fix scantailor-new26/CLAUDE.md**

Cambiare la riga versione da:
```
Version: **2.2** — forked from scantailor-advanced, migrated from Qt4 to Qt5, modernized to C++17.
```
a:
```
Version: **3.0** — forked from scantailor-advanced, migrated from Qt4 to Qt5, modernized to C++17.
```

E aggiornare i path di build da `scantailor/build-qt5` a `scantailor-new26/build-qt5` (se la build avviene lì) oppure documentare che la build usa sempre il path di `scantailor/`.

- [ ] **Step 2: Fix scantailor-phase11/CLAUDE.md**

Stesse correzioni. Inoltre aggiungere sezione specifica su Phase 11:
```markdown
## Current Focus: Phase 11 — Backend Hardening
- 11.1: Fix ImageLoader double-free bug
- 11.2: PDF import dialog
- 11.3: Surgical C++17 modernization
- 11.4: Implementation order
- 11.5: Verification checklist
```

- [ ] **Step 3: Verificare coerenza** — la versione nell'intro deve corrispondere a `## Versioning → Current version`.

---

## Chunk 2: File duplicati e pulizia

### Task 4: Rimuovere DPI_BYPASS_WORKAROUND.md duplicato

**Problema:** Lo stesso file esiste in 2 posti:
- `scantailor/docs/DPI_BYPASS_WORKAROUND.md` (3.7K)
- `~/.claude/telegram-bot/DPI_BYPASS_WORKAROUND.md` (3.7K)

**Files:**
- Keep: `C:\Users\Pass\Documents\SCANTAILOR\scantailor\docs\DPI_BYPASS_WORKAROUND.md`
- Delete: `C:\Users\Pass\.claude\telegram-bot\DPI_BYPASS_WORKAROUND.md`

- [ ] **Step 1: Confrontare** i due file per verificare che siano identici.

- [ ] **Step 2: Eliminare il duplicato** da `~/.claude/telegram-bot/` (la copia in `scantailor/docs/` è quella canonica, tracciata dal repo).

---

### Task 5: Annotare il piano Potrace abbandonato

**Problema:** `docs/superpowers/plans/2026-03-17-potrace-vectorized-pdf.md` (35K) descrive un approccio abbandonato. Potrebbe confondere sessioni future.

**Files:**
- Modify: `C:\Users\Pass\Documents\SCANTAILOR\scantailor\docs\superpowers\plans\2026-03-17-potrace-vectorized-pdf.md`

- [ ] **Step 1: Aggiungere header ABANDONED**

Inserire in cima al file:
```markdown
> **⚠️ ABANDONED** — This plan was rejected. Potrace vectorization produced poor results.
> The chosen approach is JPEG+OCR via Tesseract (see memory `project_vectorized_pdf`).
> Kept for historical reference only.
```

---

### Task 6: Pulire directory Plans/ vuota in new26

**Problema:** `scantailor-new26/Plans/` (maiuscolo) esiste ed è vuota.

**Files:**
- Delete: `C:\Users\Pass\Documents\SCANTAILOR\scantailor-new26\Plans\` (directory vuota)

- [ ] **Step 1: Verificare** che sia effettivamente vuota.
- [ ] **Step 2: Rimuovere** la directory.

---

## Chunk 3: File non tracciati e bot Telegram

### Task 7: Tracciare claude-telegram-bridge/ e docs/ nel repo git

**Problema:** `git status` mostra `claude-telegram-bridge/` e `docs/` come UNTRACKED. Questi contengono:
- Il bot Telegram completo (12 feature)
- Documentazione (DPI workaround, istruzioni firewall)
- Piano Potrace (storico)

**Files:**
- Modify: `C:\Users\Pass\Documents\SCANTAILOR\scantailor\.gitignore`

- [ ] **Step 1: Verificare .gitignore** — controllare se c'è qualcosa che blocca `docs/` o `claude-telegram-bridge/`.

- [ ] **Step 2: Verificare che config.env sia in claude-telegram-bridge/.gitignore**

Il file `claude-telegram-bridge/.gitignore` deve contenere:
```
config.env
venv/
bridge.db
bot_output.log
__pycache__/
```

- [ ] **Step 3: Decidere con Pascal** — questi file vanno committati nel branch corrente (`claude/fix-scantailor-dll-errors-eKZ1a`) o in un branch separato? O il bot merita il suo repo? (Vedi Task 8)

---

### Task 8: Decisione — Bot Telegram: stesso repo o repo separato?

**Problema:** Il bot Telegram è un progetto Python indipendente che vive dentro un repo C++/Qt5. Pascal ha indicato che necessiterebbe del suo proprio repo.

**Opzioni:**

| Opzione | Pro | Contro |
|---------|-----|--------|
| **A: Nuovo repo** `pascal-baobab/claude-telegram-bridge` | Pulito, CI indipendente, riusabile su altri progetti | Richiede setup repo, duplica config.env |
| **B: Resta in scantailor/** come subdirectory | Già lì, hooks già configurati | Inquina repo C++, git history mista |
| **C: Git submodule** | Best of both worlds | Complessità submodule |

**Raccomandazione:** Opzione A — il bot è generico (funziona con qualsiasi progetto Claude Code) e ha la sua dependency chain Python. Un repo separato è la scelta giusta.

- [ ] **Step 1: Decidere con Pascal** quale opzione.

- [ ] **Step 2 (se Opzione A):** Creare repo, spostare file, aggiornare path nei settings.

- [ ] **Step 3 (se Opzione A):** Creare CLAUDE.md per il nuovo repo:

```markdown
# Claude Telegram Bridge — Claude Code Instructions

## What is this project?
Bidirectional Telegram bot for controlling Claude Code remotely.
Handles: approval/deny, notifications, tool monitoring, git operations, prompt injection.

## Setup
1. Copy `config.env.example` → `config.env`, add bot token + chat_id
2. `pip install -r requirements.txt`
3. Run: `python telegram_bot_service.py` or double-click `start_bot.bat`

## Architecture
- `telegram_bot_service.py` — aiohttp server on 127.0.0.1:8765 + Telegram bot
- Hooks: PermissionRequest, Stop, PostToolUse, Notification, SessionStart
- SQLite: bridge.db (sessions, events, auto-approve rules, shortcuts)

## DPI Bypass
MizarFW blocks api.telegram.org via SNI inspection.
Workaround: connect to IP 149.154.167.220 without SNI, add Host header, disable SSL verify.
See `DPI_BYPASS_WORKAROUND.md` for details.

## 12 Features
0. Message chunking  1. PostToolUse monitoring  2. Quick reply buttons
3. SQLite persistence  4. Auto-approve rules  5. Transcript history
6. Git integration  7. Cost tracking  8. Error alerting
9. Multi-project  10. Prompt queue  11. Voice-to-instructions (optional)
```

---

### Task 9: Pulire vecchio bot in ~/.claude/telegram-bot/

**Problema:** `~/.claude/telegram-bot/` contiene il vecchio bot obsoleto (come confermato dalla memory `project_telegram_bot`: "Vecchio bot globale obsoleto, sostituito da per-progetto").

**Files:**
- Verify: `C:\Users\Pass\.claude\telegram-bot\telegram_bot_service.py` (confrontare con versione in `claude-telegram-bridge/`)
- Delete: intera directory `C:\Users\Pass\.claude\telegram-bot\` (dopo conferma)

- [ ] **Step 1: Confrontare** le due versioni del bot per confermare che `claude-telegram-bridge/` è la versione aggiornata.

- [ ] **Step 2: Chiedere conferma a Pascal** prima di eliminare `~/.claude/telegram-bot/`.

- [ ] **Step 3: Se confermato**, eliminare la directory (mantenere solo `claude-telegram-bridge/`).

- [ ] **Step 4: Aggiornare settings.json** — rimuovere `C:\Users\Pass\.claude\telegram-bot` da `additionalDirectories` se il bot viene spostato.

---

## Chunk 4: Correzioni minori

### Task 10: Aggiornare memory scantailor/ per riflettere stato attuale

**Problema:** Alcune memorie potrebbero essere obsolete dopo le operazioni di questo piano.

**Files:**
- Modify: `C:\Users\Pass\.claude\projects\c--Users-Pass-Documents-SCANTAILOR-scantailor\memory\project_telegram_bot.md`

- [ ] **Step 1: Dopo Task 8**, aggiornare il path del bot nella memory (se spostato in repo separato).

- [ ] **Step 2: Dopo Task 9**, rimuovere il riferimento al "vecchio bot globale" nella memory.

---

## Riepilogo

| Task | Problema | Priorità | Indipendente? |
|------|----------|----------|---------------|
| 1 | Memory vuota root | Alta | Si |
| 2 | CLAUDE.md mancante root | Alta | Si |
| 3 | CLAUDE.md incoerenti worktree | Media | Si |
| 4 | DPI doc duplicato | Bassa | Si |
| 5 | Piano Potrace non annotato | Bassa | Si |
| 6 | Plans/ vuota in new26 | Bassa | Si |
| 7 | File untracked in git | Alta | Dipende da Task 8 |
| 8 | Bot Telegram: repo separato? | Alta | **Richiede decisione Pascal** |
| 9 | Vecchio bot obsoleto | Media | Dipende da Task 8 |
| 10 | Aggiornare memory | Bassa | Dipende da Task 8-9 |

**Ordine consigliato:**
1. Task 1 + 2 + 3 (parallelo — memory e CLAUDE.md)
2. Task 4 + 5 + 6 (parallelo — pulizia duplicati)
3. Task 8 → decisione Pascal
4. Task 7 + 9 (dopo decisione)
5. Task 10 (aggiornamento memory finale)
