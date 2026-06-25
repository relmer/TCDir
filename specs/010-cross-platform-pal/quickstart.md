# Quickstart — Cross-Platform Build & Smoke Test

How to build and verify TCDir on each platform. The Windows build is unchanged;
the POSIX build is new.

## Windows (unchanged)

```powershell
# Build (Debug x64) and run tests — exactly as today
.\scripts\Build.ps1 -Configuration Debug -Platform x64
# 684/684 unit tests must still pass; behavior byte-identical (FR-014).
.\x64\Debug\tcDir.exe C:\Windows\System32
```

The Windows PAL backend is a passthrough; no observable change.

## Linux (new — CMake)

```bash
# Configure + build the POSIX backend
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j

# Smoke: colorized listing of a directory
./build/tcdir /usr/bin

# Display modes
./build/tcdir            # default
./build/tcdir -b         # bare
./build/tcdir -w         # wide
./build/tcdir --tree     # tree
```

**What to verify (maps to spec User Stories / SC):**
- Names, sizes, dates, colors, icons render; all four modes work (US1, SC-002).
- Owner shows user/group; symlinks marked with targets; dotfiles hidden unless
  shown (US2).
- `tcdir '*.cpp'` (quoted) → TCDir matches; `tcdir *.cpp` (unquoted) → shell expands,
  TCDir lists the given files (D8/hybrid).
- A case-sensitive directory lists both `Foo` and `foo`; sort is case-insensitive,
  matching is case-sensitive (D8).
- Cloud column absent; volume header absent by default; `--volume-header` (opt-in)
  shows mountpoint + fs type (FR-018/019).
- Requesting a Windows-only feature (e.g. `--streams`) prints a clear "unavailable
  on this platform" message, no crash (FR-011, SC-005).

## macOS (after Linux)

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug && cmake --build build -j
./build/tcdir ~/Documents
```

Same as Linux, plus: files in a cloud folder that are online-only show as such
(via `st_flags & SF_DATALESS`) (US3).

## Tests

```bash
# POSIX: portable runner over PAL-mocked logic (no real filesystem)
ctest --test-dir build --output-on-failure
```

```powershell
# Windows: existing MS C++ Unit Test Framework
& 'C:\Program Files\Microsoft Visual Studio\18\Enterprise\Common7\IDE\Extensions\TestPlatform\vstest.console.exe' .\x64\Debug\UnitTest.dll
```

Logic tests run against a **Mock PAL backend** returning synthetic `FileEntry`
vectors — deterministic, no disk/registry/process access (constitution Test
Isolation).

## Definition of done (MVP / Phase = User Story 1)

- Linux colorized listing in all four modes, parity with Windows for shared columns
  (SC-001/002).
- Windows build + 684 tests unchanged (SC-003).
- Large-directory listing within ~1.5× the Windows build; hot path free of
  per-entry PAL callbacks (SC-007, D4).
