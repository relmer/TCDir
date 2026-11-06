# Research: PowerShell Argument Completion

## R1. Registration mechanism

**Decision**: `Register-ArgumentCompleter -Native -CommandName <commands> -ScriptBlock { ... }`.

**Findings** (checked in PowerShell 7.6 and Windows PowerShell 5.1):

- A native completer registered under a function's name applies to that
  function. The alias installer writes functions (`function d { tcdir @args }`),
  so each alias gets completion from the registration for its function.
- When the script block returns nothing, PowerShell falls back to path
  completion. FR-005 doesn't require extra code in the script.
- Registering again for the same command name replaces the earlier
  completer, so running `--Completion pwsh` after the alias block can't
  duplicate candidates (FR-010).

**Alternatives considered**: PSReadLine predictors (`ICommandPredictor`)
require a .NET assembly. Crescendo wrappers change how tcdir is called. Both
are out of scope in the spec.

## R2. Where candidates come from

**Decision**: The script block calls tcdir for each Tab press:

```powershell
& $tcdir --Complete "w:$wordToComplete" <alias flags> <other words>
```

tcdir prints one candidate per line, and the script block turns each line
into a `CompletionResult`.

**Rationale**: Candidates come from the installed tcdir, so a newer version
offers its new switches without a new setup step (FR-006). tcdir starts in
about 20 ms, inside the 100 ms target (SC-001).

**Alternatives considered**: Embedding the candidate list in the script.
Not chosen: the list in the profile would go stale after an upgrade.

## R3. Request and response format

**Request**: `--Complete w:<word> <word> ...`. `--Complete` must be the first
argument, and tcdir handles it before normal parsing, so the words after it
are never applied as switches. They are the rest of the command line: the
alias's built-in flags first, then the words the user typed, without the
word being completed.

The `w:` prefix prevents an empty word from being dropped: Windows PowerShell
5.1 omits empty arguments when it calls a native program. A literal `--`
separator isn't used, because PowerShell versions differ in whether they
pass it to native programs.

**Response**: one line per candidate, three tab-separated fields:
completion text, list text, tooltip. No color, no header, no trailing text.
An empty response means "no candidates", so PowerShell completes paths.

The script block passes the word unquoted: PowerShell supplies
`$wordToComplete` without the quotes the user typed.

## R4. One switch catalog

**Decision**: Add `CSwitchCatalog` with a `constexpr` table of switch
definitions: prefix kind (short or long), name, value syntax, value list,
negatable, repeatable, and the help description and continuation with
`{0}`/`{1}` placeholders for the prefixes. `CUsage::DisplayUsage` builds its
rows and value grids from the table. The completer reads the same table and
strips the color markers from the descriptions for tooltips.

**Parser**: The `CCommandLine` tables stay where they are, because they map switches to
member pointers. A unit test checks that every catalog switch parses and that
every name `IsRecognizedLongSwitch` accepts is in the catalog, so the parser
and the catalog can't drift apart.

**Help output**: The `-?` and `/?` output is captured before the refactor
and compared after it, character for character.

## R5. Context-aware filtering (FR-007)

**Decision**: For each candidate switch, run `CCommandLine::Parse` on the
other words plus the candidate. The candidate is dropped when parsing fails.
Switches already present (matched through the catalog) are dropped unless the
catalog marks them repeatable. Both forms of a negatable switch are dropped
when either form is present.

**Rationale**: The parser's validation is the source of the conflict rules,
so the completer can't disagree with it (SC-005). Parsing a short command
line is in-memory work.

**Config defaults**: The completer doesn't apply `TCDIR` or config defaults,
so candidates don't depend on the environment.

## R6. Values

| Switch | Values | Completion text |
|--------|--------|-----------------|
| `-A:` | Attribute letters from the help | The typed letters plus each letter not yet typed, and `-` |
| `-O:` | `N`, `E`, `S`, `D`, each also with `-` | `-O:S`, `-O:-S`, ... |
| `-T:` | `C`, `A`, `W` | `-T:C`, ... |
| `--Size=` | `Auto`, `Bytes` | `--Size=Auto`, ... |
| `--TreeIndent=` | `1` to `8` | `--TreeIndent=4`, ... |

## R7. Profile block

**Decision**: `CAliasBlockGenerator` appends one registration per command:
`tcdir`, `tcdir.exe`, the root alias, and each sub-alias with its flags after
`--`. The registration lines sit between a "Completion" comment and the end
marker, inside the managed block, so `--remove-aliases` removes them with the
block. `--get-aliases` reports completion as installed when the block
contains `Register-ArgumentCompleter`.

`--Completion pwsh` prints the `tcdir` and `tcdir.exe` registrations with the
full path of the running tcdir, using the same generator code.
