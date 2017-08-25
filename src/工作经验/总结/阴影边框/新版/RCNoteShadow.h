/*****************************************************************************
*  版权所有（C）2008-2017，上海二三四五网络科技有限公司                      *
*  保留所有权利。                                                            *
******************************************************************************
*  作者 : 杨楠                                                               *
*  版本 : 1.0                                                                *
*****************************************************************************/

#ifndef __RCNoteShadow_h_
#define __RCNoteShadow_h_ 1

#include "ui/common/RCWndShadow.h"

BEGIN_NAMESPACE_RC

/* 注释弹框阴影
*/
class RCNoteShadow :
    public RCWndShadow
{
    /* 绘制中间区域
    @param [in] layoutRect 一整块图片大小
    @param [in] destMargin 目标阴影区域
    @param [in] pImage 图片指针
    @param [in] pLayout Gdiplus绘制指针
    @param [in] rcImage 图片大小
    @param [in] margin 阴影大小
    */
    virtual void PaintCenter(const RECT& layoutRect, const RECT& destMargin, Gdiplus::Image* pImage, Gdiplus::Graphics* pLayout, const RECT& rcImage, const RECT& margin);
};

END_NAMESPACE_RC

#endif //__RCNoteShadow_h_