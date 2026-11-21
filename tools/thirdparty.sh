#!/usr/bin/env bash
# 先解析目标，再把相应 SDK 的头和库发布到调用方选定的公共目录。
set -eu
set -o pipefail
shopt -s nullglob dotglob

fail() { printf 'thirdparty [%s]: %s\n' "${vendor:-?}" "$*" >&2; exit 1; }
usage() { fail 'expected [--header-map] [--headers-only] NAME PLATFORM [CRT] static|dynamic x86|x64 debug|release, or the CloudRebuild arguments NAME BIT dll|lib debug|release [SCOPE]'; }
valid_name() { [[ $1 =~ ^[A-Za-z0-9][A-Za-z0-9_.+-]*$ ]]; }

in_list() {
    local value=$1 item
    shift
    for item in "$@"; do [[ $item != "$value" ]] || return 0; done
    return 1
}

header_map=false
headers_only=false
while [[ ${1:-} == --* ]]; do
    case "$1" in
        --header-map) header_map=true ;;
        --headers-only) headers_only=true ;;
        *) usage ;;
    esac
    shift
done
vendor=${1:-}
valid_name "$vendor" || usage
shift
platforms=(windows linux arm android wasm mac-intel mac-apple-silicon iOS-arm iOS-intel-simulator iOS-apple-silicon-simulator)
crt=
case "${1:-}" in
    32|64|arm32|arm64|wasm32|wasm64)
        [[ $# == 3 || $# == 4 ]] || usage
        target=$1
        linkage=$2
        config=$3
        case "$target" in
            *32) bits=x86 ;;
            *64) bits=x64 ;;
        esac
        if [[ $target == wasm* ]]; then
            platform=wasm
        elif [[ -n ${THIRDPARTY_PLATFORM:-} ]]; then
            platform=$THIRDPARTY_PLATFORM
        else
            case "${CMAKE_SYSTEM_NAME:-}" in
                Windows) platform=windows ;;
                Android) platform=android ;;
                Linux) platform=linux ;;
                Darwin|iOS|macOS) fail '指定 THIRDPARTY_PLATFORM，区分 Mac CPU 或 iOS 真机、模拟器' ;;
                '') case "${CMAKE_TOOLCHAIN_FILE:-}:$OSTYPE" in
                        *android.toolchain.cmake:*) platform=android ;;
                        *:msys*|*:cygwin*|*:win32*) platform=windows ;;
                        *:linux*) platform=linux ;;
                        *) fail "目标平台不明确，请传 THIRDPARTY_PLATFORM（$OSTYPE）" ;;
                    esac ;;
                *) fail "unknown target: $CMAKE_SYSTEM_NAME" ;;
            esac
        fi
        if [[ $platform == linux && $target == arm* ]]; then platform=arm; fi
        if [[ $platform == windows ]]; then
            if [[ $config == debug ]]; then crt=mdd; else crt=mt; fi
            crt=${THIRDPARTY_CRT:-$crt}
        fi ;;
    windows)
        [[ $# == 5 ]] || usage
        platform=$1; crt=$2; linkage=$3; bits=$4; config=$5 ;;
    *)
        [[ $# == 4 ]] || usage
        platform=$1; linkage=$2; bits=$3; config=$4 ;;
esac
in_list "$platform" "${platforms[@]}" || fail "unknown platform: $platform"
case "$platform:$crt" in windows:mt|windows:md|windows:mdd) ;; windows:*) usage ;; esac
case "$linkage" in lib|static) linkage=static ;; dll|dynamic) linkage=dynamic ;; *) usage ;; esac
case "$bits" in x86|x64) ;; *) usage ;; esac
case "$config" in debug|release) ;; *) usage ;; esac

