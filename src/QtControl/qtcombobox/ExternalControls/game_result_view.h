#ifndef GAME_RESULT_VIEW_H
#define GAME_RESULT_VIEW_H
#include <QStandardItem>

class GameResultType
{
public:
	enum ResultType
	{
		ResultType_RPG = 0,
		ResultType_Ladder = 1,
		ResultType_Battle = 2, //������
		ResultType_MingJiang = 3, //����
	};

	enum TitleFlags
	{
		TitleFlags_NULL = 0x00000000,
		TitleFlags_JUN = 0x00000001, //*��
		TitleFlags_PO = 0x00000002, //*��
		TitleFlags_FU = 0x00000004, //*��
		TitleFlags_BU = 0x00000008, //*��
		TitleFlags_ZHU = 0x00000010, //*��
		TitleFlags_HUN = 0x00000020, //*��
	};

	struct Slot
	{
		bool        isMvp; //�Ƿ�mvp     war3 & rpg
		bool        isFriend; //�Ƿ����     war3 & rpg
		QString     icon; //ͷ��ͼ��·��  war3 & rpg
		QString     playerName; //�������     war3 & rpg
		bool        isEscap; //�Ƿ�����     war3 & rpg
		bool        isNew; //�Ƿ������   war3 & rpg
		bool        isDropNet; //�Ƿ����     war3 & rpg
		//QStringList headerList       ; //����ͷ�б�   war3 & rpg
		QStringList valueList; //�����б�     war3 & rpg
		QStringList valueProgressList; //���������б�  war3 & rpg
		quint64     accountId; //�˺�id      war3 & rpg
		TitleFlags  titleFlags; //�ƺ�        war3
		bool        isBiaoZhang; //����        war3 & rpg
		bool        isMe; //�Ƿ����Լ�   war3 & rpg
		quint64     totalScore; //�ܷ�        war3 & rpg

		Slot()
		{
			isMvp = false;
			isFriend = false;
			isEscap = false;
			isNew = false;
			isDropNet = false;
			accountId = false;
			titleFlags = TitleFlags_NULL;
			isBiaoZhang = false;
			isMe = false;
			totalScore = 0;
		}
	};
	typedef QList<Slot> SlotList;

	struct Camp
	{
		bool        isVictory; //�Ƿ�ʤ����    rpg & war3
		QString     campName; //��Ӫ����      rpg & war3
		QStringList headerList; //����ͷ�б�    rpg & war3
		SlotList    slotList; //��λ�б�      rpg & war3
		QString     campScore; //war3 ��Ӫ�ܷ� kill/death/assitan

		Camp()
		{
			isVictory = false;
		}
	};
	typedef QList<Camp> CampList;


	struct GameResult
	{
		ResultType   type;       //* ս������
		CampList     campList;   //* ��Ӫ�б�
		QString      tip;        //* 11��ʾ
		QString      gameTime;   //* ��Ϸʱ��(��)
		QString      startTime;  //* ��ʼʱ��
		QString      gameName;   //* ��Ϸ����
		bool         dataError;  //* ���ݴ���
		bool         invalidGame;//* ��Ч��Ϸ��

		GameResult()
		{
			type = ResultType_Ladder;
			dataError = false;
			invalidGame = false;
		}
	};
};

class GameResultPanelSlotItem:public QObject ,public QStandardItem
{
    Q_OBJECT
signals:
    void dataChanged();
public:
    void setData(const QVariant &value, int role){QStandardItem::setData(value,role);emit dataChanged();}
};

class GameResultPanelSlotItem_Ladder:public GameResultPanelSlotItem
{
public:
	enum Role
	{
		Role_CampName   = Qt::UserRole + 1,
		Role_PlayerName                   ,
		Role_Hero                         ,
		Role_KDA                          ,
		Role_HeroScore                    ,
		Role_LadderScore                  ,
		Role_HeroScoreProgress            ,
		Role_LadderScoreProgress          ,
		Role_TitleFlag                    ,
		Role_Jun                          ,
		Role_Po                           ,
		Role_Fu                           ,
		Role_Bu                           ,
		Role_Zhu                          ,
		Role_Hun                          ,
		Role_DataError                    ,
		Role_InvalidGame                  ,
	};

	GameResultPanelSlotItem_Ladder(){}
	~GameResultPanelSlotItem_Ladder(){}

