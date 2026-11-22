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

#--------------------------------------------------------------------
Variant_bit=$1
Variant_dlllib=$2
Variant_debugRelease=$3
Variant_allSame=$4
if [[ $4 != only ]]; then
    "$CLOUD_REBUILD" Variant "$Variant_bit" "$Variant_dlllib" "$Variant_debugRelease" "$Variant_allSame"
fi
librariesrely+=($'project\tVariant\t'"$Variant_dlllib")

#--------------------------------------------------------------------
root=$(cd "$(dirname "$0")/../../.." && pwd) || exit 1
case "$1" in 32|wasm32|arm32) bits=x86 ;; 64|wasm64|arm64) bits=x64 ;; *) exit 2 ;; esac
case "${THIRDPARTY_PLATFORM:-${CMAKE_SYSTEM_NAME:-$OSTYPE}}" in
    [Ww]indows|msys*|cygwin*|win32*) windows=1 ;;
    *) windows=0 ;;
esac
config=release
if [[ $1 == wasm* ]]; then
    archive=libprotobuf-lite.a
    librariesrely+=($'vendor\t'"tools/protobuf/lib/wasm/static/$bits/$archive")
else
    if [[ $windows == 1 ]]; then
        suffix=
        if [[ $3 == debug ]]; then suffix=d; config=debug; fi
        archive="libprotobuf-lite${suffix}.lib"
    else
        archive=libprotobuf-lite.a
    fi
    librariesrely+=($'link\t'"$archive")
fi
# 没有随仓交付的移动和 Apple 归档可由调用方放入独立的目标 SDK。
sdk="${CLOUD_REBUILD_SDK_ROOT:-$root}"
platform="${THIRDPARTY_PLATFORM:-${CMAKE_SYSTEM_NAME:-}}"
case "$platform" in
    android|Android|Darwin|macOS|iOS|mac-*|iOS-*) external=1 ;;
    *) external=0 ;;
esac
if [[ $external == 1 && $1 != wasm* && ! -d "$root/tools/protobuf/lib/$platform/static/$bits" ]]; then
    if [[ -z ${CLOUD_REBUILD_SDK_ROOT:-} || ! -s "$sdk/include/protobuf/google/protobuf/stubs/common.h" || ! -s "$sdk/lib/$archive" ]]; then
        echo "Prepare target protobuf headers and $archive under CLOUD_REBUILD_SDK_ROOT for $platform." >&2
        exit 1
    fi
else
    THIRDPARTY_FILES="$archive" bash "$root/tools/thirdparty.sh" protobuf "$1" lib "$config" "$4" || exit 1
fi

# 生成器在宿主执行，生成文件必须匹配目标头库。不同版本不能共用源码输出。
version=$(awk '$1 == "#define" && $2 == "GOOGLE_PROTOBUF_VERSION" { sub(/\r$/, "", $3); print $3; exit }' "$sdk/include/protobuf/google/protobuf/stubs/common.h") || exit 1
src="$root/src/ProtoMessageManager/ProtoMessageManager/src"
case "$version" in
    3003000) expected=3.3.0; destination="$src" ;;
    3019003) expected=3.19.3; destination="$sdk/include/ProtoMessageManager/protoc-3.19.3" ;;
    *) echo "Unsupported target protobuf version: $version" >&2; exit 1 ;;
esac
generator="${PROTOC_EXECUTABLE:-}"
# 隔离 SDK 可直接使用由对应版本 protoc 生成且与当前 schema 匹配的文件。
prepared=
if [[ -z $generator && $version == 3019003 ]]; then
    bundled="$src/../protoc-3.19.3"
    if command -v sha256sum >/dev/null 2>&1; then
        hash=(sha256sum)
    else
        hash=(shasum -a 256)
    fi
    # 统一 LF 和末行换行，保留字段或字符串中的全部内容。
    schema_hash=$(set -o pipefail; awk '{ sub(/\r$/, ""); print }' "$src/ProtoMessage.proto" | "${hash[@]}") || schema_hash=
    if [[ ${schema_hash%% *} == 8c122c9d5b6e481cfe3dd912810e3f8af9a5ffad6c3346f048ea2ce84b0fc585 &&
        -s $bundled/ProtoMessage.pb.h && -s $bundled/ProtoMessage.pb.cc ]]; then
        prepared="$bundled"
    fi
fi
if [[ -z $generator && -z $prepared ]]; then
    case "$OSTYPE:$(uname -m):$expected" in
        msys*:3.3.0|cygwin*:3.3.0|win32*:3.3.0) generator="$root/tools/protobuf/bin/windows/x86/protoc.exe" ;;
        linux*:x86_64:3.3.0) generator="$root/tools/protobuf/bin/linux/x64/protoc" ;;
        linux*:aarch64:3.19.3|linux*:arm64:3.19.3) generator="$root/tools/protobuf/bin/arm/x64/protoc" ;;
        *) generator=$(command -v protoc) ;;
    esac
fi
if [[ -n $generator || -n $prepared ]]; then
    if [[ -n $generator ]]; then
        actual=$("$generator" --version) || exit 1
        [[ $actual == "libprotoc $expected" ]] || { echo "Expected protoc $expected, found $actual" >&2; exit 1; }
    fi
    mkdir -p "$destination" || exit 1
    # 临时文件与最终路径同一文件系统，读者只会打开完整文件。
    generated=$(mktemp -d "$destination/.protoc.XXXXXX") || exit 1
    (
        if [[ -n $prepared ]]; then
            cp "$prepared/ProtoMessage.pb.h" "$prepared/ProtoMessage.pb.cc" "$generated/" || exit 1
        else
            "$generator" -I="$src" --cpp_out="$generated" "$src/ProtoMessage.proto" || exit 1
        fi
        for name in ProtoMessage.pb.h ProtoMessage.pb.cc; do
            if ! cmp -s "$generated/$name" "$destination/$name"; then
                mv -f "$generated/$name" "$destination/$name" || exit 1
            fi
        done
    )
    generated_status=$?
    rm -f "$generated/ProtoMessage.pb.h" "$generated/ProtoMessage.pb.cc" || exit 1
    rmdir "$generated" || exit 1
    [[ $generated_status == 0 ]] || exit 1
elif [[ $version == 3019003 || ! -s $destination/ProtoMessage.pb.cc || ! -s $destination/ProtoMessage.pb.h ||
    $src/ProtoMessage.proto -nt $destination/ProtoMessage.pb.cc || $src/ProtoMessage.proto -nt $destination/ProtoMessage.pb.h ]]; then
    echo "Set PROTOC_EXECUTABLE to a host build of protoc $expected for this schema." >&2
    exit 1
fi

printf 'CLOUD_REBUILD_LIBRARIESRELY\t%s\n' "${librariesrely[@]}" return