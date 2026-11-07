# Data Model: PowerShell Argument Completion

## ESwitchForm

`Short` (`-W`, `/W`) or `Long` (`--Tree`, `/Tree`).

## SSwitchValue

| Field | Type | Notes |
|-------|------|-------|
| `m_pszValue` | `LPCWSTR` | `D`, `Auto`, `4` |
| `m_pszDescription` | `LPCWSTR` | Plain text, as in the help |

## SSwitchDefinition

| Field | Type | Notes |
|-------|------|-------|
| `m_eForm` | `ESwitchForm` | |
| `m_pszName` | `LPCWSTR` | Casing as in the help: `W`, `Archives` |
| `m_pszValueSyntax` | `LPCWSTR` | Help label suffix: `=N`, `=Auto|Bytes`; empty for none |
| `m_chValueSeparator` | `wchar_t` | `:` for `-A:`, `=` for `--Size=`; `0` when the switch doesn't take a value |
| `m_values` | `span<const SSwitchValue>` | Completion values |
| `m_fNegatable` | `bool` | `--Icons-` |
| `m_fRepeatable` | `bool` | `-A:` |
| `m_eSection` | `ESwitchSection` | `Primary` or `Alias`, the help group |
| `m_pszDescription` | `LPCWSTR` | Help text with color markers; `{0}` short prefix, `{1}` long prefix |
| `m_pszContinuation` | `LPCWSTR` | Second help line; empty for none |
| `m_fDebugOnly` | `bool` | `--Debug` |

## SCompletionCandidate

| Field | Type | Notes |
|-------|------|-------|
| `m_strCompletion` | `wstring` | Text that replaces the word |
| `m_strListText` | `wstring` | Text in the Ctrl+Space list |
| `m_strTooltip` | `wstring` | Plain-text description |

## SCompletionRequest

| Field | Type | Notes |
|-------|------|-------|
| `m_strWord` | `wstring` | Word under the cursor |
| `m_vOtherWords` | `vector<wstring>` | Alias flags, then the other typed words |
| `m_chDefaultPrefix` | `wchar_t` | Prefix for a word without one; from the parsed command line |

## Relationships

```text
CSwitchCatalog  --rows-->  CUsage::DisplayUsage
CSwitchCatalog  --definitions-->  CCompleter  --candidates-->  stdout
CCommandLine::Parse  <--validation--  CCompleter
CAliasBlockGenerator  --registration lines-->  profile block and --Completion pwsh
```
