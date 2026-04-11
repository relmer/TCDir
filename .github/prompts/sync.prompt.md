---
description: "Sync the next unported feature, spec, or fix from TCDir (C++) to RCDir (Rust). Automatically determines what needs syncing."
agent: "agent"
argument-hint: "Optional: specify a feature/spec to sync, or leave blank to auto-detect"
---
# Sync TCDir → RCDir

You synchronize features, specs, and fixes from TCDir (C++) to RCDir (Rust). TCDir is the reference implementation; RCDir must match it in appearance, behavior, and functionality.

## CRITICAL: Workspace Validation (do this FIRST)

Before doing anything else, confirm that **both** TCDir and RCDir workspace folders are present. Check for:
- A workspace folder containing `TCDirCore/` (the TCDir project)
- A workspace folder containing `src/main.rs` and `Cargo.toml` (the RCDir project)

If either is missing, **STOP IMMEDIATELY** and display:

> **Error: Both TCDir and RCDir must be accessible.**
>
> This prompt requires both projects. In VS Code, use a multi-root workspace with both folders. Ensure both repos are available before retrying.

Do NOT proceed if this check fails.

## Step 1: Determine What to Sync

If the user specified a feature/spec, use that. Otherwise, auto-detect by working through these sources in priority order. Stop as soon as you find work to do.

### Priority A: Check sync-status.md for unsynced items

Read `TCDir/specs/sync-status.md`. Look for any row where:
- TCDir Status is "Shipped" or "In progress" AND
- RCDir Status is "Not started" or blank

Select the **first** such item. Before proceeding, **verify it's actually unsynced** (see Validation below). If it turns out to be already done, update sync-status.md to reflect reality and move to the next item.

### Validation: Confirm the item is truly unsynced

sync-status.md may be stale. Before starting work on any item, verify it hasn't already been ported:
- For spec-driven features: check if `RCDir/specs/<NNN>-<name>/` already exists with a `spec.md`
- For bug fixes/changes: search RCDir's git log and source code for evidence the change was already applied
- If the item is already done in RCDir, update sync-status.md to "Shipped" (or "Spec created" / "In progress" as appropriate) and move to the next unsynced item
- Repeat until you find an item that genuinely needs work, or all items are confirmed synced

### Priority B: Compare spec directories

If sync-status.md shows everything already synced, compare the contents of:
- `TCDir/specs/` — all spec directories present
- `RCDir/specs/` — all spec directories present

If there are spec directories in TCDir that do not exist in RCDir (ignore `sync-status.md` itself and any non-directory files):
1. Add the missing specs to `sync-status.md` as new rows with RCDir Status = "Not started"
2. Select the **first** newly added item. Run the Validation check above to confirm it's genuinely unsynced before proceeding to Step 2.

### Priority C: Compare git commit history

If all specs are in sync, examine commits on TCDir's `master` branch since the last spec was merged (use the date/version from the most recently shipped spec in sync-status.md as the starting point). Compare against RCDir's `master` branch commits in the same timeframe.

Look for significant TCDir commits that have no corresponding RCDir change. **Filter out irrelevant commits that do NOT need porting:**
- GitHub workflow/CI configuration changes (`.github/workflows/`, `.github/ISSUE_TEMPLATE/`, etc.)
- Changes to TCDir-specific build scripts that have no RCDir equivalent
- C++-specific tooling or project file changes (`.vcxproj`, `.sln`, `.filters`)
- Documentation changes that are TCDir-specific (TCDir README badges, TCDir-only changelog entries)
- Implementation-specific bug fixes for issues that cannot exist in Rust (e.g., memory safety bugs, COM lifetime issues)

**Do port:**
- Bug fixes that affect user-visible output or behavior
- New command-line switches or options
- Changes to display formatting, color handling, or output layout
- Error message changes
- Config file format changes
- Test coverage for shared behaviors

For each significant unported commit:
1. Add it to `sync-status.md` as a new row (use "Fix" or "Change" as the Spec # depending on nature)
2. Select the **first** newly added item. Run the Validation check above to confirm it's genuinely unsynced before proceeding to Step 2.

### If everything is in sync

If all three sources show nothing to do, report:

> **TCDir and RCDir are fully in sync.** No unported features, specs, or significant commits found.

Stop here.

## Step 2: Execute the Sync

### For spec-driven features: create the RCDir spec

- Read the TCDir `spec.md` thoroughly
- Invoke the `speckit.specify` agent in the **RCDir** workspace to create a new spec based on an exact copy of the TCDir spec
- In your prompt to speckit.specify, instruct it to:
  - Keep all functional requirements, acceptance scenarios, edge cases, and success criteria identical
  - Replace any C++-specific implementation details with Rust equivalents:
    - `HRESULT` / EHM macros → `Result<T, E>`
    - `wstring` / `LPCWSTR` → `String` / `&str` / `OsString`
    - Win32 API names → Rust crate equivalents (e.g., `windows` crate)
    - `CConfig`, `CCommandLine` class names → Rust module/struct naming conventions
    - `std::ifstream` / `CreateFileW` → `std::fs::read_to_string` or equivalent
    - COM / ATL patterns → idiomatic Rust
  - The spec should be language-neutral where possible; only mention Rust specifics where the spec genuinely requires it
  - Preserve the same spec number as TCDir

### For bug fixes or minor changes

- Describe the change needed in RCDir terms
- Identify the corresponding RCDir source files
- Implement the fix directly, following RCDir's coding conventions (see `.github/copilot-instructions.md` in the RCDir workspace)

## Step 3: Update sync-status.md

After creating the spec or completing the port, update the RCDir Status column in `TCDir/specs/sync-status.md`:
- "Spec created" if only the spec was ported
- "In progress" if implementation has started
- "Shipped" with version number if complete

Add any relevant notes.

## Important Rules

- **Never modify TCDir source code** — TCDir is the reference, not the target
- **Output parity is the goal** — RCDir must produce identical visible output to TCDir
- The spec in RCDir should describe the same user-facing behavior; implementation details should be idiomatic Rust
- Follow RCDir's `.github/copilot-instructions.md` for all Rust code formatting and conventions
- When adding rows to sync-status.md, preserve the existing table formatting and column alignment