	void setCampName           (const QString &s){setData(s, Role_CampName           );}
	void setPlayerName         (const QString &s){setData(s, Role_PlayerName         );}
	void setHero               (const QString &s){setData(s, Role_Hero               );}
	void setKDA                (const QString &s){setData(s, Role_KDA                );}
	void setHeroScore          (const QString &s){setData(s, Role_HeroScore          );}
	void setLadderScore        (const QString &s){setData(s, Role_LadderScore        );}
	void setHeroScoreProgress  (const QString &s){setData(s, Role_HeroScoreProgress  );}
	void setLadderScoreProgress(const QString &s){setData(s, Role_LadderScoreProgress);}
	void setTitleFlag          (int            s){setData(s, Role_TitleFlag          );}
	void setJun                (bool           s){setData(s, Role_Jun                );}
	void setPo                 (bool           s){setData(s, Role_Po                 );}
	void setFu                 (bool           s){setData(s, Role_Fu                 );}
	void setBu                 (bool           s){setData(s, Role_Bu                 );}
	void setZhu                (bool           s){setData(s, Role_Zhu                );}
	void setHun                (bool           s){setData(s, Role_Hun                );}
	void setDataError          (bool           s){setData(s, Role_DataError          );}
	void setInvalidGame        (bool           s){setData(s, Role_InvalidGame        );}

	QString campName           (){return data(Role_CampName           ).toString();}
	QString playerName         (){return data(Role_PlayerName         ).toString();}
	QString hero               (){return data(Role_Hero               ).toString();}
	QString KDA                (){return data(Role_KDA                ).toString();}
	QString heroScore          (){return data(Role_HeroScore          ).toString();}
	QString ladderScore        (){return data(Role_LadderScore        ).toString();}
	QString heroScoreProgress  (){return data(Role_HeroScoreProgress  ).toString();}
	QString ladderScoreProgress(){return data(Role_LadderScoreProgress).toString();}
	int     titleFlag          (){return data(Role_TitleFlag          ).toInt   ();}
	bool    jun                (){return data(Role_Jun                ).toBool  ();}
	bool    po                 (){return data(Role_Po                 ).toBool  ();}
	bool    fu                 (){return data(Role_Fu                 ).toBool  ();}
	bool    bu                 (){return data(Role_Bu                 ).toBool  ();}
	bool    zhu                (){return data(Role_Zhu                ).toBool  ();}
	bool    hun                (){return data(Role_Hun                ).toBool  ();}
	bool    dataError          (){return data(Role_DataError          ).toBool  ();}
	bool    invalidGame        (){return data(Role_InvalidGame        ).toBool  ();}
};

class GameResultPanelSlotItem_MingJiang:public GameResultPanelSlotItem
{
public:
	enum Role
	{
		Role_CampName   = Qt::UserRole + 1,
		Role_PlayerName                   ,
		Role_Hero                         ,
		Role_KDA                          ,
		Role_HeroScore                    ,
		Role_MingJiangScore               ,
		Role_HeroScoreProgress            ,
		Role_MingJiangScoreProgress       ,
		Role_TitleFlag                    ,
		Role_Jun                          ,
		Role_Po                           ,
		Role_Fu                           ,
		Role_Bu                           ,
		Role_Zhu                          ,
		Role_Hun                          ,
		Role_DataError                    ,
		Role_InvalidGame                  ,
	};

	GameResultPanelSlotItem_MingJiang(){}
	~GameResultPanelSlotItem_MingJiang(){}

	void setCampName               (const QString &s){setData(s, Role_CampName              );}
	void setPlayerName             (const QString &s){setData(s, Role_PlayerName            );}
	void setHero                   (const QString &s){setData(s, Role_Hero                  );}
	void setKDA                    (const QString &s){setData(s, Role_KDA                   );}
	void setHeroScore              (const QString &s){setData(s, Role_HeroScore             );}
	void setMingJiangScore         (const QString &s){setData(s, Role_MingJiangScore        );}
	void setHeroScoreProgress      (const QString &s){setData(s, Role_HeroScoreProgress     );}
	void setMingJiangScoreProgress (const QString &s){setData(s, Role_MingJiangScoreProgress);}
	void setTitleFlag              (int            s){setData(s, Role_TitleFlag             );}
	void setJun                    (bool           s){setData(s, Role_Jun                   );}
	void setPo                     (bool           s){setData(s, Role_Po                    );}
	void setFu                     (bool           s){setData(s, Role_Fu                    );}
	void setBu                     (bool           s){setData(s, Role_Bu                    );}
	void setZhu                    (bool           s){setData(s, Role_Zhu                   );}
	void setHun                    (bool           s){setData(s, Role_Hun                   );}
	void setDataError              (bool           s){setData(s, Role_DataError             );}
	void setInvalidGame            (bool           s){setData(s, Role_InvalidGame           );}

