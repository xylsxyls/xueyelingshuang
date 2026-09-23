#!/bin/sh
# Single icon build entry. Windows uses Git sh and the project's Qt/VS toolchain.
set -eu

fail() { printf '%s\n' "Icon generation failed: $*" >&2; exit 1; }
source_png=
output=
format=
resource=
name=SplitViewer
qt_dir=
vcvars=
arch=
while [ "$#" -gt 0 ]; do
    [ "$#" -ge 2 ] || fail "Missing value for $1"
    case "$1" in
        --source) source_png=$2 ;;
        --output) output=$2 ;;
        --format) format=$2 ;;
        --resource) resource=$2 ;;
        --name) name=$2 ;;
        --qt-dir) qt_dir=$2 ;;
        --vcvars) vcvars=$2 ;;
        --arch) arch=$2 ;;
        *) fail "Unknown option: $1" ;;
    esac
    shift 2
done
[ -n "$source_png" ] && [ -n "$output" ] && [ -n "$format" ] || fail 'Source, output and format are required'
host=$(uname -s)
case "$host" in
    MINGW*|MSYS*|CYGWIN*)
        source_png=$(cygpath -u "$source_png")
        output=$(cygpath -u "$output")
        [ -z "$resource" ] || resource=$(cygpath -u "$resource")
        [ -z "$qt_dir" ] || qt_dir=$(cygpath -u "$qt_dir")
        [ -z "$vcvars" ] || vcvars=$(cygpath -u "$vcvars")
        ;;
esac
[ -f "$source_png" ] || fail "Missing PNG: $source_png"
source_png=$(cd "$(dirname "$source_png")" && printf '%s/%s' "$PWD" "$(basename "$source_png")")
mkdir -p "$(dirname "$output")"
output_dir=$(cd "$(dirname "$output")" && pwd -P)
output="$output_dir/$(basename "$output")"
[ "$source_png" != "$output" ] || fail 'Output must not overwrite the source'
[ ! -e "$output" ] || [ ! "$source_png" -ef "$output" ] || fail 'Output aliases the source'
signature=$(od -An -tx1 -N8 "$source_png" | tr -d ' \r\n')
[ "$signature" = 89504e470d0a1a0a ] || fail 'The source must be PNG'
dimensions=$(od -An -tu1 -j16 -N8 "$source_png")
set -- $dimensions
[ "$#" -eq 8 ] || fail 'Incomplete PNG dimensions'
width=$(($1 * 16777216 + $2 * 65536 + $3 * 256 + $4))
height=$(($5 * 16777216 + $6 * 65536 + $7 * 256 + $8))
[ "$width" -eq "$height" ] && [ "$width" -ge 256 ] || fail 'Use a square PNG of at least 256 pixels'
case "$name" in ''|.|..|*[!A-Za-z0-9_.-]*) fail 'Invalid desktop target name' ;; esac
if [ -n "$resource" ]; then
    [ "$format" = ICO ] || fail 'RC output is Windows-only'
    resource_dir=$(cd "$(dirname "$resource")" && pwd -P)
    resource="$resource_dir/$(basename "$resource")"
    [ "$resource_dir" = "$output_dir" ] && [ "$resource" != "$output" ] &&
        [ "$resource" != "$source_png" ] || fail 'RC must be beside the ICO and distinct from other files'
    [ ! -e "$resource" ] || [ ! "$source_png" -ef "$resource" ] || fail 'RC aliases the source'
fi

case "$format" in
    ICO)
        case "$host" in MINGW*|MSYS*|CYGWIN*) ;; *) fail 'ICO generation requires Windows Qt and VS2013' ;; esac
        [ -x "$qt_dir/bin/qmake.exe" ] && [ -f "$vcvars" ] || fail 'Qt qmake and VS vcvarsall.bat are required'
        case "$arch" in x64) compiler_arch=amd64 ;; Win32) compiler_arch=x86 ;; *) fail 'Expected x64 or Win32' ;; esac
        case "$(basename "$output")" in *[!A-Za-z0-9_.-]*) fail 'Invalid ICO filename' ;; esac
        builder="$output_dir/icon-builder"
        mkdir -p "$builder"
        # This small build-time tool reuses Qt's PNG decoder/resampler, not a custom image codec.
        cat > "$builder/IconBuilder.cpp" <<'CPP'
#include <QCoreApplication>
#include <QImage>
#include <QBuffer>
#include <QDataStream>
#include <QSaveFile>
#include <QVector>
#include <QStringList>
#include <QDebug>

