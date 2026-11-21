librariesrely=()

root=$(cd "$(dirname "$0")/../../.." && pwd) || exit 1
case "$1" in 32|wasm32|arm32) bits=x86 ;; 64|wasm64|arm64) bits=x64 ;; *) exit 2 ;; esac
case "${THIRDPARTY_PLATFORM:-${CMAKE_SYSTEM_NAME:-$OSTYPE}}" in
    [Ww]indows|msys*|cygwin*|win32*) windows=1 ;;
    *) windows=0 ;;
esac
config=release
if [[ $1 == wasm* ]]; then
    archive=libzstd.a
    librariesrely+=($'vendor\t'"tools/zstandard/lib/wasm/static/$bits/$archive")
else
    if [[ $windows == 1 ]]; then
        suffix=
        if [[ $3 == debug ]]; then suffix=d; config=debug; fi
        archive="zstd${suffix}.lib"
    else
        archive=libzstd.a
    fi
    librariesrely+=($'link\t'"$archive")
fi
THIRDPARTY_FILES="$archive" bash "$root/tools/thirdparty.sh" zstandard "$1" lib "$config" "$4" || exit 1

printf 'CLOUD_REBUILD_LIBRARIESRELY\t%s\n' "${librariesrely[@]}" return