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

### User Story 3 - Setup through the alias installer (Priority: P1)

A user who runs `tcdir --set-aliases` gets completion for `tcdir` and for
every alias the installer defines, with no extra step. `--remove-aliases`
removes the completion with the aliases, and `--get-aliases` reports whether
completion is installed.

**Why this priority**: The alias installer already writes a managed block
into the PowerShell profile, so completion comes with no new setup step.

**Independent Test**: Run `tcdir --set-aliases --whatif` and check that the
block contains the completer registration for `tcdir` and each alias. Run it
for real, open a new session, and complete `d --Ar` where `d` is an alias.

**Acceptance Scenarios**:

1. **Given** no tcdir block in the profile, **When** the user runs `tcdir
   --set-aliases` and opens a new session, **Then** `tcdir --Ar` and
   `<alias> --Ar` both complete to `--Archives`.
2. **Given** the block is installed, **When** the user runs `tcdir
   --remove-aliases` and opens a new session, **Then** no completer is
   registered for `tcdir` or the aliases.
3. **Given** the block is installed, **When** the user runs `tcdir
   --get-aliases`, **Then** the output reports that completion is installed.
4. **Given** a profile block written by an earlier tcdir version without
   completion, **When** the user runs `tcdir --set-aliases`, **Then** the
   block is replaced with one that registers completion.
5. **Given** the user installs a newer tcdir, **When** they complete a switch
   added in the newer version, **Then** it is a candidate without running
   `--set-aliases` again.

---

### User Story 4 - Setup without aliases (Priority: P2)

A user who doesn't install the aliases runs `tcdir --Completion pwsh` to print the
completer script, and adds one line to their profile:
`tcdir --Completion pwsh | Out-String | Invoke-Expression`.

**Why this priority**: Not every user installs the aliases, and this is the
setup other command-line tools use.

**Independent Test**: Run the one-line setup in a fresh session and complete
`tcdir --Ar`.

**Acceptance Scenarios**:

1. **Given** a fresh session, **When** the user runs `tcdir --Completion pwsh
   | Out-String | Invoke-Expression`, **Then** `tcdir --Ar` completes to
   `--Archives` in that session.
2. **Given** the aliases are installed, **When** the user also runs the
   one-line setup, **Then** completion works once and candidates aren't
   duplicated.
3. **Given** an unsupported shell value, **When** the user runs `tcdir
   --Completion bash`, **Then** tcdir prints an error that lists the
   supported values and exits with a failure code.

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

- **FR-008**: `--set-aliases` MUST add the completer registration for `tcdir`
  and for every alias it defines to the managed profile block.
  `--remove-aliases` MUST remove it with the block. `--whatif` MUST print it.
- **FR-009**: `--get-aliases` MUST report whether the installed block
  registers completion.
- **FR-010**: `tcdir --Completion pwsh` MUST print a script that registers
  the completer for `tcdir` and `tcdir.exe` when run in a session. Running it
  in a session that already has the alias block MUST NOT duplicate
  candidates.
- **FR-011**: `--Completion` with any other value MUST print an error listing
  the supported values and exit with a failure code.
- **FR-012**: `--Completion` MUST be documented in the `-?` help. The
  interface the script uses to request candidates from tcdir MUST be
  documented in the help as part of `--Completion`.

**Behavior and compatibility**

- **FR-013**: The completer MUST work in Windows PowerShell 5.1 and
  PowerShell 7.
- **FR-014**: When tcdir can't be run while completing, the completer MUST
  return no candidates and print nothing.
- **FR-015**: Requesting candidates MUST NOT read the file system, list
  folders, or print anything except the candidates.
- **FR-016**: Command lines without `--Completion` or the candidate request
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
  completer for every word that isn't a switch.
- **SC-004**: A user with the aliases installed gets completion after one
  `--set-aliases` run and a new session, with no other setup.
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
