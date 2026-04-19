# Research: Ellipsize Long Link Target Paths

**Date**: 2026-04-19
**Feature**: 008-ellipsize-targets

## R1: Available Width Calculation

### Decision
Compute available width arithmetically from known column widths. No character counter needed.

### Rationale
CConsole has no column tracker. But all metadata columns have deterministic widths that can be computed from the same data the displayers already have access to.

### Formula
```
availableWidth = consoleWidth
    - 21                           // date+time: "MM/dd/yyyy  hh:mm tt "
    - 7                            // attributes: 7 single-char flags
    - (2 + max(cchMaxFileSize, 5)) // file size (Bytes mode) or 9 (Auto/<DIR>)
    - cloudStatusWidth             // 3-4 chars if in sync root, 0 otherwise
    - debugWidth                   // 14 if debug mode, 0 otherwise
    - ownerWidth                   // cchMaxOwnerLength + 1 if owner, 0 otherwise
    - iconWidth                    // 3 if icons active, 0 otherwise
    - treePrefixWidth              // depth * m_cTreeIndent - 1 for tree mode
    - cchFileName                  // source filename length
    - 3                            // " → " (space + arrow + space)
```

### Alternatives Considered
- **Add a column counter to CConsole**: Invasive change, modifies every Printf path. Rejected — arithmetic is sufficient.
- **Measure buffer length before/after**: Fragile — buffer contains ANSI escape sequences that don't correspond to character positions.

## R2: EllipsizePath Pure Function Design

### Decision
Create `EllipsizePath(targetPath, availableWidth)` as a pure function in `PathEllipsis.h/.cpp`. Returns a truncated wstring or the original if it fits.

### Algorithm
```
if targetPath.length() <= availableWidth:
    return targetPath  // fits, no truncation

Split targetPath into path components.

Try these forms in priority order, return first that fits:
  1. components[0] + "\" + components[1] + "\…\" + components[N-2] + "\" + components[N-1]
     (first two dirs + … + leaf dir + filename)
  2. components[0] + "\" + components[1] + "\…\" + components[N-1]
     (first two dirs + … + filename)
  3. components[0] + "\…\" + components[N-1]
     (first dir + … + filename)
  4. components[N-1]
     (just the leaf filename, truncated to availableWidth if needed)
```

### Rationale
- Pure function = trivially testable with synthetic data
- Priority order matches user's information preference (first two dirs + leaf are most important)
- Graceful degradation to leaf-only ensures something always fits

## R3: Switch Infrastructure

### Decision
Add `m_fEllipsize` as `optional<bool>` to both CConfig and CCommandLine. Default behavior: on (truncation active). Follow the exact same pattern as `--Icons` (supports negation via `-` suffix).

### Changes Required
1. **Config.h**: Add `optional<bool> m_fEllipsize;`
2. **Config.cpp**: Add `{ L"ellipsize"sv, true, &CConfig::m_fEllipsize }` and `{ L"ellipsize-"sv, false, &CConfig::m_fEllipsize }` to `s_switchMappings[]`; add to `s_switchMemberOrder[]`; bump `SWITCH_COUNT` from 9 to 10
3. **CommandLine.h**: Add `optional<bool> m_fEllipsize;`
4. **CommandLine.cpp**: Add parsing in `HandleLongSwitch()`; add to `IsRecognizedLongSwitch()` list; add to `ApplyConfigDefaults()`
5. **Usage.cpp**: Add synopsis entry, detail entry, and `s_kSwitchInfos[]` entry

### Default Behavior
When `m_fEllipsize` is nullopt (not set by user), treat as **true** (on). This means the displayer checks: `if (!m_cmdLinePtr->m_fEllipsize.has_value() || m_cmdLinePtr->m_fEllipsize.value())`.

## R4: Ellipsis Color

### Decision
Render the `…` character using `CConfig::EAttribute::Default` — visually distinct from the path text which uses the source file's color (`textAttr`).

### Implementation
The display code splits the truncated path around the `…`:
```cpp
// prefix part (e.g., "C:\Program Files\")
m_consolePtr->Printf(textAttr, L"%s", prefix.c_str());
// ellipsis in default color
m_consolePtr->Printf(CConfig::EAttribute::Default, L"%c", UnicodeSymbols::Ellipsis);
// suffix part (e.g., "\python3.12.exe")
m_consolePtr->Printf(textAttr, L"%s", suffix.c_str());
```

This means `EllipsizePath` needs to return both the result string AND the prefix/suffix split points, or return a struct with prefix + suffix separate.

### Alternative: Return a struct
```cpp
struct SEllipsizedPath {
    wstring prefix;   // e.g., "C:\Program Files\"
    wstring suffix;   // e.g., "\python3.12.exe"
    bool    fTruncated;
};
```
When `fTruncated` is false, the full path is in `prefix` and `suffix` is empty.
