# Global Copilot Instructions for relmer

## Code Formatting - CRITICAL RULES

### **NEVER** Delete Blank Lines
- **NEVER** delete blank lines between file-level constructs (functions, classes, structs)
- **NEVER** delete blank lines between different groups (e.g., C++ includes vs C includes)
- **NEVER** delete blank lines between variable declaration blocks
- Preserve all existing vertical spacing in code

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
- Follow existing patterns: `CHR`, `CBR`, `CWRA`, etc.
- Include `Error:` labels for cleanup in functions returning `HRESULT`
- Functions should only have a single exit point.  Never directly goto Error; always use EHM macros instead.

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

### Performance
- Prefer Windows Console API (`WriteConsoleW`) over C++ streams for console output
- Use large internal buffers to minimize system calls
- Flush strategically, not after every write

---

## Remember
- **Formatting preservation is non-negotiable**
- **Read-only files must stay read-only**
- **When in doubt, ask before modifying**
- **Quality over speed - take time to get formatting right**

---

*Last Updated: 2025-01-XX*
*These rules apply globally to all projects and conversations*