int main(int argc, char* argv[])
{
    QCoreApplication application(argc, argv);
    const QStringList arguments = application.arguments();
    if (arguments.size() != 3)
    {
        qCritical("Expected source PNG and output ICO");
        return 1;
    }
    const QImage source(arguments.at(1), "PNG");
    if (source.isNull() || source.width() != source.height() || source.width() < 256)
    {
        qCritical("Invalid PNG; use a square image of at least 256 pixels");
        return 1;
    }
    const int sizes[] = {16, 20, 24, 32, 40, 48, 64, 96, 128, 256};
    const int count = sizeof(sizes) / sizeof(sizes[0]);
    QVector<QByteArray> frames;
    for (int index = 0; index < count; ++index)
    {
        QByteArray bytes;
        QBuffer buffer(&bytes);
        const QImage frame = source.scaled(sizes[index], sizes[index],
            Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
        if (!buffer.open(QIODevice::WriteOnly) || !frame.save(&buffer, "PNG"))
        {
            qCritical("Could not encode an ICO frame");
            return 1;
        }
        frames.append(bytes);
    }
    QSaveFile file(arguments.at(2));
    if (!file.open(QIODevice::WriteOnly))
    {
        qCritical("Could not open the ICO output");
        return 1;
    }
    QDataStream stream(&file);
    stream.setByteOrder(QDataStream::LittleEndian);
    stream << quint16(0) << quint16(1) << quint16(count);
    quint32 offset = 6 + 16 * count;
    for (int index = 0; index < count; ++index)
    {
        const quint8 dimension = static_cast<quint8>(sizes[index] % 256);
        stream << dimension << dimension << quint8(0) << quint8(0)
            << quint16(1) << quint16(32) << quint32(frames[index].size()) << offset;
        offset += frames[index].size();
    }
    for (int index = 0; index < count; ++index)
    {
        if (file.write(frames[index]) != frames[index].size())
        {
            qCritical("Could not write an ICO frame");
            return 1;
        }
    }
    if (stream.status() != QDataStream::Ok || !file.commit())
    {
        qCritical("Could not commit the ICO output");
        return 1;
    }
    return 0;
}
CPP
        cat > "$builder/IconBuilder.pro" <<'PRO'
QT = core gui
TEMPLATE = app
CONFIG += console release
CONFIG -= app_bundle debug debug_and_release
TARGET = SplitViewerIconBuilder
SOURCES = IconBuilder.cpp
DESTDIR = .
OBJECTS_DIR = obj
PRO
        vcvars_windows=$(cygpath -w "$vcvars")
        qmake_windows=$(cygpath -w "$qt_dir/bin/qmake.exe")
        printf '@echo off\r\ncall "%s" %s >nul\r\nif errorlevel 1 exit /b 1\r\n"%s" IconBuilder.pro -spec win32-msvc2013\r\nif errorlevel 1 exit /b 1\r\nnmake /nologo\r\nexit /b %%errorlevel%%\r\n' \
            "$vcvars_windows" "$compiler_arch" "$qmake_windows" > "$builder/build.cmd"
        build_windows=$(cygpath -w "$builder/build.cmd")
        (cd "$builder" && MSYS2_ARG_CONV_EXCL='*' cmd.exe /d /c "$build_windows")
        PATH="$qt_dir/bin:$PATH" "$builder/SplitViewerIconBuilder.exe" "$source_png" "$output"
        if [ -n "$resource" ]; then
            printf '1 ICON "%s"' "$(basename "$output")" > "$resource"
        fi
        ;;
    PNG)
        [ "$(basename "$output")" = "$name.png" ] || fail 'PNG filename must match the desktop Icon name'
        cp "$source_png" "$output"
        printf '[Desktop Entry]\nType=Application\nName=SplitViewer\nExec=%s %%f\nIcon=%s\nTerminal=false\nCategories=Graphics;Viewer;' \
            "$name" "$name" > "$output_dir/$name.desktop"
        ;;
    ICNS)
        [ "$host" = Darwin ] || fail 'ICNS generation requires macOS sips and iconutil'
        [ "$width" -ge 1024 ] || fail 'Use at least 1024 pixels for macOS Retina icons'
        command -v sips >/dev/null && command -v iconutil >/dev/null || fail 'Missing macOS icon tools'
        iconset="$output_dir/SplitViewer.iconset"
        mkdir -p "$iconset"
        for size in 16 32 128 256 512; do
            for scale in 1 2; do
                suffix=
                [ "$scale" -eq 1 ] || suffix=@2x
                pixels=$((size * scale))
                sips -z "$pixels" "$pixels" "$source_png" --out "$iconset/icon_${size}x${size}${suffix}.png" >/dev/null
            done
        done
        iconutil -c icns "$iconset" -o "$output"
        ;;
    *) fail "Unsupported format: $format" ;;
esac
printf 'Created %s\n' "$output"