tools_dir=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd -P)
root=$(cd "$tools_dir/.." && pwd -P)
source_dir="$tools_dir/$vendor"
namespace=${THIRDPARTY_INCLUDE_NAME:-$vendor}
valid_name "$namespace" || fail "invalid header namespace: $namespace"
library_dir="$source_dir/lib/$platform${crt:+/$crt}/$linkage/$bits"
if [[ $headers_only == true ]]; then
    source_archives=("$source_dir"/*.tar.xz)
    [[ ! -e $source_dir/lib && ! -L $source_dir/lib && ! ${source_archives[0]+yes} && -z ${THIRDPARTY_FILES:-} ]] ||
        fail '纯头包只存 include 和许可证，不能包含库、源码包或声明待复制的库文件'
else
    [[ -d $library_dir ]] || fail "缺少 $config 目标库目录 $library_dir"
fi
headers="$source_dir/include"
[[ -d $headers ]] || fail "缺少头目录 $headers"

# 先处理共有内容，后处理所选分支；每个维度既可存在也可省略。
header_layers=("$headers")
for selected in "$platform" "$linkage" "$bits" "$config"; do
    next_layers=()
    for layer in "${header_layers[@]}"; do
        next_layers+=("$layer")
        if [[ -d $layer/$selected ]]; then next_layers+=("$layer/$selected"); fi
    done
    header_layers=("${next_layers[@]}")
done

# 收集可发布的实际文件，不进入未选平台、链接类型、位宽和配置。
header_paths=()
header_targets=()
header_layer_ids=()
# 独立 SDK（例如不同主版本）从匹配的最后一个标记开始，无须遍历更早的头树。
first_layer=0
for layer_id in "${!header_layers[@]}"; do
    if [[ -f ${header_layers[layer_id]}/.header-root ]]; then first_layer=$layer_id; fi
done
for ((layer_id=first_layer; layer_id<${#header_layers[@]}; ++layer_id)); do
    layer=${header_layers[layer_id]}
    entries=()
    for entry in "$layer"/*; do
        if [[ -d $entry ]] && in_list "${entry##*/}" "${platforms[@]}" static dynamic x86 x64 debug release; then
            continue
        fi
        entries+=("$entry")
    done
    [[ ${entries[0]+yes} ]] || continue
    complete=false
    while IFS= read -r -d '' file; do
        if [[ -z $file ]]; then complete=true; break; fi
        header_paths+=("$file")
        header_targets+=("${file#"$layer/"}")
        header_layer_ids+=("$layer_id")
    done < <(find -L "${entries[@]}" -name .header-root -prune -o -type f -print0 && printf '\0')
    [[ $complete == true ]] || fail "无法读取头目录 $layer"
done
[[ ${header_paths[0]+yes} ]] || fail "头目录没有文件 $headers"

# %q 将任意文件名编码为一行。按目标路径排序并保留最后一个文件，同时拒绝文件与目录同名。
# 使用普通数组和 POSIX awk，兼容系统自带的 Bash 3。
header_candidates() {
    local LC_ALL=C index relative
    for index in "${!header_targets[@]}"; do
        relative=${header_targets[index]}
        printf '%q\t%d\n' "$relative" "$index"
        while [[ $relative == */* ]]; do
            relative=${relative%/*}
            printf '%q\t-1\n' "$relative"
        done
    done
}
winner_ids=()
complete=false
while IFS= read -r index; do
    if [[ $index == done ]]; then complete=true; break; fi
    winner_ids+=("$index")
done < <(header_candidates | LC_ALL=C sort -t $'\t' -k1,1 -k2,2n |
    LC_ALL=C awk -F '\t' '
        function emit() { if (last >= 0) print last }
        BEGIN { last = -1 }
        {
            key = ":" $1
            if (key != previous) { emit(); previous = key; directory = 0 }
            last = $2 + 0
            if (last < 0) directory = 1
            else if (directory) { bad = 1; exit 1 }
        }
        END { if (!bad) emit() }' && printf 'done\n')
[[ $complete == true ]] || fail "头目录包含文件、目录冲突或无法生成清单 $headers"

# 头映射可用于核对同一目标库目录尚未交付的配置；正式发布还要检查实际库文件。
if [[ $header_map == true ]]; then
    for index in "${winner_ids[@]}"; do
        printf '%s\0%s\0' "${header_paths[index]#"$source_dir/"}" "${header_targets[index]}"
    done
    exit 0
fi

# 去掉归档名的 lib 前缀后，匹配最终公开头的基名。
has_library_header() {
    local base=${1#lib} index file
    for index in "${winner_ids[@]}"; do
        file=${header_targets[index]}
        case "${file##*/}" in "$base.h"|"$base.hh"|"$base.hpp"|"$base.hxx") return 0 ;; esac
    done
    return 1
}

# 库名以原始制品为准。可用换行分隔的精确文件清单声明多档案依赖。
libraries=()
if [[ -n ${THIRDPARTY_FILES:-} ]]; then
    while IFS= read -r name || [[ -n $name ]]; do
        valid_name "$name" && [[ -f $library_dir/$name ]] || fail "缺少库文件 $library_dir/$name"
        libraries+=("$library_dir/$name")
    done <<< "$THIRDPARTY_FILES"
elif [[ $headers_only == false ]]; then
    for file in "$library_dir"/*; do
        [[ -f $file ]] || continue
        name=${file##*/}
        case "$name" in
            *.dll.a) stem=${name%.dll.a}; ext=.dll.a ;;
            *.so|*.so.*) stem=${name%%.so*}; ext=${name#"$stem"} ;;
            *.a|*.lib|*.dll|*.dylib|*.pdb) stem=${name%.*}; ext=.${name##*.} ;;
            *) continue ;;
        esac
        flavor=release
        abi=
        if [[ ( $ext == .dll || $ext == .dylib ) && $stem =~ ^(.+d)([-.][0-9]+([.][0-9]+)*)$ ]]; then
            stem=${BASH_REMATCH[1]}
            abi=${BASH_REMATCH[2]}
        fi
        if [[ $stem == *d ]]; then
            if [[ $crt == mdd || -f $library_dir/${stem%d}$abi$ext ]]; then
                flavor=debug
            elif has_library_header "$stem"; then
                flavor=release
            else
                fail "$name 的末尾 d 无法区分配置，请用 THIRDPARTY_FILES 声明目标文件"
            fi
        fi
        if [[ $flavor == "$config" ]]; then libraries+=("$file"); fi
    done
