<#
.SYNOPSIS
    Builds the TCDir solution using MSBuild.

.DESCRIPTION
    This script builds the TCDir.sln solution using Visual Studio 2026's MSBuild.
    It supports building for x64 and ARM64 platforms in Debug or Release configurations.
    The script automatically detects the current architecture when using -Platform Auto.

.PARAMETER Configuration
    The build configuration. Valid values are 'Debug' or 'Release'.
    Default: Debug

.PARAMETER Platform
    The target platform. Valid values are 'x64', 'ARM64', or 'Auto'.
    'Auto' detects the current OS architecture.
    Default: Auto

.PARAMETER Target
    The build target. Valid values are:
      - Build            Build the solution (default)
      - Clean            Clean build outputs
      - Rebuild          Clean and rebuild
      - BuildAllRelease  Build Release for all platforms (x64 and ARM64)
      - CleanAll         Clean all configurations and platforms
      - RebuildAllRelease  Rebuild Release for all platforms
    Default: Build

.EXAMPLE
    .\Build.ps1
    Builds Debug configuration for the current architecture.

.EXAMPLE
    .\Build.ps1 -Configuration Release -Platform x64
    Builds Release configuration for x64.

.EXAMPLE
    .\Build.ps1 -Target BuildAllRelease
    Builds Release configuration for both x64 and ARM64 platforms.

.EXAMPLE
    .\Build.ps1 -Target Clean
    Cleans the build outputs for the current configuration and platform.

.NOTES
    Requires Visual Studio 2026 (v18.x) with the "Desktop development with C++" workload.
    ARM64 builds require the MSVC ARM64 build tools to be installed.
#>
param(
    [ValidateSet('Debug', 'Release')]
    [string]$Configuration = 'Debug',

    [ValidateSet('x64', 'ARM64', 'Auto')]
    [string]$Platform = 'Auto',

    [ValidateSet('Build', 'Clean', 'Rebuild', 'BuildAllRelease', 'CleanAll', 'RebuildAllRelease')]
    [string]$Target = 'Build'
)

# Resolve 'Auto' platform to actual architecture
if ($Platform -eq 'Auto') {
    if ([System.Runtime.InteropServices.RuntimeInformation]::OSArchitecture -eq [System.Runtime.InteropServices.Architecture]::Arm64) {
        $Platform = 'ARM64'
    } else {
        $Platform = 'x64'
    }
}

$ErrorActionPreference = 'Stop'





$script:BuildResults = @()





function Add-BuildResult {
    param(
        [Parameter(Mandatory = $true)]
        [string]$Configuration,
        [Parameter(Mandatory = $true)]
        [string]$Platform,
        [Parameter(Mandatory = $true)]
        [string]$Target,
        [Parameter(Mandatory = $true)]
        [ValidateSet('Succeeded', 'Failed', 'Skipped', 'Warning')]
        [string]$Status,
        [int]$ExitCode = 0,
        [TimeSpan]$Duration,
        [string]$Message
    )

    $script:BuildResults += [PSCustomObject]@{
        Configuration = $Configuration
        Platform      = $Platform
        Target        = $Target
        Status        = $Status
        ExitCode      = $ExitCode
        Duration      = $Duration
        Message       = $Message
    }
}





function Write-BuildSummary {
    if (-not $script:BuildResults -or $script:BuildResults.Count -eq 0) {
        return
    }

    Write-Host ''
    Write-Host 'SUMMARY' -ForegroundColor White

    foreach ($r in $script:BuildResults) {
        $statusText = $r.Status.ToUpperInvariant()
        $label      = "{0}|{1} {2}" -f $r.Configuration, $r.Platform, $r.Target
        $timeText   = ''
        $details    = ''

        if ($r.Duration -and $r.Duration -gt [TimeSpan]::Zero -and $r.Status -ne 'Skipped') {
            $minutes  = [int][Math]::Floor($r.Duration.TotalMinutes)
            $timeText = " ({0:00}:{1:00}.{2:000})" -f $minutes, $r.Duration.Seconds, $r.Duration.Milliseconds
        }

        if ($r.Message) {
            $details = " - {0}" -f $r.Message
        }
        elseif ($r.ExitCode -ne 0) {
            $details = " - ExitCode {0}" -f $r.ExitCode
        }

        $line = "{0,-20} {1}{2}{3}" -f $label, $statusText, $timeText, $details

        switch ($r.Status) {
            'Succeeded' { Write-Host $line -ForegroundColor Green }
            'Failed'    { Write-Host $line -ForegroundColor Red }
            'Warning'   { Write-Host $line -ForegroundColor Yellow }
            'Skipped'   { Write-Host $line -ForegroundColor Cyan }
            default     { Write-Host $line }
        }
    }
}

$repoRoot     = Split-Path $PSScriptRoot -Parent
$solutionPath = Join-Path $repoRoot 'TCDir.sln'

$toolsScript = Join-Path $PSScriptRoot 'VSTools.ps1'
if (-not (Test-Path $toolsScript)) {
    throw "Tool helper script not found: $toolsScript"
}

. $toolsScript

if (-not (Test-Path $solutionPath)) {
    throw "Solution not found: $solutionPath"
}

$msbuildPath = Get-VS2026MSBuildPath
if (-not $msbuildPath) {
    $msbuildPath = Get-VS2026MSBuildPath -IncludePrerelease
}

if (-not $msbuildPath) {
    throw 'VS 2026 (v18.x) MSBuild not found (via vswhere). Install VS 2026 with MSBuild.'
}





$scriptExitCode = 0

