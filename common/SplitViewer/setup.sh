#!/bin/sh
# Package existing native release files beside this script; never build or publish.
set -eu

base=
host=
version=1.0
stage=
libraries='SplitViewerCore CStringManager CSystem CTaskThreadManager Semaphore ReadWriteMutex IntDateTime LogManager CDump QssString QtControls DialogManager EO_XmlSax2Parser'

fail()
{
    printf 'Packaging failed: %s\n' "$*" >&2
    exit 1
}

cleanup()
{
    if [ -n "$stage" ]; then
        case "$stage" in
            "$base"/.splitviewer-setup.*)
                [ ! -L "$stage" ] && rm -rf -- "$stage"
                ;;
            *) return 1 ;;
        esac
    fi
}

finish()
{
    status=$?
    trap - EXIT
    if ! cleanup; then
        printf 'Packaging failed: temporary directory cleanup failed\n' >&2
        [ "$status" -ne 0 ] || status=1
    fi
    if [ "$status" -ne 0 ]; then
        printf 'Packaging exited with status %s.\n' "$status" >&2
        if [ -t 0 ] && [ "${SPLITVIEWER_PACK_NO_PAUSE:-0}" != 1 ]; then
            printf '\nPress Enter to close...'
            read -r answer || :
        fi
    fi
    exit "$status"
}
trap finish EXIT
trap 'exit 130' INT
trap 'exit 143' TERM

base=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd -P)
host=$(uname -s)

require_file()
{
    [ -s "$1" ] || fail "Required packaging input missing: $1"
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
    [ "$(od -An -tx1 -N2 "$1" | tr -d ' \r\n')" = 4d5a ] || fail "Invalid DOS signature: $1"
    set -- "$1" $(od -An -tu1 -j60 -N4 "$1")
    [ "$#" -eq 5 ] || fail 'Invalid PE input'
    offset=$(($2 + $3 * 256 + $4 * 65536 + $5 * 16777216))
    signature=$(od -An -tx1 -j"$offset" -N6 "$1" | tr -d ' \r\n')
    [ "$signature" = 504500006486 ] || fail "Expected a Windows x64 binary: $1"
}

require_notices()
{
    for relative in qtbase/LICENSE.LGPLv21 qtbase/LICENSE.LGPLv3 qtbase/LGPL_EXCEPTION.txt \
        qtbase/src/3rdparty/libjpeg/README qtimageformats/src/3rdparty/libtiff/COPYRIGHT \
        qtimageformats/src/3rdparty/libwebp/COPYING; do
        require_file "$qtsource/$relative"
    done
}

copy_notices()
{
    destination=$1
    require_notices
    copy_file "$qtsource/qtbase/LICENSE.LGPLv21" "$destination/Qt/LICENSE.LGPLv21"
    copy_file "$qtsource/qtbase/LICENSE.LGPLv3" "$destination/Qt/LICENSE.LGPLv3"
    copy_file "$qtsource/qtbase/LGPL_EXCEPTION.txt" "$destination/Qt/LGPL_EXCEPTION.txt"
    copy_file "$qtsource/qtbase/src/3rdparty/libjpeg/README" "$destination/Qt/libjpeg/README"
    copy_file "$qtsource/qtimageformats/src/3rdparty/libtiff/COPYRIGHT" "$destination/Qt/libtiff/COPYRIGHT"
    copy_file "$qtsource/qtimageformats/src/3rdparty/libwebp/COPYING" "$destination/Qt/libwebp/COPYING"
}

native_qt_sources()
{
    qtsource=${QTSOURCEDIR:-}
    if [ -z "$qtsource" ]; then
        query=${QMAKE:-qmake}
        require_tool "$query"
        prefix=$("$query" -query QT_INSTALL_PREFIX)
        [ -n "$prefix" ] || fail 'qmake did not return a Qt prefix'
        qtsource="$prefix/../Src"
    fi
    require_notices
}

copy_resources()
{
    [ -d "$base/res" ] || fail "Missing local resources: $base/res"
    mkdir -p "$1/res"
    cp -pR "$base/res/." "$1/res/"
    if [ -d "$base/licenses" ]; then
        mkdir -p "$1/licenses"
        cp -pR "$base/licenses/." "$1/licenses/"
    fi
    copy_notices "$1/licenses"
}

