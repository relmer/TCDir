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
docs(readme): update README with new command-line options
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

## Development Workflow

### GitHub Issues

All features, enhancements, and non-trivial bugs are tracked as GitHub issues. Issues feed into the sync table in `specs/sync-status.md` to ensure TCDir changes are also ported to RCDir.

### Feature Lifecycle

1. **Issue** — create or identify a GitHub issue (`gh issue create`)
2. **Spec branch** — create a feature branch via `.specify` tooling (e.g., `007-symlink-junction-targets`)
3. **Spec** — write the feature specification in `specs/<###-feature>/spec.md`, referencing the GH issue
4. **Plan & tasks** — generate implementation plan and task list via speckit
5. **Implement** — work through tasks, committing per phase, tests passing before each commit
6. **Merge** — merge to master with `Closes #N` in the merge commit to auto-close the issue
7. **Tag & release** — push an annotated tag (e.g., `v5.4.1499`) to trigger the release workflow
8. **Sync table** — update `specs/sync-status.md` with the new row (including GH issue number)
9. **Port to RCDir** — copy spec to RCDir, plan, implement, update sync table when shipped

### Lightweight Changes (no spec)

Bug fixes and small enhancements that don't warrant a full spec still get:
- A GitHub issue
- A row in `specs/sync-status.md` (no spec number, just the issue link)
- `Closes #N` in the commit that ships the fix

### Version & Build Numbers

- `TCDirCore/Version.h` is auto-incremented by the pre-build script on every build
- Always include `Version.h` in commits after building
- Bump the minor version at the start of a new feature (e.g., 5.3 → 5.4)
- Preserve existing indentation and column alignment
- Functions should have a single exit point (use EHM macros)

## Pull Requests

1. Create a feature branch from `master`
2. Make your changes with conventional commit messages
3. Ensure all tests pass (`.\scripts\RunTests.ps1`)
4. Update CHANGELOG.md if adding user-visible changes
5. Submit PR with clear description of changes
