#!/usr/bin/env bash
set -euo pipefail
root=$(cd "$(dirname "$0")" && pwd)
bits=${1:-64}
crt=${2:-MT}
case "$bits" in
    64) folder=x64; arch=x86_64; target=win64 ;;
    32) folder=x86; arch=x86; target=win32 ;;
    *) echo 'Use 32 or 64; other platforms need their own FFmpeg target SDK.' >&2; exit 1 ;;
esac
case "$crt" in MT|MD) ;; *) echo 'Use MT or MD.' >&2; exit 1 ;; esac
case "${OSTYPE:-}" in
    msys*|cygwin*) command -v cl.exe >/dev/null; command -v link.exe >/dev/null ;;
    *) echo 'This reproducer uses MSVC; for POSIX SDKs retain the same LGPL-only shared-library policy.' >&2; exit 1 ;;
esac
mkdir -p "$root/build"
archive="$root/ffmpeg-6.1.6.tar.xz"
if [[ ! -f "$archive" ]]; then
    archive="$root/build/ffmpeg-6.1.6.tar.xz"
fi
source_url='https://ffmpeg.org/releases/ffmpeg-6.1.6.tar.xz'
source_sha256='d4fcb164028dd3beee5d92c0ac72e46aac6973c75ea12dc14de07bf8f407370a'
if [[ ! -f "$archive" ]]; then
    if command -v curl >/dev/null; then
        curl --fail --location --retry 3 "$source_url" --output "$archive.partial"
    elif command -v wget >/dev/null; then
        wget --tries=3 --output-document="$archive.partial" "$source_url"
    else
        echo 'Download requires curl or wget.' >&2
        exit 1
    fi
    printf '%s  %s\n' "$source_sha256" "$archive.partial" | sha256sum --check --status
    mv "$archive.partial" "$archive"
fi
printf '%s  %s\n' "$source_sha256" "$archive" | sha256sum --check --status
tar -xf "$archive" -C "$root/build"
work="$root/build/$folder-$crt-shared"
dest="$root/build/install/$folder-$crt-shared"
mkdir -p "$work"
cd "$work"
"$root/build/ffmpeg-6.1.6/configure" \
    --prefix="$dest" --toolchain=msvc --arch="$arch" --target-os="$target" \
    --enable-cross-compile --enable-shared --disable-static --disable-programs \
    --disable-doc --disable-debug --disable-x86asm --disable-autodetect \
    --disable-avdevice --disable-avfilter --disable-postproc --disable-hwaccels \
    --disable-mediafoundation --disable-d3d11va --disable-dxva2 \
    --disable-bzlib --disable-iconv --disable-lzma --disable-zlib --disable-everything \
    --enable-avcodec --enable-avformat --enable-avutil --enable-swresample --enable-swscale \
    --enable-protocol=file,pipe \
    --enable-demuxer=aac,avi,flv,h264,hevc,image2,image2pipe,matroska,mjpeg,mov,mp3,mpegts,mpegvideo,wav \
    --enable-muxer=adts,avi,flv,image2,image2pipe,matroska,mjpeg,mp3,mp4,mpegts,null,wav \
    --enable-decoder=aac,bmp,h264,hevc,mjpeg,mp3,mpeg4,pcm_s16be,pcm_s16le,pcm_u8,rawvideo \
    --enable-encoder=bmp,mjpeg,mpeg4,pcm_s16be,pcm_s16le,pcm_u8,rawvideo \
    --enable-parser=aac,h264,hevc,mjpeg,mpeg4video,mpegaudio \
    --enable-bsf=aac_adtstoasc,extract_extradata,h264_mp4toannexb,hevc_mp4toannexb \
    --extra-cflags="-$crt -Z7 -D__func__=__FUNCTION__" \
    --extra-ldflags='-DEBUG -INCREMENTAL:NO'
"${MAKE:-make}" -j"${JOBS:-2}"
"${MAKE:-make}" install
libdir="$root/lib/$folder"
if [[ "$crt" == MD ]]; then libdir="$libdir/MD"; fi
mkdir -p "$libdir"
cp "$dest/bin/"*.dll "$libdir/"
cp "$dest/lib/"*.lib "$libdir/"
for component in avformat avcodec avutil swscale swresample; do
    cp "$work/lib$component/"*.pdb "$libdir/"
done