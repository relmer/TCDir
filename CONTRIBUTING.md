# Contributing to TCDir

## Commit Messages

This project uses [Conventional Commits](https://www.conventionalcommits.org/). Please format your commit messages as:

```
<type>(<scope>): <description>

[optional body]

[optional footer(s)]
```

### Types

| Type       | Description                                      |
|------------|--------------------------------------------------|
| `feat`     | New feature                                      |
| `fix`      | Bug fix                                          |
| `docs`     | Documentation only                               |
| `style`    | Formatting, no code change                       |
| `refactor` | Code change that neither fixes nor adds feature  |
| `perf`     | Performance improvement                          |
| `test`     | Adding or fixing tests                           |
| `chore`    | Build process, tooling, dependencies             |
| `ci`       | CI/CD changes                                    |
| `build`    | Build system changes                             |

### Examples

```
feat(cloud): add sync root detection for cloud status display
fix(display): correct column alignment for wide filenames
docs: update README with new command-line options
chore(build): skip ARM64 tests on x64 hosts
```

## Building

Requires Visual Studio 2026 (v18.x) with "Desktop development with C++" workload.

```powershell
# Build Debug for current architecture
.\scripts\Build.ps1

# Build Release for all platforms
.\scripts\Build.ps1 -Target BuildAllRelease

# Run tests
.\scripts\RunTests.ps1
```

Or use the VS Code tasks (Ctrl+Shift+B).

## Code Style

- See [.github/copilot-instructions.md](.github/copilot-instructions.md) for detailed formatting rules
- Every `.cpp` file must include `"pch.h"` as its first `#include`
- Use quoted includes (`"header.h"`) for project headers; system headers go in `pch.h`
- Preserve existing indentation and column alignment
- Functions should have a single exit point (use EHM macros)

## Pull Requests

1. Create a feature branch from `master`
2. Make your changes with conventional commit messages
3. Ensure all tests pass (`.\scripts\RunTests.ps1`)
4. Update CHANGELOG.md if adding user-visible changes
5. Submit PR with clear description of changes
