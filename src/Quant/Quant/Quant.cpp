#include "Quant.h"
#include "QuantParam.h"
#include "CSystem/CSystemAPI.h"
#include "NetSender/NetSenderAPI.h"
#include "ClientReceive.h"
#include "QtControls/COriginalButton.h"
#include "Config.h"
#include "DialogManager/DialogManagerAPI.h"
#include "StockCharge/StockChargeAPI.h"
#include "SQLite/SQLiteAPI.h"
#include "SqlString/SqlStringAPI.h"
#include "HiRedis/HiRedisAPI.h"
#include "IntDateTime/IntDateTimeAPI.h"
#include "CStringManager/CStringManagerAPI.h"
#include "Util.h"
#include "Cini/CiniAPI.h"
#include "RedisManager.h"
#include "Market.h"
#include "CompetitionManager.h"
#include "QuantStrategyManager.h"
#include "Strategy.h"
#include "Fund.h"
#include <QMessageBox>
#include <QScrollArea>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QMessageBox>
#include "Display.h"
#include "CSystem/CSystemAPI.h"
#include "Ctxt/CtxtAPI.h"
#include "AnalyzeTask.h"
#include "StockManager.h"
#include "StockCharge/StockChargeAPI.h"
#include "CollectTask.h"
#include "VerifyTask.h"
#include "VerifyManager.h"
#include "StrategyPlotWidget.h"

Quant::Quant(QWidget* parent):
	QMainWindow(parent),
	m_threadId(0)
{
	ui.setupUi(this);
	m_button = new COriginalButton(this);
	m_calc = new COriginalButton(this);
	m_createFile = new COriginalButton(this);
	m_saveFile = new COriginalButton(this);
	m_initRedis = new COriginalButton(this);
	m_profit = new COriginalButton(this);
	m_displayResult = new COriginalButton(this);
	m_analyze = new COriginalButton(this);
	m_collect = new COriginalButton(this);
	m_verify = new COriginalButton(this);
	init();
}

Quant::~Quant()
{
	// 清理资源
	CompetitionManager::instance().uninit();
	CTaskThreadManager::Instance().Uninit(m_threadId);
}

void Quant::init()
{
	if (!check())
	{
		return;
	}

	m_threadId = CTaskThreadManager::Instance().Init();

	m_clientReceive = new ClientReceive;
	NetSender::instance().initClientReceive(m_clientReceive);
	ProcessWork::instance().initReceive();
	NetSender::instance().initClient(PROJECT_QUANT, std::string("QuantServer") + QUANT_SERVER_VERSION, g_config.m_computerName);

	HiRedis& redis = RedisManager::instance().m_redis;

	bool connect = redis.connect("127.0.0.1");
	if (!connect)
	{
		int result = system("start D:\\redis7.4.2\\redis-server.exe");
		connect = redis.connect("127.0.0.1");
		//TipDialogParam tipDialogParam;
		//tipDialogParam.m_tip = QStringLiteral("redis not start");
		//tipDialogParam.m_parent = windowHandle();
		//DialogManager::instance().makeDialog(tipDialogParam);
		//return;
	}
	redis.selectDbIndex(0);

	QPalette pattle;
	pattle.setColor(QPalette::Background, QColor(50, 0, 0, 255));
	setPalette(pattle);

#ifdef _DEBUG
	setWindowTitle("Quantd");
#else
	setWindowTitle("Quant1.0");
#endif

	QColor normal(180, 0, 0, 255);
	QColor hover(0, 180, 0, 255);
	QColor passed(0, 0, 180, 255);
	QColor disable(100, 0, 0, 255);

	m_button->setText(QStringLiteral("send_test"));
	m_button->setBkgColor(normal, hover, passed, disable);
	QObject::connect(m_button, &COriginalButton::clicked, this, &Quant::onButtonClicked);

	m_calc->setText(QStringLiteral("calc"));
	m_calc->setBkgColor(normal, hover, passed, disable);
	QObject::connect(m_calc, &COriginalButton::clicked, this, &Quant::onCalcClicked);

	m_createFile->setText(QStringLiteral("create_file"));
	m_createFile->setBkgColor(normal, hover, passed, disable);
	QObject::connect(m_createFile, &COriginalButton::clicked, this, &Quant::onCreateFileClicked);

	m_saveFile->setText(QStringLiteral("save_file"));
	m_saveFile->setBkgColor(normal, hover, passed, disable);
	QObject::connect(m_saveFile, &COriginalButton::clicked, this, &Quant::onSaveFileClicked);

	m_initRedis->setText(QStringLiteral("init_redis"));
	m_initRedis->setBkgColor(normal, hover, passed, disable);
	QObject::connect(m_initRedis, &COriginalButton::clicked, this, &Quant::onInitRedisClicked);

	m_profit->setText(QStringLiteral("profit"));
	m_profit->setBkgColor(normal, hover, passed, disable);
	QObject::connect(m_profit, &COriginalButton::clicked, this, &Quant::onProfitClicked);

	m_displayResult->setText(QStringLiteral("display_result"));
	m_displayResult->setBkgColor(normal, hover, passed, disable);
	QObject::connect(m_displayResult, &COriginalButton::clicked, this, &Quant::onDisplayResultClicked);

	m_analyze->setText(QStringLiteral("analyze"));
	m_analyze->setBkgColor(normal, hover, passed, disable);
	QObject::connect(m_analyze, &COriginalButton::clicked, this, &Quant::onAnalyzeClicked);

	m_collect->setText(QStringLiteral("collect"));
	m_collect->setBkgColor(normal, hover, passed, disable);
	QObject::connect(m_collect, &COriginalButton::clicked, this, &Quant::onCollectClicked);

	m_verify->setText(QStringLiteral("verify"));
	m_verify->setBkgColor(normal, hover, passed, disable);
	QObject::connect(m_verify, &COriginalButton::clicked, this, &Quant::onVerifyClicked);

	QObject::connect(this, &Quant::historyFutureSignal, this, &Quant::onHistoryFutureSignal, Qt::QueuedConnection);

	StockCharge::instance().init("0.00016", 5);

	// 初始化竞赛管理器
	CompetitionManager::instance().init();

	// 初始化股票管理器
	StockManager::instance().init();

	// 初始化展示类
	m_display = std::make_shared<Display>();

	RCSend("量化回测系统初始化完成");
}

