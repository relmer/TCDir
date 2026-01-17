# TCDir

TCDir ("Technicolor Directory") is a fast, colorized directory listing tool for Windows consoles.
It’s designed as a practical `dir`-style command with useful defaults (color by extension/attributes, sorting, recursion, wide output, and a multi-threaded enumerator).
![TCDir basic listing](Assets/TCDir.png)
Hat tip to [Chris Kirmse](https://github.com/ckirmse) whose excellent [ZDir](https://github.com/ckirmse/ZDir) from the '90s was the original inspiration for TCDir.

## Requirements

- Windows 10/11
- PowerShell 7 (`pwsh`) to run the build/test scripts
- Visual Studio 2026 (v18.x)
  - The free Community edition is fine, but any edition will work
  - Workload: **Desktop development with C++**
  - Individual components (usually included with the workload, but worth confirming):
    - **MSVC v14x x64/x86 build tools**
    - **Windows 10/11 SDK**
    - **C++ unit test framework** (to build/run the `UnitTest` project)
    - Optional (ARM64 builds/tests): **MSVC v14x ARM64 build tools**
  - The scripts use `vswhere.exe` to locate MSBuild and the test runner
- Optional: VS Code (the repo includes `.vscode/` tasks wired up to `Build.ps1` and `RunTests.ps1`)

## Quick start

Build:

- Visual Studio: open `TCDir.sln` and **Build Solution**
- VS Code: run a build task (e.g. **Build Release x64 (no test)**)
- Command line: `pwsh -NoProfile -ExecutionPolicy Bypass -File .\Build.ps1 -Configuration Release -Platform x64 -Target Build`

Run:

- `.\x64\Release\TCDir.exe`

## Usage

Show help:

- `TCDir.exe /?`

![TCDir help](Assets/TCDir%20Help.png)

Basic syntax:

- `TCDIR [drive:][path][filename] [/A[[:]attributes]] [/O[[:]sortorder]] [/S] [/W] [/P] [/M] [/Env] [/Config]`

Common switches:

- `/A[:]<attributes>`: filter by file attributes
- `/O[:]<sortorder>`: sort results
  - both `/oe` and `/o:e` forms are supported
  - `N` name, `E` extension, `S` size, `D` date/time
  - prefix `-` to reverse
- `/S`: recurse into subdirectories
- `/W`: wide listing format
- `/P`: show performance timing information
- `/M`: enable multi-threaded enumeration (default); use `/M-` to disable
- `/Env`: show `TCDIR` environment variable help/syntax/current value
- `/Config`: show current color configuration

Examples:

- List a folder: `TCDir.exe C:\src`
- Recurse and sort by extension: `TCDir.exe C:\src /s /o:e`
- Wide listing: `TCDir.exe /w`

![TCDir wide listing](Assets/TCDir%20Wide.png)

![TCDir recursive listing](Assets/TCDir%20Subdirectories.png)

![TCDir recursive wide listing](Assets/TCDir%20Subdirectories%20Wide.png)

## Configuration (TCDIR environment variable)

TCDir supports customizing colors (and default switch behavior) via the `TCDIR` environment variable.

Syntax:

- CMD: `set TCDIR=[ -<Switch> | /<Switch>] | [<Item> | Attr:<fileattr> | <.ext>] = <Fore> [on <Back>][;...]`
- PowerShell: `$env:TCDIR = "[ -<Switch> | /<Switch>] | [<Item> | Attr:<fileattr> | <.ext>] = <Fore> [on <Back>][;...]"`

Example:

- CMD: `set TCDIR=-W;D=LightGreen;S=Yellow;Attr:H=DarkGrey;.cpp=White on Blue`
- PowerShell: `$env:TCDIR = "-W;D=LightGreen;S=Yellow;Attr:H=DarkGrey;.cpp=White on Blue"`

Decoded breakdown of the example:

- `-W` sets the default switch `/W` (wide listing) on
- `D=LightGreen` sets the **Date** display item color to LightGreen
- `S=Yellow` sets the **Size** display item color to Yellow
- `Attr:H=DarkGrey` sets the **Hidden** file attribute color to DarkGrey
- `.cpp=White on Blue` sets the `.cpp` extension color to White text on a Blue background

![TCDir with TCDIR environment variable](Assets/TCDir%20Env%20Variable.png)

To see the full list of supported colors and a nicely formatted explanation, run:

- `TCDir.exe /Env`

![TCDir /Env help](Assets/TCDir%20Env.png)

To see your current color configuration:

- `TCDir.exe /Config`

![TCDir /Config output](Assets/TCDir%20Config.png)

## Building

### Build options

- Visual Studio: open `TCDir.sln` and **Build Solution**
- VS Code: the repo includes `.vscode/` (tasks/launch/settings), with tasks wired up to `Build.ps1` and `RunTests.ps1`
- Command line: use the PowerShell build scripts below

### Build scripts

- Build: `pwsh -File .\Build.ps1 -Configuration <Debug|Release> -Platform <x64|ARM64> -Target Build`
- Clean: `pwsh -File .\Build.ps1 -Configuration <Debug|Release> -Platform <x64|ARM64> -Target Clean`
- Rebuild: `pwsh -File .\Build.ps1 -Configuration <Debug|Release> -Platform <x64|ARM64> -Target Rebuild`
- Build both Release targets: `pwsh -File .\Build.ps1 -Target BuildAllRelease`

Build outputs land under:

- `x64\Debug\TCDir.exe`, `x64\Release\TCDir.exe`
- `ARM64\Debug\TCDir.exe`, `ARM64\Release\TCDir.exe`

## Tests

Run unit tests:

- `pwsh -File .\RunTests.ps1`

(Uses Visual Studio’s `vstest.console.exe`, discovered via `vswhere`.)

## Versioning

The build number is auto-incremented by a pre-build script. Details are in BUILD_VERSION.md.

## License

MIT License. See `LICENSE`.
