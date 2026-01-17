# IncrementVersion.ps1
# Automatically increments the build number and updates the year in Version.h before each build

$repoRoot = Split-Path $PSScriptRoot -Parent
$versionFile = "$repoRoot\TCDirCore\Version.h"

# Check if the version file exists
if (-not (Test-Path $versionFile)) {
    Write-Error "Version file not found: $versionFile"
    exit 1
}

# Read the file content
$content = Get-Content $versionFile -Raw

# Get current year
$currentYear = (Get-Date).Year

# Find and increment the build number
if ($content -match '#define VERSION_BUILD (\d+)') {
    $buildNumber = [int]$matches[1] + 1
    $content = $content -replace '#define VERSION_BUILD \d+', "#define VERSION_BUILD $buildNumber"
    
    # Update the year
    $content = $content -replace '#define VERSION_YEAR \d+', "#define VERSION_YEAR $currentYear"
    
    # Write back to file (preserve line endings)
    Set-Content -Path $versionFile -Value $content -NoNewline
    
    Write-Host "Build number incremented to: $buildNumber" -ForegroundColor Green
    Write-Host "Year updated to: $currentYear" -ForegroundColor Green
    exit 0
} else {
    Write-Error "Could not find VERSION_BUILD in $versionFile"
    exit 1
}