try {
    if ($Target -eq 'BuildAllRelease' -or $Target -eq 'CleanAll' -or $Target -eq 'RebuildAllRelease') {
        $platformsToBuild = @('x64', 'ARM64')
        $arm64Installed = Test-VSVCPlatformInstalled -MSBuildPath $msbuildPath -Platform 'ARM64'

        if (-not $arm64Installed) {
            Write-Host 'ARM64 C++ build targets not installed; skipping ARM64.' -ForegroundColor Cyan
            $platformsToBuild = @('x64')
        }

        # Determine configurations based on target
        if ($Target -eq 'CleanAll') {
            $configsToBuild = @('Debug', 'Release')
            $msbuildTarget = 'Clean'
        }
        elseif ($Target -eq 'RebuildAllRelease') {
            $configsToBuild = @('Release')
            $msbuildTarget = 'Rebuild'
        }
        else {
            # BuildAllRelease
            $configsToBuild = @('Release')
            $msbuildTarget = 'Build'
        }

        # Determine preferred tool architecture based on OS
        $preferredArch = 'x64'
        if ([System.Runtime.InteropServices.RuntimeInformation]::OSArchitecture -eq [System.Runtime.InteropServices.Architecture]::Arm64) {
            $preferredArch = 'ARM64'
        }

        # Add skipped results for ARM64 if not installed
        if (-not $arm64Installed) {
            foreach ($config in $configsToBuild) {
                Add-BuildResult -Configuration $config -Platform 'ARM64' -Target $msbuildTarget -Status 'Skipped' -Message 'ARM64 C++ build tools not installed'
            }
        }

        foreach ($config in $configsToBuild) {
            foreach ($platformToBuild in $platformsToBuild) {
                $msbuildArgs = @(
                    $solutionPath,
                    "-p:Configuration=$config",
                    "-p:Platform=$platformToBuild",
                    "-p:PreferredToolArchitecture=$preferredArch",
                    "-t:$msbuildTarget"
                )

                Write-Host "Using MSBuild: $msbuildPath"
                Write-Host "Building: $solutionPath ($config|$platformToBuild) Target=$msbuildTarget"

                $stopwatch = [System.Diagnostics.Stopwatch]::StartNew()
                & $msbuildPath @msbuildArgs
                $stopwatch.Stop()

                if ($LASTEXITCODE -ne 0) {
                    Add-BuildResult -Configuration $config -Platform $platformToBuild -Target $msbuildTarget -Status 'Failed' -ExitCode $LASTEXITCODE -Duration $stopwatch.Elapsed
                    $scriptExitCode = $LASTEXITCODE
                    break
                }

                Add-BuildResult -Configuration $config -Platform $platformToBuild -Target $msbuildTarget -Status 'Succeeded' -Duration $stopwatch.Elapsed
            }

            if ($scriptExitCode -ne 0) {
                break
            }
        }

        if ($scriptExitCode -ne 0) {
            foreach ($config in $configsToBuild) {
                foreach ($platformToBuild in $platformsToBuild) {
                    $existing = $script:BuildResults |
                        Where-Object { $_.Configuration -eq $config -and $_.Platform -eq $platformToBuild -and $_.Target -eq $msbuildTarget } |
                        Select-Object -First 1

                    if (-not $existing) {
                        Add-BuildResult -Configuration $config -Platform $platformToBuild -Target $msbuildTarget -Status 'Skipped' -Message 'Skipped due to previous failure'
                    }
                }
            }
        }
    }
    else {
        if ($Platform -eq 'ARM64') {
            if (-not (Test-VSVCPlatformInstalled -MSBuildPath $msbuildPath -Platform 'ARM64')) {
                Add-BuildResult -Configuration $Configuration -Platform $Platform -Target $Target -Status 'Failed' -ExitCode 1 -Message 'ARM64 C++ build tools not installed'
                throw 'ARM64 C++ build targets are not installed in this Visual Studio instance. Install the MSVC ARM64 build tools (Desktop development with C++), or build x64 instead.'
            }
        }

        # Determine preferred tool architecture based on OS
        $preferredArch = 'x64'
        if ([System.Runtime.InteropServices.RuntimeInformation]::OSArchitecture -eq [System.Runtime.InteropServices.Architecture]::Arm64) {
            $preferredArch = 'ARM64'
        }

        $msbuildArgs = @(
            $solutionPath,
            "-p:Configuration=$Configuration",
            "-p:Platform=$Platform",
            "-p:PreferredToolArchitecture=$preferredArch"
        )

        if ($Target -ne 'Build') {
            $msbuildArgs += "-t:$Target"
        }

        Write-Host "Using MSBuild: $msbuildPath"
        Write-Host "Building: $solutionPath ($Configuration|$Platform) Target=$Target"

        $stopwatch = [System.Diagnostics.Stopwatch]::StartNew()
        & $msbuildPath @msbuildArgs
        $stopwatch.Stop()
        if ($LASTEXITCODE -ne 0) {
            Add-BuildResult -Configuration $Configuration -Platform $Platform -Target $Target -Status 'Failed' -ExitCode $LASTEXITCODE -Duration $stopwatch.Elapsed
            $scriptExitCode = $LASTEXITCODE
        }
        else {
            Add-BuildResult -Configuration $Configuration -Platform $Platform -Target $Target -Status 'Succeeded' -Duration $stopwatch.Elapsed
        }
    }
}
catch {
    if ($scriptExitCode -eq 0) {
        $scriptExitCode = 1
    }

    Write-Host $_ -ForegroundColor Red
}
finally {
    Write-BuildSummary
}

exit $scriptExitCode
