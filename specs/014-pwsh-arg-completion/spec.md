# Feature Specification: PowerShell Argument Completion

**Feature Branch**: `014-pwsh-arg-completion`

**Created**: 2026-11-06

**Status**: Draft

**Input**: User description: "PowerShell tab completion for tcdir switches and values. The alias profile block registers a completer for tcdir and the aliases, the completer calls back into tcdir for candidates so the list comes from the installed version, and a switch prints the completer script for users who don't install aliases."

## User Scenarios & Testing *(mandatory)*

### User Story 1 - Complete switches (Priority: P1)

A PowerShell user types `tcdir --Ar` and presses Tab, and the command line
becomes `tcdir --Archives`. Pressing Tab after `tcdir --` cycles through
every long switch. Ctrl+Space lists them with a one-line description of each,
taken from the `-?` help.

**Why this priority**: Switch completion is the reason for the feature.

**Independent Test**: Register the completer in a PowerShell session, then
complete `tcdir --Ar`, `tcdir -` and `tcdir /` and compare the candidates
against the `-?` help.

**Acceptance Scenarios**:

1. **Given** the completer is registered, **When** the user completes
   `tcdir --Ar`, **Then** the only candidate is `--Archives`.
2. **Given** the same session, **When** the user completes `tcdir --`,
   **Then** every long switch in the `-?` help is a candidate, in the casing
   the help uses.
3. **Given** the same session, **When** the user completes `tcdir -`,
   **Then** the candidates are the short switches (`-A`, `-B`, `-O`, `-S`,
   `-T`, `-W` and the others in the help) followed by the long switches.
4. **Given** the same session, **When** the user completes `tcdir /` or
   `tcdir /Ar`, **Then** the candidates use the `/` prefix (`/Archives`).
5. **Given** the same session, **When** the user lists candidates with
   Ctrl+Space, **Then** each candidate has the description from the `-?`
   help as its tooltip.

---

### User Story 2 - Keep path completion (Priority: P1)

When the word under the cursor isn't a switch, Tab completes file and folder
paths as it does without the completer.

**Why this priority**: Paths are the most common tcdir arguments. A completer
that broke them would be worse than none.

**Independent Test**: Complete `tcdir C:\Pro` and `tcdir .\sr` with and
without the completer registered, and compare the candidates.

**Acceptance Scenarios**:

1. **Given** the completer is registered, **When** the user completes
   `tcdir C:\Pro`, **Then** the candidates are the same paths PowerShell
   offers without the completer.
2. **Given** the completer is registered, **When** the user completes an
   empty word after `tcdir `, **Then** the candidates are the paths in the
   current folder.
3. **Given** a word that starts with `-` but is a path, such as `.\-notes`,
   **When** the user completes it, **Then** path completion applies.

---

### User Story 3 - Enable and disable completion (Priority: P1)

A user runs `tcdir --Enable-Completion` once, opens a new session, and gets
completion for `tcdir` and for every alias the alias installer defined.
`tcdir --Disable-Completion` removes it. Completion is a separate managed block
in the profile, apart from the alias block, so it can be turned on
without aliases.

**Why this priority**: One command turns completion on, with or without
aliases.

**Independent Test**: Run `tcdir --Enable-Completion --whatif` and check the
completion block for `tcdir`, `tcdir.exe` and each alias. Run it for real,
open a new session, and complete `tcdir --Tr`.

**Acceptance Scenarios**:

1. **Given** no completion block, **When** the user runs `tcdir
   --Enable-Completion` and opens a new session, **Then** `tcdir --Tr`
   completes to `--Tree`.
2. **Given** an alias block in the profile, **When** the user enables
   completion, **Then** the completion block registers every alias with its
   flags, and `<alias> --Tr` completes.
3. **Given** a completion block, **When** the user runs `tcdir
   --Disable-Completion` and opens a new session, **Then** no completer is
   registered, and the alias block is untouched.
4. **Given** a completion block, **When** the user runs `--set-aliases` or
   `--remove-aliases`, **Then** the completion block is regenerated for the
   new set of aliases.
