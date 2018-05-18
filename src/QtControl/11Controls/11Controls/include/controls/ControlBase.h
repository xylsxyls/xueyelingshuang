#pragma once
#include "../core/QssString.h"
#include <QColor>
#include "ControlsMacro.h"

class QShowEvent;
class QWidget;
template <class QBase>
class ControlShow;
/** �ؼ�ͨ��������ع���
*/
template <class QBase>
class ControlBase
{
protected:
	/** ��������
	*/
	virtual ~ControlBase();

protected:
	/** ������ʾ��ָ��
	@param [in] show ��ʾ��ָ��
	*/
	void setControlShow(ControlShow<QBase>* show);

	/** ��������
	@param [in] fontName ������
	@param [in] isItem �����Ƿ�Ϊ�ڵ�
	@param [in] rePaint �Ƿ������ػ�
	*/
	void setFontFace(const std::wstring& fontName, bool isItem, bool rePaint);

	/** �������ζ�Ӧֵ
	@param [in] keyWord �ؼ���
	@param [in] valuePx ����ֵ����������px
	@param [in] isItem �����Ƿ�Ϊ�ڵ�
	@param [in] rePaint �Ƿ������ػ�
	*/
	void setPxValue(const std::wstring& keyWord, qint32 valuePx, bool isItem, bool rePaint);

	/** ����������Ӧֵ��solid
	@param [in] keyWord �ؼ���
	@param [in] valuePx ����ֵ����������px
	@param [in] isItem �����Ƿ�Ϊ�ڵ�
	@param [in] rePaint �Ƿ������ػ�
	*/
	void setPxSolidValue(const std::wstring& keyWord, qint32 valuePxSolid, bool isItem, bool rePaint);

	/** ���ö�Ӧֵ
	@param [in] keyWord �ؼ���
	@param [in] value ֵ
	@param [in] isItem �����Ƿ�Ϊ�ڵ�
	@param [in] rePaint �Ƿ������ػ�
	*/
	void setKeyValue(const std::wstring& keyWord, const std::wstring& value, bool isItem, bool rePaint);

	/** ������ɫ
	@param [in] colorStateMap ��ɫ����
	@param [in] keyWord �ؼ���
	@param [in] isItem �����Ƿ�Ϊ�ڵ�
	@param [in] rePaint �Ƿ������ػ�
	*/
	void setColorStateMap(const std::map<qint32, std::map<qint32, QColor>>& colorStateMap,
						  const std::wstring& keyWord,
						  bool isItem,
						  bool rePaint);

	/** ����ͼƬ
	@param [in] imageStateMap ͼƬ����
	@param [in] imagePath ·��
	@param [in] stateCount ״̬����
	@param [in] keyWord �ؼ���
	@param [in] isItem �����Ƿ�Ϊ�ڵ�
	@param [in] rePaint �Ƿ������ػ�
	*/
	void setImageStateMap(const std::map<qint32, std::map<qint32, qint32>>& imageStateMap,
						  const std::wstring& imagePath,
						  qint32 stateCount,
						  const std::wstring& keyWord,
						  bool isItem,
						  bool rePaint);

	/** ����ͼƬ
	@param [in] imageStateMap ͼƬ����
	@param [in] imagePath ·��
	@param [in] stateCount ״̬����
	@param [in] keyWord �ؼ���
	@param [in] itemName �ڵ���
	@param [in] rePaint �Ƿ������ػ�
	*/
	void setImageStateMap(const std::map<qint32, std::map<qint32, qint32>>& imageStateMap,
						  const std::wstring& imagePath,
						  qint32 stateCount,
						  const std::wstring& keyWord,
						  const std::wstring& itemName,
						  bool rePaint);

private:
	ControlShow<QBase>* m_show;
};

#include "ControlBase.inl"
