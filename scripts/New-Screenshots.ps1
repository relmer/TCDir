[CmdletBinding()]
param(
    [string]$TcDirPath  = (Join-Path (Split-Path $PSScriptRoot -Parent) 'x64\Release\TCDir.exe'),
    [string]$OutputPath = (Join-Path (Split-Path $PSScriptRoot -Parent) 'Assets'),
    [int]$Columns       = 120,
    [string[]]$Only
)

#
# Regenerates the README screenshots.
#
# Each shot runs tcdir against a generated demo folder, captures the colored
# output, converts it to HTML with the Windows Terminal "Campbell" colors and
# the CaskaydiaCove Nerd Font, and renders the page to PNG with headless
# Microsoft Edge.
#
# The demo folder is mapped to a drive letter with subst, so no personal path
# appears in a screenshot.  TCDIR is cleared and USERPROFILE points at a
# folder with a sample .tcdirconfig, so the output doesn't depend on your
# settings.
#
# Use -Only to regenerate some shots, for example -Only 'TCDir Info'.
#

$ErrorActionPreference = 'Stop'

$script:RepoRoot   = Split-Path $PSScriptRoot -Parent
$script:SampleTime = [datetime]::new(2026, 10, 14, 9, 30, 0, [DateTimeKind]::Local)
$script:FontFamily = "'CaskaydiaCove Nerd Font', 'CaskaydiaCove NF', 'Cascadia Code', Consolas, monospace"
$script:FontSize   = 16
$script:LineHeight = 1.25
$script:CharWidth  = 0.586     # Cascadia advance width in em
$script:Padding    = 16
$script:Background = '#0C0C0C'

$script:Palette = @{
    30 = '#0C0C0C'; 31 = '#C50F1F'; 32 = '#13A10E'; 33 = '#C19C00'
    34 = '#0037DA'; 35 = '#881798'; 36 = '#3A96DD'; 37 = '#CCCCCC'
    90 = '#767676'; 91 = '#E74856'; 92 = '#16C60C'; 93 = '#F9F1A5'
    94 = '#3B78FF'; 95 = '#B4009E'; 96 = '#61D6D6'; 97 = '#F2F2F2'
}

$script:SampleEnv = 'D=LightGreen;S=Yellow;.md=LightMagenta,U+F48A;.png=Black on Magenta'





#
# Demo files are copies of real files from this repository, so sizes, icons
# and compression ratios match real projects.  A value that starts with '*'
# is generated text instead of a copy.
#

function Copy-DemoFiles {
    param([string]$Path, [System.Collections.Specialized.OrderedDictionary]$Files)

    $offset = 0
    foreach ($rel in $Files.Keys) {
        $full   = Join-Path $Path $rel
        $source = $Files[$rel]
        New-Item -ItemType Directory -Force (Split-Path $full -Parent) | Out-Null

        switch ($source) {
            '*csv'   { Set-Content -Path $full -Value (Get-SampleCsv) -Encoding utf8 }
            '*notes' { Set-Content -Path $full -Value (Get-SampleNotes) -Encoding utf8 }
            default  {
                $from = if ([System.IO.Path]::IsPathRooted($source)) { $source } else { Join-Path $script:RepoRoot $source }
                Copy-Item $from $full
            }
        }

        (Get-Item $full -Force).LastWriteTime = $script:SampleTime.AddDays(-$offset).AddMinutes(-37 * $offset)
        $offset++
    }

    Get-ChildItem $Path -Recurse -Directory -Force | ForEach-Object { $_.LastWriteTime = $script:SampleTime }
}





function Get-SampleCsv {
    $random = [System.Random]::new(42)
    $lines  = [System.Collections.Generic.List[string]]::new()
    $lines.Add('date,region,product,units,revenue')

    foreach ($day in 0..119) {
        foreach ($region in 'North', 'South', 'East', 'West') {
            $units = $random.Next(5, 400)
            $lines.Add(('{0:yyyy-MM-dd},{1},{2},{3},{4:F2}' -f $script:SampleTime.AddDays(-$day), $region,
                        ('Widget', 'Gadget', 'Gizmo')[$random.Next(3)], $units, $units * 19.95))
        }
    }

    return $lines
}





