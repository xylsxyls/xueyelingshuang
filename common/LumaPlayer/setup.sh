#!/bin/sh
# Source entry: setup.sh --prepare (called by version). Deployment entry: sh setup.sh.
# Product inputs/output are beside the script; system runtimes come from installed SDKs.
set -eu

base=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd -P)
host=$(uname -s)
version=1.0
stage=
mode=${1:-}

fail()
{
    printf 'Packaging failed: %s\n' "$*" >&2
    exit 1
}

cleanup()
{
    if [ -n "$stage" ]; then
        case "$stage" in
            "$base"/.luma-setup.*)
                [ ! -L "$stage" ] && rm -rf -- "$stage"
                ;;
        esac
    fi
}
finish()
{
    status=$?
    trap - EXIT
    if ! cleanup; then
        printf 'Packaging failed: temporary directory cleanup failed\n' >&2
        if [ "$status" -eq 0 ]; then
            status=1
        fi
    fi
    if [ "$status" -ne 0 ]; then
        printf 'Packaging exited with status %s.\n' "$status" >&2
    fi
    if [ "$status" -ne 0 ] && [ "$mode" != --prepare ] && [ -t 0 ] && [ "${LUMAPLAYER_PACK_NO_PAUSE:-0}" != 1 ]; then
        printf '\nPress Enter to close...'
        read -r answer || :
    fi
    exit "$status"
}
trap finish EXIT
trap 'exit 130' INT
trap 'exit 143' TERM

require_file()
{
    [ -s "$1" ] || fail "Required local input missing: $1"
}

require_tool()
{
    command -v "$1" >/dev/null 2>&1 || fail "Required packaging tool missing: $1"
}

copy_file()
{
    require_file "$1"
    mkdir -p "$(dirname "$2")"
    cp -p "$1" "$2"
}

require_pe_x64()
{
    require_file "$1"
    set -- "$1" $(od -An -tu1 -j60 -N4 "$1")
    [ "$#" -eq 5 ] || fail 'Invalid PE input'
    offset=$(($2 + $3 * 256 + $4 * 65536 + $5 * 16777216))
    signature=$(od -An -tx1 -j"$offset" -N6 "$1" | tr -d ' \r\n')
    [ "$signature" = 504500006486 ] || fail "Expected a Windows x64 binary: $1"
}

prepare()
{
    [ -f "$base/LumaPlayer.cpp" ] && [ -f "$base/../version_release.sh" ] ||
        fail '--prepare must be called on src/LumaPlayer/LumaPlayer/setup.sh'
    root=$(CDPATH= cd "$base/../../.." && pwd -P)
    output="$root/lib"
    assets="$root/common/LumaPlayer"
    mkdir -p "$output"
    copy_file "$base/setup.sh" "$output/setup.sh"
    copy_file "$base/LumaPlayer.iss" "$output/LumaPlayer.iss"
    # These are existing, maintained distribution assets, not trial/build output.
    [ -d "$assets/res" ] && [ -d "$assets/karaoke/licenses" ] ||
        fail 'Repository product resources and license materials are required'
    mkdir -p "$output/res" "$output/karaoke/licenses"
    cp -pR "$assets/res/." "$output/res/"
    cp -pR "$assets/karaoke/licenses/." "$output/karaoke/licenses/"
    case "$host" in
        MINGW*|MSYS*|CYGWIN*) ;;
        Linux)
            copy_file "$base/res/LumaPlayer.png" "$output/LumaPlayer.png"
            ;;
        Darwin) ;;
        *) fail "Unsupported preparation platform: $host" ;;
    esac
    printf 'Prepared local packaging inputs: %s\n' "$output"
}