pack_windows()
{
    require_tool cygpath
    require_file "$base/SplitViewer.iss"
    require_pe_x64 "$base/SplitViewer1.0.exe"
    for name in $libraries; do
        require_pe_x64 "$base/$name.dll"
    done
    [ -d "$base/res" ] || fail "Missing local resources: $base/res"
    qt=$(cygpath -u "${QTDIR:-${SystemDrive:-C:}/Qt/Qt5.5.1_64_2013/5.5/msvc2013_64}")
    if [ -n "${VC120DIR:-}" ]; then
        crt=$(cygpath -u "$VC120DIR")
    elif [ -n "${VS120COMNTOOLS:-}" ]; then
        crt=$(cygpath -u "$VS120COMNTOOLS/../../VC/redist/x64/Microsoft.VC120.CRT")
    else
        crt=$(cygpath -u "${SystemDrive:-C:}/Program Files (x86)/Microsoft Visual Studio 12.0/VC/redist/x64/Microsoft.VC120.CRT")
    fi
    qtsource=$(cygpath -u "${QTSOURCEDIR:-$qt/../Src}")
    require_notices
    require_file "$qtsource/qtbase/src/3rdparty/angle/LICENSE"
    for name in Qt5Core Qt5Gui Qt5Widgets libEGL libGLESv2 opengl32sw D3Dcompiler_47; do
        require_pe_x64 "$qt/bin/$name.dll"
    done
    for plugin in platforms/qwindows imageformats/qgif imageformats/qico \
        imageformats/qjpeg imageformats/qtiff imageformats/qwebp; do
        require_pe_x64 "$qt/plugins/$plugin.dll"
    done
    for name in msvcp120 msvcr120; do
        require_pe_x64 "$crt/$name.dll"
    done
    QTDIR=$(cygpath -w "$qt")
    VC120DIR=$(cygpath -w "$crt")
    QTSOURCEDIR=$(cygpath -w "$qtsource")
    export QTDIR VC120DIR QTSOURCEDIR
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
    case "$compiler" in
        *[Ii][Ss][Ii][Dd][Ee].[Ee][Xx][Ee]) ;;
        *[Ii][Ss][Cc][Cc].[Ee][Xx][Ee]) ;;
        '') ;;
        *) fail 'INNO_COMPILER must point to ISIDE.exe or ISCC.exe' ;;
    esac
    [ -n "$compiler" ] && [ -f "$compiler" ] ||
        fail 'Install a complete Inno Setup 6.3+ distribution or set INNO_COMPILER to its ISCC.exe'
    # A fresh output prevents a previous installer from being reported as this build.
    stage=$(mktemp -d "$base/.splitviewer-setup.XXXXXX")
    script=$(cygpath -w "$base/SplitViewer.iss")
    output=$(cygpath -w "$stage")
    # Both official compiler entry points read the output directory from the ISS.
    SPLITVIEWER_INSTALLER_OUTPUT_DIR="$output"
    export SPLITVIEWER_INSTALLER_OUTPUT_DIR
    case "$compiler" in
        *[Ii][Ss][Ii][Dd][Ee].[Ee][Xx][Ee]) set -- /cc "$script" ;;
        *[Ii][Ss][Cc][Cc].[Ee][Xx][Ee]) set -- "$script" ;;
    esac
    printf 'Inno compiler: %s\n' "$compiler"
    compiler_log="$stage/compiler.log"
    if MSYS2_ARG_CONV_EXCL='*' "$compiler" "$@" > "$compiler_log" 2>&1; then
        cat "$compiler_log"
    else
        cat "$compiler_log" >&2
        if grep -Eq 'not trusted|Could not load ISCmplr' "$compiler_log"; then
            fail "Inno Setup files are inconsistent in $(dirname "$compiler"). Repair/reinstall the official distribution; do not disable its integrity checks."
        fi
        fail 'Inno compilation failed; see the compiler error above'
    fi
    filename="SplitViewer-$version-Windows-x64-Setup.exe"
    # Inno's bootstrap architecture can differ from the x64 installed payload.
    require_file "$stage/$filename"
    [ "$(od -An -tx1 -N2 "$stage/$filename" | tr -d ' \r\n')" = 4d5a ] ||
        fail 'Inno did not produce a Windows installer'
    result="$base/$filename"
    mv "$stage/$filename" "$result"
}