function Get-SampleNotes {
    return @(
        'Release checklist'
        ''
        '- Bump the version in TCDirCore\Version.h'
        '- Update CHANGELOG.md and the README What''s New table'
        '- Build x64 and ARM64 Release'
        '- Run the unit tests'
        '- Regenerate the README screenshots'
        '- Sign the binaries and publish the release'
    )
}





function New-DemoZip {
    param([string]$ZipPath, [System.Collections.Specialized.OrderedDictionary]$Entries)

    Add-Type -AssemblyName System.IO.Compression
    Add-Type -AssemblyName System.IO.Compression.FileSystem

    $zip = [System.IO.Compression.ZipFile]::Open($ZipPath, 'Create')

    try {
        $offset = 0
        foreach ($entryName in $Entries.Keys) {
            $entry               = $zip.CreateEntry($entryName, 'Optimal')
            $entry.LastWriteTime = $script:SampleTime.AddDays(-1 - $offset).AddMinutes(-53 * $offset)
            $output              = $entry.Open()
            $input               = [System.IO.File]::OpenRead($Entries[$entryName])

            try {
                $input.CopyTo($output)
            }
            finally {
                $input.Dispose()
                $output.Dispose()
            }

            $offset++
        }
    }
    finally {
        $zip.Dispose()
    }

    (Get-Item $ZipPath).LastWriteTime = $script:SampleTime
}





function New-DemoTree {
    param([string]$Path)

    Copy-DemoFiles $Path ([ordered]@{
        '.gitignore'                   = '.gitignore'
        'README.md'                    = 'README.md'
        'LICENSE'                      = 'LICENSE'
        'CHANGELOG.md'                 = 'CHANGELOG.md'
        'build.ps1'                    = 'scripts\Build.ps1'
        'launch.json'                  = '.vscode\launch.json'
        'notes.txt'                    = '*notes'
        'data.csv'                     = '*csv'
        'logo.png'                     = 'Assets\icon-mockups\tcdir-final.png'
        'screenshot.png'               = 'Assets\TCDir Tree.png'
        'tcdir.exe'                    = $TcDirPath
        'docs\archive-info.md'         = 'specs\013-archive-info\spec.md'
        'docs\build-version.md'        = 'BUILD_VERSION.md'
        'docs\icon.svg'                = 'Assets\icon-mockups\tcdir-rcdir-shared.svg'
        'src\Console.cpp'              = 'TCDirCore\Console.cpp'
        'src\Console.h'                = 'TCDirCore\Console.h'
        'src\ZipReader.cpp'            = 'TCDirCore\ZipReader.cpp'
        'src\ZipReader.h'              = 'TCDirCore\ZipReader.h'
        'src\tests\ZipReaderTests.cpp' = 'UnitTest\ZipReaderTests.cpp'
        'assets\tcdir.ico'             = 'Assets\TCDir.ico'
        'assets\banner.png'            = 'Assets\TCDir-social.png'
        '.vscode\settings.json'        = '.vscode\settings.json'
    })

    #
    # release.zip contains a nested source.zip, so the archive screenshots
    # include an archive inside an archive
    #

    $sourceZip = Join-Path $script:StageRoot 'source.zip'
    New-DemoZip $sourceZip ([ordered]@{
        'Console.cpp'   = Join-Path $Path 'src\Console.cpp'
        'Console.h'     = Join-Path $Path 'src\Console.h'
        'ZipReader.cpp' = Join-Path $Path 'src\ZipReader.cpp'
        'ZipReader.h'   = Join-Path $Path 'src\ZipReader.h'
    })

    New-DemoZip (Join-Path $Path 'release.zip') ([ordered]@{
        'tcdir.exe'             = Join-Path $Path 'tcdir.exe'
        'README.md'             = Join-Path $Path 'README.md'
        'LICENSE'               = Join-Path $Path 'LICENSE'
        'CHANGELOG.md'          = Join-Path $Path 'CHANGELOG.md'
        'docs/archive-info.md'  = Join-Path $Path 'docs\archive-info.md'
        'docs/build-version.md' = Join-Path $Path 'docs\build-version.md'
        'source.zip'            = $sourceZip
    })

    (Get-Item (Join-Path $Path '.gitignore') -Force).Attributes = 'Hidden, Archive'
    (Get-Item (Join-Path $Path 'LICENSE')).Attributes           = 'ReadOnly, Archive'
}





