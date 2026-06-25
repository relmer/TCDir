# Phase 0 Research — Cross-Platform Support

Consolidated decisions for the cross-platform port. Each item: **Decision /
Rationale / Alternatives considered**. Source material: `win32-surface-inventory.md`
and the design discussion captured in the spec's Clarifications.

## D1. Vehicle: port C++ TCDir (keep wide) behind a PAL

- **Decision**: Keep `TCDirCore` wide (`wchar_t`/`wstring`) and platform-agnostic;
  introduce a PAL with a passthrough Windows backend and a new POSIX backend. Do
  **not** convert the core to narrow UTF-8, and do **not** fork RCDir for Linux.
- **Rationale**: Converting the pervasive `wstring`/`L""` path-and-name plumbing to
  UTF-8 `std::string` would touch essentially every file and trade a
  compile-enforced encoding invariant for a manifest-enforced one. Keeping the core
  wide leaves the Windows product byte-identical and quarantines all divergence in
  the POSIX backend. On Linux `wchar_t` is 32-bit, so "wide" = UTF-32; the boundary
  transcode is UTF-8 ↔ UTF-32, lossless, with no ANSI-codepage hazard.
- **Alternatives**: (a) Narrow-UTF-8 rewrite of the core — far larger, re-audits
  every name/width site, risks regressions on the shipping product. (b) Make the
  Rust RCDir cross-platform instead — breaks the TCDir↔RCDir 1:1 mirror that makes
  porting changes tractable. (c) Win32 emulation shim only (fake `windows.h`) —
  forces eager per-entry conversion and bakes Win32-isms into the portable layer.

## D2. Entry model: accessors over native storage (+ thin type-shim)

- **Decision**: Parameterize the entry over its native storage; read metadata
  through uniform accessors. Native = `WIN32_FIND_DATAW` on Windows, a POSIX bundle
  (`{ wstring name; struct stat; uint32_t linkTag; }`) on Linux/macOS. A thin
  type-shim defines only the *incidental* ambient Win32 names needed to compile the
  core off-Windows (`HRESULT`, a few integer typedefs).
- **Rationale**: Zero-conversion native storage on each platform; lazy reads
  (e.g. directory check from `d_type` without an `lstat`); comparator/displayers
  become epoch- and encoding-agnostic. Avoids reimplementing `CompareFileTime`/
  `lstrcmpiW` as Win32-shaped APIs.
- **Alternatives**: Win32 type-shim for the whole entry (eager conversion, Win32-ism
  debt, hot-path cost — the very per-entry work issue #12 minimized).

## D3. File attributes: PAL-owned, name-addressed domain

- **Decision**: `TCDirCore` knows **nothing** about attributes. The PAL owns the
  vocabulary, validation, per-entry classification, `/A` filter parse+match, the
  attribute-**column rendering**, and attribute **help text**. The interchange
  currency is opaque entry tokens, rule-handles, and attribute **names** — never
  `FILE_ATTRIBUTE_*` or `st_mode`. Two exceptions stay in core as structural
  predicates: **IsDirectory** and **IsSymlink** (the PAL *classifies*; the core
  *understands* them because traversal/sort/layout depend on them).
- **Theming**: config is parsed in `TCDirCore`; each attribute-referencing rule is
  validated by the PAL at load (returns a rule-handle) and *matched* by the PAL.
  To keep the hot path callback-free, the PAL attaches a compact **themed-match
  set** (bits over the registered rule-handles) to each entry during the
  enumeration pass; `TCDirCore` resolves color/icon locally from a priority-ordered
  rule list (first match wins) — **no per-entry PAL call**.
- **Rationale**: Attribute sets genuinely differ per platform — Linux has
  executable/setuid/device-type/immutable that Windows lacks; Windows has
  system/archive/cloud that Linux lacks; a few (encrypted/compressed) have rough
  analogues. No Win32 bitmask can represent POSIX-only attributes. PAL-ownership
  lets each platform present its own model (e.g. Linux can render a `drwxr-xr-x`
  mode string while Windows renders `RHSA`).
- **Alternatives**: A portable `FileAttributes` flags enum in the core (still leaks
  a shared attribute vocabulary into core; the enum becomes a PAL *implementation
  detail* instead). A shimmed `dwFileAttributes` DWORD (can't model POSIX-only
  attributes).

## D4. Enumeration boundary: per-directory, bulk

- **Decision**: The PAL's enumeration entry point is **per-directory** (and a
  per-explicit-path-list variant for shell-expanded input). It returns/fills a
  `vector<FileEntry>` of fully classified, enriched entries in one call — the OS
  enumeration loop (`FindFirstFile`/`readdir`+`lstat`) lives *inside* the backend.
