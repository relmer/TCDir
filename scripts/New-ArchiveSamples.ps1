[CmdletBinding()]
param(
    [string]$OutputPath = (Join-Path (Split-Path $PSScriptRoot -Parent) 'TestData\Archives')
)

#
# Builds small sample archives for manual testing of --Archives.
#
# Each format gets its own folder with three archives:
#   level0.<ext>  sample files only
#   level1.<ext>  sample files plus level0.<ext>
#   level2.<ext>  sample files plus level1.<ext>
#
# The sample files are generated text, so no real files end up in an archive.
#
# Tools: tar.exe (bsdtar, ships with Windows), makecab.exe (ships with
# Windows) and rar.exe (WinRAR, optional).  The LZH writer is implemented
# below because no Windows tool creates LZH archives.
#

$ErrorActionPreference = 'Stop'

$script:SampleTime = [datetime]::new(2026, 1, 1, 12, 0, 0, [DateTimeKind]::Local)
$script:RarPath    = 'C:\Program Files\WinRAR\rar.exe'





function New-SampleTree {
    param([string]$Path)

    $files = [ordered]@{
        'readme.txt'          = "Sample archive for TCDir manual testing.`r`n"
        'docs\guide.md'       = "# Guide`r`n`r`nPlaceholder text.`r`n"
        'docs\notes.txt'      = "Line one`r`nLine two`r`n"
        'src\hello.c'         = "#include <stdio.h>`r`n`r`nint main (void) { puts (`"hello`"); return 0; }`r`n"
        'src\hello.h'         = "#pragma once`r`n"
        'src\lib\util.cpp'    = "int Add (int a, int b) { return a + b; }`r`n"
        'data\numbers.csv'    = "a,b,c`r`n1,2,3`r`n4,5,6`r`n"
        'data\empty.dat'      = ''
    }

    foreach ($rel in $files.Keys) {
        $full = Join-Path $Path $rel
        New-Item -ItemType Directory -Force (Split-Path $full -Parent) | Out-Null
        [System.IO.File]::WriteAllText($full, $files[$rel])
    }

    Get-ChildItem $Path -Recurse | ForEach-Object { $_.LastWriteTime = $script:SampleTime }
    (Get-Item $Path).LastWriteTime = $script:SampleTime
}





function Invoke-Tar {
    param([string]$Archive, [string]$SourceDir, [string[]]$Options)

    $items = Get-ChildItem $SourceDir -Name
    & tar.exe -c -f $Archive @Options -C $SourceDir @items
    if ($LASTEXITCODE -ne 0) {
        throw "tar.exe failed to create $Archive"
    }
}





function Invoke-MakeCab {
    param([string]$Archive, [string]$SourceDir)

    $ddf   = Join-Path ([System.IO.Path]::GetTempPath()) ("tcdir-{0}.ddf" -f [guid]::NewGuid())
    $lines = @(
        '.OPTION EXPLICIT'
        ".Set CabinetNameTemplate=$(Split-Path $Archive -Leaf)"
        ".Set DiskDirectoryTemplate=$(Split-Path $Archive -Parent)"
        '.Set Cabinet=on'
        '.Set Compress=on'
        '.Set MaxDiskSize=0'
        '.Set InfFileName=NUL'
        '.Set RptFileName=NUL'
    )

    $files = Get-ChildItem $SourceDir -Recurse -File | Group-Object { $_.DirectoryName }
    foreach ($group in $files) {
        $relDir = [System.IO.Path]::GetRelativePath($SourceDir, $group.Name)
        if ($relDir -eq '.') { $relDir = '' }
        $lines += ".Set DestinationDir=$relDir"
        $lines += $group.Group | ForEach-Object { "`"$($_.FullName)`"" }
    }

    Set-Content -Path $ddf -Value $lines -Encoding ascii
    try {
        & makecab.exe /F $ddf | Out-Null
        if ($LASTEXITCODE -ne 0) {
            throw "makecab.exe failed to create $Archive"
        }
    }
    finally {
        Remove-Item $ddf -Force
    }
}





