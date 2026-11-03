#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/** 验证纯C NetWork门面与NetPeer依赖类型可以通过唯一公开头共存
@return 0通过，否则为失败步骤
*/
int netSenderProviderCProbe(void);

#ifdef __cplusplus
}
#endif