pack_windows()
{
    require_tool cygpath
    require_file "$base/LumaPlayer.iss"
    require_pe_x64 "$base/LumaPlayer1.0.exe"
    # Preflight the same SDK paths used by the Inno script before opening its IDE.
    qt=$(cygpath -u "${QTDIR:-${SystemDrive:-C:}/Qt/Qt5.5.1_64_2013/5.5/msvc2013_64}")
    if [ -n "${VC120DIR:-}" ]; then
        crt=$(cygpath -u "$VC120DIR")
    elif [ -n "${VS120COMNTOOLS:-}" ]; then
        crt=$(cygpath -u "$VS120COMNTOOLS/../../VC/redist/x64/Microsoft.VC120.CRT")
    else
        crt=$(cygpath -u "${SystemDrive:-C:}/Program Files (x86)/Microsoft Visual Studio 12.0/VC/redist/x64/Microsoft.VC120.CRT")
    fi
    for name in Qt5Core Qt5Gui Qt5Widgets Qt5Multimedia Qt5Network Qt5Xml libEGL libGLESv2 opengl32sw; do
        require_pe_x64 "$qt/bin/$name.dll"
    done
    for plugin in platforms/qwindows audio/qtaudio_windows imageformats/qico imageformats/qjpeg; do
        require_pe_x64 "$qt/plugins/$plugin.dll"
    done
    for name in msvcp120 msvcr120; do
        require_pe_x64 "$crt/$name.dll"
    done
    crt14=${VC14DIR:-}
    if [ -n "$crt14" ]; then
        crt14=$(cygpath -u "$crt14")
    else
        for relative in ../tools ../../tools ../../../tools; do
            if [ -s "$base/$relative/平台库/v14/x64/msvcp140.dll" ]; then
                crt14="$base/$relative/平台库/v14/x64"
                break
            fi
        done
    fi
    [ -n "$crt14" ] || fail 'Set VC14DIR to the matching x64 v14 runtime DLL directory'
    for name in msvcp140 msvcp140_1 vcruntime140 vcruntime140_1; do
        require_pe_x64 "$crt14/$name.dll"
    done
    QTDIR=$(cygpath -w "$qt")
    VC120DIR=$(cygpath -w "$crt")
    VC14DIR=$(cygpath -w "$crt14")
    export QTDIR VC120DIR VC14DIR
    compiler=${INNO_COMPILER:-}
    if [ -n "$compiler" ]; then
        compiler=$(cygpath -u "$compiler")
    else
        for location in "${ProgramW6432:-${ProgramFiles:-C:/Program Files}}" \
            "${SystemDrive:-C:}/Program Files (x86)" "${LOCALAPPDATA:-}/Programs"; do
            location=$(cygpath -u "$location")
            for release in 6 7; do
                for name in ISIDE.exe ISCC.exe; do
                    if [ -f "$location/Inno Setup $release/$name" ]; then
                        compiler="$location/Inno Setup $release/$name"
                        break 3
                    fi
                done
            done
        done
        if [ -z "$compiler" ]; then
            compiler=$(command -v ISIDE.exe || command -v ISCC.exe || :)
        fi
    fi
    [ -n "$compiler" ] && [ -f "$compiler" ] ||
        fail 'Install Inno Setup 6.3+ or set INNO_COMPILER to ISIDE.exe/ISCC.exe'
    script=$(cygpath -w "$base/LumaPlayer.iss")
    case "$compiler" in
        *[Ii][Ss][Ii][Dd][Ee].[Ee][Xx][Ee])
            MSYS2_ARG_CONV_EXCL='*' "$compiler" /cc "$script" ||
                fail 'Inno compilation failed; an older installer is not this build'
            ;;
        *[Ii][Ss][Cc][Cc].[Ee][Xx][Ee])
            MSYS2_ARG_CONV_EXCL='*' "$compiler" "$script" ||
                fail 'Inno compilation failed; an older installer is not this build'
            ;;
        *) fail 'INNO_COMPILER must point to ISIDE.exe or ISCC.exe' ;;
    esac
    result="$base/LumaPlayer-$version-Windows-x64-Setup.exe"
}

copy_resources()
{
    destination=$1
    require_file "$base/karaoke/UVR-MDX-NET-Inst_HQ_5.onnx"
    [ -d "$base/res" ] && [ -d "$base/karaoke/licenses" ] || fail 'Missing local resources/licenses'
    mkdir -p "$destination/res" "$destination/karaoke/licenses"
    cp -pR "$base/res/." "$destination/res/"
    cp -pR "$base/karaoke/licenses/." "$destination/karaoke/licenses/"
    copy_file "$base/karaoke/UVR-MDX-NET-Inst_HQ_5.onnx" "$destination/karaoke/UVR-MDX-NET-Inst_HQ_5.onnx"
}

