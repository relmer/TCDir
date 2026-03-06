<#
.SYNOPSIS
    Updates local winget manifest templates from a published GitHub release.

.DESCRIPTION
    Reads the version from Version.h, downloads the release assets from
    GitHub, computes SHA256 hashes, and updates the three winget manifest
    files (version, locale, installer) with the correct version, URLs,
    and hashes.

.PARAMETER Tag
    The release tag to use (e.g., v5.1.1149). If omitted, derives
    the tag from Version.h.

.PARAMETER DryRun
    Show what would be updated without writing files.

.EXAMPLE
    .\scripts\UpdateWingetManifest.ps1
    .\scripts\UpdateWingetManifest.ps1 -Tag v5.1.1149
    .\scripts\UpdateWingetManifest.ps1 -DryRun
#>

[CmdletBinding()]
param(
    [string]$Tag,
    [switch]$DryRun
)

$ErrorActionPreference = 'Stop'

$repoRoot   = Split-Path $PSScriptRoot -Parent
$wingetDir  = Join-Path $repoRoot 'winget'
$versionFile = Join-Path $repoRoot 'TCDirCore\Version.h'

$owner = 'relmer'
$repo  = 'TCDir'

# --- Read version from Version.h ---

if (-not $Tag) {
    $content = Get-Content $versionFile -Raw
    $major = [regex]::Match($content, '#define VERSION_MAJOR\s+(\d+)').Groups[1].Value
    $minor = [regex]::Match($content, '#define VERSION_MINOR\s+(\d+)').Groups[1].Value
    $build = [regex]::Match($content, '#define VERSION_BUILD\s+(\d+)').Groups[1].Value

    if (-not $major -or -not $minor -or -not $build) {
        Write-Error "Could not parse version from $versionFile"
        exit 1
    }

    $version = "$major.$minor.$build"
    $Tag = "v$version"
} else {
    if ($Tag -notmatch '^v(\d+\.\d+\.\d+)$') {
        Write-Error "Invalid tag format: $Tag (expected v#.#.#)"
        exit 1
    }
    $version = $Matches[1]
}

Write-Host "Version: $version  Tag: $Tag" -ForegroundColor Cyan

# --- Verify the release exists on GitHub ---

$releaseUrl = "https://api.github.com/repos/$owner/$repo/releases/tags/$Tag"
Write-Host "Checking release: $releaseUrl"

try {
    $release = Invoke-RestMethod -Uri $releaseUrl -Headers @{ Accept = 'application/vnd.github.v3+json' }
} catch {
    Write-Error "Release not found for tag $Tag. Has the release workflow completed?"
    exit 1
}

Write-Host "Release found: $($release.name)" -ForegroundColor Green

# --- Download assets and compute hashes ---

$baseUrl = "https://github.com/$owner/$repo/releases/download/$Tag"
$assets = @(
    @{ Name = 'TCDir.exe';       Url = "$baseUrl/TCDir.exe";       Arch = 'x64'   },
    @{ Name = 'TCDir-ARM64.exe'; Url = "$baseUrl/TCDir-ARM64.exe"; Arch = 'arm64' }
)

$tempDir = Join-Path ([System.IO.Path]::GetTempPath()) "winget-update-$Tag"
New-Item -ItemType Directory -Path $tempDir -Force | Out-Null

$hashes = @{}

foreach ($asset in $assets) {
    $dest = Join-Path $tempDir $asset.Name
    Write-Host "Downloading $($asset.Name)..."
    Invoke-WebRequest -Uri $asset.Url -OutFile $dest -UseBasicParsing

    $hash = (Get-FileHash $dest -Algorithm SHA256).Hash.ToUpper()
    $hashes[$asset.Arch] = $hash
    Write-Host "  SHA256: $hash" -ForegroundColor DarkGray
}

# Clean up temp files
Remove-Item $tempDir -Recurse -Force -ErrorAction SilentlyContinue

# --- Update manifest files ---

$versionYaml  = Join-Path $wingetDir "$owner.$repo.yaml"
$localeYaml   = Join-Path $wingetDir "$owner.$repo.locale.en-US.yaml"
$installerYaml = Join-Path $wingetDir "$owner.$repo.installer.yaml"

function Update-YamlField {
    param(
        [string]$Content,
        [string]$Field,
        [string]$Value
    )
    $Content -replace "(?m)^($Field):\s+.*$", "`$1: $Value"
}

function Update-InstallerHash {
    param(
        [string]$Content,
        [string]$Arch,
        [string]$Url,
        [string]$Hash
    )
    # Match the architecture block and update URL + hash
    $pattern = "(?ms)(- Architecture: $Arch\s+InstallerUrl:)\s+\S+(\s+InstallerSha256:)\s+\S+"
    $replacement = "`$1 $Url`$2 $Hash"
    $Content -replace $pattern, $replacement
}

# --- version.yaml ---
$content = Get-Content $versionYaml -Raw
$content = Update-YamlField $content 'PackageVersion' $version

if ($DryRun) {
    Write-Host "`n--- $versionYaml ---" -ForegroundColor Yellow
    Write-Host $content
} else {
    Set-Content $versionYaml $content -NoNewline
    Write-Host "Updated: $versionYaml" -ForegroundColor Green
}

# --- locale.yaml ---
$content = Get-Content $localeYaml -Raw
$content = Update-YamlField $content 'PackageVersion' $version
$content = Update-YamlField $content 'ReleaseNotesUrl' "https://github.com/$owner/$repo/releases/tag/$Tag"

if ($DryRun) {
    Write-Host "`n--- $localeYaml ---" -ForegroundColor Yellow
    Write-Host $content
} else {
    Set-Content $localeYaml $content -NoNewline
    Write-Host "Updated: $localeYaml" -ForegroundColor Green
}

# --- installer.yaml ---
$content = Get-Content $installerYaml -Raw
$content = Update-YamlField $content 'PackageVersion' $version
$content = Update-InstallerHash $content 'x64'   "$baseUrl/TCDir.exe"       $hashes['x64']
$content = Update-InstallerHash $content 'arm64' "$baseUrl/TCDir-ARM64.exe" $hashes['arm64']

if ($DryRun) {
    Write-Host "`n--- $installerYaml ---" -ForegroundColor Yellow
    Write-Host $content
} else {
    Set-Content $installerYaml $content -NoNewline
    Write-Host "Updated: $installerYaml" -ForegroundColor Green
}

Write-Host "`nDone. Run 'winget validate $wingetDir' to verify." -ForegroundColor Cyan