bool Quant::check()
{
	return true;
	return m_button != nullptr;
}

void Quant::resizeEvent(QResizeEvent* eve)
{
	QMainWindow::resizeEvent(eve);

	if (!check())
	{
		return;
	}

	std::vector<COriginalButton*> vecButton;
	vecButton.push_back(m_button);
	vecButton.push_back(m_calc);
	vecButton.push_back(m_createFile);
	vecButton.push_back(m_saveFile);
	vecButton.push_back(m_initRedis);
	vecButton.push_back(m_profit);
	vecButton.push_back(m_displayResult);
	vecButton.push_back(m_analyze);
	vecButton.push_back(m_collect);
	vecButton.push_back(m_verify);

	int32_t cowCount = 4;
	int32_t width = 140;
	int32_t height = 30;
	int32_t space = 10;

	int32_t index = -1;
	while (index++ != vecButton.size() - 1)
	{
		vecButton[index]->setGeometry(index % cowCount * (width + space), index / cowCount * (height + space), width, height);
	}
}

void Quant::closeEvent(QCloseEvent* eve)
{
	ProcessWork::instance().uninitReceive();
	delete m_clientReceive;
	m_clientReceive = nullptr;
}

void Quant::onButtonClicked()
{
	NetSender::instance().sendServer(PROJECT_QUANT, std::to_string(QUANT_PRINT));
}

void Quant::onCalcClicked()
{
	BigNumber all;
	{
		BigNumber sell = 6.08;
		BigNumber buy = 6.0575;
		BigNumber money = 0;
		BigNumber position = 1000;
		int day = 200;
		while (day-- != 0)
		{
			if (day % 10 == 0 || day % 10 == 1 || day % 10 == 5 || day % 10 == 6)
			{
				sell = 6.08;
			}
			else if (day % 10 == 2)
			{
				sell = 6.08;
			}
			else if (day % 10 == 7)
			{
				sell = 6.08;
			}
			else
			{
				sell = 6.08;
			}
			auto sellFee = StockCharge::instance().sellFee("002567", sell, position * 100);
			money = money + sell * position * 100 - sellFee;
			position = (money / (buy * 100)).toPrec(0, BigNumber::ROUND_OFF);
			auto buyFee = StockCharge::instance().buyFee("002567", buy, position * 100);
			money = money - buy * position * 100 - buyFee;
		}
		all = ((((all + money + buy * position * 100) / (buy * 100000)).toPrec(4) - 1) * 100).toPrec(2);
	}
	TipDialogParam tipDialogParam;
	tipDialogParam.m_tip = QString::fromStdString(all.toString());
	tipDialogParam.m_parent = windowHandle();
	DialogManager::instance().makeDialog(tipDialogParam);
}

void Quant::onCreateFileClicked()
{
	std::vector<std::string> vecIni = CSystem::findFilePath(g_config.m_iniDbPath, 2, "ini");
	int32_t iniIndex = -1;
	while (iniIndex++ != vecIni.size() - 1)
	{
		const std::string& path = vecIni[iniIndex];
		std::string stock = CSystem::GetName(path, 3);
		SQLite lite(g_config.m_dbPath + "/" + stock + ".db");
		lite.open();
		std::string sqlString = SqlString::createTableString(stock, Util::allField(true));
		auto prepare = lite.preparedCreator(sqlString);
		SQLiteResultSet result = lite.execute(prepare);
		lite.close();
	}
	RCSend("create success");
}