pack_linux()
{
    require_tool dpkg-deb
    require_tool ldd
    deploy=${LINUXDEPLOYQT:-linuxdeployqt}
    require_tool "$deploy"
    native_qt_sources
    executable="$base/SplitViewer"
    [ -f "$executable" ] || executable="$base/SplitViewer1.0"
    require_file "$executable"
    [ "$(od -An -tx1 -N4 "$executable" | tr -d ' \r\n')" = 7f454c46 ] ||
        fail 'Linux packaging requires a native ELF product'
    [ "$(od -An -tx1 -j4 -N2 "$executable" | tr -d ' \r\n')" = 0201 ] ||
        fail 'Linux packaging requires a 64-bit little-endian ELF product'
    machine=$(od -An -tx1 -j18 -N2 "$executable" | tr -d ' \r\n')
    case "$machine" in
        3e00) architecture=amd64 ;;
        b700) architecture=arm64 ;;
        *) fail "Unsupported ELF machine: $machine" ;;
    esac
    require_file "$base/SplitViewer.png"
    stage=$(mktemp -d "$base/.splitviewer-setup.XXXXXX")
    appdir="$stage/AppDir"
    appbin="$appdir/usr/bin"
    mkdir -p "$appbin" "$appdir/usr/lib" "$appdir/usr/share/applications" "$appdir/usr/share/icons/hicolor/256x256/apps"
    copy_file "$executable" "$appbin/SplitViewer"
    copy_resources "$appbin"
    for name in $libraries; do
        for library in "$base/lib$name.so"*; do
            [ ! -e "$library" ] || cp -pL "$library" "$appdir/usr/lib/"
        done
    done
    copy_file "$base/SplitViewer.png" "$appdir/usr/share/icons/hicolor/256x256/apps/SplitViewer.png"
    printf '[Desktop Entry]\nType=Application\nName=SplitViewer\nExec=SplitViewer %%f\nIcon=SplitViewer\nTerminal=false\nCategories=Graphics;Viewer;\n' > "$appdir/usr/share/applications/SplitViewer.desktop"
    LD_LIBRARY_PATH="$appdir/usr/lib:$base${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}" \
        "$deploy" "$appdir/usr/share/applications/SplitViewer.desktop" \
            -bundle-non-qt-libs -no-strip -always-overwrite ||
        fail 'Linux runtime deployment failed; verify native Qt/qmake and image plugins'
    for binary in "$appbin/SplitViewer" "$appdir/usr/lib/"*.so* "$appdir/usr/plugins/"*/*.so*; do
        [ -e "$binary" ] || continue
        LD_LIBRARY_PATH="$appdir/usr/lib" ldd "$binary" > "$stage/dependencies.txt" ||
            fail "Dependency inspection failed: $binary"
        ! grep -q 'not found' "$stage/dependencies.txt" || fail "Unresolved dependencies: $binary"
    done
    package="$stage/deb"
    mkdir -p "$package/opt/SplitViewer" "$package/usr/bin" "$package/usr/share/applications" \
        "$package/usr/share/icons/hicolor/256x256/apps" "$package/DEBIAN"
    cp -pR "$appdir/." "$package/opt/SplitViewer/"
    cp -p "$appdir/usr/share/icons/hicolor/256x256/apps/SplitViewer.png" "$package/usr/share/icons/hicolor/256x256/apps/"
    cp -p "$appdir/usr/share/applications/SplitViewer.desktop" "$package/usr/share/applications/"
    # Convert relative file arguments before changing the resource working directory.
    printf '%s\n' '#!/bin/sh' 'app=/opt/SplitViewer/usr' \
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
        'exec "$app/bin/SplitViewer" "$@"' > "$package/usr/bin/SplitViewer"
    chmod 755 "$package/usr/bin/SplitViewer" "$package/opt/SplitViewer/usr/bin/SplitViewer"
    printf 'Package: splitviewer\nVersion: %s\nSection: graphics\nPriority: optional\nArchitecture: %s\nMaintainer: SplitViewer\nDepends: libc6, libstdc++6, libgcc1 | libgcc-s1\nDescription: SplitViewer image and embedded-window viewer\n' "$version" "$architecture" > "$package/DEBIAN/control"
    result="$base/SplitViewer-${version}-Linux-${architecture}.deb"
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
    native_qt_sources
    bundle="$base/SplitViewer.app"
    [ -d "$bundle" ] || bundle="$base/SplitViewer1.0.app"
    require_file "$bundle/Contents/Info.plist"
    executable=$(/usr/libexec/PlistBuddy -c 'Print :CFBundleExecutable' "$bundle/Contents/Info.plist")
    case "$executable" in ''|*/*|..|.) fail 'Invalid CFBundleExecutable' ;; esac
    file "$bundle/Contents/MacOS/$executable" | grep -q 'Mach-O' ||
        fail 'macOS packaging requires a native Mach-O bundle'
    stage=$(mktemp -d "$base/.splitviewer-setup.XXXXXX")
    app="$stage/SplitViewer.app"
    ditto "$bundle" "$app"
    appbin="$app/Contents/MacOS"
    copy_resources "$appbin"
    for name in $libraries; do
        for library in "$base/lib$name.dylib" "$base/lib$name."[0-9]*.dylib; do
            [ ! -e "$library" ] || cp -pL "$library" "$appbin/"
        done
    done
    set -- "$app" -dmg -no-strip -always-overwrite -verbose=1
    for library in "$appbin/"*.dylib; do
        [ ! -e "$library" ] || set -- "$@" "-executable=$library"
    done
    DYLD_LIBRARY_PATH="$base${DYLD_LIBRARY_PATH:+:$DYLD_LIBRARY_PATH}" "$deploy" "$@" ||
        fail 'macOS deployment failed; verify native Qt/macdeployqt and image plugins'
    require_file "$stage/SplitViewer.dmg"
    result="$base/SplitViewer-$version-macOS.dmg"
    mv "$stage/SplitViewer.dmg" "$result"
}

case "${1:-}" in
    --help)
        [ "$#" -eq 1 ] || fail 'Unexpected arguments'
        printf '%s\n' 'sh setup.sh: package existing release files beside this script for the current OS.' \
            'Windows: Inno Setup 6.3+, matching Qt/VS2013 x64 SDKs; QTDIR/VC120DIR select runtimes.' \
            'QTSOURCEDIR selects the matching Qt source tree containing license notices.' \
            'INNO_COMPILER/LINUXDEPLOYQT/MACDEPLOYQT/QMAKE can select installed tools.' \
            'Linux requires linuxdeployqt/dpkg-deb; macOS requires macdeployqt and a native .app.' \
            'SPLITVIEWER_PACK_NO_PAUSE=1 disables the interactive failure pause.'
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