#
# A small project for the first README image: a folder and an archive with a
# few files each, listed together in tree view
#

function New-Showcase {
    param([string]$Path)

    Copy-DemoFiles $Path ([ordered]@{
        '.gitignore'        = '.gitignore'
        'build.ps1'         = 'scripts\Build.ps1'
        'LICENSE'           = 'LICENSE'
        'logo.png'          = 'Assets\icon-mockups\tcdir-final.png'
        'README.md'         = 'README.md'
        'src\Console.cpp'   = 'TCDirCore\Console.cpp'
        'src\Console.h'     = 'TCDirCore\Console.h'
        'src\ZipReader.cpp' = 'TCDirCore\ZipReader.cpp'
    })

    New-DemoZip (Join-Path $Path 'release.zip') ([ordered]@{
        'tcdir.exe'            = (Resolve-Path $TcDirPath).Path
        'CHANGELOG.md'         = Join-Path $script:RepoRoot 'CHANGELOG.md'
        'docs/archive-info.md' = Join-Path $script:RepoRoot 'specs\013-archive-info\spec.md'
    })

    (Get-Item (Join-Path $Path '.gitignore') -Force).Attributes = 'Hidden, Archive'
    (Get-Item (Join-Path $Path 'LICENSE')).Attributes           = 'ReadOnly, Archive'
}




function New-SampleHome {
    param([string]$Path)

    New-Item -ItemType Directory -Force $Path | Out-Null

    @(
        '# TCDir settings'
        'Icons'
        'Tree-'
        ''
        '# Colors'
        '.log = DarkGrey'
        'dir:assets = LightCyan'
    ) | Set-Content -Path (Join-Path $Path '.tcdirconfig') -Encoding utf8
}





function Get-FreeDriveLetter {
    $used = (Get-PSDrive -PSProvider FileSystem).Name

    foreach ($letter in 'T','U','V','W','X','Y','Z','S','R','Q') {
        if ($used -notcontains $letter) {
            return $letter
        }
    }

    throw 'No free drive letter for subst'
}





