# Tasks: Nerd Font File & Folder Icons

**Input**: Design documents from `/specs/003-file-icons/`
**Prerequisites**: plan.md, spec.md, data-model.md, quickstart.md, research.md

**Tests**: Included — the project has comprehensive test infrastructure (Microsoft C++ Unit Test Framework) and 42 documented test scenarios in quickstart.md.

**Organization**: Tasks grouped by user story. US3 (Icon Colors Match) is merged into US1 since color resolution is inherent in the unified `GetDisplayStyleForFile` implementation. US6 (Well-Known Folder Icons) is merged into US5 since the well-known dir table is created in Foundational and the resolver is implemented in US1 — US6 has no unique implementation tasks beyond what US1+US4 already provide.

## Format: `[ID] [P?] [Story] Description`

- **[P]**: Can run in parallel (different files, no dependencies on incomplete tasks)
- **[Story]**: Which user story this task belongs to (e.g., US1, US2)
- Exact file paths included in descriptions

---

## Phase 1: Setup

**Purpose**: Add new library dependency required by font detection APIs

- [X] T001 Add `#pragma comment(lib, "gdi32.lib")` to TCDirCore/pch.h alongside existing cldapi.lib pragma

---

## Phase 2: Foundational (Blocking Prerequisites)

**Purpose**: Icon mapping infrastructure and CLI activation mechanism that ALL user stories depend on

**CRITICAL**: No user story work can begin until this phase is complete

- [X] T002 [P] Create TCDirCore/IconMapping.h with WideCharPair struct, SIconMappingEntry struct, NfIcon namespace (42 constexpr constants), and extern declarations for default tables and attribute precedence array
- [X] T003 Create TCDirCore/IconMapping.cpp with constexpr CodePointToWideChars implementation, static_asserts for key code points, default extension icon table (75+ entries), well-known directory icon table (27 entries), and attribute precedence array (PSHERC0TA order)
- [X] T004 [P] Add /Icons and /Icons- long switch (optional&lt;bool&gt; m_fIcons) to TCDirCore/CommandLine.h and s_krgLongSwitches[] in TCDirCore/CommandLine.cpp
- [X] T005 Add SFileDisplayStyle struct, EIconActivation enum, icon mapping members (m_mapExtensionToIcon, m_mapWellKnownDirToIcon, m_mapFileAttributeToIcon, fallback icons, cloud icons, m_fIcons), and icon method declarations to TCDirCore/Config.h

**Checkpoint**: Icon data tables exist, CLI flag parses, Config header ready for implementation

---

## Phase 3: User Story 1 + User Story 3 — File-Type Icons with Correct Colors in Normal Mode (Priority: P1) 🎯 MVP

**Goal**: Icons appear before filenames in normal listings. Each icon uses the same color as its filename, resolved through the unified precedence chain (attribute → well-known dir → extension → type fallback) with independent color/icon locking.

**Independent Test**: Run `tcdir /Icons` in a directory with .cpp, .h, .exe, .txt, folders, and symlinks. Verify each entry shows an appropriate icon glyph in the correct color before the filename.

- [X] T006 [US1] Implement InitializeExtensionToIconMap (seed m_mapExtensionToIcon from g_rgDefaultExtensionIcons) in TCDirCore/Config.cpp
- [X] T007 [US1] Implement InitializeWellKnownDirToIconMap (seed m_mapWellKnownDirToIcon from g_rgDefaultWellKnownDirIcons) in TCDirCore/Config.cpp
- [X] T008 [US1] Implement GetDisplayStyleForFile unified precedence resolver in TCDirCore/Config.cpp — single walk over attribute/well-known-dir/extension/type-fallback levels with independent color and icon locking per R6 design
- [X] T009 [US1] Add fIconsActive parameter to displayer construction and storage in TCDirCore/ResultsDisplayerWithHeaderAndFooter.h and TCDirCore/ResultsDisplayerWithHeaderAndFooter.cpp
- [X] T010 [US1] Implement icon column display (CodePointToWideChars + Printf with wchar_t[3] buffer) before filename in TCDirCore/ResultsDisplayerNormal.cpp — adjust truncation logic to account for +2 character width (icon + space)
- [X] T011 [US1] Wire CLI /Icons flag to fIconsActive and pass to displayer construction in TCDirCore/TCDir.cpp (CLI-only path — full detection chain added in US2)

