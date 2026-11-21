librariesrely=()

:<<!EOF!
#--------------------------------------------------------------------
DllRelyTest_bit=$1
DllRelyTest_dlllib=$2
DllRelyTest_debugRelease=$3
DllRelyTest_allSame=$4
if [[ $4 != only ]]; then
    "$CLOUD_REBUILD" DllRelyTest "$DllRelyTest_bit" "$DllRelyTest_dlllib" "$DllRelyTest_debugRelease" "$DllRelyTest_allSame"
fi
librariesrely+=($'project\tDllRelyTest\t'"$DllRelyTest_dlllib")
!EOF!

root=$(cd "$(dirname "$0")/../../.." && pwd) || exit 1
case "$1" in 32|wasm32|arm32) bits=x86 ;; 64|wasm64|arm64) bits=x64 ;; *) exit 2 ;; esac
case "${THIRDPARTY_PLATFORM:-${CMAKE_SYSTEM_NAME:-$OSTYPE}}" in
    [Ww]indows|msys*|cygwin*|win32*) windows=1 ;;
    *) windows=0 ;;
esac
components=(avformat avcodec swscale swresample avutil)
if [[ $1 != wasm* && $windows == 1 ]]; then
    # Windows builds can enable ANSI file paths using CStringManager.
    if [[ $4 != only ]]; then
        "$CLOUD_REBUILD" CStringManager "$1" "$2" "$3" "$4"
    fi
    librariesrely+=($'project\tCStringManager\t'"$2")
    suffix=
    if [[ $3 == debug ]]; then suffix=d; fi
    names=()
    for name in "${components[@]}"; do
        names+=("$name$suffix.lib")
        librariesrely+=($'link\t'"$name$suffix.lib")
    done
    for name in "avcodec-60$suffix.dll" "avformat-60$suffix.dll" "swscale-7$suffix.dll" "swresample-4$suffix.dll" "avutil-58$suffix.dll"; do
        names+=("$name")
    done
    THIRDPARTY_FILES="$(printf '%s\n' "${names[@]}")" bash "$root/tools/thirdparty.sh" ffmpeg "$1" dll "$3" "$4" || exit 1
    base="SoundTouchDLL$suffix"
    THIRDPARTY_FILES="$(printf '%s\n' "$base.lib" "$base.dll" "$base.pdb")" bash "$root/tools/thirdparty.sh" soundtouch "$1" dll "$3" "$4" || exit 1
    librariesrely+=($'link\t'"$base.lib")
else
    for vendor in ffmpeg soundtouch openh264; do
        case $vendor in ffmpeg) names=("${components[@]}") ;; soundtouch) names=(SoundTouchDLL) ;; openh264) names=(openh264) ;; esac
        files=()
        for name in "${names[@]}"; do
            archive="lib$name.a"
            files+=("$archive")
            if [[ $1 == wasm* ]]; then
                librariesrely+=($'vendor\t'"tools/$vendor/lib/wasm/static/$bits/$archive")
            else
                librariesrely+=($'link\t'"$archive")
            fi
        done
        THIRDPARTY_FILES="$(printf '%s\n' "${files[@]}")" bash "$root/tools/thirdparty.sh" "$vendor" "$1" lib release "$4" || exit 1
    done
fi

printf 'CLOUD_REBUILD_LIBRARIESRELY\t%s\n' "${librariesrely[@]}" return