pack_linux()
{
    require_tool dpkg-deb
    deploy=${LINUXDEPLOYQT:-linuxdeployqt}
    require_tool "$deploy"
    require_tool ldd
    executable="$base/LumaPlayer"
    [ -f "$executable" ] || executable="$base/LumaPlayer1.0"
    require_file "$executable"
    [ "$(od -An -tx1 -N4 "$executable" | tr -d ' \r\n')" = 7f454c46 ] || fail 'Linux packaging requires a native ELF player'
    machine=$(od -An -tx1 -j18 -N2 "$executable" | tr -d ' \r\n')
    case "$machine" in
        3e00) architecture=amd64 ;;
        b700) architecture=arm64 ;;
        0300) architecture=i386 ;;
        2800) architecture=armhf ;;
        *) fail "Unsupported ELF machine: $machine" ;;
    esac
    require_file "$base/LumaPlayer.png"
    require_file "$base/karaoke/libonnxruntime.so"
    stage=$(mktemp -d "$base/.luma-setup.XXXXXX")
    appdir="$stage/AppDir"
    appbin="$appdir/usr/bin"
    mkdir -p "$appbin" "$appdir/usr/lib" "$appdir/usr/share/applications" "$appdir/usr/share/icons/hicolor/256x256/apps"
    copy_file "$executable" "$appbin/LumaPlayer"
    copy_resources "$appbin"
    for name in CStringManager CSystem CTaskThreadManager Semaphore ReadWriteMutex IntDateTime LogManager CDump QssString QtControls DialogManager EO_XmlSax2Parser LumaPlayerCore KissFFTCpp FFmpegCpp SoundTouchDLL; do
        for library in "$base/lib$name.so"*; do
            [ ! -e "$library" ] || cp -pP "$library" "$appdir/usr/lib/"
        done
    done
    for library in "$base"/libavcodec.so* "$base"/libavformat.so* "$base"/libavutil.so* "$base"/libswresample.so* "$base"/libswscale.so*; do
        [ ! -e "$library" ] || cp -pP "$library" "$appdir/usr/lib/"
    done
    for library in "$base/karaoke/"*.so*; do
        [ ! -e "$library" ] || cp -pP "$library" "$appbin/karaoke/"
    done
    copy_file "$base/LumaPlayer.png" "$appdir/usr/share/icons/hicolor/256x256/apps/LumaPlayer.png"
    printf '[Desktop Entry]\nType=Application\nName=LumaPlayer\nExec=LumaPlayer %%f\nIcon=LumaPlayer\nTerminal=false\nCategories=AudioVideo;Player;\n' > "$appdir/usr/share/applications/LumaPlayer.desktop"
    LD_LIBRARY_PATH="$appdir/usr/lib:$base${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}" \
        "$deploy" "$appdir/usr/share/applications/LumaPlayer.desktop" \
            "-executable=$appbin/karaoke/libonnxruntime.so" -bundle-non-qt-libs -no-strip -always-overwrite ||
        fail 'Linux runtime deployment failed; verify native Qt/qmake and dependencies'
    for binary in "$appbin/LumaPlayer" "$appbin/karaoke/"*.so* "$appdir/usr/lib/"*.so*; do
        [ -e "$binary" ] || continue
        LD_LIBRARY_PATH="$appdir/usr/lib:$appbin/karaoke" \
            ldd "$binary" > "$stage/dependencies.txt" || fail "Dependency inspection failed: $binary"
        ! grep -q 'not found' "$stage/dependencies.txt" || fail "Unresolved dependencies: $binary"
    done
    package="$stage/deb"
    mkdir -p "$package/opt/LumaPlayer" "$package/usr/bin" "$package/usr/share/applications" "$package/usr/share/icons/hicolor/256x256/apps" "$package/DEBIAN"
    cp -pR "$appdir/." "$package/opt/LumaPlayer/"
    cp -p "$appdir/usr/share/icons/hicolor/256x256/apps/LumaPlayer.png" "$package/usr/share/icons/hicolor/256x256/apps/"
    cp -p "$appdir/usr/share/applications/LumaPlayer.desktop" "$package/usr/share/applications/"
    # Preserve paths passed from another working directory before switching to resources.
    printf '%s\n' '#!/bin/sh' 'app=/opt/LumaPlayer/usr' \
        'for argument do' \
        '    case "$argument" in' \
        '        /*) ;;' \
        '        *) [ ! -f "$argument" ] || argument="$PWD/$argument" ;;' \
        '    esac' \
        '    set -- "$@" "$argument"' \
        '    shift' \
        'done' \
        'export LD_LIBRARY_PATH="$app/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"' \
        'cd "$app/bin" || exit 1' \
        'exec "$app/bin/LumaPlayer" "$@"' > "$package/usr/bin/LumaPlayer"
    chmod 755 "$package/usr/bin/LumaPlayer" "$package/opt/LumaPlayer/usr/bin/LumaPlayer"
    printf 'Package: lumaplayer\nVersion: %s\nSection: video\nPriority: optional\nArchitecture: %s\nMaintainer: LumaPlayer\nDepends: libc6, libstdc++6, libgcc1 | libgcc-s1, libasound2, libpulse0\nDescription: LumaPlayer video player with karaoke\n' "$version" "$architecture" > "$package/DEBIAN/control"
    result="$base/LumaPlayer-${version}-Linux-${architecture}.deb"
    dpkg-deb --build --root-owner-group -Zxz -z9 "$package" "$stage/installer.deb" || fail 'dpkg-deb failed'
    dpkg-deb --info "$stage/installer.deb" >/dev/null || fail 'Invalid Debian package'
    mv "$stage/installer.deb" "$result"
}

