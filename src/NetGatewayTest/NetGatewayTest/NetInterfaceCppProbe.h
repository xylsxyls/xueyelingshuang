#pragma once

/** 独立翻译单元通过C++主类入口调用真实库
@return 0为通过，非零为失败步骤
*/
extern "C" int netInterfaceCppProbe(void);