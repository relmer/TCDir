# Win32 Surface Inventory — the PAL / shim contract

**Feature**: `010-cross-platform-pal` · **Issue**: #8 · **Created**: 2026-06-24
**Status**: Draft (analysis artifact — no code changes)

## Purpose

This is the inventory of every Win32 **type**, **API**, **struct field**, and
**constant** that `TCDirCore` depends on. It is the *contract* that the Platform
Abstraction Layer (PAL) seam is expressed in, and the precise surface a Linux
backend must satisfy.

Design premise (decided in design discussion, see issue #8 thread):

- `TCDirCore` stays **wide** (`wchar_t`/`std::wstring`) and **Win32-typed**; its
  logic is not rewritten.
- A **PAL seam** funnels TCDirCore's OS calls through abstract operations.
- The **Windows backend** is a near-empty passthrough (native Win32, zero
  conversion) — the shipping product is behaviourally unchanged.
- The **Linux backend** does the translation: UTF-8↔UTF-32 strings, a Win32
  **type shim** (subset of `windows.h`), and the semantic `lstat`→`WIN32_FIND_DATAW`
  mapping. Windows-only subsystems are `cfg`-gated/stubbed.

### Disposition legend

Two **orthogonal** axes. The first is the question "does this feature exist on
Linux?"; the second "does it need a Win32 type defined?" A seam can be both
(e.g. enumeration is PASS *and* SHIM).

| Tag | Axis | Meaning |
|-----|------|---------|
| **PASS** | feature present | Operation needed on **both** platforms. Windows backend forwards to the native Win32 call (passthrough); Linux backend reimplements with POSIX. |
| **GATE** | feature absent | Windows-only feature; compiled out / stubbed on Linux (no-op or "unsupported"). The opposite of PASS. |
| **SHIM** | type need | Requires a Win32 *type/struct* defined on Linux (subset `windows.h`) + data mapping. Orthogonal to PASS/GATE; usually co-occurs with PASS. |
| **PORT** | already done | Already portable (std C++ / CRT); no backend work. |

---

## 1. Summary — subsystems & seams

| # | Subsystem | Disposition | Primary files | Seam |
|---|-----------|-------------|---------------|------|
| 1 | Directory enumeration | PASS + SHIM | MultiThreadedLister, DirectoryLister, MaskGrouper | `IFileEnumerator` |
| 2 | Entry data model (`WIN32_FIND_DATAW`) | SHIM | DirectoryInfo.h (FileInfo), all displayers, FileComparator | the struct contract |
| 3 | File times | PASS + SHIM | FileComparator, ResultsDisplayerNormal | part of entry model |
| 4 | Console output | PASS | Console, TuiWidgets, Ehm, Usage, TCDir | `IConsole` |
| 5 | Console input / TUI | PASS | TuiWidgets | `IConsoleInput` |
| 6 | Owner (SID) | PASS | ResultsDisplayerNormal | `IOwnerResolver` |
| 7 | Drive / volume info | PASS | DriveInfo | `IVolumeInfo` |
| 8 | Reparse points / symlinks | PASS + SHIM | ReparsePointResolver, Config | `ILinkResolver` |
| 9 | Environment / known folders | PASS | EnvironmentProvider, ProfilePathResolver, Usage | `IEnvironment` |
| 10 | Parent-shell detection | PASS | ProfilePathResolver | `IProcessInfo` |
| 11 | NTFS alternate data streams | GATE | DirectoryLister | gated |
| 12 | Cloud (OneDrive) status | GATE (Linux) / future (macOS) | ResultsDisplayerWithHeaderAndFooter, ResultsDisplayerNormal | gated |
| 13 | Nerd-Font detection (GDI) | GATE | NerdFontDetector | gated |
| 14 | Nerd-Font install (registry/fonts) | GATE | NerdFontRegistrar, NerdFontSystemAccess, NerdFontInstaller, NerdFontPackage | gated |
| 15 | Windows Terminal settings | GATE | WindowsTerminalSettings | gated |
| 16 | PowerShell aliases | GATE | AliasManager, AliasBlockGenerator | gated |
| 17 | Path composition (`PathCch*`) | PASS/PORT | NerdFont*, WindowsTerminalSettings | prefer `std::filesystem` |
| 18 | String transcode | PORT | Console, ConfigFileReader, ProfileFileManager, NerdFontPackage | std codecvt |
| 19 | Threading / work queue | PORT | MultiThreadedLister, WorkQueue | none (std) |
| 20 | Error handling (EHM/HRESULT) | SHIM | Ehm.h/.cpp (everywhere) | HRESULT shim |

---

## 2. Entry data model — `WIN32_FIND_DATAW` (the core SHIM)

`FileInfo : public WIN32_FIND_DATAW` (DirectoryInfo.h:43). The struct flows
through the comparator and all four displayers, which read its fields directly.
The Linux backend must populate a shimmed `WIN32_FIND_DATAW` from `readdir`/`lstat`.

### Fields actually consumed

| Field | Read at | POSIX source |
|-------|---------|--------------|
| `dwFileAttributes` | FileComparator, all displayers, Config, MaskGrouper, MultiThreadedLister, DirectoryLister | derived from `st_mode` + dotfile convention (see §6) |
| `ftCreationTime` | FileComparator:158, ResultsDisplayerNormal:263 | `st_ctim`/`statx` btime if available, else mtime |
| `ftLastAccessTime` | FileComparator:163, ResultsDisplayerNormal:266 | `st_atim` |
| `ftLastWriteTime` | FileComparator:169, ResultsDisplayerNormal:226/270 | `st_mtim` |
| `nFileSizeHigh` / `nFileSizeLow` | FileComparator:218-222, DirectoryLister:497, ResultsDisplayerNormal:326 | `st_size` split hi/lo |
| `cFileName` (`WCHAR[260]`) | comparator, all displayers, lister dedup, Config, ReparsePointResolver | `d_name` (UTF-8) → UTF-32 transcode |
| `dwReserved0` (reparse tag) | Config:1955/2036, ReparsePointResolver:265 | symlink → `IO_REPARSE_TAG_SYMLINK`; else 0 |
| `cAlternateFileName` | — (not read) | leave empty |

### Recommended design — parameterize the entry over its native storage

Rather than forcing POSIX data into a fake `WIN32_FIND_DATAW` (storing converted
FILETIME ticks etc.), **parameterize `FileInfo` over its native storage and read
through uniform accessors.** TCDirCore's ~40 direct field reads
(`.dwFileAttributes`, `.ftLastWriteTime`, `.cFileName`, …) are replaced by
accessor calls; the native blob stays native on each platform (zero conversion).

```cpp
#if defined(_WIN32)
    using NativeEntry = WIN32_FIND_DATAW;        // name+attrs+times+size in one
#else
    struct NativeEntry { wstring name; struct stat st; uint32_t linkTag; };
#endif

struct FileInfo : NativeEntry {                  // (or template<TNative>)
    bool          IsDirectory() const;
    bool          IsSymlink()   const;
    uint64_t      Size()        const;
    int64_t       LastWriteNs() const;           // portable comparable time
    const wchar_t* Name()       const;           // UTF-16 (Win) / UTF-32 (Linux)
};
```

- **Windows**: accessors read `WIN32_FIND_DATAW` fields — free; `FILETIME` stays native.
- **Linux**: accessors read `struct stat`/`timespec` — free; epoch/encoding handled
  lazily in the accessor, not baked into stored data.
- **FILETIME** follows the same rule: store native (`FILETIME` vs `timespec`),
  compare via `LastWriteNs()` instead of `CompareFileTime`.

**Cost**: rewrite the ~40 field-access sites to accessors (mechanical; also makes
the comparator/displayers encoding- and epoch-agnostic).

**Asymmetry to record**:
1. Windows returns name+attrs+times+size from **one** `FindFirstFile`; POSIX needs
   `readdir` (name + `d_type`) **plus a per-entry `lstat`** for size/times. That is
   a real Linux per-entry cost (`d_type` covers only the dir/symlink check). Given
   the listing path's allocation sensitivity, batch/stat carefully.
2. "POSIX" (not just Linux): `readdir`/`lstat`/`readlink` also cover macOS/BSD.

### Accessor conversion reference (Linux)

- **Time**: Unix `timespec` (since 1970) → portable ns, or to Win ticks if ever
  needed: `ticks = (sec + 11644473600) * 10'000'000 + nsec/100` (Win epoch 1601, 100 ns).
- **Name width**: Linux `wchar_t` is **32-bit** → `Name()` yields **UTF-32**;
  boundary transcode is UTF-8 (`d_name`) ↔ UTF-32, lossless.
- **Size**: `Size()` returns `st_size` directly (no hi/lo split needed once the
  comparator/displayers use the accessor instead of `nFileSize{High,Low}`).

---

## 3. Win32 types the shim must define (Linux)

Minimal `windows.h` subset (a purpose-built header, additive, Linux-only build):

- **Scalars**: `DWORD`, `WORD`, `BYTE`, `BOOL`, `WCHAR`, `LPWSTR`/`LPCWSTR`,
  `HANDLE`, `HKEY`, `LONG`, `LPVOID`, `ULONGLONG`.
- **Structs**: `WIN32_FIND_DATAW`, `FILETIME`, `SYSTEMTIME`, `LARGE_INTEGER`/
  `ULARGE_INTEGER` (used for size hi/lo math in FileComparator/DirectoryLister).
- **Sentinels/constants**: `INVALID_HANDLE_VALUE`, `MAX_PATH` (260),
  `TRUE`/`FALSE`, `ERROR_SUCCESS`.
- **`HRESULT` + macros**: `S_OK`, `S_FALSE`, `E_*`, `FAILED()`, `SUCCEEDED()`,
  `HRESULT_FROM_WIN32` — consumed pervasively via EHM (Ehm.h). The richest shim
  item after the find-data struct.
- **`FILE_ATTRIBUTE_*`** and **`IO_REPARSE_TAG_SYMLINK`** — see §6.

---

## 4. API surface by subsystem

### 1. Directory enumeration — PASS (+SHIM data)
- `FindFirstFileW` / `FindNextFileW` / `FindClose` — MultiThreadedLister.cpp,
  DirectoryLister.cpp, MaskGrouper.cpp:110.
- `GetFileAttributesW` (probe), `CreateFileW` (config/WT read-write — see §17).
- Linux: `opendir`/`readdir`/`closedir` + `lstat`, populating the §2 struct.
  The OS does the file-mask match today (spec passed to `FindFirstFile`); on
  Linux the backend must apply the glob itself (`fnmatch`) — **behavioural note**.

### 2/3. Entry model + times — SHIM / PASS
- `CompareFileTime` (FileComparator:174) → integer compare of the 64-bit tick value.
- `FileTimeToSystemTime` (ResultsDisplayerNormal:226) → `localtime_r`/`gmtime_r`
  for display.

### 4. Console output — PASS
- `GetStdHandle(STD_OUTPUT_HANDLE/STD_ERROR_HANDLE)` — Console, Ehm, TCDir, Usage, TuiWidgets.
- `GetConsoleMode`/`SetConsoleMode` (enable VT — `ENABLE_VIRTUAL_TERMINAL_PROCESSING`) — Console.cpp:107-114, TCDir.cpp.
- `GetConsoleScreenBufferInfo` (terminal width) — Console.cpp:143.
- `WriteConsoleW` (interactive) / `WriteFile` (redirected, already UTF-8) — Console.cpp:606/641, TuiWidgets.cpp:908/951.
- Linux: width via `ioctl(TIOCGWINSZ)`; redirect detection via `isatty`; VT is
  on by default; output is UTF-8 to `STDOUT_FILENO` (TCDirCore is UTF-32 → transcode at write).

### 5. Console input / TUI — PASS
- `GetConsoleMode`/`SetConsoleMode` (raw input: `ENABLE_EXTENDED_FLAGS |
  ENABLE_WINDOW_INPUT`), `FlushConsoleInputBuffer` — TuiWidgets.cpp:90-139.
- Linux: `termios` raw mode (`tcgetattr`/`tcsetattr`, `VMIN`/`VTIME`).

### 6. Owner (SID) — PASS
- `GetNamedSecurityInfoW(SE_FILE_OBJECT, OWNER_SECURITY_INFORMATION)` +
  `LookupAccountSidW` — ResultsDisplayerNormal.cpp:590-597.
- Linux: `st_uid`/`st_gid` → `getpwuid_r`/`getgrgid_r`.

### 7. Drive / volume info — PASS
- `GetDriveType`, `GetVolumeInformation` (label/serial/fs), `WNetGetConnection`
  (UNC mapping) — DriveInfo.cpp:50-106.
- Linux: `statvfs` + mount table (`/proc/mounts`); no drive letters — the
  "Volume in drive X" header degrades to mountpoint/fs-type, or is suppressed.

### 8. Reparse points / symlinks — PASS (+SHIM tag)
- `CreateFileW(FILE_FLAG_OPEN_REPARSE_POINT)` + `DeviceIoControl
  (FSCTL_GET_REPARSE_POINT)` — ReparsePointResolver.cpp:283-299.
- `dwReserved0` reparse tag (`IO_REPARSE_TAG_SYMLINK`) — Config, ReparsePointResolver:265.
- Linux: `readlink`; symlink ⇒ set reparse attr + symlink tag in the shimmed struct.

### 9. Environment / known folders — PASS
- `GetEnvironmentVariableW` (`TCDIR`, `LOCALAPPDATA`) — EnvironmentProvider.cpp:24-35, Usage, NerdFontInstaller, WindowsTerminalSettings.
- `SHGetKnownFolderPath(FOLDERID_Documents)` — ProfilePathResolver.cpp:304.
- `GetModuleFileNameW` (own exe path) — AliasManager.cpp:72, NerdFontInstaller.cpp:938.
- Linux: `getenv`; `$XDG_CONFIG_HOME`/`~/.config`, `~`; `/proc/self/exe`.

### 10. Parent-shell detection — PASS
- `CreateToolhelp32Snapshot` + `Process32FirstW`/`Process32NextW` —
  ProfilePathResolver.cpp:27-32 (walk to identify the host shell).
- `OpenProcessToken`+`GetTokenInformation(TokenElevation)` (am-I-elevated) — ProfilePathResolver.cpp:362-365.
- Linux: `/proc/<ppid>/{stat,comm,exe}`; elevation via `geteuid()==0`.

### 11. NTFS streams — GATE
- `FindFirstStreamW`/`FindNextStreamW` — DirectoryLister.cpp:546-588. No POSIX
  equivalent → `/streams` becomes a no-op / "unsupported" on Linux.

### 12. Cloud status — GATE on Linux; possible provider on macOS
- `CfGetPlaceholderStateFromFindData` (cfapi) — ResultsDisplayerNormal.cpp:549.
- Cloud attribute bits (`PINNED`/`UNPINNED`/`RECALL_ON_*`/`OFFLINE`) —
  ResultsDisplayerWithHeaderAndFooter.cpp:549-561. Bits never set off-Windows.
- **Placeholder support is per-OS, not "Windows-only":** each OS ships its own
  framework.
  - **Windows**: Cloud Filter API (`cfapi`) + the standardized attribute bits above.
  - **macOS**: the **File Provider** framework. A not-yet-downloaded ("dataless")
    file exposes a **stat-level signal** — `st_flags & SF_DATALESS` (via
    `lstat`/`getattrlist`) — so a macOS provider is *tractable* and fits the entry
    model (a flag check, not a heavy framework).
  - **Linux**: **no standardized substrate** in the VFS. Providers full-sync or use
    FUSE mounts (Dropbox's online-only is FUSE, not a standard attr); Box has no
    official Linux desktop client. Only fragile per-provider heuristics are possible.
- Disposition: **GATE on Linux** (no substrate); **future provider on macOS** via
  `SF_DATALESS`. Do not assume "POSIX ⇒ no cloud status".

### 13. Nerd-Font detection (GDI) — GATE
- `CreateFontW`, `EnumFontFamiliesExW` — NerdFontDetector.cpp:141-258.
- Linux: terminal-capability probing or `fontconfig` (future provider), else assume-on/flag.

### 14. Nerd-Font install — GATE
- Registry: `RegCreateKeyExW`/`RegOpenKeyExW`/`RegQueryValueExW`/`RegSetValueExW`/
  `RegEnumValueW`/`RegDeleteValueW`/`RegCloseKey` — NerdFontRegistrar.cpp, NerdFontSystemAccess.h.
- Fonts: `AddFontResourceW`/`RemoveFontResourceW` — NerdFontRegistrar.cpp:300/412.
- Elevation/download: `ShellExecuteExW`, `WaitForSingleObject`, `InternetOpenW`,
  `URLDownloadToFileW` — NerdFontInstaller.cpp, NerdFontPackage.cpp.
- Entire `/install-nerdfonts` flow is Windows-only. Gate; a Linux story (copy to
  `~/.local/share/fonts` + `fc-cache`) is a separate feature.

### 15/16. Windows Terminal settings + PS aliases — GATE (Linux analogs differ)
- **WT settings**: surgical JSON edit of WT `settings.json`
  (`%LOCALAPPDATA%\Packages\…\LocalState\`), setting `profiles.defaults.font.face`
  (modern) or legacy `fontFace` to the Nerd Font, preserving the rest
  (WindowsTerminalSettings.cpp:234-247). **No single Linux equivalent**: terminal
  config is fragmented — gnome-terminal (dconf), Konsole (profile files), Alacritty
  (`alacritty.toml`), kitty (`kitty.conf`), WezTerm (`wezterm.lua`), xterm (X
  resources). A Linux story = per-terminal providers (own feature). Gate.
- **PS aliases**: a managed, `DO NOT EDIT`-delimited block of PowerShell `function`
  wrappers written into `$PROFILE`, re-runnable (AliasBlockGenerator.cpp:29-87).
  **The pattern ports** ("managed block in an rc file") but is **per-shell**:
  bash/zsh → `alias`/functions in `~/.bashrc`/`~/.zshrc`; fish →
  `~/.config/fish/functions/`. Cheap first cut: **pwsh runs on Linux**, so the
  existing generator could target PowerShell-on-Linux nearly unchanged. Gate for
  now; revisit as a per-shell feature.

### 17. Path composition — PASS / prefer PORT
- `PathCchAppend`/`PathCchCombine`/`PathCchRemoveFileSpec`/… — NerdFont*,
  WindowsTerminalSettings (all in GATE'd subsystems). General path work elsewhere
  already uses `std::filesystem::path` (portable). **No PathCch in the hot listing path.**

### 18. String transcode — PORT
- `MultiByteToWideChar`/`WideCharToMultiByte(CP_UTF8)` — Console.cpp:621-632,
  ConfigFileReader.cpp:126-137, ProfileFileManager.cpp, NerdFontPackage.cpp.
- These are the existing UTF-8↔UTF-16 seams; the Linux backend generalizes them
  to UTF-8↔UTF-32 (std `codecvt`/`c32rtomb` or a small helper). **Centralize**
  into one transcode utility used by both backends.

### 19. Threading — PORT (no work)
- `std::jthread` (hardware_concurrency, auto-join), `std::mutex`, `std::unique_lock`,
  `std::condition_variable`, `std::atomic` — MultiThreadedLister, WorkQueue.h.
  Fully portable; **no `CreateThread`/Win32 sync anywhere.**

### 20. Error handling — SHIM
- The EHM macro family + `HRESULT`/`S_OK`/`FAILED()` are used in essentially every
  `.cpp`. Ehm.h must compile on Linux: provide the `HRESULT` typedef, the codes,
  and `HRESULT_FROM_WIN32`/`GetLastError` shim (`errno`-based).

---

## 5. FILE_ATTRIBUTE_* constants used (+ POSIX mapping)

| Constant | Used by | Linux mapping |
|----------|---------|---------------|
| `DIRECTORY` | comparator, all displayers, listers | `S_ISDIR(st_mode)` |
| `REPARSE_POINT` | Config, ReparsePointResolver, MTLister | `S_ISLNK` |
| `READONLY` | FileAttributeMap, Config, Usage | no owner write bit (`!(st_mode & S_IWUSR)`) |
| `HIDDEN` | FileAttributeMap, Config, Usage, CommandLine | dotfile convention (`name[0]=='.'`) |
| `SYSTEM` | FileAttributeMap, IconMapping, Usage | — (never set) |
| `ARCHIVE`,`TEMPORARY`,`ENCRYPTED`,`COMPRESSED`,`SPARSE_FILE` | attr maps, Usage, /A filter | — (never set; some have rough analogues, defer) |
| `NORMAL` | CreateFileW calls (GATE'd) | n/a |
| `OFFLINE`,`PINNED`,`UNPINNED`,`RECALL_ON_OPEN`,`RECALL_ON_DATA_ACCESS` | cloud header (GATE) | never set |
| `NOT_CONTENT_INDEXED`,`INTEGRITY_STREAM`,`NO_SCRUB_DATA` | /A filter table (CommandLine) | never set |

The attribute *bit values* must exist in the shim (the `/A` filter and attribute
column reference them by name); Linux simply never sets the unsupported ones.

---

## 6. What this buys us (sizing)

- **Hot listing path is small & PASS-able**: enumeration (3 files) + the
  `WIN32_FIND_DATAW` SHIM + console (1-2 files). A bare Linux listing needs only
  seams 1-4 + the entry model + HRESULT/transcode plumbing.
- **~7 subsystems are GATE** (streams, cloud, GDI detect, font install, WT
  settings, aliases) — compiled out, not ported.
- **Threading and the work queue are free** (already std).
- **Transcode seams already exist** — generalize, don't invent.

So the Linux MVP (Phase 1: bare colorized listing) is bounded by seams **1, 2,
3, 4, 9, 20** — the rest is either GATE or later phases.

---

## 7. Open questions

1. **Entry metadata: shim vs. accessor** (the central design fork). How does
   TCDirCore read is-dir / size / mtime / name portably?
   - **A — Win32 type shim**: leave the ~40 raw field reads
     (`wfd.dwFileAttributes`, `wfd.ftLastWriteTime`, `CompareFileTime`, `lstrcmpiW`)
     and define a fake `WIN32_FIND_DATAW`/`FILETIME`/constants on Linux, filling the
     fake struct from `lstat`. Pro: ~zero TCDirCore edits, Windows untouched. Con:
     emulates Win32 semantics, **eager per-entry conversion** (timespec→FILETIME,
     hi/lo size, name transcode) in the hot path, must reimplement `CompareFileTime`
     and a UTF-32 `lstrcmpiW`.
   - **B — accessors**: replace the ~40 reads with `IsDirectory()`/`Size()`/
     `LastWriteNs()`/`Name()` over native per-platform storage (no fake struct).
     Pro: zero-conversion native storage, **lazy** (e.g. `IsDirectory()` uses
     `d_type` only), epoch/encoding-agnostic, one portable name-fold helper. Con:
     ~40 mechanical edits mirrored into RCDir; low-risk re-test of the Windows path.
   - Two tips toward **B**: (i) A's eager fill is precisely the per-entry hot-path
     work issue #12 just minimized; (ii) B makes TCDir read metadata via methods —
     *converging* with idiomatic RCDir rather than diverging.
   - **Leaning: hybrid** — accessors (B) for entry metadata (time/size/attr/name);
     thin type-shim (A) for incidental surface (`HRESULT`, `FILE_ATTRIBUTE_*` bit
     values, `DWORD`).
2. **Seam style for the OS *calls*** (separate from #1) — keep call sites calling
   `FindFirstFileW`, or refactor enum/console/owner to `IFileEnumerator`/`IConsole`
   interfaces? Likely interfaces for the few hot seams, shim for the rest.
3. **File-mask matching** moves in-process on Linux (`fnmatch`) — confirm glob
   semantics match Win32 (`*`/`?`, case-insensitivity policy).
4. **Volume header** on Linux — suppress, or show mountpoint/fs-type?
5. **`wchar_t` 32-bit audit** — enumerate any spot assuming 16-bit/surrogates
   (display-width math is the main candidate; UTF-32 is arguably more correct).
6. **Build system** — CMake alongside the `.vcxproj`, or migrate? (separate spec).
7. **Birth time** — `statx(STATX_BTIME)` availability floor on target distros.
8. **GATE'd convenience features stay out of the core port** — terminal-font
   auto-config (per-emulator) and shell aliases (per-shell: bash/zsh/fish/pwsh/…)
   are future per-backend features, **not** Phase 1. macOS cloud status
   (`SF_DATALESS`) is likewise a later provider, not core.