function Invoke-Rar {
    param([string]$Archive, [string]$SourceDir)

    Push-Location $SourceDir
    try {
        & $script:RarPath a -r -idq $Archive '*' | Out-Null
        if ($LASTEXITCODE -ne 0) {
            throw "rar.exe failed to create $Archive"
        }
    }
    finally {
        Pop-Location
    }
}





#
# CRC-16/ARC, the checksum LHA stores for each entry's data.
#

function Get-Crc16 {
    param([byte[]]$Bytes)

    [uint32]$crc = 0
    foreach ($b in $Bytes) {
        $crc = $crc -bxor $b
        for ($i = 0; $i -lt 8; $i++) {
            if ($crc -band 1) { $crc = ($crc -shr 1) -bxor 0xA001 }
            else              { $crc = $crc -shr 1 }
        }
    }

    return [uint16]$crc
}





#
# Writes an LZH archive with level 2 headers and stored (-lh0-) entries.
#

function Write-Lzh {
    param([string]$Archive, [string]$SourceDir)

    $epoch  = [uint32]([DateTimeOffset]$script:SampleTime).ToUnixTimeSeconds()
    $stream = [System.IO.MemoryStream]::new()
    $writer = [System.IO.BinaryWriter]::new($stream)

    foreach ($item in Get-ChildItem $SourceDir -Recurse | Sort-Object FullName) {
        $rel   = [System.IO.Path]::GetRelativePath($SourceDir, $item.FullName)
        $isDir = $item.PSIsContainer
        [byte[]]$data = [byte[]]::new(0)
        if (-not $isDir) {
            $data = [System.IO.File]::ReadAllBytes($item.FullName)
        }

        $parts   = $rel -split '\\'
        $dirPart = @()
        if ($isDir) {
            $dirPart = $parts
        }
        elseif ($parts.Count -gt 1) {
            $dirPart = $parts[0..($parts.Count - 2)]
        }
        $extHdrs = [System.Collections.Generic.List[byte[]]]::new()

        # Common header: the header CRC, patched in once the header is complete
        $extHdrs.Add([byte[]](0x00, 0x00, 0x00))

        if (-not $isDir) {
            $extHdrs.Add([byte[]](@(0x01) + [System.Text.Encoding]::ASCII.GetBytes($parts[-1])))
        }

        if ($dirPart) {
            $dirBytes = [System.Collections.Generic.List[byte]]::new()
            foreach ($p in $dirPart) {
                $dirBytes.AddRange([System.Text.Encoding]::ASCII.GetBytes($p))
                $dirBytes.Add(0xFF)
            }
            $extHdrs.Add([byte[]](@(0x02) + $dirBytes.ToArray()))
        }

        $extTotal   = ($extHdrs | ForEach-Object { $_.Length + 2 } | Measure-Object -Sum).Sum
        $headerSize = 26 + $extTotal
        if ($headerSize % 256 -eq 0) {
            throw "LZH header size for $rel is a multiple of 256"
        }

        $method       = if ($isDir) { '-lhd-' } else { '-lh0-' }
        $headerStream = [System.IO.MemoryStream]::new()
        $header       = [System.IO.BinaryWriter]::new($headerStream)

        $header.Write([uint16]$headerSize)
        $header.Write([System.Text.Encoding]::ASCII.GetBytes($method))
        $header.Write([uint32]$data.Length)
        $header.Write([uint32]$data.Length)
        $header.Write($epoch)
        $header.Write([byte]0x20)
        $header.Write([byte]2)
        $header.Write([uint16](Get-Crc16 $data))
        $header.Write([byte][char]'W')

        foreach ($ext in $extHdrs) {
            $header.Write([uint16]($ext.Length + 2))
            $header.Write([byte[]]$ext)
        }

        $header.Write([uint16]0)
        $header.Flush()

        # The common header's CRC field follows its size and type bytes
        $headerBytes = $headerStream.ToArray()
        $crcOffset   = 24 + 2 + 1
        $headerCrc   = Get-Crc16 $headerBytes
        $headerBytes[$crcOffset]     = [byte]($headerCrc -band 0xFF)
        $headerBytes[$crcOffset + 1] = [byte]($headerCrc -shr 8)

        $writer.Write($headerBytes)
        $writer.Write($data)
    }

    $writer.Write([byte]0)
    $writer.Flush()
    [System.IO.File]::WriteAllBytes($Archive, $stream.ToArray())
}