- **Rationale**: One boundary crossing per directory (not per file); each backend
  runs its tightest native loop with platform-optimal tricks (Linux `d_type` to
  skip `lstat`, batched `statx`; Windows `FindFirstFileEx`/`FindExInfoBasic`).
  Aligns with the existing per-directory threading/work-queue model. Preserves the
  #12 allocation profile (`m_vMatches` filled once; reserve + move-once sort kept).
  Symlink-target resolution and `/A` visibility fold into the same pass.
- **Alternatives**: Per-file PAL calls (`NextEntry()`) — N virtual crossings per
  directory, re-pays the per-entry cost, blocks backend-native batching.

## D5. Console & TUI: IConsole / termios

- **Decision**: An `IConsole` seam. Windows backend uses `WriteConsoleW` (interactive)
  / `WriteFile` (redirected, existing UTF-8 path); POSIX writes UTF-8 to
  `STDOUT_FILENO`, width via `ioctl(TIOCGWINSZ)`, redirect via `isatty`, raw input
  via `termios`. VT/ANSI is already the color path and is on by default on POSIX.
- **Rationale**: Centralizes the few console call sites; honors the constitution's
  "prefer `WriteConsoleW`" on Windows while enabling POSIX output.
- **Alternatives**: Narrow stdio everywhere (loses Windows console reliability).

## D6. Windows-only subsystems: gate

- **Decision**: Gate (compile out / stub with a clear "unavailable on this platform"
  message) NTFS alternate streams, cloud status (Windows cfapi), GDI Nerd-Font
  detection, Nerd-Font install (registry/fonts), Windows Terminal settings, and the
  PowerShell alias generator. macOS cloud status (`st_flags & SF_DATALESS`) and the
  per-terminal/per-shell convenience features are **post-MVP** providers.
- **Rationale**: These have no portable substrate (FR-011). macOS *does* expose a
  stat-level cloud signal, so macOS cloud is a later provider, not a dead end.
- **Alternatives**: Per-provider Linux heuristics now (fragile; out of scope).

## D7. Build system: CMake alongside MSBuild

- **Decision**: Add `CMakeLists.txt` for the POSIX (and optionally Windows) build;
  leave `TCDir.sln`/`.vcxproj` untouched for the Windows product.
- **Rationale**: MSBuild can't target Linux/macOS toolchains; CMake-alongside keeps
  the shipping Windows build identical while enabling cross-platform CI.
- **Alternatives**: Migrate fully to CMake (unnecessary churn/risk to Windows now);
  Meson (less ubiquitous in this ecosystem).

## D8. Name case & pattern handling (from spec Clarifications)

- **Decision**: Case-insensitive **sort** for display; case-sensitive (OS-native)
  **matching**. Hybrid pattern handling: show shell-expanded lists as-is; self-match
  quoted/recursive patterns; always self-enumerate a directory argument so the
  core's hidden/attribute/sort policy applies.
- **Rationale**: Mirrors Windows (`dir`), eza, lsd, desktop `ls`, and default shell
  globbing; verified empirically on a case-sensitive NTFS directory.
- **Alternatives**: Fully case-sensitive (ASCII sort — only matches `ls` in C locale);
  fully case-insensitive (surprises on POSIX matching).

## D9. Testing strategy

- **Decision**: Reuse the existing 684 Windows tests unchanged. Introduce a **Mock
  PAL backend** that returns synthetic entries so core logic (sort, dedup, layout,
  theming resolution, filter dispatch) is tested with no real filesystem. Add a
  portable runner for the POSIX build.
- **Rationale**: Satisfies the constitution's Test Isolation principle directly —
  the PAL is the injectable seam. Deterministic, machine-independent.
- **Alternatives**: Integration tests against real dirs (non-deterministic; violates
  isolation) — reserved for a small smoke suite only.
