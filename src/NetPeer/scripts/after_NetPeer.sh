#!/bin/bash
set -e
# 普通头文件和消费Macro.h已发布；移除内部文件并选定统一NetSender入口。
script_root=$(cd -- "$(dirname -- "$0")/../../.." && pwd) || exit 1
sdk_root="${CLOUD_REBUILD_SDK_ROOT:-$script_root}"
sdk_root="${sdk_root//\\//}"
source_header="$sdk_root/include/NetPeer/NetSenderAPI.h"
sender_directory="$sdk_root/include/NetSender"

[[ -s "$source_header" && -f "$sdk_root/include/NetPeer/NetPeerMacro.h" ]] || exit 1
rm -f -- "$sdk_root/include/NetPeer/NetDirectorySnapshot.h"
rm -f -- "$sdk_root/include/NetPeer/NetEndpointFileStream.h"
rm -f -- "$sdk_root/include/NetPeer/NetEndpointRouterState.h"
rm -f -- "$sdk_root/include/NetPeer/NetFileTransferState.h"
rm -f -- "$sdk_root/include/NetPeer/NetGatewayClientChannelState.h"
rm -f -- "$sdk_root/include/NetPeer/NetGatewayDirectoryStoreState.h"
rm -f -- "$sdk_root/include/NetPeer/NetGatewayDiscoveryState.h"
rm -f -- "$sdk_root/include/NetPeer/NetHostSocketState.h"
rm -f -- "$sdk_root/include/NetPeer/NetLinkTransportState.h"
rm -f -- "$sdk_root/include/NetPeer/NetPeerBindings.h"
rm -f -- "$sdk_root/include/NetPeer/NetPeerClientState.h"
rm -f -- "$sdk_root/include/NetPeer/NetPeerFileIoState.h"
rm -f -- "$sdk_root/include/NetPeer/NetPeerMacroDll.h"
rm -f -- "$sdk_root/include/NetPeer/NetPeerMacroLib.h"
rm -f -- "$sdk_root/include/NetPeer/NetPeerSenderState.h"
rm -f -- "$sdk_root/include/NetPeer/NetRelayTransportState.h"
rm -f -- "$sdk_root/include/NetPeer/NetSenderCState.h"
rm -f -- "$sdk_root/include/NetPeer/NetSessionState.h"
rm -f -- "$sdk_root/include/NetPeer/NetTransportEntropy.h"
rm -f -- "$sdk_root/include/NetPeer/NetTransportState.h"
[[ ! -d "$sender_directory/NetSenderAPI.h" ]] || exit 1
mkdir -p -- "$sender_directory" || exit 1
staged_header=$(mktemp "$sender_directory/.NetSenderAPI.XXXXXX") || exit 1
trap 'rm -f -- "$staged_header"' EXIT
cp -p -- "$source_header" "$staged_header" && mv -f -- "$staged_header" "$sender_directory/NetSenderAPI.h"