void Quant::onSaveFileClicked()
{
	std::vector<std::string> vecIni = CSystem::findFilePath(g_config.m_iniDbPath, 2, "ini");
	int32_t iniIndex = -1;
	while (iniIndex++ != vecIni.size() - 1)
	{
		const std::string& path = vecIni[iniIndex];
		std::string stock = CSystem::GetName(path, 3);
		SQLite lite(g_config.m_dbPath + "/" + stock + ".db");
		lite.open();
		std::vector<std::string> vecField = Util::allField(false);
		std::string fields;
		for (uint32_t fieldIndex = 0; fieldIndex < vecField.size(); ++fieldIndex)
		{
			fields += vecField[fieldIndex] + ",";
		}
		if (!fields.empty())
		{
			fields.pop_back();
		}
		Cini ini(path, true);
		std::vector<std::string> vecSection = ini.getAllSection();
		for (uint32_t dateIndex = 0; dateIndex < vecSection.size(); ++dateIndex)
		{
			std::vector<int32_t> dateInfo;
			dateInfo.resize((uint32_t)Overall::COUNT + (uint32_t)ObserveTime::COUNT +
				(uint32_t)ObserveTime::COUNT * (uint32_t)RangeTime::COUNT * (uint32_t)TransType::COUNT);
			::memset((char*)dateInfo.data(), 0, dateInfo.size() * sizeof(int32_t));
			const std::string& section = vecSection[dateIndex];
			std::string date = section.substr(4, section.size() - 4);
			dateInfo[0] = atoi(date.c_str());
			std::vector<std::string> vecAllKey = ini.getAllKey(section);
			for (uint32_t keyIndex = 0; keyIndex < vecAllKey.size(); ++keyIndex)
			{
				const std::string& key = vecAllKey[keyIndex];
				if (key == "open")
				{
					dateInfo[(uint32_t)Overall::OPEN] = (uint32_t)(atof(ini.readIni(key, section).c_str()) * 100);
				}
				else if (key == "close")
				{
					dateInfo[(uint32_t)Overall::CLOSE] = (uint32_t)(atof(ini.readIni(key, section).c_str()) * 100);
				}
				else
				{
					ObserveTime observeTime = Util::observeStringToTime(key);
					if (observeTime == ObserveTime::COUNT)
					{
						continue;
					}
					std::vector<std::string> vecPrice = CStringManager::split(
						CStringManager::Replace(ini.readIni(key, section).c_str(), ".", ""), ",");
					if (vecPrice.empty())
					{
						continue;
					}
					dateInfo[(uint32_t)Overall::COUNT + (uint32_t)observeTime] = atoi(vecPrice[0].c_str());
					for (uint32_t priceIndex = 1; priceIndex < vecPrice.size(); ++priceIndex)
					{
						dateInfo[(uint32_t)Overall::COUNT + (uint32_t)ObserveTime::COUNT +
							(uint32_t)observeTime * (uint32_t)RangeTime::COUNT * (uint32_t)TransType::COUNT +
							priceIndex - 1] = atoi(vecPrice[priceIndex].c_str());
					}
				}
			}
			std::string sqlString = SqlString::deleteString(stock, "overall_0=" + std::to_string(dateInfo[0]));
			auto prepare = lite.preparedCreator(sqlString);
			lite.execute(prepare);
			sqlString = SqlString::insertString(stock, fields);
			prepare = lite.preparedCreator(sqlString);
			for (uint32_t dataIndex = 0; dataIndex < dateInfo.size(); ++dataIndex)
			{
				prepare.setInt(dataIndex, dateInfo[dataIndex]);
			}
			SQLiteResultSet result = lite.execute(prepare);
		}
		lite.close();
	}
	RCSend("save db success");
}

void Quant::onInitRedisClicked()
{
	HiRedis& redis = RedisManager::instance().m_redis;
	std::vector<std::string> vecDb = CSystem::findFilePath(g_config.m_dbPath, 2, "db");
	int32_t index = -1;
	while (index++ != vecDb.size() - 1)
	{
		const std::string& dbPath = vecDb[index];
		std::string dbName = CSystem::GetName(dbPath, 3);
		SQLite lite(dbPath);
		lite.open();
		std::string field;
		for (uint32_t overallIndex = 0; overallIndex < (uint32_t)Overall::COUNT; ++overallIndex)
		{
			std::string fieldName = "overall_" + std::to_string(overallIndex);
			field += fieldName + ",";
			redis.deleteOrderGroupElementsByScore(dbName + ":" + fieldName, g_config.m_allBeginTime, g_config.m_allEndTime);
		}
		for (uint32_t observeIndex = 0; observeIndex < (uint32_t)ObserveTime::COUNT; ++observeIndex)
		{
			std::string fieldName = "observe_" + std::to_string(observeIndex);
			field += "observe_" + std::to_string(observeIndex) + ",";
			redis.deleteOrderGroupElementsByScore(dbName + ":" + fieldName, g_config.m_allBeginTime, g_config.m_allEndTime);
		}
		for (uint32_t observeIndex = 0; observeIndex < (uint32_t)ObserveTime::COUNT; ++observeIndex)
		{
			for (uint32_t rangeIndex = 0; rangeIndex < (uint32_t)RangeTime::COUNT; ++rangeIndex)
			{
				for (uint32_t transIndex = 0; transIndex < (uint32_t)TransType::COUNT; ++transIndex)
				{
					std::string fieldName = CStringManager::Format("price_%d_%d_%d", observeIndex, rangeIndex, transIndex);
					field += fieldName + ",";
					redis.deleteOrderGroupElementsByScore(dbName + ":" + fieldName, g_config.m_allBeginTime, g_config.m_allEndTime);
				}
			}
		}
		field.pop_back();
		std::string sqlString = SqlString::selectString(dbName, field, "overall_0>" + std::to_string(g_config.m_allBeginTime));
		auto prepare = lite.preparedCreator(sqlString);
		SQLiteResultSet result = lite.execute(prepare);
		if (result.empty())
		{
			continue;
		}
		std::vector<std::vector<std::string>> vecResult = result.toVector();
		int32_t dateIndex = -1;
		while (dateIndex++ != vecResult.size() - 1)
		{
			const std::vector<std::string>& dateInfo = vecResult[dateIndex];
			if (dateInfo.empty())
			{
				continue;
			}
			int32_t dateScore = std::atoi(dateInfo[0].c_str());
			int32_t infoIndex = -1;
			while (infoIndex++ != dateInfo.size() - 1)
			{
				if (infoIndex < (uint32_t)Overall::COUNT)
				{
					std::string groupName = dbName + ":overall_" + std::to_string(infoIndex);
					bool res = redis.setOrderGroup(groupName, dateScore, dateInfo[0] + dateInfo[infoIndex]);
				}
				else if (infoIndex < (uint32_t)Overall::COUNT + (uint32_t)ObserveTime::COUNT)
				{
					std::string groupName = dbName + ":observe_" +
						std::to_string(infoIndex - (uint32_t)Overall::COUNT);
					bool res = redis.setOrderGroup(groupName, dateScore, dateInfo[0] + dateInfo[infoIndex]);
				}
				else
				{
					uint32_t observeIndex = 0;
					uint32_t rangeIndex = 0;
					uint32_t transIndex = 0;
					Util::getAllIndex(infoIndex, observeIndex, rangeIndex, transIndex,
						(uint32_t)Overall::COUNT + (uint32_t)ObserveTime::COUNT);
					std::string groupName = CStringManager::Format("%s:price_%d_%d_%d",
						dbName.c_str(), observeIndex, rangeIndex, transIndex);
					bool res = redis.setOrderGroup(groupName, dateScore, dateInfo[0] + dateInfo[infoIndex]);
				}
			}
		}
	}
	RCSend("init redis success");
}