**Checkpoint**: `tcdir /Icons` shows icons in normal mode with correct colors. `tcdir` without /Icons shows classic output. US1 acceptance scenarios 1–4 and US3 acceptance scenarios 1–4 are satisfied.

---

## Phase 4: User Story 2 — Nerd Font Auto-Detection with Manual Override (Priority: P1)

**Goal**: Icons are automatically detected and enabled when a Nerd Font is available. The 6-step layered detection chain (CLI → env var → WezTerm → canary probe → font enumeration → OFF) determines activation. CLI flags always win.

**Independent Test**: On a system with a Nerd Font as console font, run `tcdir` (no flags) and verify icons appear automatically. On a non-NF system, verify classic output. Test `/Icons` and `/Icons-` overrides in both environments.

- [X] T012 [P] [US2] Create TCDirCore/NerdFontDetector.h with EDetectionResult enum and CNerdFontDetector class (Detect method, protected virtual ProbeConsoleFontForGlyph and IsNerdFontInstalled, private static IsWezTerm and IsConPtyTerminal)
- [X] T013 [US2] Create TCDirCore/NerdFontDetector.cpp implementing the layered detection chain: WezTerm short-circuit → ConPTY check → classic conhost canary probe via GetGlyphIndicesW (R1) → system font enumeration via EnumFontFamiliesExW (R2) → fallback OFF
- [X] T014 [US2] Implement full detection orchestration (CLI → config.m_fIcons → CNerdFontDetector::Detect) in TCDirCore/TCDir.cpp, replacing the CLI-only path from T011
- [X] T015 [US2] Add Icons/Icons- switch handling to CConfig env var processing (IsSwitchName and ProcessSwitchOverride) in TCDirCore/Config.cpp

**Checkpoint**: `tcdir` auto-detects NF availability. `/Icons` and `/Icons-` override detection. `TCDIR=Icons` and `TCDIR=Icons-` control default state. US2 acceptance scenarios 1–10 are satisfied.

---

## Phase 5: User Story 4 — Environment Variable Icon Configuration (Priority: P2)

**Goal**: Users customize icons through the existing TCDIR environment variable using the extended `[color][,icon]` comma syntax. Supports U+XXXX hex code points, literal glyphs, empty (suppress), and the `dir:` prefix for well-known directories.

**Independent Test**: Set `TCDIR=.cpp=Green,U+E61D;dir:.git=,U+E5FB;.obj=,` and verify: .cpp files show C++ icon in Green, .git directories show Git icon, .obj files show no icon (suppressed).

- [X] T016 [US4] Implement ParseIconValue (U+XXXX hex parsing, literal glyph extraction, empty = suppressed, validation with ErrorInfo) in TCDirCore/Config.cpp
- [X] T017 [US4] Extend ProcessColorOverrideEntry with first-comma splitting to separate color and icon values per R5 design in TCDirCore/Config.cpp
- [X] T018 [US4] Implement icon override dispatch methods (ProcessFileExtensionIconOverride, ProcessWellKnownDirIconOverride, ProcessFileAttributeIconOverride) with source tracking in TCDirCore/Config.cpp
- [X] T019 [US4] Add duplicate and conflict detection for icon entries (first-write-wins, ErrorInfo on subsequent duplicates) in TCDirCore/Config.cpp

**Checkpoint**: `TCDIR=.cpp=Green,U+E61D` sets Green color + C++ icon. Entries without comma behave identically to pre-feature (zero regression). US4 acceptance scenarios 1–8 are satisfied.

---

## Phase 6: User Story 5 + User Story 6 — Wide/Bare Display Modes + Well-Known Folder Icons (Priority: P3)

**Goal**: Icons appear in wide (`/W`) and bare (`/B`) display modes with correct alignment. In wide mode, directory brackets are suppressed when icons are active (folder icon provides distinction). Well-known folder icons (`.git`, `src`, `docs`, etc.) display specific icons — this is already implemented via the foundational table (T003) and unified resolver (T008); this phase confirms it works across all display modes.

**Independent Test**: Run `tcdir /W /Icons` and `tcdir /B /Icons` in a directory with well-known folders (.git, src, node_modules) and mixed files. Verify icons appear, columns align, and directories show without brackets in wide mode.

- [X] T020 [P] [US5] Add icon display, directory bracket suppression when icons active, and column width adjustment (+2 for icon+space) to TCDirCore/ResultsDisplayerWide.cpp
- [X] T021 [P] [US5] Add icon display before each filename to TCDirCore/ResultsDisplayerBare.cpp

