/*****************************************************************************
*  版权所有（C）2008-2017，上海二三四五网络科技有限公司                      *
*  保留所有权利。                                                            *
******************************************************************************
*  作者 : 杨楠                                                               *
*  版本 : 1.0                                                                *
*****************************************************************************/

#include "ui/common/RCNoteShadow.h"
#include "crtdbg.h"

BEGIN_NAMESPACE_RC

void RCNoteShadow::PaintCenter(const RECT& layoutRect, const RECT& destMargin, Gdiplus::Image* pImage, Gdiplus::Graphics* pLayout, const RECT& rcImage, const RECT& margin)
{
    Gdiplus::Rect rcdest(layoutRect.left + destMargin.left + m_nRadius,
        layoutRect.top + destMargin.top + m_nRadius,
        layoutRect.right - destMargin.right - layoutRect.left - destMargin.left - m_nRadius,
        layoutRect.bottom - destMargin.bottom - layoutRect.top - destMargin.top - m_nRadius);
    pLayout->DrawImage(pImage,
        rcdest,
        rcImage.left + margin.left,
        rcImage.top + margin.top,
        rcImage.right - margin.right - rcImage.left - margin.left,
        rcImage.bottom - margin.bottom - rcImage.top - margin.top,
        Gdiplus::UnitPixel);
}

END_NAMESPACE_RC