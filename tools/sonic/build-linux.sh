#!/usr/bin/env bash
set -e
root=$(cd "$(dirname "$0")" && pwd)
if [[ $# != 2 || ( "$1" != 32 && "$1" != 64 ) ]]; then
    echo "Usage: build-linux.sh 32-or-64 absolute-work-directory" >&2
    exit 2
fi
bits=$1
work=$2
mkdir -p "$work"
tar -xJf "$root/sonic-0.2.0-luma-r1.tar.xz" -C "$work"
folder=x86
if [[ "$bits" == 64 ]]; then
    folder=x64
fi
mkdir -p "$root/lib/$folder"
"${CC:-cc}" -m"$bits" -O2 -g -fPIC -c "$work/sonic-0.2.0-luma-r1/sonic.c" -o "$work/sonic-$bits.o"
"${AR:-ar}" rcs "$root/lib/$folder/libsonicstatic$bits.a" "$work/sonic-$bits.o"