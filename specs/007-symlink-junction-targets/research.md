# Research: Symlink & Junction Target Display

**Date**: 2026-04-18
**Feature**: 007-symlink-junction-targets

## R1: Reparse Data Buffer Format

### Decision
Define `REPARSE_DATA_BUFFER` manually in the reparse resolver header. Use `PrintName` for display (clean user path); fall back to `SubstituteName` with `\??\` prefix stripped only if `PrintName` is empty.

### Rationale
- `REPARSE_DATA_BUFFER` is defined in `<ntifs.h>` (WDK kernel header), not available in user-mode `<windows.h>`.
- Defining it manually is standard practice for user-mode tools — Microsoft's own sample code does this.
- `PrintName` is the display-ready path: no `\??\` prefix for junctions, preserves relative paths for symlinks.
- `SubstituteName` is the NT-internal path — only needed as fallback.

### Alternatives Considered
- **Include `<ntifs.h>`**: Requires WDK installation, adds kernel headers to a user-mode project. Rejected — unnecessary dependency.
- **Use `GetFinalPathNameByHandle`**: Resolves to absolute final path (follows chains, loses as-stored relative paths). Rejected — spec requires as-stored display (FR-004).

### Buffer Layout Reference

```
Junction (IO_REPARSE_TAG_MOUNT_POINT = 0xA0000003):
  MountPointReparseBuffer:
    SubstituteNameOffset/Length  →  "\??\C:\Target\Path"
    PrintNameOffset/Length       →  "C:\Target\Path"
    PathBuffer[]                 →  UTF-16LE, both strings

Symlink (IO_REPARSE_TAG_SYMLINK = 0xA000000C):
  SymbolicLinkReparseBuffer:
    SubstituteNameOffset/Length  →  "\??\C:\Target" or "..\relative"
    PrintNameOffset/Length       →  "C:\Target" or "..\relative"
    Flags                        →  0 = absolute, 1 = SYMLINK_FLAG_RELATIVE
    PathBuffer[]                 →  UTF-16LE, both strings

AppExecLink (IO_REPARSE_TAG_APPEXECLINK = 0x8000001B):
  ULONG Version (= 3)
  WCHAR[] StringList:
    String 1: Package ID         (NUL-terminated)
    String 2: App User Model ID  (NUL-terminated)
    String 3: Target Exe Path    (NUL-terminated)  ← display this
