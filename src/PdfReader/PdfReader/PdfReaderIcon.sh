#!/usr/bin/env bash
# Generate platform icons from the single PdfReader PNG source.
# Usage: bash PdfReaderIcon.sh icon SOURCE.png OUTPUT.{ico,icns,png}
# ICO output also generates a sibling .rc file for the Windows resource compiler.
# Windows: Git Bash + system PowerShell; macOS: sips/iconutil; Linux: PNG directly.
set -euo pipefail

system_name="$(uname -s)"
case "$system_name" in
    MINGW*|MSYS*|CYGWIN*) platform=windows ;;
    Darwin) platform=macos ;;
    Linux) platform=linux ;;
    *) echo "Unsupported platform: $system_name" >&2; exit 2 ;;
esac

# Convert a caller path to the active shell's filesystem syntax.
shell_path() {
    if command -v cygpath >/dev/null 2>&1; then cygpath -u "$1"; else printf '%s\n' "$1"; fi
}

# Build an icon into a temporary/intermediate directory, never into res.
generate_icon() {
    [[ $# == 2 ]] || { echo "icon requires SOURCE and OUTPUT" >&2; return 2; }
    local source output output_dir helper
    source="$(shell_path "$1")"
    output="$(shell_path "$2")"
    [[ -f "$source" ]] || { echo "Missing PNG: $source" >&2; return 2; }
    output_dir="$(dirname "$output")"
    mkdir -p "$output_dir"
    case "$output" in
        *.ico)
            [[ "$platform" == windows ]] || { echo "ICO generation requires Windows PowerShell." >&2; return 2; }
            local icon_name
            icon_name="$(basename "$output")"
            case "$icon_name" in *[!A-Za-z0-9_.-]*) echo "Invalid ICO filename." >&2; return 2 ;; esac
            helper="$(mktemp "$output_dir/pdfreader-icon.XXXXXX.ps1")"
            trap 'rm -f -- "$helper"' EXIT
            cat > "$helper" <<'PDFREADER_ICON_POWERSHELL'
param(
    [Parameter(Mandatory=$true)] [string]$Source,
    [Parameter(Mandatory=$true)] [string]$Output
)

# Build a multi-size Windows icon from the single PNG source, preserving alpha.
# Source: square PNG. Output: generated ICO in the build intermediate directory.
$ErrorActionPreference = 'Stop'
Add-Type -AssemblyName System.Drawing
$sourcePath = (Resolve-Path -LiteralPath $Source).Path
$outputPath = [IO.Path]::GetFullPath($Output)
if ($sourcePath -eq $outputPath) { throw 'Source and output must differ.' }
$image = [Drawing.Image]::FromFile($sourcePath)
try
{
    if ($image.Width -ne $image.Height) { throw 'The icon source must be square.' }
    $sizes = @(16, 24, 32, 48, 64, 128, 256)
    $frames = New-Object 'System.Collections.Generic.List[byte[]]'
    foreach ($size in $sizes)
    {
        $bitmap = New-Object Drawing.Bitmap($size, $size, [Drawing.Imaging.PixelFormat]::Format32bppArgb)
        $graphics = [Drawing.Graphics]::FromImage($bitmap)
        $stream = New-Object IO.MemoryStream
        try
        {
            $graphics.Clear([Drawing.Color]::Transparent)
            $graphics.CompositingMode = [Drawing.Drawing2D.CompositingMode]::SourceCopy
            $graphics.InterpolationMode = [Drawing.Drawing2D.InterpolationMode]::HighQualityBicubic
            $graphics.PixelOffsetMode = [Drawing.Drawing2D.PixelOffsetMode]::HighQuality
            $graphics.DrawImage($image, (New-Object Drawing.Rectangle(0, 0, $size, $size)))
            $bitmap.Save($stream, [Drawing.Imaging.ImageFormat]::Png)
            $frames.Add($stream.ToArray())
        }
        finally
        {
            $stream.Dispose()
            $graphics.Dispose()
            $bitmap.Dispose()
        }
    }
    [IO.Directory]::CreateDirectory([IO.Path]::GetDirectoryName($outputPath)) | Out-Null
    $outputStream = New-Object IO.MemoryStream
    $writer = New-Object IO.BinaryWriter($outputStream)
    try
    {
        $writer.Write([uint16]0)
        $writer.Write([uint16]1)
        $writer.Write([uint16]$sizes.Count)
        $offset = 6 + 16 * $sizes.Count
        for ($index = 0; $index -lt $sizes.Count; ++$index)
        {
            $dimension = $sizes[$index] % 256
            $writer.Write([byte]$dimension)
            $writer.Write([byte]$dimension)
            $writer.Write([byte]0)
            $writer.Write([byte]0)
            $writer.Write([uint16]1)
            $writer.Write([uint16]32)
            $writer.Write([uint32]$frames[$index].Length)
            $writer.Write([uint32]$offset)
            $offset += $frames[$index].Length
        }
        foreach ($frame in $frames) { $writer.Write([byte[]]$frame) }
        $writer.Flush()
        [IO.File]::WriteAllBytes($outputPath, $outputStream.ToArray())
    }
    finally
    {
        $writer.Dispose()
        $outputStream.Dispose()
    }
}
finally
{
    $image.Dispose()
}
PDFREADER_ICON_POWERSHELL
            powershell.exe -NoProfile -ExecutionPolicy Bypass -File "$(cygpath -w "$helper")" \
                -Source "$(cygpath -w "$source")" -Output "$(cygpath -w "$output")"
            rm -f -- "$helper"
            trap - EXIT
            printf '101 ICON "%s"\n' "$icon_name" > "${output%.ico}.rc"
            ;;
        *.icns)
            [[ "$platform" == macos ]] || { echo "ICNS generation requires macOS sips/iconutil." >&2; return 2; }
            local iconset size scale pixels suffix
            iconset="$output_dir/PdfReader.iconset"
            mkdir -p "$iconset"
            for size in 16 32 128 256 512; do
                for scale in 1 2; do
                    pixels=$((size * scale))
                    suffix=""
                    [[ "$scale" == 1 ]] || suffix="@2x"
                    /usr/bin/sips -z "$pixels" "$pixels" "$source" \
                        --out "$iconset/icon_${size}x${size}${suffix}.png" >/dev/null
                done
            done
            /usr/bin/iconutil -c icns "$iconset" -o "$output"
            ;;
        *.png)
            # Linux uses the source PNG without conversion.
            [[ "$source" -ef "$output" ]] || cp -p "$source" "$output"
            ;;
        *) echo "Expected an .ico, .icns or .png output." >&2; return 2 ;;
    esac
}

case "${1:-help}" in
    icon) shift; generate_icon "$@" ;;
    help|--help|-h)
        printf '%s\n' 'Usage: bash PdfReaderIcon.sh icon SOURCE.png OUTPUT.{ico,icns,png}'
        ;;
    *) echo "Unknown action: $1" >&2; exit 2 ;;
esac