**Checkpoint**: All three display modes show icons correctly. Wide mode suppresses brackets. Column alignment accounts for icon width. US5 acceptance scenarios 1–5 and US6 acceptance scenarios 1–3 are satisfied. US6 scenario 4 (dir: override) was satisfied in Phase 5.

---

## Phase 7: User Story 7 — Enhanced Cloud Status Symbols (Priority: P3)

**Goal**: When icons are active in a cloud sync root, cloud status symbols upgrade from Unicode geometric shapes (○ ◐ ●) to Nerd Font glyphs (cloud outline, cloud check, pin). When icons are inactive, original symbols are preserved.

**Independent Test**: Run `tcdir /Icons` in a OneDrive-synced folder. Verify cloud-only files show cloud outline glyph, locally available files show cloud-check glyph, and pinned files show pin glyph. Run `tcdir /Icons-` and verify original circles.

- [X] T022 [US7] Implement GetCloudStatusIcon method (returns NfIcon glyph when icons active, original Unicode shape when not) in TCDirCore/Config.cpp
- [X] T023 [US7] Upgrade cloud status column output to call GetCloudStatusIcon in TCDirCore/ResultsDisplayerNormal.cpp
- [X] T024 [US7] Add per-entry cloud status with NF glyph upgrade (FR-033) in TCDirCore/ResultsDisplayerWide.cpp

**Checkpoint**: Cloud symbols upgrade to NF glyphs with icons active. Zero regression when icons off. US7 acceptance scenarios 1–4 are satisfied.

---

## Phase 8: Polish & Cross-Cutting Concerns

**Purpose**: Diagnostics, help text, version bump, and documentation updates

- [X] T025 Extend DisplayUsage (`/?`) with /Icons and /Icons- flag documentation in TCDirCore/Usage.cpp
- [X] T026 Extend DisplayEnvVarHelp (`/env`) with icon syntax documentation ([color][,icon], U+XXXX, dir: prefix, literal glyph) in TCDirCore/Usage.cpp
- [X] T027 Extend DisplayCurrentConfiguration (`/config`) with icon detection status, resolved state, and icon mapping table with source indicators in TCDirCore/Usage.cpp
- [X] T028 [P] Bump VERSION_MAJOR from 4 to 5 in TCDirCore/Version.h

---

## Phase 9: Tests

**Purpose**: Comprehensive test coverage for all user stories. 42 test scenarios documented in quickstart.md.

- [X] T029 [P] Create UnitTest/IconMappingTests.cpp — BMP/surrogate encoding (scenarios 1–2), table completeness and no-duplicate checks (scenarios 3–5), CodePointToWideChars edge cases
- [X] T030 [P] Create UnitTest/NerdFontDetectorTests.cpp — WezTerm detection (scenario 6), ConPTY delegation (scenario 7), classic conhost canary probe (scenario 8), multiple env vars (scenario 9), empty env vars (scenario 10) using NerdFontDetectorProbe derivation pattern + CTestEnvironmentProvider
- [X] T031 Extend UnitTest/ConfigTests.cpp with icon parsing tests (scenarios 11–22: comma syntax, U+XXXX, literal glyph, suppression, dir: prefix, attr: prefix, duplicates, Icons switch, invalid hex, surrogate rejection) and precedence tests (scenarios 23–27: hidden .cpp fallthrough, hidden .git dir, attribute icon lock, plain extension, unknown extension)
- [X] T032 [P] Extend UnitTest/CommandLineTests.cpp with /Icons and /Icons- parsing tests (scenarios 28–32: enable, disable, long switch, nullopt default, CLI-over-env-var precedence)
- [X] T033 Extend UnitTest/ResultsDisplayerTests.cpp with icon display tests for all modes (scenarios 33–42: normal icon/spacing/color, wide brackets/cloud-status/NF-glyph, bare icon+filename, column alignment)
- [X] T034 Extend UnitTest/DirectoryListerScenarioTests.cpp with end-to-end icon scenarios (mixed file types, icons on/off, wide+bare modes with icons)

---

## Dependencies & Execution Order

### Phase Dependencies