	QString campName              (){return data(Role_CampName              ).toString();}
	QString playerName            (){return data(Role_PlayerName            ).toString();}
	QString hero                  (){return data(Role_Hero                  ).toString();}
	QString KDA                   (){return data(Role_KDA                   ).toString();}
	QString heroScore             (){return data(Role_HeroScore             ).toString();}
	QString mingJiangScore        (){return data(Role_MingJiangScore        ).toString();}
	QString heroScoreProgress     (){return data(Role_HeroScoreProgress     ).toString();}
	QString mingJiangScoreProgress(){return data(Role_MingJiangScoreProgress).toString();}
	int     titleFlag             (){return data(Role_TitleFlag             ).toInt   ();}
	bool    jun                   (){return data(Role_Jun                   ).toBool  ();}
	bool    po                    (){return data(Role_Po                    ).toBool  ();}
	bool    fu                    (){return data(Role_Fu                    ).toBool  ();}
	bool    bu                    (){return data(Role_Bu                    ).toBool  ();}
	bool    zhu                   (){return data(Role_Zhu                   ).toBool  ();}
	bool    hun                   (){return data(Role_Hun                   ).toBool  ();}
	bool    dataError             (){return data(Role_DataError             ).toBool  ();}
	bool    invalidGame           (){return data(Role_InvalidGame           ).toBool  ();}
};


class GameResultPanelSlotItem_Max:public GameResultPanelSlotItem
{
public:
    enum Role
    {
        Role_IsMvp             = Qt::UserRole + 1,
        Role_IsFriend                            ,
        Role_Icon                                ,
        Role_PlayerName                          ,
        Role_IsEscap                             ,
        Role_IsNew                               ,
        Role_IsDropNet                           ,
        Role_HeaderList                          ,
        Role_ValueList                           ,
        Role_ValueProgressList                   ,
        Role_AccountId                           ,
        Role_Ex                                  ,
        Role_IsBiaoZhang                         ,
        Role_IsMe                                ,
    };

    void setIsMvp            (bool              s){setData(s, Role_IsMvp            );}
    void setIsFriend         (bool              s){setData(s, Role_IsFriend         );}
    void setIcon             (const QString&    s){setData(s, Role_Icon             );}
    void setPlayerName       (const QString&    s){setData(s, Role_PlayerName       );}
    void setIsEscap          (bool              s){setData(s, Role_IsEscap          );}
    void setIsNew            (bool              s){setData(s, Role_IsNew            );}
    void setIsDropNet        (bool              s){setData(s, Role_IsDropNet        );}
    void setHeaderList       (const QStringList s){setData(s, Role_HeaderList       );}
    void setValueList        (const QStringList s){setData(s, Role_ValueList        );}
    void setValueProgressList(const QStringList s){setData(s, Role_ValueProgressList);}
    void setAccountId        (quint64           s){setData(s, Role_AccountId        );}
    void setEx               (quint64           s){setData(s, Role_Ex               );}
    void setIsBiaoZhang      (bool              s){setData(s, Role_IsBiaoZhang      );}
    void setIsMe             (bool              s){setData(s, Role_IsMe             );}

    bool        isMvp            (){return data(Role_IsMvp            ).toBool      ();}
    bool        isFriend         (){return data(Role_IsFriend         ).toBool      ();}
    QString     icon             (){return data(Role_Icon             ).toString    ();}
    QString     playerName       (){return data(Role_PlayerName       ).toString    ();}
    bool        isEscap          (){return data(Role_IsEscap          ).toBool      ();}
    bool        isNew            (){return data(Role_IsNew            ).toBool      ();}
    bool        isDropNet        (){return data(Role_IsDropNet        ).toBool      ();}
    QStringList headerList       (){return data(Role_HeaderList       ).toStringList();}
    QStringList valueList        (){return data(Role_ValueList        ).toStringList();}
    QStringList valueProgressList(){return data(Role_ValueProgressList).toStringList();}
    quint64     accountId        (){return data(Role_AccountId        ).toULongLong ();}
    quint64     ex               (){return data(Role_Ex               ).toULongLong ();}
    bool        isBiaoZhang      (){return data(Role_IsBiaoZhang      ).toBool      ();}
    bool        isMe             (){return data(Role_IsMe             ).toBool      ();}
};


class GameResultPanelCampItem_Max:public GameResultPanelSlotItem
{
public:
    enum Role{
        Role_IsVictory = Qt::UserRole + 1,
        Role_CampName                    ,
        Role_HeaderList                  ,
        Role_CampScore                   ,
    };

    void setIsVictory (bool               s){setData(s, Role_IsVictory );}
    void setCampName  (const QString&     s){setData(s, Role_CampName  );}
    void setHeaderList(const QStringList& s){setData(s, Role_HeaderList);}
    void setCampScore (const QString&     s){setData(s, Role_CampScore );}

    bool        isVictory (){return data(Role_IsVictory ).toBool      ();}
    QString     campName  (){return data(Role_CampName  ).toString    ();}
    QStringList headerList(){return data(Role_HeaderList).toStringList();}
    QString     campScore (){return data(Role_CampScore ).toString    ();}
};




#endif //GAME_RESULT_VIEW_H