fi
[[ $headers_only == true || ${libraries[0]+yes} ]] || fail "缺少 $config 库文件 $library_dir"

# 先在目标文件系统内准备本次完整产物，以便通过重命名发布和恢复。
sdk=${CLOUD_REBUILD_SDK_ROOT:-$root}
mkdir -p "$sdk"
sdk=$(cd "$sdk" && pwd -P)
stage=$(mktemp -d "$sdk/.thirdparty-$vendor.XXXXXXXX")
destination="$sdk/include/$namespace"
lock="$sdk/.thirdparty-publish.lock"
locked=false
committed=false
new_headers=false
cleanup() {
    local status=$? restored=true file
    trap - EXIT
    trap '' HUP INT TERM
    if [[ $locked == true && $committed == false ]]; then
        for file in "$stage/added-lib"/*; do
            rm -f -- "$sdk/lib/${file##*/}" || restored=false
        done
        for file in "$stage/previous-lib"/*; do
            mv -f -- "$file" "$sdk/lib/" || restored=false
        done
        if [[ -d $stage/previous-headers || $new_headers == true ]]; then
            if [[ -d $destination ]]; then
                mv -- "$destination" "$stage/withdrawn-headers" || restored=false
            fi
            if [[ -d $stage/previous-headers && ! -e $destination ]]; then
                mv -- "$stage/previous-headers" "$destination" || restored=false
            fi
        fi
    fi
    if [[ $locked == true && $restored == true ]]; then rmdir -- "$lock" || restored=false; fi
    if [[ $restored == true ]]; then
        if [[ $stage == "$sdk/.thirdparty-$vendor."* && -d $stage ]]; then rm -rf -- "$stage"; fi
    else
        printf 'thirdparty [%s]: 无法完整恢复，保留备份 %s\n' "$vendor" "$stage" >&2
        status=1
    fi
    exit "$status"
}
trap cleanup EXIT
trap 'exit 1' HUP INT TERM
mkdir -p "$stage/headers" "$stage/lib" "$stage/previous-lib" "$stage/added-lib"
# 每层使用同一份最终清单批量复制。以实际内容发布符号链接头，保持回执与交付字节一致。
for layer_id in "${!header_layers[@]}"; do
    for index in "${winner_ids[@]}"; do
        if [[ ${header_layer_ids[index]} == "$layer_id" ]]; then
            printf './%s\0' "${header_targets[index]}"
        fi
    done > "$stage/header-list"
    [[ -s $stage/header-list ]] || continue
    tar -chf - -C "${header_layers[layer_id]}" --null -T "$stage/header-list" |
        tar -xf - -C "$stage/headers" || fail "无法复制头目录 ${header_layers[layer_id]}"
done
if [[ $headers_only == false ]]; then cp -P "${libraries[@]}" "$stage/lib/"; fi

# 多个发布者可以各自准备文件。同一公共 SDK 的检查、替换及回滚必须串行。
for ((attempt=0; attempt<100; attempt++)); do
    if mkdir -- "$lock" 2>/dev/null; then locked=true; break; fi
    if [[ -L $lock || ( -e $lock && ! -d $lock ) ]]; then
        [[ ! -e $lock && ! -L $lock ]] || fail "无效的公共 SDK 发布锁 $lock"
    fi
    sleep 0.1
done
[[ $locked == true ]] || fail "公共 SDK 正在发布或存在未释放的锁 $lock"
publish_dirs=("$sdk/include" "$destination")
if [[ $headers_only == false ]]; then publish_dirs+=("$sdk/lib"); fi
for dir in "${publish_dirs[@]}"; do
    [[ ! -L $dir && ( ! -e $dir || -d $dir ) ]] || fail "公共目录类型错误 $dir"
done
for file in "$stage/lib"/*; do
    target="$sdk/lib/${file##*/}"
    [[ ! -d $target && ( ! -e $target || -f $target ) ]] || fail "公共库路径类型错误 $target"
done
mkdir -p "$sdk/include"
if [[ $headers_only == false ]]; then mkdir -p "$sdk/lib"; fi
if [[ -d $destination ]]; then
    mv -- "$destination" "$stage/previous-headers"
else
    new_headers=true
fi
mv -- "$stage/headers" "$destination"
for file in "$stage/lib"/*; do
    name=${file##*/}
    if [[ -e $sdk/lib/$name || -L $sdk/lib/$name ]]; then
        mv -- "$sdk/lib/$name" "$stage/previous-lib/"
    else
        : > "$stage/added-lib/$name"
    fi
    mv -- "$file" "$sdk/lib/"
done
committed=true