void Quant::onProfitClicked()
{
	RCSend("开始策略竞赛...");
	g_config.m_time.SetWatchTime(0);
	g_config.m_completeTaskCount = 0;
	g_config.m_ignoreTaskCount = 0;

	int32_t profitBeginTime = 20250815;
	int32_t profitEndTime = 20250829;
	std::string stock = "600975";

	std::vector<std::vector<int32_t>> result =
		Util::getAllStockData(stock, g_config.m_allBeginTime, g_config.m_allEndTime);

	// 创建市场数据
	auto marketData = std::make_shared<Market>();
	marketData->init(profitBeginTime, profitEndTime);
	marketData->addStock(stock);

	// 创建竞赛配置
	CompetitionConfig config;
	config.beginTime = profitBeginTime;
	config.endTime = profitEndTime;
	config.stocks = { stock };
	config.marketData = marketData;
	config.initialFund = g_config.m_initialFund; // 100万初始资金

	// 生成策略参数组合
	// 卖出时间点：0=10:40, 1=10:50, 2=11:00
	// 买入时间点：0=13:40, 1=13:50, 2=14:00  
	// 反追参数：7,8,9
	// 降价参数：1,2,3
	std::vector<std::vector<int32_t>> params =
	{
		{(int32_t)ObserveTime::TIME1100},
		{(int32_t)ObserveTime::TIME1400},
		{ 8 },
		{ 1 }
	};
	config.allParam = Util::combinatoricsToAllParam(params);

	if (0)
	{
		std::vector<std::vector<int32_t>> params =
		{
			{
				(int32_t)ObserveTime::TIME0940,
				(int32_t)ObserveTime::TIME0950, (int32_t)ObserveTime::TIME1000, (int32_t)ObserveTime::TIME1010,
				(int32_t)ObserveTime::TIME1020, (int32_t)ObserveTime::TIME1030, (int32_t)ObserveTime::TIME1040,
				(int32_t)ObserveTime::TIME1050, (int32_t)ObserveTime::TIME1100, (int32_t)ObserveTime::TIME1110,
				(int32_t)ObserveTime::TIME1120,
			},
			{
				(int32_t)ObserveTime::TIME1310,
				(int32_t)ObserveTime::TIME1320, (int32_t)ObserveTime::TIME1330, (int32_t)ObserveTime::TIME1340,
				(int32_t)ObserveTime::TIME1350, (int32_t)ObserveTime::TIME1400, (int32_t)ObserveTime::TIME1410,
				(int32_t)ObserveTime::TIME1420, (int32_t)ObserveTime::TIME1430, (int32_t)ObserveTime::TIME1440,
				(int32_t)ObserveTime::TIME1450,
			},
			{ 7, 8, 9, 10, 11, 12, 13 },
			{ 1, 2, 3, 4, 5, 6 }
		};
		config.allParam = Util::combinatoricsToAllParam(params);
	}

	//CompetitionManager::instance().addCompetition(StrategyMode::S1100B1400, config);

	{
		CompetitionConfig config;
		config.beginTime = profitBeginTime;
		config.endTime = profitEndTime;
		config.stocks = { stock };
		config.marketData = marketData;
		config.initialFund = g_config.m_initialFund; // 100万初始资金

		// 挂卖价时间点
		// 直卖价时间点
		// 挂买价时间点
		// 直买价时间点
		// 观察价A时间点
		// 观察价B时间点
		// 观察价B-A大于等于多少时当天直买，第二天早上不卖
		// 观察价B-A小于等于多少时当天不买，剩余的部分，当天直买，第二天早上卖
		// 当天早上卖出后反追差价

		//第5名, param = 10:00, 11 : 00, 14 : 40, 14 : 40, 09 : 50, 14 : 40, 0, -1, 8
		//第5名, Wave, tProfit = 65304.24元, trade = 8176.96元, tAnnual = 321.12%, annual = 20.33%
		//第4名, param = 10 : 30, 10 : 30, 14 : 40, 14 : 40, 09 : 50, 14 : 40, 0, -1, 8
		//第4名, Wave, tProfit = 65304.24元, trade = 8176.96元, tAnnual = 321.12%, annual = 20.33%
		//第3名, param = 10 : 00, 10 : 30, 14 : 40, 14 : 40, 09 : 50, 14 : 40, 0, -1, 8
		//第3名, Wave, tProfit = 65304.24元, trade = 8176.96元, tAnnual = 321.12%, annual = 20.33%
		//第2名, param = 10 : 10, 11 : 10, 14 : 40, 14 : 40, 09 : 50, 14 : 40, 0, -1, 8
		//第2名, Wave, tProfit = 65304.24元, trade = 8176.96元, tAnnual = 321.12%, annual = 20.33%
		//第1名, param = 10 : 10, 11 : 00, 14 : 40, 14 : 40, 09 : 50, 14 : 40, 0, -1, 8
		//第1名, Wave, tProfit = 65304.24元, trade = 8176.96元, tAnnual = 321.12%, annual = 20.33%
		//param = 10:10, 10:20, 14:00, 14:40, 10:40, 13:40, 0, -1, 8, 0
		std::vector<std::vector<int32_t>> params =
		{
			//10:00, 11:20, 14:00, 14:10, 10:50, 14:00, 0, -2, 7, 7, 1
			{ (int32_t)ObserveTime::TIME1000 },
			{ (int32_t)ObserveTime::TIME1120 },
			{ (int32_t)ObserveTime::TIME1400 },
			{ (int32_t)ObserveTime::TIME1410 },
			{ (int32_t)ObserveTime::TIME1050 },
			{ (int32_t)ObserveTime::TIME1400 },
			{ 0 },
			{ -2 },
			{ 7 },
			{ 7 },
			{ 1 }
		};
		config.allParam = Util::combinatoricsToAllParam(params);

		if (0)
		{
			std::vector<std::vector<int32_t>> params =
			{
				{
					//(int32_t)ObserveTime::TIME0940, (int32_t)ObserveTime::TIME0950,
					(int32_t)ObserveTime::TIME1000, (int32_t)ObserveTime::TIME1010,
					(int32_t)ObserveTime::TIME1020, (int32_t)ObserveTime::TIME1030, (int32_t)ObserveTime::TIME1040,
					(int32_t)ObserveTime::TIME1050, (int32_t)ObserveTime::TIME1100//, (int32_t)ObserveTime::TIME1110,
					//(int32_t)ObserveTime::TIME1120,
				},
				{
					//(int32_t)ObserveTime::TIME0940, (int32_t)ObserveTime::TIME0950,
					//(int32_t)ObserveTime::TIME1000, (int32_t)ObserveTime::TIME1010,
					(int32_t)ObserveTime::TIME1020, (int32_t)ObserveTime::TIME1030, (int32_t)ObserveTime::TIME1040,
					(int32_t)ObserveTime::TIME1050, (int32_t)ObserveTime::TIME1100, (int32_t)ObserveTime::TIME1110,
					(int32_t)ObserveTime::TIME1120,
				},
				{
					//(int32_t)ObserveTime::TIME1310, (int32_t)ObserveTime::TIME1320,
					(int32_t)ObserveTime::TIME1330, (int32_t)ObserveTime::TIME1340,
					(int32_t)ObserveTime::TIME1350, (int32_t)ObserveTime::TIME1400, (int32_t)ObserveTime::TIME1410,
					//(int32_t)ObserveTime::TIME1420, (int32_t)ObserveTime::TIME1430, (int32_t)ObserveTime::TIME1440,
					//(int32_t)ObserveTime::TIME1450,
				},
				{
					//(int32_t)ObserveTime::TIME1310, (int32_t)ObserveTime::TIME1320,
					(int32_t)ObserveTime::TIME1330, (int32_t)ObserveTime::TIME1340,
					(int32_t)ObserveTime::TIME1350, (int32_t)ObserveTime::TIME1400, (int32_t)ObserveTime::TIME1410,
					(int32_t)ObserveTime::TIME1420, (int32_t)ObserveTime::TIME1430, (int32_t)ObserveTime::TIME1440,
					//(int32_t)ObserveTime::TIME1450,
				},
				{
					//(int32_t)ObserveTime::TIME0940, (int32_t)ObserveTime::TIME0950,
					//(int32_t)ObserveTime::TIME1000, (int32_t)ObserveTime::TIME1010,
					(int32_t)ObserveTime::TIME1020, (int32_t)ObserveTime::TIME1030, (int32_t)ObserveTime::TIME1040,
					(int32_t)ObserveTime::TIME1050, (int32_t)ObserveTime::TIME1100, (int32_t)ObserveTime::TIME1110,
					(int32_t)ObserveTime::TIME1120,
				},
				{
					//(int32_t)ObserveTime::TIME1310,
					(int32_t)ObserveTime::TIME1320, (int32_t)ObserveTime::TIME1330, (int32_t)ObserveTime::TIME1340,
					(int32_t)ObserveTime::TIME1350, (int32_t)ObserveTime::TIME1400, (int32_t)ObserveTime::TIME1410,
					//(int32_t)ObserveTime::TIME1420, (int32_t)ObserveTime::TIME1430, (int32_t)ObserveTime::TIME1440,
					//(int32_t)ObserveTime::TIME1450,
				},
				{ 0 },
				{ -2 },
				{ 5, 6, 7, 8 },
				{ 5, 6, 7, 8, 9, 10, 100 },
				{ 0, 1 }
			};
			config.allParam = Util::combinatoricsToAllParam(params);
		}

		CompetitionManager::instance().addCompetition(StrategyMode::WAVE, config);
	}
	CompetitionManager::instance().setParam(true);

	// 开始竞赛
	if (!CompetitionManager::instance().startCompetition())
	{
		RCSend("策略竞赛启动失败");
		return;
	}
	RCSend("策略竞赛已开始");
}

