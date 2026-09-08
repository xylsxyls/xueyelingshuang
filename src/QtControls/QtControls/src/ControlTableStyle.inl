#ifndef CONTROLTABLESTYLE_INL__
#define CONTROLTABLESTYLE_INL__

#include "ControlTableStyle.h"
#include "ControlShow.h"
#include "ControlSubStyle.h"

template<class QBase>
ControlTableStyle<QBase>::~ControlTableStyle()
{

}

template<class QBase>
void ControlTableStyle<QBase>::setTableGridLineColor(const QColor& color, bool rePaint)
{
	ControlBase<QBase>::setKeyValue(L"gridline-color", QssHelper::QColorToWString(color), false, rePaint);
}

template<class QBase>
void ControlTableStyle<QBase>::setTableHeaderSectionHeight(qint32 height, bool rePaint)
{
	ControlShow<QBase>* show = tableControlShow();
	if (show == nullptr)
	{
		return;
	}
	ControlSubStyle::setDescendantSubControlPxValue(&show->m_controlStyle, L"QHeaderView", L"section", L"height", qMax(height, 0));
	repaintTableControl(rePaint);
}

template<class QBase>
void ControlTableStyle<QBase>::setTableHeaderSectionWidth(qint32 width, bool rePaint)
{
	ControlShow<QBase>* show = tableControlShow();
	if (show == nullptr)
	{
		return;
	}
	ControlSubStyle::setDescendantSubControlPxValue(&show->m_controlStyle, L"QHeaderView", L"section", L"width", qMax(width, 0));
	repaintTableControl(rePaint);
}

template<class QBase>
void ControlTableStyle<QBase>::setTableHeaderSectionBorderWidth(qint32 width, bool rePaint)
{
	ControlShow<QBase>* show = tableControlShow();
	if (show == nullptr)
	{
		return;
	}
	ControlSubStyle::setDescendantSubControlPxSolidValue(&show->m_controlStyle, L"QHeaderView", L"section", L"border", qMax(width, 0));
	repaintTableControl(rePaint);
}

template<class QBase>
void ControlTableStyle<QBase>::setTableHeaderSectionBorderColor(const QColor& color, bool rePaint)
{
	ControlShow<QBase>* show = tableControlShow();
	if (show == nullptr)
	{
		return;
	}
	ControlSubStyle::setDescendantSubControlKeyValue(&show->m_controlStyle, L"QHeaderView", L"section", L"border-color", QssHelper::QColorToWString(color));
	repaintTableControl(rePaint);
}

template<class QBase>
void ControlTableStyle<QBase>::setTableHeaderSectionBackgroundColor(const QColor& color, bool rePaint)
{
	ControlShow<QBase>* show = tableControlShow();
	if (show == nullptr)
	{
		return;
	}
	ControlSubStyle::setDescendantSubControlKeyValue(&show->m_controlStyle, L"QHeaderView", L"section", L"background-color", QssHelper::QColorToWString(color));
	repaintTableControl(rePaint);
}

template<class QBase>
void ControlTableStyle<QBase>::setTableHeaderSectionTextColor(const QColor& color, bool rePaint)
{
	ControlShow<QBase>* show = tableControlShow();
	if (show == nullptr)
	{
		return;
	}
	ControlSubStyle::setDescendantSubControlKeyValue(&show->m_controlStyle, L"QHeaderView", L"section", L"color", QssHelper::QColorToWString(color));
	repaintTableControl(rePaint);
}

template<class QBase>
void ControlTableStyle<QBase>::setTableCornerButtonBackgroundColor(const QColor& color, bool rePaint)
{
	ControlShow<QBase>* show = tableControlShow();
	if (show == nullptr)
	{
		return;
	}
	ControlSubStyle::setDescendantSubControlKeyValue(&show->m_controlStyle, L"QTableCornerButton", L"section", L"background-color", QssHelper::QColorToWString(color));
	repaintTableControl(rePaint);
}

template<class QBase>
void ControlTableStyle<QBase>::setTableViewportBackgroundColor(const QColor& color, bool rePaint)
{
	ControlShow<QBase>* show = tableControlShow();
	if (show == nullptr)
	{
		return;
	}
	ControlSubStyle::setDescendantKeyValue(&show->m_controlStyle, L"QTableView", L"background-color", QssHelper::QColorToWString(color));
	repaintTableControl(rePaint);
}

template<class QBase>
ControlShow<QBase>* ControlTableStyle<QBase>::tableControlShow()
{
	return dynamic_cast<ControlShow<QBase>*>(this);
}

template<class QBase>
void ControlTableStyle<QBase>::repaintTableControl(bool rePaint)
{
	if (rePaint)
	{
		ControlShow<QBase>* show = tableControlShow();
		if (show != nullptr)
		{
			show->repaint();
		}
	}
}

#endif