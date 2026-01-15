param(
    [ValidateSet('Debug', 'Release')]
    [string]$Configuration = 'Debug',

    [ValidateSet('x64', 'ARM64')]
    [string]$Platform = 'x64',

    [ValidateSet('Build', 'Clean', 'Rebuild', 'BuildAllRelease')]
    [string]$Target = 'Build'
)

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

$repoRoot     = $PSScriptRoot
$solutionPath = Join-Path $repoRoot 'TCDir.sln'

$toolsScript = Join-Path $repoRoot 'scripts\VSTools.ps1'
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





function Test-VSVCPlatformInstalled {
    param(
        [Parameter(Mandatory = $true)]
        [string]$MSBuildPath,
        [Parameter(Mandatory = $true)]
        [string]$Platform
    )

    $msbuildDir = Split-Path $MSBuildPath -Parent
    if (-not $msbuildDir) {
        return $false
    }

    $vsRoot = Split-Path (Split-Path (Split-Path (Split-Path $msbuildDir -Parent) -Parent) -Parent) -Parent
    if (-not $vsRoot) {
        return $false
    }

    $vcRoot = Join-Path $vsRoot 'MSBuild\Microsoft\VC'
    if (-not (Test-Path $vcRoot)) {
        return $false
    }

    $platformPaths = Get-ChildItem $vcRoot -Directory -ErrorAction SilentlyContinue |
        ForEach-Object { Join-Path $_.FullName ("Platforms\\$Platform") }

    foreach ($platformPath in $platformPaths) {
        if (Test-Path $platformPath) {
            return $true
        }
    }

    return $false
}





$scriptExitCode = 0

try {
    if ($Target -eq 'BuildAllRelease') {
        $platformsToBuild = @('x64', 'ARM64')
        if (-not (Test-VSVCPlatformInstalled -MSBuildPath $msbuildPath -Platform 'ARM64')) {
            Write-Host 'ARM64 C++ build targets not installed; skipping Release|ARM64 build.' -ForegroundColor Cyan
            Add-BuildResult -Configuration 'Release' -Platform 'ARM64' -Target 'Build' -Status 'Skipped' -Message 'ARM64 C++ build tools not installed'
            $platformsToBuild = @('x64')
        }

        foreach ($platformToBuild in $platformsToBuild) {
            $msbuildArgs = @(
                $solutionPath,
                "-p:Configuration=Release",
                "-p:Platform=$platformToBuild"
            )

            Write-Host "Using MSBuild: $msbuildPath"
            Write-Host "Building: $solutionPath (Release|$platformToBuild) Target=Build"

            $stopwatch = [System.Diagnostics.Stopwatch]::StartNew()
            & $msbuildPath @msbuildArgs
            $stopwatch.Stop()

            if ($LASTEXITCODE -ne 0) {
                Add-BuildResult -Configuration 'Release' -Platform $platformToBuild -Target 'Build' -Status 'Failed' -ExitCode $LASTEXITCODE -Duration $stopwatch.Elapsed
                $scriptExitCode = $LASTEXITCODE
                break
            }

            Add-BuildResult -Configuration 'Release' -Platform $platformToBuild -Target 'Build' -Status 'Succeeded' -Duration $stopwatch.Elapsed
        }

        if ($scriptExitCode -ne 0) {
            foreach ($platformToBuild in $platformsToBuild) {
                $existing = $script:BuildResults | Where-Object { $_.Configuration -eq 'Release' -and $_.Platform -eq $platformToBuild -and $_.Target -eq 'Build' } | Select-Object -First 1
                if (-not $existing) {
                    Add-BuildResult -Configuration 'Release' -Platform $platformToBuild -Target 'Build' -Status 'Skipped' -Message 'Skipped due to previous failure'
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

        $msbuildArgs = @(
            $solutionPath,
            "-p:Configuration=$Configuration",
            "-p:Platform=$Platform"
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
