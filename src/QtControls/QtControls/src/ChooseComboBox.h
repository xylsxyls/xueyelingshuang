#pragma once
#include "HtmlComboBox.h"
#include "QtControlsMacro.h"

class Label;
/** �����࣬����ѡ��ؼ�
*/
class QtControlsAPI ChooseComboBox : public HtmlComboBox
{
	Q_OBJECT
public:
	/** ���캯��
	@param [in] parent ������ָ��
	*/
	ChooseComboBox(QWidget* parent = nullptr);

	/** ��������
	*/
	~ChooseComboBox();

public:
	/** ��ʼ��ѡ��ؼ��������֣������һ��ִ��
	@param [in] text ��������
	*/
	void setComboBoxText(const QString& text);

	/** ����ѡ��ͼ��ͼƬ
	@param [in] borderImgPath ͼƬ·��
	@param [in] borderImgStateCount ͼƬ״̬����
	@param [in] borderImgNormal ��̬
	@param [in] borderImgHover ��ͣ״̬
	@param [in] borderImgDisabled ����״̬
	@param [in] rePaint �Ƿ���������
	*/
	void setChooseImage(const QString& borderImgPath,
						qint32 borderImgStateCount = 4,
						qint32 borderImgNormal = 1,
						qint32 borderImgHover = 2,
						qint32 borderImgDisabled = 4,
						bool rePaint = false);

	/** ����ѡ��ͼ���ƫ����
	@param [in] origin ƫ����
	*/
	void setChooseOrigin(qint32 origin);

	/** ����ѡ�������ֵ����1��ʼ
	@param [in] index ����ֵ
	*/
	void setChooseByIndex(qint32 index);

	/** ����ѡ��
	@param [in] id ѡ��ID
	*/
	void setChooseById(qint64 id);

	/** ����ѡ��ͼ���С
	@param [in] width ���
	@param [in] height �߶�
	*/
	void setChooseSize(qint32 width, qint32 height);

	/** ����������ڵ�߶�
	@param [in] height �ڵ�߶�
	@param [in] rePaint �Ƿ������ػ�
	*/
	void setListItemHeight(qint32 height, bool rePaint = false);

	/** ���
	*/
	void clear();

	/** ��ȡѡ��������ֵ
	@return ����ѡ��������ֵ
	*/
	qint32 currentChoosedIndex();

	/** ��ȡѡ�����ID
	@return ����ѡ����ID
	*/
	qint64 currentChoosedId();

	/** ��ȡѡ��������
	@return ����ѡ��������
	*/
	QString currentChoosedText();

	/** ȥ��ѡ�����ѡ��ͼ��
	*/
	void clearChoose();

Q_SIGNALS:
	void itemChoosed(qint32 index, qint64 id, const QString& text);

protected:
	void resizeEvent(QResizeEvent* eve);
	void mouseMoveEvent(QMouseEvent* eve);
	void paintEvent(QPaintEvent* eve);

protected:
	void updateChooseGeometry();
	bool check();
	void init();

private slots:
	void onItemPressed(qint32 index);

protected:
	Label* m_choose;
	qint32 m_chooseOrigin;
	qint32 m_listItemHeight;
	QString m_comboBoxText;
};