void Quant::onDisplayResultClicked()
{
	if (!m_display)
	{
		RCSend("展示组件未初始化");
		return;
	}

	RCSend("开始展示竞赛结果...");

	displayAllStrategies();

	RCSend("竞赛结果展示完成");
}

void Quant::onAnalyzeClicked()
{
	std::vector<std::string> vecStock = { "600975" };
	for (size_t stockIndex = 0; stockIndex < vecStock.size(); ++stockIndex)
	{
		std::vector<uint32_t> vecThreadIds;
		int32_t threadIdIndex = -1;
		while (threadIdIndex++ != g_config.m_cpuCoreCount * 2 - 1)
		{
			vecThreadIds.push_back(CTaskThreadManager::Instance().Init());
		}
		threadIdIndex = -1;
		LockFreeQueue<std::vector<int>> queue;

		const std::string& stock = vecStock[stockIndex];
		std::vector<std::string> vecFilePath = CSystem::findFilePath("D:\\stock\\" + stock, 2, "txt");
		for (size_t index = 0; index < vecFilePath.size(); ++index)
		{
			RCSend("index = %d", index);
			const std::string& filePath = vecFilePath[index];
			std::shared_ptr<AnalyzeTask> spTask(new AnalyzeTask);
			spTask->setParam(filePath, &queue);
			++threadIdIndex;
			uint32_t threadId = vecThreadIds[threadIdIndex % vecThreadIds.size()];
			CTaskThreadManager::Instance().GetThreadInterface(threadId)->PostTask(spTask);
		}

		threadIdIndex = -1;
		while (threadIdIndex++ != vecThreadIds.size() - 1)
		{
			CTaskThreadManager::Instance().WaitForEnd(vecThreadIds[threadIdIndex]);
		}

		Cini ini(g_config.m_currentExePath + stock + ".ini", true);
		std::vector<std::string> strAllDay = ini.getAllSection();
		std::vector<int32_t> allDay = Util::groupToInt(strAllDay, 4);
		std::map<int32_t, std::vector<int32_t>> dayMap;
		std::vector<int32_t> dayInfo;
		while (queue.pop(&dayInfo))
		{
			if (dayInfo.empty())
			{
				continue;
			}
			dayMap[dayInfo[0]] = dayInfo;
		}
		for (auto itDay = dayMap.begin(); itDay != dayMap.end(); ++itDay)
		{
			int32_t day = itDay->first;
			dayInfo = itDay->second;
			std::string sectionDay = "date" + std::to_string(day);
			auto it = std::find(allDay.begin(), allDay.end(), day);
			if (it != allDay.end())
			{
				ini.deleteSection(sectionDay);
			}
			ini.writeIni("open", Util::transYuan(dayInfo[1]), sectionDay);
			ini.writeIni("close", Util::transYuan(dayInfo[2]), sectionDay);

			int32_t count = (int32_t)RangeTime::COUNT * (int32_t)TransType::COUNT;
			for (int32_t timeIndex = (int32_t)ObserveTime::TIME0930; timeIndex < (int32_t)ObserveTime::COUNT; ++timeIndex)
			{
				std::string observeStr;
				observeStr += Util::transYuan(dayInfo[(int32_t)Overall::COUNT + timeIndex]) + ",";
				int32_t index = -1;
				while (index++ != count - 1)
				{
					observeStr += Util::transYuan(dayInfo[(int32_t)Overall::COUNT +
						(int32_t)ObserveTime::COUNT +
						timeIndex * count +
						index]) + ",";
				}
				std::string timeStr = Util::observeTimeToString((ObserveTime)timeIndex);
				if (timeStr.empty() || observeStr.empty())
				{
					continue;
				}
				observeStr.pop_back();
				ini.writeIni(timeStr, observeStr, sectionDay);
			}
		}
	}
	RCSend("analyze success");
}

