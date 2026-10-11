#!/usr/bin/env bash
set -euo pipefail
root=$(cd "$(dirname "$0")" && pwd)
case "${OSTYPE:-}" in
    msys*|cygwin*)
        MSYS2_ARG_CONV_EXCL='*' MSYS_NO_PATHCONV=1 cmd.exe /d /c "$(cygpath -w "$root/build-windows.cmd")"
        exit $? ;;
esac
bits=${1:-64}
case "$bits" in
    64) folder=x64 ;;
    32) folder=x86 ;;
    arm64) folder=arm64 ;;
    arm32) folder=arm32 ;;
    *) echo 'Use 32, 64, arm32 or arm64; cross builds also require CMAKE_TOOLCHAIN_FILE.' >&2; exit 1 ;;
esac
mkdir -p "$root/build" "$root/lib/$folder" "$root/include/soundtouchlinux"
tar -xf "$root/soundtouch-2.4.0.tar.gz" -C "$root/build" --exclude=libSoundTouchDll.so
args=()
flags='-DSOUNDTOUCH_FLOAT_SAMPLES=1 -DSOUNDTOUCH_PREVENT_CLICK_AT_RATE_CROSSOVER=1'
if [[ -n "${CMAKE_TOOLCHAIN_FILE:-}" ]]; then
    args+=("-DCMAKE_TOOLCHAIN_FILE=$CMAKE_TOOLCHAIN_FILE")
elif [[ $(uname -m) == x86_64 || $(uname -m) == i?86 ]]; then
    case "$bits" in 32|64) flags="$flags -m$bits" ;; esac
fi
cmake -S "$root/build/soundtouch" -B "$root/build/shared-$bits" \
    -DCMAKE_BUILD_TYPE=Release -DBUILD_SHARED_LIBS=OFF -DSOUNDTOUCH_DLL=ON -DSOUNDSTRETCH=OFF \
    -DCMAKE_POSITION_INDEPENDENT_CODE=ON \
    -DCMAKE_INSTALL_NAME_DIR=@rpath \
    "-DCMAKE_CXX_FLAGS=$flags" \
    "${args[@]}"
cmake --build "$root/build/shared-$bits" --config Release --target SoundTouchDLL
case "${OSTYPE:-}" in
    darwin*) cp "$root/build/shared-$bits/libSoundTouchDLL.dylib" "$root/lib/$folder/" ;;
    *) cp "$root/build/shared-$bits/libSoundTouchDLL.so" "$root/lib/$folder/" ;;
esac
cp "$root/build/soundtouch/include/"*.h "$root/include/soundtouchlinux/"
cp "$root/build/soundtouch/source/SoundTouchDLL/SoundTouchDLL.h" "$root/include/soundtouchlinux/"
if [ -f "$root/build/shared-$bits/include/soundtouch_config.h" ]; then
    cp "$root/build/shared-$bits/include/soundtouch_config.h" "$root/include/soundtouchlinux/"
fi