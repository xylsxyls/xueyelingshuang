#pragma once
#include <stdint.h>

/** 既有七个Core断言的统一入口，编号保持1—7
*/
class SplitViewerCoreTests
{
public:
    /** 建立独立前置场景并判定指定Core用例
    @param [in] id 登记表中的1—7，其他返回失败
    @return 所选独立断言通过
    */
    static bool run(int32_t id);
};