#ifndef CGENERALSTYLE_H
#define CGENERALSTYLE_H

#include <QObject>
#include <QColor>
#include <QFont>
#include <tchar.h>
#include "ControlsMacro.h"

class Label;
class QTextDocument;
class ControlsAPI CGeneralStyle : public QObject
{
    Q_OBJECT
private:
    CGeneralStyle();
    ~CGeneralStyle();

    static CGeneralStyle* gInstance;
    QString mWar3LobbyResourcePath;
	QString mPlatformResourcePath;
	QString mSystemAvatarDirectory;
	QString mCustomAvatarDirectory;


    QColor  mFontColr;
    QFont   mFont;

	Label* m_htmlRender;
	QTextDocument* m_doc;

	int mFontFaceId_BaiduZongYi;

	void initPath();
	void initCustomerFont();
	

public:
    enum PingLevel
    {
        PingLevel_Perfect = 0,
        PingLevel_Good    = 1,
        PingLevel_Normal  = 2,
        PingLevel_Bad     = 3,
    };

	//��ͭI   0~1000
	//��ͭII  1001~1050
	//��ͭIII 1051~1100
	//����I   1101~1150
	//����II  1151~1200
	//����III 1201~1300
	//�ƽ�I   1301~1400
	//�ƽ�II  1401~1500
	//�ƽ�III 1501~1600
	//��ʯ    1601~2000
	//����    2001+
	//����    top10
	enum BattleLevel
	{
		BattleLevel_Bronze01 ,
		BattleLevel_Bronze02 ,
		BattleLevel_Bronze03 ,
		BattleLevel_Sliver01 ,
		BattleLevel_Sliver02 ,
		BattleLevel_Sliver03 ,
		BattleLevel_Golden01 ,
		BattleLevel_Golden02 ,
		BattleLevel_Golden03 ,
		BattleLevel_Diamond  ,
		BattleLevel_King     ,
	};

    static CGeneralStyle* instance();
    static void           release();

	QString fontFace_BaiDuZongYi();
    QString war3lobbyResourcePath();
	QString platformResourcePath();

   PingLevel getPingLevel(quint64 ping); //* ��ȡping�ȼ�
	QColor    getPingColor(quint64 ping); //* ��ȡping����ɫ

	BattleLevel getBattleLevel(quint64 score); //* ��ȡ���������ֵȼ�

    QColor  fontColor();
    QFont   font();

	void initSystemAvatarDirectory();
	void initCustomAvatarDirectory();
	QString getUserAvatar(quint64 userid, quint64 imgid); //* ��ȡid


	/** ��html�ı���ʹ��Label�ؼ������壬�����С��ͨ��painter��ȡ
	@param [in] painter �滭ָ��
	@param [in] rect �滭����
	@param [in] text �ı�
	@param [in] flag ���뷽ʽ
	*/
	void drawHtmlTextByLabel(QPainter* painter,
							 const QRect& rect,
							 const QString& html,
							 Qt::Alignment flag = Qt::AlignLeft | Qt::AlignTop);

	/** ��html�ı���ʹ��Label�ؼ������壬�����С��ͨ��painter��ȡ
	@param [in] painter �滭ָ��
	@param [in] rect �滭����
	@param [in] text �ı�
	*/
	void drawHtmlTextByQTextDocument(QPainter* painter,
									 const QRect& rect,
									 const QString& html);

};

#endif // CGENERALSTYLE_H
