#!/bin/bash
set -e
xueyelingshuang="${CLOUD_REBUILD_SDK_ROOT:-${xueyelingshuang:-${XUEYELINGSHUANG:?}}}"
rm -f -- "$xueyelingshuang/include/ProtoMessageManager/ProtoMessage.pb.h"
rm -f -- "$xueyelingshuang/include/ProtoMessageManager/ProtoMessageProtobuf.h"
