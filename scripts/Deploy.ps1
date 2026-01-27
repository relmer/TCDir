<#
.SYNOPSIS
    Deploys release builds to a destination folder.

.DESCRIPTION
    Copies release executables to a deployment folder specified by the
    TCDIR_DEPLOY_PATH environment variable. The ARM64 binary is renamed
    to TCDir_ARM64.exe to distinguish it from the x64 version.

.PARAMETER Force
    Overwrite existing files without prompting.

.EXAMPLE
    # First, set the environment variable (one-time setup):
    [Environment]::SetEnvironmentVariable('TCDIR_DEPLOY_PATH', 'C:\Path\To\Utils', 'User')
    
    # Then run the script:
    .\scripts\Deploy.ps1

.NOTES
    The TCDIR_DEPLOY_PATH environment variable must be set to the destination folder.
    This keeps machine-specific paths out of the repository.
#>

[CmdletBinding()]
param(
    [switch]$Force
)

$ErrorActionPreference = 'Stop'

$repoRoot = Split-Path $PSScriptRoot -Parent

# Get deployment path from environment variable
$deployPath = [Environment]::GetEnvironmentVariable('TCDIR_DEPLOY_PATH', 'User')
if (-not $deployPath) {
    $deployPath = [Environment]::GetEnvironmentVariable('TCDIR_DEPLOY_PATH', 'Process')
}

if (-not $deployPath) {
    Write-Error @"
TCDIR_DEPLOY_PATH environment variable is not set.

To set it (one-time setup), run:
    [Environment]::SetEnvironmentVariable('TCDIR_DEPLOY_PATH', 'C:\Your\Deploy\Path', 'User')

Then restart your terminal and run this script again.
"@
    exit 1
}

if (-not (Test-Path $deployPath)) {
    Write-Error "Deployment path does not exist: $deployPath"
    exit 1
}

# Define source and destination mappings
$deployments = @(
    @{
        Source      = Join-Path $repoRoot 'x64\Release\TCDir.exe'
        Destination = Join-Path $deployPath 'TCDir.exe'
        Description = 'x64 Release'
    },
    @{
        Source      = Join-Path $repoRoot 'ARM64\Release\TCDir.exe'
        Destination = Join-Path $deployPath 'TCDir_ARM64.exe'
        Description = 'ARM64 Release'
    }
)

$copied = 0
$skipped = 0

foreach ($deploy in $deployments) {
    $src = $deploy.Source
    $dst = $deploy.Destination
    $desc = $deploy.Description

    if (-not (Test-Path $src)) {
        Write-Warning "$desc not found: $src (skipping)"
        $skipped++
        continue
    }

    $srcInfo = Get-Item $src
    $copyNeeded = $true

    if ((Test-Path $dst) -and -not $Force) {
        $dstInfo = Get-Item $dst
        if ($srcInfo.LastWriteTime -le $dstInfo.LastWriteTime) {
            Write-Host "$desc is up to date: $(Split-Path $dst -Leaf)" -ForegroundColor DarkGray
            $copyNeeded = $false
            $skipped++
        }
    }

    if ($copyNeeded) {
        Copy-Item -Path $src -Destination $dst -Force
        Write-Host "$desc deployed: $(Split-Path $dst -Leaf)" -ForegroundColor Green
        $copied++
    }
}

Write-Host ""
Write-Host "Deployment complete: $copied copied, $skipped skipped" -ForegroundColor Cyan