pack_macos()
{
    require_tool ditto
    require_tool file
    deploy=${MACDEPLOYQT:-macdeployqt}
    require_tool "$deploy"
    bundle="$base/LumaPlayer.app"
    [ -d "$bundle" ] || bundle="$base/LumaPlayer1.0.app"
    require_file "$bundle/Contents/Info.plist"
    require_file "$base/karaoke/libonnxruntime.dylib"
    executable=$(/usr/libexec/PlistBuddy -c 'Print :CFBundleExecutable' "$bundle/Contents/Info.plist")
    case "$executable" in ''|*/*|..|.) fail 'Invalid CFBundleExecutable' ;; esac
    file "$bundle/Contents/MacOS/$executable" | grep -q 'Mach-O' || fail 'macOS packaging requires a native Mach-O bundle'
    stage=$(mktemp -d "$base/.luma-setup.XXXXXX")
    app="$stage/LumaPlayer.app"
    ditto "$bundle" "$app"
    appbin="$app/Contents/MacOS"
    copy_resources "$appbin"
    for library in "$base/karaoke/"*.dylib; do
        [ ! -e "$library" ] || cp -pP "$library" "$appbin/karaoke/"
    done
    for name in CStringManager CSystem CTaskThreadManager Semaphore ReadWriteMutex IntDateTime LogManager CDump QssString QtControls DialogManager EO_XmlSax2Parser LumaPlayerCore KissFFTCpp FFmpegCpp SoundTouchDLL avcodec avformat avutil swresample swscale; do
        for library in "$base/lib$name.dylib" "$base/lib$name."[0-9]*.dylib; do
            [ ! -e "$library" ] || cp -pP "$library" "$appbin/"
        done
    done
    set -- "$app" -dmg -no-strip -always-overwrite -verbose=1
    for library in "$appbin/"*.dylib "$appbin/karaoke/"*.dylib; do
        [ ! -e "$library" ] || set -- "$@" "-executable=$library"
    done
    DYLD_LIBRARY_PATH="$base${DYLD_LIBRARY_PATH:+:$DYLD_LIBRARY_PATH}" \
        "$deploy" "$@" ||
        fail 'macOS deployment failed; verify native Qt/macdeployqt and dependencies'
    require_file "$stage/LumaPlayer.dmg"
    result="$base/LumaPlayer-${version}-macOS.dmg"
    mv "$stage/LumaPlayer.dmg" "$result"
}

case "${1:-}" in
    --prepare) [ "$#" -eq 1 ] || fail 'Unexpected arguments'; prepare; exit 0 ;;
    --help)
        printf '%s\n' 'sh setup.sh: package this directory for the current OS, without compiling or installing.' \
            'Source-only --prepare: stage setup.sh, its configuration and product resources into lib.' \
            'Tools: Windows Inno Setup; Ubuntu linuxdeployqt/dpkg-deb; macOS macdeployqt.' \
            'Windows: QTDIR/VC120DIR select SDK runtimes; VC14DIR selects four app-local v14 DLLs.' \
            'INNO_COMPILER/LINUXDEPLOYQT/MACDEPLOYQT can select installed tools.'
        exit 0
        ;;
    '') [ "$#" -eq 0 ] || fail 'Unexpected arguments' ;;
    *) fail "Unknown option: $1" ;;
esac
case "$host" in
    MINGW*|MSYS*|CYGWIN*) pack_windows ;;
    Linux) pack_linux ;;
    Darwin) pack_macos ;;
    *) fail "Unsupported packaging platform: $host" ;;
esac
require_file "$result"
printf 'Installer ready: %s\n' "$result"