#!/usr/bin/env bash
# 先解析目标，再把相应 SDK 的头和库发布到调用方选定的公共目录。
set -eu
set -o pipefail
shopt -s nullglob dotglob

fail() { printf 'thirdparty [%s]: %s\n' "${vendor:-?}" "$*" >&2; exit 1; }
usage() { fail 'expected --target-platform BIT, --host-platform, --host-bits, [--pause-missing] --host-tool NAME TOOL, [--pause-missing] [--print-rely|--header-map] [--headers-only] NAME PLATFORM [CRT] static|dynamic x86|x64 debug|release, or the CloudRebuild arguments NAME BIT dll|lib debug|release [SCOPE]'; }
valid_name() { [[ $1 =~ ^[A-Za-z0-9][A-Za-z0-9_.+-]*$ ]]; }

in_list() {
    local value=$1 item
    shift
    for item in "$@"; do [[ $item != "$value" ]] || return 0; done
    return 1
}

# 缺少包内输入时，由调用者选择严格失败或按键跳过当前步骤。
missing_input() {
    if [[ $pause_missing != true ]]; then fail "$1"; fi
    printf 'thirdparty [%s]: %s\n按任意键跳过此步骤并继续构建，Ctrl+C 取消。\n' "$vendor" "$1" >&2
    IFS= read -r -s -n 1 || fail '输入已关闭，未收到跳过确认'
    printf 'thirdparty [%s]: 已跳过此步骤。\n' "$vendor" >&2
    exit 0
}

pause_missing=false
if [[ ${1:-} == --pause-missing ]]; then pause_missing=true; shift; fi

# 宿主查询仅查看运行此脚本的机器及包内 bin，不发布 SDK。
case "${1:-}" in
    --host-platform|--host-bits|--host-tool)
        query=$1
        if [[ $query == --host-tool ]]; then
            [[ $# == 3 ]] || usage
            vendor=$2
            tool=$3
            valid_name "$vendor" && valid_name "$tool" || usage
        else
            [[ $# == 1 ]] || usage
        fi
        machine=$(uname -m)
        system=$(uname -s)
        case "$system" in
            MINGW*|MSYS*|CYGWIN*)
                host=windows
                machine=${PROCESSOR_ARCHITEW6432:-${PROCESSOR_ARCHITECTURE:-$machine}} ;;
            Linux)
                case "$machine" in arm*|aarch64*) host=arm ;; *) host=linux ;; esac ;;
            Darwin)
                case "$machine" in arm64|aarch64) host=mac-apple-silicon ;; *) host=mac-intel ;; esac ;;
            *) fail "不支持的宿主系统 $system ($machine)" ;;
        esac
        case "$machine" in
            x86_64|amd64|AMD64|arm64|ARM64|aarch64) widths=(x64 x86) ;;
            i?86|x86|X86|arm|armv*) widths=(x86) ;;
            *) fail "不支持的宿主架构 $system $machine" ;;
        esac
        case "$query" in
            --host-platform) printf '%s\n' "$host"; exit 0 ;;
            --host-bits) printf '%s\n' "${widths[0]}"; exit 0 ;;
        esac
        tools_dir=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd -P)
        if [[ $host == windows && $tool != *.exe ]]; then tool+=.exe; fi
        checked=()
        for width in "${widths[@]}"; do
            relative="bin/$host/$width/$tool"
            checked+=("$tools_dir/$vendor/$relative")
            if [[ -f "$tools_dir/$vendor/$relative" ]]; then
                printf '%s\n' "$relative"
                exit 0
            fi
        done
        printf -v paths '  %s\n' "${checked[@]}"
        missing_input "找不到宿主工具 $tool，已检查"$'\n'"$paths" ;;
esac

target_prefixes=(arm android wasm mac-intel mac-apple-silicon iOS-arm iOS-intel-simulator iOS-apple-silicon-simulator)
platforms=(windows linux "${target_prefixes[@]}")