void Quant::onCollectClicked()
{
	showMinimized();
	std::shared_ptr<CollectTask> spCollectTask(new CollectTask);
	spCollectTask->setParam(10);
	CTaskThreadManager::Instance().GetThreadInterface(m_threadId)->PostTask(spCollectTask);
}

void Quant::onVerifyClicked()
{
	int32_t profitBeginTime = 20250321;
	int32_t profitEndTime = 20250407;
	std::string stock = "600975";

	// 创建市场数据
	auto marketData = std::make_shared<Market>();
	marketData->init(g_config.m_allBeginTime, g_config.m_allEndTime);
	marketData->addStock(stock);

	// 创建竞赛配置
	CompetitionConfig config;
	config.beginTime = profitBeginTime;
	config.endTime = profitEndTime;
	config.stocks = { stock };
	config.marketData = marketData;
	config.initialFund = g_config.m_initialFund; // 100万初始资金

	// 挂卖价时间点
	// 直卖价时间点
	// 挂买价时间点
	// 直买价时间点
	// 观察价A时间点
	// 观察价B时间点
	// 观察价B-A大于等于多少时当天直买，第二天早上不卖
	// 观察价B-A小于等于多少时当天不买，剩余的部分，当天直买，第二天早上卖
	// 当天早上卖出后反追差价
	std::vector<std::vector<int32_t>> params =
	{
		{
			//(int32_t)ObserveTime::TIME0940, (int32_t)ObserveTime::TIME0950,
			(int32_t)ObserveTime::TIME1000, (int32_t)ObserveTime::TIME1010,
			(int32_t)ObserveTime::TIME1020, (int32_t)ObserveTime::TIME1030, (int32_t)ObserveTime::TIME1040,
			(int32_t)ObserveTime::TIME1050, (int32_t)ObserveTime::TIME1100//, (int32_t)ObserveTime::TIME1110,
			//(int32_t)ObserveTime::TIME1120,
		},
		{
			//(int32_t)ObserveTime::TIME0940, (int32_t)ObserveTime::TIME0950,
			//(int32_t)ObserveTime::TIME1000, (int32_t)ObserveTime::TIME1010,
			(int32_t)ObserveTime::TIME1020, (int32_t)ObserveTime::TIME1030, (int32_t)ObserveTime::TIME1040,
			(int32_t)ObserveTime::TIME1050, (int32_t)ObserveTime::TIME1100, (int32_t)ObserveTime::TIME1110,
			(int32_t)ObserveTime::TIME1120,
		},
		{
			//(int32_t)ObserveTime::TIME1310, (int32_t)ObserveTime::TIME1320,
			(int32_t)ObserveTime::TIME1330, (int32_t)ObserveTime::TIME1340,
			(int32_t)ObserveTime::TIME1350, (int32_t)ObserveTime::TIME1400, (int32_t)ObserveTime::TIME1410,
			//(int32_t)ObserveTime::TIME1420, (int32_t)ObserveTime::TIME1430, (int32_t)ObserveTime::TIME1440,
			//(int32_t)ObserveTime::TIME1450,
		},
		{
			//(int32_t)ObserveTime::TIME1310, (int32_t)ObserveTime::TIME1320,
			(int32_t)ObserveTime::TIME1330, (int32_t)ObserveTime::TIME1340,
			(int32_t)ObserveTime::TIME1350, (int32_t)ObserveTime::TIME1400, (int32_t)ObserveTime::TIME1410,
			(int32_t)ObserveTime::TIME1420, (int32_t)ObserveTime::TIME1430, (int32_t)ObserveTime::TIME1440,
			//(int32_t)ObserveTime::TIME1450,
		},
		{
			//(int32_t)ObserveTime::TIME0940, (int32_t)ObserveTime::TIME0950,
			//(int32_t)ObserveTime::TIME1000, (int32_t)ObserveTime::TIME1010,
			(int32_t)ObserveTime::TIME1020, (int32_t)ObserveTime::TIME1030, (int32_t)ObserveTime::TIME1040,
			(int32_t)ObserveTime::TIME1050, (int32_t)ObserveTime::TIME1100, (int32_t)ObserveTime::TIME1110,
			(int32_t)ObserveTime::TIME1120,
		},
		{
			//(int32_t)ObserveTime::TIME1310,
			(int32_t)ObserveTime::TIME1320, (int32_t)ObserveTime::TIME1330, (int32_t)ObserveTime::TIME1340,
			(int32_t)ObserveTime::TIME1350, (int32_t)ObserveTime::TIME1400, (int32_t)ObserveTime::TIME1410,
			//(int32_t)ObserveTime::TIME1420, (int32_t)ObserveTime::TIME1430, (int32_t)ObserveTime::TIME1440,
			//(int32_t)ObserveTime::TIME1450,
		},
		{ 0 },
		{ -2 },
		{ 7, 8 },
		{ 7, 8, 100 },
		{ 0, 1 }
	};
	config.allParam = Util::combinatoricsToAllParam(params);

	std::shared_ptr<VerifyTask> spVerifyTask(new VerifyTask);
	spVerifyTask->setParam(this, profitBeginTime, profitEndTime, 2, 1, StrategyMode::WAVE, config, false, true, true);
	CTaskThreadManager::Instance().GetThreadInterface(m_threadId)->PostTask(spVerifyTask);
}

