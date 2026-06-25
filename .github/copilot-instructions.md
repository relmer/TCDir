# Global Copilot Instructions for relmer

## Code Formatting - CRITICAL RULES

### **NEVER** Delete Blank Lines
- **NEVER** delete blank lines between file-level constructs (functions, classes, structs)
- **NEVER** delete blank lines between different groups (e.g., C++ includes vs C includes)
- **NEVER** delete blank lines between variable declaration blocks
- Preserve all existing vertical spacing in code

### Project-Specific Vertical Spacing

#### Top-Level Constructs (File Scope)
- **EXACTLY 5 blank lines** between all top-level file constructs:
  - Between preprocessor directives (#include, #define, etc.) and first function
  - Between include blocks and namespace declarations
  - Between namespace and struct/class definitions
  - Between structs/classes and global variables
  - Between global variables and first function
  - Between all function definitions
  - **After the last function in the file**
- **NEVER** add more than 5 blank lines
- **NEVER** delete blank lines if it would result in fewer than 5

#### Function/Block Internal Spacing
- **EXACTLY 3 blank lines** between variable definitions at the top of a function/block and the first real statement
- **1 blank line** for standard code separation within functions

#### Correct Spacing Example:
```cpp
#include "pch.h"

#include "Header.h"
#include "Header2.h"




namespace ns = std::something;





struct MyStruct
{
    int value;
};





static int g_globalVar = 0;





void Function1()
{
    Type var1;
    Type var2;

    Type var3 = value;  // Different semantic group



    // Code section
    DoSomething();
}





void Function2()
{
    // ...
}
```

### Special Code Patterns

### **NEVER** Break Column Alignment
- **NEVER** break existing column alignment in variable declarations
- **NEVER** break alignment of:
  - Type names
  - Pointer/reference symbols (`*`, `&`)
  - Variable names
  - Assignment operators (`=`)
  - Initialization values
- **ALWAYS** preserve exact column positions when replacing lines
- When modifying a line, ensure replacement maintains same indentation as original

### Indentation Rules
- **ALWAYS** preserve exact indentation when replacing code
- **NEVER** start code at column 1 unless original was at column 1
- Count spaces carefully - if original had 12 spaces, replacement must have 12 spaces
- Use spaces for indentation (match existing code style)

### Example of CORRECT editing:
```cpp
// Original:
  std::wcerr << L"Error: " << msg << L"\n";

// CORRECT replacement (preserves 12-space indentation):
            g_pConsole->Printf (CConfig::Error, L"Error: %s\n", msg);

// WRONG replacement (broken indentation):
g_pConsole->Printf (CConfig::Error, L"Error: %s\n", msg);
```

---

## File Modification Rules

### Scope of Changes
- **ONLY** modify the files explicitly requested
- If a change requires modifying other files, **ASK FIRST**
- When told to modify file X, do not make "helpful" changes to files Y or Z

---

## Code Changes - Best Practices

### When Replacing Code
1. Read the original line(s) carefully
2. Note the exact indentation level (count spaces)
3. Note any column alignment with surrounding lines
4. Apply changes while preserving formatting
5. Double-check indentation before submitting

### When Showing Code Changes
- **NEVER** show full file contents unless explicitly asked
- Use minimal, surgical edits with `// ...existing code...` comments
- Show only the lines being changed plus minimal context

### Before Applying Changes
- Verify you understand which files should/shouldn't be modified
- Check if files are from other projects (read-only)
- Confirm you're preserving all formatting rules above

---

## C++ Specific Guidelines

### Precompiled Headers
- Every `.cpp` file MUST include `"pch.h"` as its **first** `#include`
- **NEVER** use angle-bracket includes (`<header>`) anywhere except `pch.h`
- All system headers, Windows SDK headers, and STL headers belong in `pch.h`
- Individual `.cpp` and `.h` files use only quoted includes (`"header.h"`) for project headers

### Code Organization
- **Strongly prefer class members over free/global functions.** Organize helper logic as members (usually `private static`) of a cohesive class. A free or global function requires a *very convincing* justification (e.g. a genuine non-member `operator`, or a required C-ABI / entry-point symbol). When extracting helpers, give them a class home — do NOT leave them as file-local free functions.
- **Do NOT use anonymous namespaces.** Group related helpers as `static` members of a class rather than burying them in an anonymous namespace or in file-local free functions.
- Keep translation units focused: when a `.cpp` grows large or spans multiple responsibilities, split it into cohesive `.h`/`.cpp` units, one clear responsibility each.
- Shared string literals, paths, and tunables are defined ONCE as `constexpr`/`const` (in a constants header when shared across files), never duplicated at use sites.

### Smart Pointers
- Prefer `unique_ptr` for exclusive ownership
- Use `shared_ptr` when shared ownership is needed
- Consider raw pointers for non-owning references (observers)
- For signal handlers: use smart pointers for global objects that need cleanup on `std::exit()`

### Modern C++ Features
- Use `using namespace std;` in `pch.h` for convenience (project preference)
- Prefer `<filesystem>` over old file APIs
- Use `std::atomic` for thread-safe flags
- Use structured bindings where appropriate

### Error Handling
- Use custom error handling macros (EHM) when present in codebase
- Follow existing patterns: `CHR`, `CBR`, `CWRA`, `CHRF`, `CBRF`, `BAIL_OUT_IF`, etc.
- Include `Error:` labels for cleanup in functions returning `HRESULT`
- Functions should only have a single exit point.  Never directly goto Error; always use EHM macros instead.
- **EHM applies to ALL functions with failable operations**, regardless of return type (HRESULT/enum/int/struct/void). For non-HRESULT returns, keep a vestigial `HRESULT hr = S_OK;` at the top purely for the macros and return the normal result at `Error:`. For `void`, `Error:` MUST be followed by an explicit `return;` (never a lonely `;`).
- **NEVER use early `return` statements** in functions that use the EHM pattern — flatten with `BAIL_OUT_IF`/`CBR`/`CHR` instead.
- **NEVER pass a non-trivial function call as a macro argument** (EHM, assertions, etc.). Trivial accessors only (`.size()`, `.empty()`, `.c_str()`, `.load()`, member access). Capture failable/side-effecting results in a local first, then pass the local to the macro.
- `IGNORE_RETURN_VALUE`'s second argument is ALWAYS a plain reset value (`S_OK`, `false`, `0`), NEVER a call — capture the result first, then reset.
- **Prefer EHM bail-out over body-wrapping.** Use `CBR`/`CHR`/`CHRF` at the top with a jump to `Error:` rather than wrapping the body in `if (precondition) { … }`. Inside loops prefer guard-style `continue`/`break` over nested `if`.
- Default to asserting variants (`CHRA`/`CWRA`/`CBRA`/`CPRA`) for bug-indicating internal failures; use non-asserting (`CHR`/`CWR`/`CBR`/`CPR`) only for expected user/external failures.

### Variable & Member Declarations
- Declare **ALL** local variables at the **top** of the function (or top of a necessary local block). Do NOT declare at point of first use.
- Column-align sequential declarations into four columns: **type**, **pointer/reference symbol**, **name**, then **`=` / initial value**. This applies to local-variable blocks, class/struct **member** declaration blocks, and constructor initializer lists alike.
- The type column is left-aligned and padded to the **longest type** in the block. The name column is fixed for the whole block.
- If **any** line in a block has a `*` or `&`, **all** lines reserve that column — non-pointer/non-reference lines put a space placeholder there so the name and value columns stay aligned.
- Prefer in-class member initialization (in the `.h`) over constructor initializer lists for members initialized from constants. Only members bound to constructor parameters (including reference members) belong in the initializer list.

Correct member-block alignment (note the reserved `&`/`*` column and aligned `=`):
```cpp
const string   & m_input;
size_t           m_pos        = 0;
int              m_line       = 1;
bool             m_allowJsonc = false;
JsonParseError   m_error;
```

Correct local-declaration alignment:
```cpp
HRESULT          hr             = S_OK;
WAVEFORMATEX   * mixFormat      = nullptr;
WAVEFORMATEX     desiredFormat  = {};
REFERENCE_TIME   bufferDuration = 1000000;
BYTE           * buffer         = nullptr;
```

### No Unnecessary Scope Blocks
- Do NOT introduce `{ … }` blocks that aren't required by control flow or lifetime semantics. Scope blocks must serve a purpose (loop/conditional body, RAII lifetime). Hoist locals to function top instead.

### Naming
- File-scope statics use Hungarian `s_<typePrefix><Name>`. Type prefixes: `k` = constant, `psz` = null-terminated string pointer (narrow or wide), `ch` = char, `hkey` = registry key handle, `h` = other handle. E.g. `s_kpszHost`, `s_kchBullet`, `s_kRomCatalog`.
- Statics (both variables AND functions) require the `s_` prefix.
- Registry-key handle variables use the `hkey` prefix, never bare `h`.
- Be consistent with abbreviations: prefer the short canonical form (e.g. `Nf`) everywhere rather than mixing it with the spelled-out form.

### No Magic Numbers
- All numeric literals must be named constants with clear intent. Exceptions: `0`, `1`, `-1`, `nullptr`, and `sizeof` expressions.

### Spacing
- **Function-call/declaration spacing.** Space before a non-empty paren (`fn (arg)`, `Class::Method (a, b)`); **NO** space before empty parens (`fn()`, `obj.GetThing()`). Keywords taking parens (`if`, `for`, `while`, `switch`, `return`, `sizeof`) always get a space.
- **Cast spacing.** Space after a C-style cast: `(float) value`, `(Word) addr` — never `(float)value`.

### Comments
- Function and class comment blocks use 80 `/` characters as delimiters, with one empty comment line before and after the text.
- **Function documentation comments belong in the `.cpp`** inside the `////` block — NOT in the `.h`. Headers carry only terse one-liner comments (or none).
- Add inline comments only for non-obvious "why" — never restate the name of the function being called.
- No phase/task/spec references in comments.

### Wrapped Function Parameters
- **Calls** and **`.h` declarations**: wrap and align continuation parameters under the opening `(`. Within a block of `.h` declarations, align the opening-paren columns.
- **`.cpp` definitions**: the first parameter wraps to the next line (indented one level), one parameter per line, column-aligned like variable declarations.

### Function Size & Structure
- Keep functions focused and short — ideally under ~50 lines (roughly one screen)
- Aggressively factor out helper functions that do just one thing
- Avoid excessive nesting: if a function requires more than 2–3 levels of indentation beyond the EHM pattern, extract that inner logic into its own function
- Each function should have a single clear purpose

### Unit Testing — Isolation Rules
- Unit tests **MUST NEVER** rely on or alter real system state
- **ALL** system services **MUST** be mocked or abstracted behind interfaces:
  - **File system**: No reading/writing actual files on disk — use in-memory data or mock I/O
  - **Registry**: No accessing the Windows registry — mock all registry calls
  - **Network**: No real HTTP/socket calls — mock network layers
  - **Process/environment**: No inspecting real processes, environment variables, or console handles
  - **System APIs**: No calling `SHGetKnownFolderPath`, `CreateToolhelp32Snapshot`, `OpenProcessToken`, etc. directly in tests
  - **Current directory**: No depending on the current working directory — use explicit test paths
- Tests must be **deterministic** and **repeatable** regardless of the machine or user running them
- If a module uses system APIs, inject its dependencies through an interface so tests can substitute mocks
- **No test may run the real `tcdir` binary** — test the library functions directly with mocked dependencies
- Temp files are acceptable **only** in integration tests, never in unit tests

---

## Communication Rules

### When Explaining Changes
- Be concise and direct
- Explain the "why" not just the "what"
- If you make a mistake, acknowledge it immediately and clearly

### Before Major Changes
- Summarize what files will be modified
- Explain pros/cons if there are trade-offs
- Ask for confirmation if approach is unclear

### When Rules Conflict
- **Formatting rules ALWAYS take priority**
- File modification rules come second
- Code style preferences come third
- When in doubt, **ASK** before proceeding

---

## Visual Studio / Windows Specific

### Build Integration
- Always run build after making changes
- If using VS Code, always use a build task.  Only call msbuild directly if there's no build task.
- Use `get_errors` to verify specific files
- Fix compilation errors before considering task complete
- Check for both errors (C-codes) and warnings

### Pre-Commit Gates
- **ALL** tests MUST pass before committing
- Code analysis MUST be clean (zero warnings) before committing
- Build MUST succeed with no errors before committing

### Commit Frequency
- During spec implementation, commit **at least once per completed phase**
- Each commit must leave the codebase in a compilable, tests-passing state
- Do not batch an entire feature into a single commit

### Performance
- Prefer Windows Console API (`WriteConsoleW`) over C++ streams for console output
- Use large internal buffers to minimize system calls
- Flush strategically, not after every write

---

## Shell and Terminal Rules

### PowerShell is the Default Shell
- **ALL** terminal windows use PowerShell, not CMD
- **ALWAYS** format commands for PowerShell syntax

---


---

## Commit Messages

- Use [Conventional Commits](https://www.conventionalcommits.org/) format: `type(scope): description`
- **Scope is always required** — never omit it
- Types: `feat`, `fix`, `docs`, `style`, `refactor`, `perf`, `test`, `chore`, `ci`, `build`
- Use bullet list in the body for multiple changes OR additional details about the changes
- Examples:
  - `refactor(config): table-driven switches and override pipeline`
  - `fix(display): correct column alignment for wide filenames`
  - `docs(readme): update README with new command-line options`

---

## Remember
- **Formatting preservation is non-negotiable**
- **Read-only files must stay read-only**
- **When in doubt, ask before modifying**
- **Quality over speed - take time to get formatting right**

---

*Last Updated: 2026-04-20*
*These rules apply globally to all projects and conversations*

---

## Tone & Personality

- **Default to dry, lightly snarky.** Concise quips, casual asides, and
  gentle ribbing of bad ideas (including my own) are encouraged.
- **Technical accuracy is non-negotiable.** Never sacrifice correctness,
  precision, or honest uncertainty for a joke. If the punchline conflicts
  with the truth, drop the punchline.
- **Brevity beats banter.** One well-placed remark beats five mediocre
  ones. Don't pad responses to make room for humor.
- **Punch up, not down.** Snark at machines, processes, flaky tools, and
  bad code — never at the user.
- **Chat only.** This applies to interactive replies. Commit messages,
  code comments, CHANGELOG entries, README content, and other artifacts
  stay neutral and professional.

<!-- SPECKIT START -->
For additional context about technologies to be used, project structure,
shell commands, and other important information, read the current plan:
`specs/010-cross-platform-pal/plan.md` (cross-platform support, issue #8).
<!-- SPECKIT END -->