```

## R2: Win32 API Call Sequence

### Decision
Use `CreateFileW` with `FILE_FLAG_OPEN_REPARSE_POINT | FILE_FLAG_BACKUP_SEMANTICS` and zero access rights, followed by `DeviceIoControl(FSCTL_GET_REPARSE_POINT)` with a stack-allocated 16KB buffer.

### Rationale
- Zero desired access (`0` for `dwDesiredAccess`) is sufficient — `FSCTL_GET_REPARSE_POINT` doesn't require read access.
- `FILE_FLAG_BACKUP_SEMANTICS` is required for opening directories.
- `FILE_FLAG_OPEN_REPARSE_POINT` opens the link itself, not the target.
- `MAXIMUM_REPARSE_DATA_BUFFER_SIZE` (16384) is defined in `<winnt.h>` (already available via `<windows.h>`).
- Stack allocation avoids heap overhead; 16KB on the stack is safe for a non-recursive function.

### Alternatives Considered
- **Heap allocation**: Unnecessary — 16KB is well within stack limits, and reparse resolution is not recursive.
- **Smaller buffer with retry**: Adds complexity for no benefit — 16KB covers all reparse buffers.
- **`GENERIC_READ` access**: Overly permissive — fails on more permission-restricted entries than `0` access.

### Failure Handling
- `CreateFileW` returns `INVALID_HANDLE_VALUE` → return empty string.
- `DeviceIoControl` fails → close handle, return empty string.
- Unrecognized reparse tag → return empty string.
- All failures are silent per FR-011.

## R3: Integration Point — Where to Resolve

### Decision
Resolve reparse targets in `AddMatchToList()` in `DirectoryLister.cpp`, after `FileInfo` construction from `WIN32_FIND_DATA` and before `push_back`.

### Rationale
- `AddMatchToList()` is the single shared path for both `CDirectoryLister` (single-threaded) and `CMultiThreadedLister` (multi-threaded).
- The directory path is available from `CDirectoryInfo::m_dirPath` to construct the full path for `CreateFileW`.
- Resolution happens per-entry, only when `FILE_ATTRIBUTE_REPARSE_POINT` is set — zero overhead for non-reparse files.
- In the multi-threaded case, resolution happens on the worker thread — no contention since each worker has its own directory.

### Alternatives Considered
- **Resolve at display time**: Would require the displayer to know the directory path and perform I/O during rendering. Violates separation of concerns — displayers should only format data.
- **Resolve in a separate pass**: Unnecessary complexity — inline resolution in `AddMatchToList` is simpler and equally efficient.

## R4: Display Rendering Strategy

### Decision
Split the existing `Printf(textAttr, L"%s\n", cFileName)` into three calls: filename (no newline), arrow with `Information` attribute, target with resolved color, then newline. Only when `m_strReparseTarget` is non-empty.

### Rationale
- Minimal change to existing rendering — one `Printf` becomes a conditional three-part print.
- Arrow color (`Information` attribute) is a resolved `WORD` from the existing config system.
- Target color: for directories (junctions, dir symlinks), use `EAttribute::Directory`; for files (file symlinks, AppExecLinks), look up target extension in `m_mapExtensionToTextAttr`.
- Change applies to both `ResultsDisplayerNormal` and `ResultsDisplayerTree` — same pattern in both.

### Alternatives Considered
- **Build a composite format string**: More complex, harder to color individual segments.
- **Add a virtual `DisplayLinkTarget()` method**: Over-engineering for a two-line change in two files.

## R5: Target Color Resolution for File Extensions

### Decision
Add a `GetTextAttrForExtension(const wstring& path)` helper to `CConfig` that extracts the file extension and looks it up in `m_mapExtensionToTextAttr`. Returns the default text attribute if not found.

### Rationale
- `m_mapExtensionToTextAttr` is already populated with ~150 extension→color mappings.
- Extracting extension from a path is trivial via `filesystem::path::extension()`.
- This helper is useful beyond this feature (general-purpose color lookup by extension).
- The alternative (calling `GetDisplayStyleForFile` with a fake `WIN32_FIND_DATA`) is hacky and couples to struct layout.

### Alternatives Considered
- **Reuse `GetDisplayStyleForFile()`**: Requires constructing a `WIN32_FIND_DATA` with the target filename — wasteful and awkward.
- **Inline the lookup at call sites**: Duplicates logic in Normal and Tree displayers.

## R6: Testability Strategy

### Decision
Separate reparse resolution (Win32 API calls) from buffer parsing (pure functions) and display (Console output). Test each layer independently:
1. **Buffer parsing**: Unit test with synthetic `REPARSE_DATA_BUFFER` byte arrays — pure functions, no mocking needed.
2. **Path prefix stripping**: Unit test the `\??\` strip logic — pure string function.
3. **Display rendering**: Use `CCapturingConsole` to capture output and verify `→ target` appears in the right format.
4. **Resolution integration**: Pre-populate `FileInfo::m_strReparseTarget` in tests — test display without needing file system mocks for `DeviceIoControl`.

### Rationale
- Buffer parsing is the most critical logic to test — and it's pure (no I/O).
- The existing `FileSystemMock` doesn't support `CreateFileW`/`DeviceIoControl` hooking.
- Building a new mock for `DeviceIoControl` is possible but high effort for low value — the Win32 call sequence is trivial.
- Pre-populating `m_strReparseTarget` tests the display path end-to-end without system dependencies.

### Alternatives Considered
- **Mock `CreateFileW` + `DeviceIoControl`**: High effort, fragile IAT hooking. The parsing logic (which is where bugs live) can be tested without this.
- **Interface abstraction for reparse I/O**: Adds an interface and virtual dispatch for a function called 0-5 times per directory. Over-engineering per Principle V.

## R7: AppExecLink Parsing

### Decision
Parse the GenericReparseBuffer as: skip 4-byte version ULONG, then walk three NUL-terminated UTF-16 strings. The third string is the target executable path.

### Rationale
- No public Microsoft-documented structure exists; this layout is reverse-engineered but stable and widely used (Chromium, Git for Windows, and other tools use it).
- The version field has been 3 on all known Windows 10/11 builds.
- Defensive: validate version == 3, validate string bounds within buffer, return empty on any anomaly.

### Alternatives Considered
- **Skip AppExecLink entirely**: These are common in `WindowsApps` — users encounter them. Worth supporting.
- **Show only `[AppExecLink]` without target**: Less useful — the target exe path is the actionable information.
