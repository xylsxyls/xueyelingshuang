#include "StockSave.h"
#include <stdint.h>
#include "StockMarket/StockMarketAPI.h"
#include "StockIndex/StockIndexAPI.h"
#include "CStopWatch/CStopWatchAPI.h"
#include "Ctxt/CtxtAPI.h"
#include "CStringManager/CStringManagerAPI.h"
#include "CSystem/CSystemAPI.h"
#include "CMouse/CMouseAPI.h"
#include "CKeyboard/CKeyboardAPI.h"
#include <iostream>

BOOL CALLBACK ConsoleHandler(DWORD eve)
{
	if (eve == CTRL_CLOSE_EVENT)
	{
		//关闭退出事件
		//RCSend("close StockSave");
	}
	return FALSE;
}

int32_t consoleCloseResult = ::SetConsoleCtrlHandler(ConsoleHandler, TRUE);

//在D盘下需要有所有股票的文件，第一列股票代码，第二列名字，第三列现价，需要先群体换\t\t为\t
void saveStock()
{
	MysqlCpp mysql;
	mysql.connect("127.0.0.1", 3306, "root", "");
	mysql.selectDb("stockmarket");

	std::vector<std::string> vecFields;
	vecFields.push_back("stock varchar(6) primary key");
	vecFields.push_back("name varchar(63)");
	mysql.execute(mysql.PreparedStatementCreator(SqlString::createTableString("stock", vecFields)), false);

	Ctxt txt("D:\\stockall.txt");
	txt.LoadTxt(Ctxt::SPLIT, "\t");
	int32_t index = 0;
	while (index++ != txt.m_vectxt.size() - 2)
	{
		std::string stock = txt.m_vectxt[index][0];
		stock = CStringManager::Mid(stock, 2, 6);
		std::string& name = txt.m_vectxt[index][1];
		if (CStringManager::Replace(name, "ST", "ST") != 0)
		{
			continue;
		}
		auto prepare = mysql.PreparedStatementCreator(SqlString::insertString("stock", "stock,name"));
		prepare->setString(0, stock);
		prepare->setString(1, name);
		mysql.execute(prepare, false);
	}
	mysql.commit();
}

//win7 非登录客户端运行
void saveMarketFile()
{
	printf("请输入主机ip地址\n");
	std::string ip;
	std::cin >> ip;
	MysqlCpp mysql;
	mysql.connect(ip, 3306, "root", "");
	mysql.selectDb("stockmarket");
	HWND hWnd = FindWindow("ConsoleWindowClass", NULL);
	auto vecStock = mysql.execute(mysql.PreparedStatementCreator(SqlString::selectString("stock", "stock")))->toVector();
	int32_t index = -1;
	printf("请输入从第几个开始下载，输入1表示从头开始下载，一共%d个\n", vecStock.size());
	scanf("%d", &index);
	CSystem::ClearScanf();
	index = index - 2;
	Sleep(5000);
	while (index++ != vecStock.size() - 1)
	{
		LOG_SEND("hasSave = %d", index + 1);
		const std::string& stock = vecStock[index][0];
		CKeyboard::InputString(stock);
		CKeyboard::InputString("\n", 50);
		CMouse::MoveAbsolute(xyls::Point(200, 200), 50);
		CMouse::RightClick(50);
		CMouse::MoveOpposite(xyls::Point(93, 133), 50);
		CMouse::LeftClick(50);
		CMouse::MoveOpposite(xyls::Point(153, 0), 50);
		CMouse::LeftClick(50);
		
		Sleep(500);
		CSystem::setClipboardData(hWnd, "D:\\stock\\" + stock + ".txt");
		CKeyboard::KeyDown(CKeyboard::Ctrl);
		CKeyboard::KeyDown('V');
		CKeyboard::KeyUp(CKeyboard::Ctrl);
		CKeyboard::KeyUp('V');
		Sleep(500);

		CKeyboard::InputString("\n", 50);
		Sleep(500);
		CKeyboard::InputString("\n", 50);
		Sleep(3000);
		CKeyboard::InputString("\n", 50);
		Sleep(200);
	}
}

void saveMarketIndex()
{
	StockAllMarket::instance().save("D:\\stock");
	StockAllIndex::instance().save();
}

int32_t main()
{
	printf("请输入要执行的程序\n");
	printf("1.存储所有股票代码 2.存储开高低收文件，5秒后开始 3.存储开高低收和指标\n");
	CStopWatch watch;
	int32_t id = 0;
	scanf("%d", &id);
	CSystem::ClearScanf();
	switch (id)
	{
	case 1:
	{
		saveStock();
	}
	break;
	case 2:
	{
		saveMarketFile();
	}
	break;
	case 3:
	{
		saveMarketIndex();
	}
	break;
	default:
		break;
	}
	printf("time = %d\n", watch.GetWatchTime());
	printf("end\n");
	getchar();
	return 0;
}