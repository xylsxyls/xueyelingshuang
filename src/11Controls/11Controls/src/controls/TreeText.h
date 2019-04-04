#pragma once
#include <stdint.h>
#include <QColor>

struct TreeText
{
	//�����С
	int32_t m_fontSize;
	//������ɫ
	QColor m_textColor;
	//����
	QString m_fontFace;
	//��������
	QString m_text;
	//���䷽ʽ
	Qt::Alignment m_align;
	//���ƫ����
	int32_t m_origin;
	//������ɫ
	QColor m_backgroundColor;
	//��ͣ��ɫ
	QColor m_hoverColor;
	//������ɫ
	QColor m_selectedColor;
	//������ɫ
	QColor m_disabledColor;
	//Ƕ���ͼƬ��imageΪ�ղ��ửͼ
	QImage m_image;
	//�͵�ǰ�������Ͻǵ����λ��
	QPoint m_imageLeftTop;
	//widgetΪ��ʱ����Ƕ��
	QWidget* m_widget;

	/** ���캯��
	*/
	TreeText()
	{
		init();
	}

	/** ���캯��
	@param [in] text ��������
	*/
	TreeText(const QString& text)
	{
		init();
		m_text = text;
	}

	/** ���캯��
	@param [in] text ��������
	*/
	TreeText(const char* text)
	{
		init();
		m_text = text;
	}

	/** ��ʼ��
	*/
	void init()
	{
		m_fontSize = 12;
		m_textColor = QColor(255, 255, 255, 255);
		m_fontFace = QStringLiteral("΢���ź�");
		m_align = Qt::AlignCenter;
		m_origin = 0;
		m_backgroundColor = QColor(0, 0, 0, 0);
		m_hoverColor = QColor(0, 0, 0, 0);
		m_selectedColor = QColor(0, 0, 0, 0);
		m_disabledColor = QColor(0, 0, 0, 0);
		m_imageLeftTop = QPoint(0, 0);
		m_widget = nullptr;
	}
};