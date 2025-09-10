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

Quant::Quant(QWidget* parent):
	QMainWindow(parent),
	m_allBeginTime(20250201),
	m_allEndTime(30250101)
{
	ui.setupUi(this);
	m_button = new COriginalButton(this);
	m_calc = new COriginalButton(this);
	m_createFile = new COriginalButton(this);
	m_saveFile = new COriginalButton(this);
	m_initRedis = new COriginalButton(this);
	m_profit = new COriginalButton(this);
	init();
}

Quant::~Quant()
{

}

void Quant::init()
{
	if (!check())
	{
		return;
	}

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
	pattle.setColor(QPalette::Background, QColor(100, 0, 0, 255));
	setPalette(pattle);

	m_button->setText(QStringLiteral("send_test"));
	m_button->setBkgColor(QColor(255, 0, 0, 255), QColor(0, 255, 0, 255), QColor(0, 0, 255, 255), QColor(255, 0, 0, 255));
	QObject::connect(m_button, &COriginalButton::clicked, this, &Quant::onButtonClicked);

	m_calc->setText(QStringLiteral("calc"));
	m_calc->setBkgColor(QColor(255, 0, 0, 255), QColor(0, 255, 0, 255), QColor(0, 0, 255, 255), QColor(255, 0, 0, 255));
	QObject::connect(m_calc, &COriginalButton::clicked, this, &Quant::onCalcClicked);

	m_createFile->setText(QStringLiteral("create_file"));
	m_createFile->setBkgColor(QColor(255, 0, 0, 255), QColor(0, 255, 0, 255), QColor(0, 0, 255, 255), QColor(255, 0, 0, 255));
	QObject::connect(m_createFile, &COriginalButton::clicked, this, &Quant::onCreateFileClicked);

	m_saveFile->setText(QStringLiteral("save_file"));
	m_saveFile->setBkgColor(QColor(255, 0, 0, 255), QColor(0, 255, 0, 255), QColor(0, 0, 255, 255), QColor(255, 0, 0, 255));
	QObject::connect(m_saveFile, &COriginalButton::clicked, this, &Quant::onSaveFileClicked);

	m_initRedis->setText(QStringLiteral("init_redis"));
	m_initRedis->setBkgColor(QColor(255, 0, 0, 255), QColor(0, 255, 0, 255), QColor(0, 0, 255, 255), QColor(255, 0, 0, 255));
	QObject::connect(m_initRedis, &COriginalButton::clicked, this, &Quant::onInitRedisClicked);

	m_profit->setText(QStringLiteral("profit"));
	m_profit->setBkgColor(QColor(255, 0, 0, 255), QColor(0, 255, 0, 255), QColor(0, 0, 255, 255), QColor(255, 0, 0, 255));
	QObject::connect(m_profit, &COriginalButton::clicked, this, &Quant::onProfitClicked);
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
					ObserveTime observeTime;
					if (key == "time0930")
					{
						observeTime = ObserveTime::TIME0930;
					}
					else if (key == "time1040")
					{
						observeTime = ObserveTime::TIME1040;
					}
					else if (key == "time1050")
					{
						observeTime = ObserveTime::TIME1050;
					}
					else if (key == "time1100")
					{
						observeTime = ObserveTime::TIME1100;
					}
					else if (key == "time1110")
					{
						observeTime = ObserveTime::TIME1110;
					}
					else if (key == "time1340")
					{
						observeTime = ObserveTime::TIME1340;
					}
					else if (key == "time1350")
					{
						observeTime = ObserveTime::TIME1350;
					}
					else if (key == "time1400")
					{
						observeTime = ObserveTime::TIME1400;
					}
					else if (key == "time1410")
					{
						observeTime = ObserveTime::TIME1410;
					}
					else
					{
						continue;
					}
					std::vector<std::string> vecPrice = CStringManager::split(ini.readIni(key, section), ",");
					if (vecPrice.empty())
					{
						continue;
					}
					dateInfo[(uint32_t)Overall::COUNT + (uint32_t)observeTime] = (int32_t)(atof(vecPrice[0].c_str()) * 100);
					for (uint32_t priceIndex = 1; priceIndex < vecPrice.size(); ++priceIndex)
					{
						dateInfo[(uint32_t)Overall::COUNT + (uint32_t)ObserveTime::COUNT +
							(uint32_t)observeTime * (uint32_t)RangeTime::COUNT * (uint32_t)TransType::COUNT +
							priceIndex - 1] = (int32_t)(atof(vecPrice[priceIndex].c_str()) * 100);
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
			redis.deleteOrderGroupElementsByScore(dbName + ":" + fieldName, m_allBeginTime, m_allEndTime);
		}
		for (uint32_t observeIndex = 0; observeIndex < (uint32_t)ObserveTime::COUNT; ++observeIndex)
		{
			std::string fieldName = "observe_" + std::to_string(observeIndex);
			field += "observe_" + std::to_string(observeIndex) + ",";
			redis.deleteOrderGroupElementsByScore(dbName + ":" + fieldName, m_allBeginTime, m_allEndTime);
		}
		for (uint32_t observeIndex = 0; observeIndex < (uint32_t)ObserveTime::COUNT; ++observeIndex)
		{
			for (uint32_t rangeIndex = 0; rangeIndex < (uint32_t)RangeTime::COUNT; ++rangeIndex)
			{
				for (uint32_t transIndex = 0; transIndex < (uint32_t)TransType::COUNT; ++transIndex)
				{
					std::string fieldName = CStringManager::Format("price_%d_%d_%d", observeIndex, rangeIndex, transIndex);
					field += fieldName + ",";
					redis.deleteOrderGroupElementsByScore(dbName + ":" + fieldName, m_allBeginTime, m_allEndTime);
				}
			}
		}
		field.pop_back();
		std::string sqlString = SqlString::selectString(dbName, field, "overall_0>" + std::to_string(m_allBeginTime));
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
}

void Quant::onProfitClicked()
{
	std::string stock = "600975";
	std::vector<std::vector<int32_t>> vecData = Util::getAllStockData(stock, m_allBeginTime, m_allEndTime);
	
	int x = 3;
}