$formats = @(
    @{ Ext = '.zip';      Build = { param($a, $s) Invoke-Tar $a $s @('--format', 'zip') } }
    @{ Ext = '.iso';      Build = { param($a, $s) Invoke-Tar $a $s @('--format', 'iso9660', '--options', 'iso9660:!pad') } }
    @{ Ext = '.7z';       Build = { param($a, $s) Invoke-Tar $a $s @('--format', '7zip') } }
    @{ Ext = '.rar';      Build = { param($a, $s) Invoke-Rar $a $s } }
    @{ Ext = '.tar';      Build = { param($a, $s) Invoke-Tar $a $s @('--format', 'ustar') } }
    @{ Ext = '.tar.gz';   Build = { param($a, $s) Invoke-Tar $a $s @('--format', 'ustar', '--gzip') } }
    @{ Ext = '.tgz';      Build = { param($a, $s) Invoke-Tar $a $s @('--format', 'ustar', '--gzip') } }
    @{ Ext = '.tar.bz2';  Build = { param($a, $s) Invoke-Tar $a $s @('--format', 'ustar', '--bzip2') } }
    @{ Ext = '.tbz';      Build = { param($a, $s) Invoke-Tar $a $s @('--format', 'ustar', '--bzip2') } }
    @{ Ext = '.tbz2';     Build = { param($a, $s) Invoke-Tar $a $s @('--format', 'ustar', '--bzip2') } }
    @{ Ext = '.tar.xz';   Build = { param($a, $s) Invoke-Tar $a $s @('--format', 'ustar', '--xz') } }
    @{ Ext = '.txz';      Build = { param($a, $s) Invoke-Tar $a $s @('--format', 'ustar', '--xz') } }
    @{ Ext = '.tar.zst';  Build = { param($a, $s) Invoke-Tar $a $s @('--format', 'ustar', '--zstd') } }
    @{ Ext = '.tzst';     Build = { param($a, $s) Invoke-Tar $a $s @('--format', 'ustar', '--zstd') } }
    @{ Ext = '.tar.lzma'; Build = { param($a, $s) Invoke-Tar $a $s @('--format', 'ustar', '--lzma') } }
    @{ Ext = '.tlz';      Build = { param($a, $s) Invoke-Tar $a $s @('--format', 'ustar', '--lzip') } }
    @{ Ext = '.cab';      Build = { param($a, $s) Invoke-MakeCab $a $s } }
    @{ Ext = '.cpio';     Build = { param($a, $s) Invoke-Tar $a $s @('--format', 'newc') } }
    @{ Ext = '.xar';      Build = { param($a, $s) Invoke-Tar $a $s @('--format', 'xar') } }
    @{ Ext = '.lzh';      Build = { param($a, $s) Write-Lzh $a $s } }
    @{ Ext = '.lha';      Build = { param($a, $s) Write-Lzh $a $s } }
)

$staging = Join-Path ([System.IO.Path]::GetTempPath()) ("tcdir-samples-{0}" -f [guid]::NewGuid())

try {
    foreach ($format in $formats) {
        $ext = $format.Ext

        if ($ext -eq '.rar' -and -not (Test-Path $script:RarPath)) {
            Write-Warning "Skipping $ext because $($script:RarPath) is missing"
            continue
        }

        $formatDir = Join-Path $OutputPath $ext.TrimStart('.')
        if (Test-Path $formatDir) {
            Remove-Item $formatDir -Recurse -Force
        }
        New-Item -ItemType Directory -Force $formatDir | Out-Null

        $inner = $null
        for ($level = 0; $level -le 2; $level++) {
            $src = Join-Path $staging "$($ext.TrimStart('.'))-$level"
            New-SampleTree $src

            if ($inner) {
                Copy-Item $inner $src
            }

            $archive = Join-Path $formatDir "level$level$ext"
            & $format.Build $archive $src
            (Get-Item $archive).LastWriteTime = $script:SampleTime
            $inner = $archive
        }

        Write-Host "Built $formatDir" -ForegroundColor Cyan
    }
}
finally {
    if (Test-Path $staging) {
        Remove-Item $staging -Recurse -Force
    }
}