# 带前缀的 BIT 已确定目标，只有 32/64 需要区分 Windows 和 Linux 宿主。
resolve_target_platform() {
    case "$1" in
        32|64)
            case "$(uname -s)" in
                MINGW*|MSYS*|CYGWIN*) platform=windows ;;
                Linux) platform=linux ;;
                *) fail "目标 $1 只适用于 Windows 或 Linux 宿主" ;;
            esac ;;
        *32|*64)
            platform=${1%??}
            in_list "$platform" "${target_prefixes[@]}" || usage ;;
        *) usage ;;
    esac
}

if [[ ${1:-} == --target-platform ]]; then
    [[ $# == 2 ]] || usage
    resolve_target_platform "$2"
    printf '%s\n' "$platform"
    exit 0
fi

header_map=false
print_rely=false
headers_only=false
while [[ ${1:-} == --* ]]; do
    case "$1" in
        --header-map) header_map=true ;;
        --print-rely) print_rely=true ;;
        --headers-only) headers_only=true ;;
        *) usage ;;
    esac
    shift
done
[[ $header_map == false || ( $print_rely == false && $pause_missing == false ) ]] || usage
vendor=${1:-}
valid_name "$vendor" || usage
shift
crt=
case "${1:-}" in
    *32|*64)
        [[ $# == 3 || $# == 4 ]] || usage
        target=$1
        linkage=$2
        config=$3
        case "$target" in
            *32) bits=x86 ;;
            *64) bits=x64 ;;
        esac
        resolve_target_platform "$target"
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
# 版本包的原生头沿用库名，Wasm 回执按完整包目录名查找头文件。
namespace=$vendor
if [[ $platform != wasm ]]; then
    if [[ $vendor =~ ^(.+)-[0-9]+\.[0-9]+\.[0-9]+$ || $vendor =~ ^(.*[^0-9])[0-9]+_[0-9]+_[0-9]+$ ]]; then
        namespace=${BASH_REMATCH[1]}
    fi
fi
namespace=${THIRDPARTY_INCLUDE_NAME:-$namespace}
valid_name "$namespace" || fail "invalid header namespace: $namespace"
library_dir="$source_dir/lib/$platform${crt:+/$crt}/$linkage/$bits"
# 同一库的 Windows 制品可单独指定名称，目标平台由上面的统一规则决定。
if [[ $platform == windows && -n ${THIRDPARTY_WINDOWS_FILES:-} ]]; then THIRDPARTY_FILES=$THIRDPARTY_WINDOWS_FILES; fi
headers="$source_dir/include"
if [[ $header_map == false && $headers_only == false ]]; then pause_missing=true; fi
if [[ $headers_only == true ]]; then
    source_archives=("$source_dir"/*.tar.xz)
    [[ ! -e $source_dir/lib && ! -L $source_dir/lib && ! ${source_archives[0]+yes} && -z ${THIRDPARTY_FILES:-} ]] ||
        fail '纯头包只存 include 和许可证，不能包含库、源码包或声明待复制的库文件'
fi
[[ -d $headers ]] || missing_input "缺少头目录 $headers"

# 显式配置目录决定整组文件及同名覆盖关系。平铺目录按二进制名的 d 后缀分组。
select_directory() {
    local dir=$1 mode=$2 selected_dir=$1 file name stem flavor found=false
    libraries=()
    common_files=()
    if [[ -d $dir/debug || -d $dir/release ]]; then
        selected_dir="$dir/$mode"
        [[ -d $selected_dir ]] || return 1
        for file in "$dir"/*; do
            if [[ -f $file && ! -f "$selected_dir/${file##*/}" ]]; then common_files+=("$file"); fi
        done
    fi
    for file in "$selected_dir"/*; do
        [[ -f $file ]] || continue
        name=${file##*/}
        case "$name" in
            *.dll.a) stem=${name%.dll.a} ;;
            *.so|*.so.*) stem=${name%%.so*} ;;
            *.a|*.lib|*.dll|*.dylib|*.pdb) stem=${name%.*} ;;
            *) libraries+=("$file"); continue ;;
        esac
        if [[ $selected_dir == "$dir" ]]; then
            # 带版本号的库可采用 food-1.dll、libfood.1.dylib 或 foo-1d.dll。
            if [[ $stem =~ ^(.+d)[.-][0-9]+([.][0-9]+)*$ ]]; then stem=${BASH_REMATCH[1]}; fi
            flavor=release
            if [[ $stem == *d ]]; then flavor=debug; fi
            [[ $flavor == "$mode" ]] || continue
        fi
        libraries+=("$file")
        if [[ $name != *.pdb ]]; then found=true; fi
    done
    [[ $found == true ]]
}