5. **Given** the user installs a newer tcdir, **When** they complete a switch
   added in the newer version, **Then** it is a candidate without enabling
   completion again.
6. **Given** `--whatif`, **When** the user enables or disables completion,
   **Then** tcdir prints the change and doesn't modify the profile.

---

### User Story 4 - Offer completion from the alias wizard (Priority: P2)

When `--set-aliases` runs and the target profile has no completion block,
the last wizard step offers to enable tab completion too.

**Why this priority**: Users who set up aliases discover completion without
reading the help.

**Independent Test**: Run `tcdir --set-aliases --whatif` against a profile
without a completion block, answer yes, and check that the preview includes
the completion block.

**Acceptance Scenarios**:

1. **Given** no completion block, **When** the user selects Yes, **Then** the
   alias block and a completion block are written.
2. **Given** no completion block, **When** the user declines, **Then** only
   the alias block is written.
3. **Given** an existing completion block, **When** the wizard runs, **Then**
   the wizard skips that step and regenerates the completion block.

---

### User Story 5 - Complete switch values (Priority: P2)

After a switch that takes a value, Tab completes the value: sort keys after
`-O:`, attribute letters after `-A:`, time fields after `-T:`, `Auto` and
`Bytes` after `--Size=`, and `1` to `8` after `--TreeIndent=`.

**Why this priority**: Value letters are the least familiar part of the syntax,
and the help lists them in a separate section.

**Independent Test**: Complete `tcdir -O:`, `tcdir -A:`, `tcdir -T:` and
`tcdir --Size=` and compare the candidates against the help.

**Acceptance Scenarios**:

1. **Given** the completer is registered, **When** the user completes
   `tcdir -O:`, **Then** each sort key in the help is a candidate with its
   description, both with and without the `-` reverse prefix.
2. **Given** the same session, **When** the user completes `tcdir -A:H`,
   **Then** the candidates extend `H` with each other attribute letter, so
   several attributes can be combined.
3. **Given** the same session, **When** the user completes `tcdir --Size=`,
   **Then** the candidates are `--Size=Auto` and `--Size=Bytes`.
4. **Given** a switch that doesn't take a value, **When** the user types `:` after
   it and completes, **Then** there are no candidates.

---

### User Story 6 - Context-aware candidates (Priority: P3)

Switches already on the command line, and switches that would conflict with
them, aren't candidates. After `--Tree`, `-W` isn't offered. After `-W`,
`-W` isn't offered again.

**Why this priority**: The candidate list gets shorter and every candidate is
valid, but the feature is useful without this.

**Independent Test**: Complete `tcdir --Tree -` and check that `-W` and `-B`
aren't candidates, then run each candidate with `--Tree` and check that none
produces a conflict error.

**Acceptance Scenarios**:

1. **Given** `tcdir -W ` on the command line, **When** the user completes
   `-`, **Then** `-W` isn't a candidate.
2. **Given** `tcdir --Tree ` on the command line, **When** the user
   completes `-`, **Then** no candidate conflicts with `--Tree`.
3. **Given** a switch that can repeat, such as `-A:`, **When** it is already
   on the command line, **Then** it remains a candidate.
4. **Given** a switch whose negated form exists (`--Icons-`), **When** the
   positive form is on the command line, **Then** neither form is offered.

---

### Edge Cases

- The cursor is in the middle of a word: candidates replace the whole word.
- The word is quoted (`'--Ar`): completion applies to the text inside the
  quotes.
- The command line contains a pipe or a second command: only the tcdir
  command's words are considered.
- tcdir isn't on the PATH when Tab is pressed: no candidates, and path
  completion applies. No error is printed.
- The alias block's tcdir path no longer exists: same as above.
- The switch prefix is configured to `/` in `TCDIR`: candidates for an empty
  prefix word use `/`.
- Windows PowerShell 5.1 and PowerShell 7 are both supported.
- Completion is case insensitive: `tcdir --ar` completes to `--Archives`.

## Requirements *(mandatory)*

### Functional Requirements

**Candidates**