- **Setup (Phase 1)**: No dependencies — start immediately
- **Foundational (Phase 2)**: Depends on Setup — **BLOCKS all user stories**
- **US1+US3 (Phase 3)**: Depends on Foundational — delivers the MVP
- **US2 (Phase 4)**: Depends on Phase 3 (needs displayer wiring to replace CLI-only path)
- **US4 (Phase 5)**: Depends on Phase 3 (needs Config icon infrastructure). Independent of US2.
- **US5+US6 (Phase 6)**: Depends on Phase 3 (needs icon display pattern from Normal displayer). Independent of US2/US4.
- **US7 (Phase 7)**: Depends on Phase 3 (needs icon active state). Independent of US2/US4/US5.
- **Polish (Phase 8)**: Depends on Phases 3–7 (needs all features implemented)
- **Tests (Phase 9)**: Depends on Phases 3–7 (tests the implementations)

### User Story Dependencies

- **US1+US3 (P1)**: Can start after Foundational — no dependencies on other stories
- **US2 (P1)**: Can start after US1+US3 — replaces CLI-only activation with full detection chain
- **US4 (P2)**: Can start after US1+US3 — extends Config parsing. Independent of US2.
- **US5+US6 (P3)**: Can start after US1+US3 — extends displayers. Independent of US2/US4.
- **US7 (P3)**: Can start after US1+US3 — extends cloud output. Independent of US2/US4/US5.

### Within Each User Story

- Header files before implementation files
- Config infrastructure before displayer integration
- Core implementation before wiring/orchestration

### Parallel Opportunities

**After Foundational completes:**
- US4, US5+US6, and US7 can all proceed in parallel (different files, no cross-dependencies)
- US2 should complete before US4 (T015 adds env var switch handling needed for full TCDIR parsing)

**Within Foundational (Phase 2):**
- T002 (IconMapping.h) and T004 (CommandLine) — different files, parallel
- T003 depends on T002 (needs header)
- T005 depends on T002 (references NfIcon types)

**Within Tests (Phase 9):**
- T029 (IconMappingTests) and T030 (NerdFontDetectorTests) — new files, parallel
- T032 (CommandLineTests) — different file from T031, parallel with T031
- T033 and T034 — sequential (displayer tests before end-to-end)

---

## Parallel Example: After Foundational

```
# Parallel batch 1 — after Phase 3 (US1+US3) completes:
Developer A: Phase 4 (US2) — NerdFontDetector + detection orchestration
Developer B: Phase 6 (US5+US6) — Wide/Bare displayers
Developer C: Phase 7 (US7) — Cloud status upgrade

# Parallel batch 2 — after US2 completes:
Developer A: Phase 5 (US4) — TCDIR icon parsing
Developer B: Phase 8 (Polish) — Usage + Version
```

---

## Implementation Strategy

### MVP First (User Story 1+3 Only)

1. Complete Phase 1: Setup (T001)
2. Complete Phase 2: Foundational (T002–T005)
3. Complete Phase 3: US1+US3 (T006–T011)
4. **STOP and VALIDATE**: `tcdir /Icons` shows icons in normal mode with correct colors
5. This is a shippable MVP — manual `/Icons` flag enables the feature

### Incremental Delivery

1. Setup + Foundational → Infrastructure ready
2. US1+US3 → Icons in normal mode with `/Icons` override → **MVP!**
3. US2 → Auto-detection makes icons zero-config → Feature complete for most users
4. US4 → TCDIR customization → Power user configuration
5. US5+US6 → Wide/Bare modes + well-known folders → Full display mode coverage
6. US7 → Cloud status upgrade → Visual polish
7. Polish + Tests → Production ready with v5.0 version bump

### Suggested MVP Scope

Phase 1 + Phase 2 + Phase 3 = **11 tasks** (T001–T011). This delivers icons in normal mode with the `/Icons` flag, which is sufficient to demo and validate the core feature.

---

## Notes

- [P] = different files, no dependencies on incomplete tasks in the same phase
- [US*] label maps task to specific user story for traceability
- US3 (icon colors match) is inherent in the GetDisplayStyleForFile unified resolver (US1 T008)
- US6 (well-known folder icons) is inherent in the foundational table (T003) + US1 resolver (T008) + US4 dir: override (T018)
- All new .h/.cpp files must be added to the corresponding .vcxproj and .vcxproj.filters
- Test files follow the existing ConfigProbe/NerdFontDetectorProbe derivation pattern
- Commit after each task or logical group; stop at any checkpoint to validate
- 42 test scenarios from quickstart.md are mapped to tasks T029–T034