function Invoke-TcDir {
    param([string]$WorkingDirectory, [string[]]$Arguments, [hashtable]$Environment)

    $outFile = [System.IO.Path]::GetTempFileName()
    $quoted  = ($Arguments | ForEach-Object { if ($_ -match '[\s"]') { '"' + $_ + '"' } else { $_ } }) -join ' '
    $sets    = ($Environment.Keys | ForEach-Object { "set `"$_=$($Environment[$_])`"&& " }) -join ''

    try {
        Push-Location $WorkingDirectory
        & cmd.exe /d /c "$sets`"$TcDirPath`" $quoted > `"$outFile`" 2>&1"
        $text = [System.IO.File]::ReadAllText($outFile, [System.Text.Encoding]::UTF8)
    }
    finally {
        Pop-Location
        Remove-Item $outFile -Force
    }

    return $text
}





#
# Converts tcdir's colored output to HTML spans.  tcdir sets colors with
# "ESC[<fg>;<bg>m" and resets with "ESC[0m".
#

function ConvertFrom-Ansi {
    param([string]$Text)

    $html   = [System.Text.StringBuilder]::new()
    $fg     = 37
    $bg     = 40
    $open   = $false
    $tokens = [regex]::Split($Text, "(`e\[[0-9;]*[A-Za-z])")

    foreach ($token in $tokens) {
        if ($token -match "^`e\[([0-9;]*)m$") {
            foreach ($code in ($Matches[1] -split ';' | Where-Object { $_ -ne '' } | ForEach-Object { [int]$_ })) {
                if ($code -eq 0)                                         { $fg = 37; $bg = 40 }
                elseif ($script:Palette.ContainsKey($code))              { $fg = $code }
                elseif ($script:Palette.ContainsKey($code - 10))         { $bg = $code }
            }
            continue
        }

        if ($token -match "^`e\[") {
            continue
        }

        if ($token.Length -eq 0) {
            continue
        }

        $color   = $script:Palette[$fg]
        $bgColor = if ($bg -eq 40) { 'transparent' } else { $script:Palette[$bg - 10] }
        $escaped = [System.Net.WebUtility]::HtmlEncode($token)

        [void]$html.Append("<span style=`"color:$color;background:$bgColor`">$escaped</span>")
    }

    return $html.ToString()
}





function Get-PromptHtml {
    param([string]$CommandLine, [string]$Folder)

    $first, $rest = $CommandLine -split ' ', 2
    $prompt   = [System.Net.WebUtility]::HtmlEncode("PS $($script:Drive):\$Folder> ")
    $command  = [System.Net.WebUtility]::HtmlEncode($first)
    $argsHtml = if ($rest) { ' ' + [System.Net.WebUtility]::HtmlEncode($rest) } else { '' }
    $cmdColor = if ($first.StartsWith('$')) { $script:Palette[92] } else { $script:Palette[93] }

    return "<span style=`"color:$($script:Palette[37])`">$prompt</span>" +
           "<span style=`"color:$cmdColor`">$command</span>" +
           "<span style=`"color:$($script:Palette[90])`">$argsHtml</span>"
}





function Measure-Lines {
    param([string]$Text)

    $plain = [regex]::Replace($Text, "`e\[[0-9;]*[A-Za-z]", '')
    $lines = $plain -split "`r?`n"

    while ($lines.Count -gt 0 -and [string]::IsNullOrWhiteSpace($lines[-1])) {
        $lines = $lines[0..($lines.Count - 2)]
    }

    $widths     = @($lines | ForEach-Object { [System.Globalization.StringInfo]::new($_).LengthInTextElements })
    $maxColumns = ($widths | Measure-Object -Maximum).Maximum

    #
    # A line longer than the terminal width wraps onto more rows
    #

    $rows = 0
    foreach ($cx in $widths) {
        $rows += [Math]::Max(1, [int][Math]::Ceiling($cx / $Columns))
    }

    return [pscustomobject]@{ Count = $rows; MaxColumns = [Math]::Min([int]$maxColumns, $Columns) }
}





function Save-Screenshot {
    param([string]$Name, [object[]]$Steps, [string]$StageRoot)

    $body    = [System.Text.StringBuilder]::new()
    $cLines  = 0
    $cMax    = 0

    foreach ($step in $Steps) {
        [void]$body.Append((Get-PromptHtml $step.Display $step.Folder) + "`n")
        $cLines++
        $cMax = [Math]::Max($cMax, $step.Display.Length + 12)

        if ($step.Output) {
            $trimmed = $step.Output.TrimEnd()
            [void]$body.Append((ConvertFrom-Ansi $trimmed) + "`n`n")
            $measure = Measure-Lines $trimmed
            $cLines += $measure.Count + 1
            $cMax    = [Math]::Max($cMax, $measure.MaxColumns)
        }
    }

    $cMax   = [Math]::Min($cMax, $Columns)
    $width  = [int][Math]::Ceiling($cMax * $script:CharWidth * $script:FontSize) + 2 * $script:Padding + 8
    $height = [int][Math]::Ceiling($cLines * $script:LineHeight * $script:FontSize) + 2 * $script:Padding

    $html = @"
<!doctype html>
<html><head><meta charset="utf-8"><style>
html, body { margin: 0; background: $($script:Background); }
pre { margin: 0; padding: $($script:Padding)px; font-family: $($script:FontFamily); font-size: $($script:FontSize)px;
      line-height: $($script:LineHeight); color: $($script:Palette[37]); white-space: pre-wrap; overflow-wrap: anywhere;
      width: $($cMax)ch;
      font-variant-ligatures: none; font-feature-settings: 'calt' 0, 'liga' 0; }
</style></head><body><pre>$($body.ToString().TrimEnd())</pre></body></html>
"@

    $htmlPath = Join-Path $StageRoot "$Name.html"
    $pngPath  = Join-Path $OutputPath "$Name.png"
    [System.IO.File]::WriteAllText($htmlPath, $html, [System.Text.UTF8Encoding]::new($false))

    #
    # msedge.exe starts a second process and exits before the PNG is written.
    # Start-Process -Wait blocks until that process tree exits.
    #

    $edgeArgs = @(
        '--headless=new', '--disable-gpu', '--hide-scrollbars', '--force-device-scale-factor=2'
        "--user-data-dir=`"$(Join-Path $StageRoot 'edge-profile')`""
        "--window-size=$width,$height"
        "--screenshot=`"$pngPath`""
        ([System.Uri]::new($htmlPath).AbsoluteUri)
    )

    Start-Process -FilePath $script:EdgePath -ArgumentList $edgeArgs -Wait -WindowStyle Hidden
    if (-not (Test-Path $pngPath)) {
        throw "Edge did not write $pngPath"
    }

    Write-Host "Saved $pngPath (${width}x${height} CSS px)" -ForegroundColor Cyan
}





function New-Step {
    param([string]$Display, [string[]]$Arguments, [hashtable]$Environment = @{}, [string]$Folder = 'Demo')

    return [pscustomobject]@{ Display = $Display; Arguments = $Arguments; Environment = $Environment; Folder = $Folder; Output = $null }
}





if (-not (Test-Path $TcDirPath)) {
    throw "tcdir not found at $TcDirPath. Build it first (scripts\Build.ps1 -Configuration Release)."
}

$script:EdgePath = @(
    "${env:ProgramFiles(x86)}\Microsoft\Edge\Application\msedge.exe"
    "$env:ProgramFiles\Microsoft\Edge\Application\msedge.exe"
) | Where-Object { Test-Path $_ } | Select-Object -First 1

if (-not $script:EdgePath) {
    throw 'Microsoft Edge was not found'
}

$shots = [ordered]@{
    'TCDir'                         = @(New-Step 'tcdir --Tree --Archives' @('--Tree', '--Archives') @{} 'Project')
    'TCDir Help'                    = @(New-Step 'tcdir -?' @('-?'))
    'TCDir Tree'                    = @(New-Step 'tcdir --Tree' @('--Tree'))
    'TCDir Subdirectories'          = @(New-Step 'tcdir -s' @('-s'))
    'TCDir Wide'                    = @(New-Step 'tcdir -w' @('-w'))
    'TCDir Env Variable'            = @(
                                        New-Step "`$env:TCDIR = `"$script:SampleEnv`"" $null
                                        New-Step 'tcdir' @() @{ TCDIR = $script:SampleEnv }
                                      )
    'TCDir Env'                     = @(New-Step 'tcdir --Env' @('--Env') @{ TCDIR = $script:SampleEnv })
    'TCDir Config'                  = @(New-Step 'tcdir --Config' @('--Config'))
    'TCDir Settings'                = @(New-Step 'tcdir --Settings' @('--Settings'))
    'TCDir Archives'                = @(New-Step 'tcdir --Archives --Tree release.zip' @('--Archives', '--Tree', 'release.zip'))
    'TCDir Info'                    = @(New-Step 'tcdir --Info release.zip' @('--Info', 'release.zip'))
}

$stageRoot = Join-Path ([System.IO.Path]::GetTempPath()) ("tcdir-screenshots-{0}" -f [guid]::NewGuid())
$demoRoot  = Join-Path $stageRoot 'Drive'
$homeRoot  = Join-Path $demoRoot 'Users\Demo'
$drive     = Get-FreeDriveLetter
$script:Drive = $drive
$script:StageRoot = $stageRoot

New-Item -ItemType Directory -Force (Join-Path $demoRoot 'Demo') | Out-Null
New-DemoTree   (Join-Path $demoRoot 'Demo')
New-Showcase   (Join-Path $demoRoot 'Project')
New-SampleHome $homeRoot

& subst.exe "${drive}:" $demoRoot
if ($LASTEXITCODE -ne 0) {
    throw "subst failed for ${drive}:"
}

try {
    foreach ($name in $shots.Keys) {
        if ($Only -and $Only -notcontains $name) {
            continue
        }

        foreach ($step in $shots[$name]) {
            if ($null -eq $step.Arguments) {
                continue
            }

            $environment = @{ COLUMNS = "$Columns"; TCDIR = ''; USERPROFILE = "${drive}:\Users\Demo" }
            foreach ($key in $step.Environment.Keys) {
                $environment[$key] = $step.Environment[$key]
            }

            $step.Output  = Invoke-TcDir "${drive}:\$($step.Folder)" $step.Arguments $environment
        }

        Save-Screenshot $name $shots[$name] $stageRoot
    }
}
finally {
    & subst.exe "${drive}:" /D | Out-Null
    Remove-Item $stageRoot -Recurse -Force -ErrorAction SilentlyContinue
}
