
:<<!EOF!
#--------------------------------------------------------------------
DllRelyTest_bit=$1
DllRelyTest_dlllib=$2
DllRelyTest_debugRelease=$3
DllRelyTest_allSame=$4
"$CLOUD_REBUILD" DllRelyTest $DllRelyTest_bit $DllRelyTest_dlllib $DllRelyTest_debugRelease $DllRelyTest_allSame
!EOF!

#--------------------------------------------------------------------
CStringManager_bit=$1
CStringManager_dlllib=$2
CStringManager_debugRelease=$3
CStringManager_allSame=$4
"$CLOUD_REBUILD" CStringManager $CStringManager_bit $CStringManager_dlllib $CStringManager_debugRelease $CStringManager_allSame

#--------------------------------------------------------------------
FFmpegCpp_bit=$1
FFmpegCpp_dlllib=$2
FFmpegCpp_debugRelease=$3
FFmpegCpp_allSame=$4
"$CLOUD_REBUILD" FFmpegCpp $FFmpegCpp_bit $FFmpegCpp_dlllib $FFmpegCpp_debugRelease $FFmpegCpp_allSame

#--------------------------------------------------------------------
CTaskThreadManager_bit=$1
CTaskThreadManager_dlllib=$2
CTaskThreadManager_debugRelease=$3
CTaskThreadManager_allSame=$4
"$CLOUD_REBUILD" CTaskThreadManager $CTaskThreadManager_bit $CTaskThreadManager_dlllib $CTaskThreadManager_debugRelease $CTaskThreadManager_allSame

#--------------------------------------------------------------------
LockFreeQueue_bit=$1
LockFreeQueue_dlllib=$2
LockFreeQueue_debugRelease=$3
LockFreeQueue_allSame=$4
"$CLOUD_REBUILD" LockFreeQueue $LockFreeQueue_bit $LockFreeQueue_dlllib $LockFreeQueue_debugRelease $LockFreeQueue_allSame

#--------------------------------------------------------------------
LogManager_bit=$1
LogManager_dlllib=$2
LogManager_debugRelease=$3
LogManager_allSame=$4
"$CLOUD_REBUILD" LogManager $LogManager_bit $LogManager_dlllib $LogManager_debugRelease $LogManager_allSame

#--------------------------------------------------------------------
KissFFTCpp_bit=$1
KissFFTCpp_dlllib=$2
KissFFTCpp_debugRelease=$3
KissFFTCpp_allSame=$4
"$CLOUD_REBUILD" KissFFTCpp $KissFFTCpp_bit $KissFFTCpp_dlllib $KissFFTCpp_debugRelease $KissFFTCpp_allSame

# Header-only audio capture stays inside Core; FFT implementation belongs to KissFFTCpp.
root="$(cd "$(dirname "$0")/../../.." && pwd)"
mkdir -p "$root/include/miniaudio"
cp "$root/tools/LumaPlayerKaraoke/miniaudio/include/miniaudio.h" "$root/include/miniaudio/miniaudio.h"
mkdir -p "$root/include/onnxruntime"
cp "$root/tools/LumaPlayerKaraoke/directml-1.24.4/include/"*.h "$root/include/onnxruntime/" || exit 1
# Prepare local build resources, not common publication or server model downloads.
karaoke="$root/tools/LumaPlayerKaraoke"
mkdir -p "$root/lib/karaoke" || exit 1
cp -f "$karaoke/models/UVR-MDX-NET-Inst_HQ_5.onnx" "$root/lib/karaoke/" || exit 1
if [[ "$OSTYPE" == msys* || "$OSTYPE" == cygwin* ]]; then
    if [[ "$1" != '64' ]]; then
        echo 'The bundled ONNX Runtime is Windows x64; provide a matching runtime for other architectures.' >&2
        exit 1
    fi
    runtime="$karaoke/directml-1.24.4"
    mkdir -p "$root/lib/karaoke/licenses" || exit 1
    for file in onnxruntime.dll onnxruntime_providers_shared.dll DirectML.dll; do
        cp -f "$runtime/lib/x64/$file" "$root/lib/karaoke/" || exit 1
    done
    cp -f "$runtime/licenses/"*.txt "$root/lib/karaoke/licenses/" || exit 1
fi