void Quant::onHistoryFutureSignal()
{
	RCSend("onHistoryFutureSignal");
	auto detectMap = VerifyManager::instance().getResult();
	StrategyPlotWidget* widget = new StrategyPlotWidget(nullptr);
	widget->setStrategyData(detectMap);
	widget->show();
}

void Quant::startProgressMonitoring()
{
	// 创建一个定时器来监控竞赛进度
	QTimer* progressTimer = new QTimer(this);
	connect(progressTimer, &QTimer::timeout, this, [this, progressTimer]() {
		int progress = 100;// CompetitionManager::instance().getProgress();
		RCSend("竞赛进度: %d%%", progress);

		// 更新进度条或其他UI元素
		// ui.progressBar->setValue(progress);

		if (progress >= 100)
		{
			progressTimer->stop();
			progressTimer->deleteLater();

			//if (CompetitionManager::instance().isCompleted())
			//{
			//	RCSend("策略竞赛已完成");
			//
			//	// 自动显示结果（可选）
			//	// onDisplayResultClicked();
			//}
			//else
			//{
			//	RCSend("策略竞赛被中断");
			//}
		}
	});

	progressTimer->start(1000); // 每秒更新一次进度
}

void Quant::displayAllStrategies()
{
	if (!m_display)
	{
		QMessageBox::warning(this, "错误", "展示组件未正确初始化");
		return;
	}

	// 获取竞赛结果
	CompetitionFinalResult result;// = CompetitionManager::instance().getFinalResult();

	// 设置结果到展示类
	m_display->setCompetitionResult(result);

	// 创建展示窗口
	QWidget* displayWindow = new QWidget(this);
	displayWindow->setWindowTitle("量化策略回测结果");
	displayWindow->resize(1000, 700);

	// 创建滚动区域
	QScrollArea* scrollArea = new QScrollArea(displayWindow);
	scrollArea->setWidgetResizable(true);

	// 创建滚动内容
	QWidget* scrollContent = new QWidget(scrollArea);
	QVBoxLayout* mainLayout = new QVBoxLayout(scrollContent);

	// 添加标题
	QLabel* titleLabel = new QLabel("策略竞赛结果汇总", scrollContent);
	titleLabel->setStyleSheet("font-size: 20px; font-weight: bold; margin: 10px;");
	mainLayout->addWidget(titleLabel);

	// 添加统计信息
	QLabel* statsLabel = new QLabel(
		QString("总策略数: %1, 完成策略数: %2, 最佳收益率: %3%, 平均收益率: %4%")
		.arg(result.totalStrategies)
		.arg(result.completedStrategies)
		.arg(result.bestReturn.toDouble() * 100, 0, 'f', 2)
		.arg(result.averageReturn.toDouble() * 100, 0, 'f', 2),
		scrollContent);
	statsLabel->setStyleSheet("font-size: 14px; margin: 10px;");
	mainLayout->addWidget(statsLabel);

	// 创建策略展示网格
	QGridLayout* gridLayout = new QGridLayout();
	int columns = 2; // 每行2个策略窗口

	// 为每个策略创建展示部件
	for (size_t i = 0; i < result.rankedResults.size(); ++i)
	{
		// 创建策略展示框架
		QFrame* strategyFrame = new QFrame(scrollContent);
		strategyFrame->setFrameStyle(QFrame::Box);
		strategyFrame->setLineWidth(1);
		strategyFrame->setFixedSize(480, 200);

		QVBoxLayout* frameLayout = new QVBoxLayout(strategyFrame);

		// 排名和参数
		QLabel* rankLabel = new QLabel(
			QString("排名: %1").arg(i + 1), strategyFrame);
		rankLabel->setStyleSheet("font-weight: bold; font-size: 16px;");
		frameLayout->addWidget(rankLabel);

		QString paramsText = "参数: ";
		for (size_t j = 0; j < result.rankedResults[i].params.size(); ++j)
		{
			paramsText += QString::number(result.rankedResults[i].params[j]);
			if (j < result.rankedResults[i].params.size() - 1)
			{
				paramsText += ", ";
			}
		}
		QLabel* paramsLabel = new QLabel(paramsText, strategyFrame);
		frameLayout->addWidget(paramsLabel);

		// 关键指标
		QGridLayout* metricsLayout = new QGridLayout();

		QLabel* returnLabel = new QLabel(
			QString("收益率: %1%").arg((double)result.rankedResults[i].totalReturn * 100, 0, 'f', 2),
			strategyFrame);
		QLabel* annualLabel = new QLabel(
			QString("年化: %1%").arg(result.rankedResults[i].annualReturn.toDouble() * 100, 0, 'f', 2),
			strategyFrame);
		QLabel* drawdownLabel = new QLabel(
			QString("回撤: %1%").arg(result.rankedResults[i].maxDrawdown.toDouble() * 100, 0, 'f', 2),
			strategyFrame);
		QLabel* winRateLabel = new QLabel(
			QString("胜率: %1%").arg(result.rankedResults[i].winRate.toDouble() * 100, 0, 'f', 2),
			strategyFrame);
		QLabel* healthLabel = new QLabel(
			QString("健康值: %1").arg(result.rankedResults[i].healthScore.toDouble(), 0, 'f', 1),
			strategyFrame);

		// 设置颜色
		if (result.rankedResults[i].totalReturn >= 0)
		{
			returnLabel->setStyleSheet("color: green; font-weight: bold;");
		}
		else
		{
			returnLabel->setStyleSheet("color: red; font-weight: bold;");
		}

		if (result.rankedResults[i].healthScore.toDouble() >= 80)
		{
			healthLabel->setStyleSheet("color: green; font-weight: bold;");
		}
		else if (result.rankedResults[i].healthScore.toDouble() >= 60)
		{
			healthLabel->setStyleSheet("color: orange; font-weight: bold;");
		}
		else
		{
			healthLabel->setStyleSheet("color: red; font-weight: bold;");
		}

		metricsLayout->addWidget(returnLabel, 0, 0);
		metricsLayout->addWidget(annualLabel, 0, 1);
		metricsLayout->addWidget(drawdownLabel, 1, 0);
		metricsLayout->addWidget(winRateLabel, 1, 1);
		metricsLayout->addWidget(healthLabel, 2, 0, 1, 2);

		frameLayout->addLayout(metricsLayout);

		// 添加到网格布局
		int row = (int32_t)i / columns;
		int col = (int32_t)i % columns;
		gridLayout->addWidget(strategyFrame, row, col);
	}

	mainLayout->addLayout(gridLayout);
	mainLayout->addStretch();

	// 设置滚动内容
	scrollArea->setWidget(scrollContent);

	// 设置主布局
	QVBoxLayout* windowLayout = new QVBoxLayout(displayWindow);
	windowLayout->addWidget(scrollArea);

	// 显示窗口
	displayWindow->show();
}