- **FR-001**: Completing a word that starts with `--`, `-` or `/` MUST offer
  the switches that start with the typed text, compared without regard to
  case, with the prefix the user typed.
- **FR-002**: Every switch in the `-?` help MUST be offered, in the casing
  the help uses. Switches that aren't in the help MUST NOT be offered.
- **FR-003**: Each candidate MUST include the switch's description from the
  `-?` help as its tooltip.
- **FR-004**: After a switch that takes a value, completing the value MUST
  offer the values the help lists for that switch, with their descriptions.
  For attribute letters, candidates MUST extend the letters already typed.
- **FR-005**: When the word isn't a switch or a switch value, the completer
  MUST return no candidates, so PowerShell completes paths.
- **FR-006**: The candidate list MUST come from the tcdir that runs when Tab
  is pressed, so a newer tcdir offers its switches without a new setup step.
- **FR-007**: Switches already on the command line MUST be left out unless
  they can repeat. Switches that conflict with ones on the command line MUST
  be left out, using the same rules as the command-line validation.

**Setup**

- **FR-008**: `--Enable-Completion` MUST write a managed completion block,
  separate from the alias block, to the profile that contains the alias block,
  or to the current user's all-hosts profile when no profile has one. The block
  MUST register the completer for `tcdir`, `tcdir.exe` and every alias in
  the alias block, with each alias's flags. Running it again MUST replace the
  block.
- **FR-009**: `--Disable-Completion` MUST remove the completion block from
  every profile that has one, and MUST NOT change the alias block.
- **FR-010**: `--set-aliases` and `--remove-aliases` MUST regenerate the
  completion block in the profile they change when it has one.
- **FR-011**: `--set-aliases` MUST offer to enable completion when the
  target profile has no completion block. `--get-aliases` MUST report
  whether completion is enabled.
- **FR-012**: `--Enable-Completion` and `--Disable-Completion` MUST accept
  `--whatif`, and MUST be documented in the `-?` help.

**Behavior and compatibility**

- **FR-013**: The completer MUST work in Windows PowerShell 5.1 and
  PowerShell 7.
- **FR-014**: When tcdir can't be run while completing, the completer MUST
  return no candidates and print nothing.
- **FR-015**: Requesting candidates MUST NOT read the file system, list
  folders, or print anything except the candidates.
- **FR-016**: Command lines without the completion switches or the candidate request
  MUST produce the output they produce today.

### Key Entities

- **Switch definition**: A switch's short or long form, whether it takes a
  value, its value list, whether it repeats, its negated form, and its help
  description. The help text and the candidates both come from this
  definition.
- **Completion request**: The command line, the cursor position and the word
  under the cursor, sent from PowerShell to tcdir.
- **Candidate**: The completion text, the list text, and the
  tooltip.

## Success Criteria *(mandatory)*

### Measurable Outcomes

- **SC-001**: Candidates appear within 100 ms of pressing Tab on a typical
  machine.
- **SC-002**: Every switch and value in the `-?` help is offered, and nothing
  else is, as checked by comparing the candidate list with the help.
- **SC-003**: Path completion offers the same candidates with and without the
  completer for every word that isn't a- **SC-004**: A user gets completion after one `--Enable-Completion` run (or one
  `--set-aliases` run with completion accepted) and a new session.
r setup.
- **SC-005**: No candidate that User Story 6 offers causes a command-line
  conflict error when added to the command line.

## Assumptions

- PowerShell is the only shell in scope. cmd has no completion interface, and
  bash and other shells are out of scope.
- PowerShell completers registered for native commands apply to functions
  with the same name. This was checked in PowerShell 7.6 and Windows
  PowerShell 5.1 with a function that calls tcdir with `@args`, the form the
  alias installer writes.
- The completer runs tcdir once per Tab press. tcdir starts fast enough for
  SC-001, so candidates aren't cached.
- Switch definitions are moved into one table that the help, the parser and
  the completer share, so the three can't differ. Help output stays the
  same.
- Inline gray-text suggestions (PSReadLine predictors) are out of scope.
