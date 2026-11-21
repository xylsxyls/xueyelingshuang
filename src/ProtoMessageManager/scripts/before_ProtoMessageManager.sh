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
THIRDPARTY_FILES="$archive" bash "$root/tools/thirdparty.sh" protobuf "$1" lib "$config" "$4" || exit 1
# protoc 在构建主机运行。Wasm 复用已有源码，干净检出时补齐一对生成文件。
src="$root/src/ProtoMessageManager/ProtoMessageManager/src"
if [[ $1 != wasm* || ! -s $src/ProtoMessage.pb.h || ! -s $src/ProtoMessage.pb.cc ]]; then
    generator="${PROTOC_EXECUTABLE:-}"
    if [[ -z $generator ]]; then
        case "$OSTYPE:$(uname -m)" in
            msys*|cygwin*|win32*) generator="$root/tools/protobuf/bin/windows/x86/protoc.exe" ;;
            linux*:x86_64) generator="$root/tools/protobuf/bin/linux/x64/protoc" ;;
            linux*:aarch64|linux*:arm64) generator="$root/tools/protobuf/bin/arm/x64/protoc" ;;
        esac
    fi
    if [[ -n $generator ]]; then
        actual=$("$generator" --version) || exit 1
        [[ $actual == "libprotoc 3.3.0" ]] || { echo "Expected protoc 3.3.0, found $actual" >&2; exit 1; }
        # 在当前构建的临时目录先生成；同一schema的并行编译不得重写正在读取的源码
        generated=$(mktemp -d) || exit 1
        if ! (
            "$generator" -I="$src" --cpp_out="$generated" "$src/ProtoMessage.proto" || exit 1
            for name in ProtoMessage.pb.h ProtoMessage.pb.cc; do
                if ! cmp -s "$generated/$name" "$src/$name"; then
                    cp "$generated/$name" "$src/$name" || exit 1
                fi
            done
        ); then
            rm -f "$generated/ProtoMessage.pb.h" "$generated/ProtoMessage.pb.cc"
            rmdir "$generated"
            exit 1
        fi
        rm -f "$generated/ProtoMessage.pb.h" "$generated/ProtoMessage.pb.cc" || exit 1
        rmdir "$generated" || exit 1
    elif [[ ! -s $src/ProtoMessage.pb.cc || ! -s $src/ProtoMessage.pb.h ]]; then
        echo "Set PROTOC_EXECUTABLE to a host build of protoc 3.3.0." >&2
        exit 1
    fi
fi

printf 'CLOUD_REBUILD_LIBRARIESRELY\t%s\n' "${librariesrely[@]}" return