requested_config=$config
libraries=()
if [[ $headers_only == false ]]; then
    runtimes=("$crt")
    if [[ $platform == windows && $config == debug && $crt == mdd ]]; then runtimes+=(md mt); fi
    modes=("$config")
    if [[ $config == debug ]]; then modes+=(release); fi
    selected=false
    for mode in "${modes[@]}"; do
        for runtime in "${runtimes[@]}"; do
            dir="$source_dir/lib/$platform${runtime:+/$runtime}/$linkage/$bits"
            [[ -d $dir ]] || continue
            if select_directory "$dir" "$mode"; then
                library_dir=$dir
                config=$mode
                selected=true
                break 2
            fi
        done
    done
    if [[ $selected == false && ( $header_map == false || ! -d $library_dir ) ]]; then
        missing_input "缺少 $config 可用库 $library_dir（$platform/$linkage/$bits）"
    fi
fi

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
[[ ${header_paths[0]+yes} ]] || missing_input "头目录没有文件 $headers"

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

# 公共依赖随所选配置一起发布。显式清单只调整业务库的选择及链接顺序。
if [[ $headers_only == false && ${common_files[0]+yes} ]]; then
    libraries=("${common_files[@]}" "${libraries[@]}")
fi
if [[ -n ${THIRDPARTY_FILES:-} ]]; then
    names=()
    while IFS= read -r name || [[ -n $name ]]; do
        valid_name "$name" || fail "无效的库文件名 $name"
        names+=("$name")
    done <<< "$THIRDPARTY_FILES"

    ordered=()
    if [[ ${common_files[0]+yes} ]]; then ordered=("${common_files[@]}"); fi
    for name in "${names[@]}"; do
        fallback=
        if [[ $requested_config == debug && $config == release && $name =~ ^(.+)d([.-].+)$ ]]; then
            fallback=${BASH_REMATCH[1]}${BASH_REMATCH[2]}
        fi
        match=
        for file in "${libraries[@]}"; do
            if [[ ${file##*/} == "$name" ]]; then match=$file; break; fi
            if [[ ${file##*/} == "$fallback" ]]; then match=$file; fi
        done
        [[ -n $match ]] || missing_input "所选 $config 配置缺少库文件 $name"
        if [[ ! ${ordered[0]+yes} ]] || ! in_list "$match" "${ordered[@]}"; then ordered+=("$match"); fi
    done
    libraries=("${ordered[@]}")
fi

# 从本次选中的制品产生链接记录，保留显式文件清单的链接顺序。
rely_records=()
if [[ $print_rely == true && $headers_only == false ]]; then
    for file in "${libraries[@]}"; do
        name=${file##*/}
        case "$platform:$name" in
            wasm:*.a) kind=vendor; value="tools/${file#"$tools_dir/"}" ;;
            wasm:*) continue ;;
            *:*.a|*:*.lib|*:*.so|*:*.so.*|*:*.dylib) kind=link; value=$name ;;
            *) continue ;;
        esac
        case "$value" in *..*|*';'*|*$'\r'*|*$'\n'*|*$'\t'*) fail "文件名无法写入 CloudRebuild 记录 $value" ;; esac
        rely_records+=("$kind"$'\t'"$value")
    done
fi

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
    if [[ $status == 0 && $committed == true && ${rely_records[0]+yes} ]]; then
        printf '%s\n' "${rely_records[@]}" || exit 1
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
