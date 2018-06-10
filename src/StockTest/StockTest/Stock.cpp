#include "stock.h"
#include <stdint.h>
#include <vector>
#include "Ctxt/CtxtAPI.h"
#include "CStringManager/CStringManagerAPI.h"
#include "BigNumber/BigNumberAPI.h"
#include "CSystem/CSystemAPI.h"
#include "CMouse/CMouseAPI.h"
#include "CKeyboard/CKeyboardAPI.h"
#include "D:\\SendToMessageTest.h"

void Stock::insertDatabase(MysqlCpp& mysql)
{
	mysql.execute(mysql.PreparedStatementCreator(SqlString::destroyTableString("stock")));
	std::vector<std::string> vec;
	vec.push_back("shijian varchar(20)");
	vec.push_back("chengjiao varchar(20)");
	vec.push_back("xianshou varchar(20)");
	vec.push_back("bishu varchar(20)");
	vec.push_back("maimai varchar(20)");
	auto state = mysql.PreparedStatementCreator(SqlString::createTableString("stock", vec));
	mysql.execute(state); //¡ý¡ü--

	Ctxt txt("D:\\Table.txt");
	txt.LoadTxt(2, "\t");

    int index = 0;
    while (index++ != txt.m_vectxt.size() - 2)
    {
        auto& lineVec = txt.m_vectxt[index];
        for (auto itData = lineVec.begin(); itData != lineVec.end();)
        {
            if (*itData == "" || *itData == "\r")
            {
                itData = lineVec.erase(itData);
                continue;
            }
            ++itData;
        }
    }

    index = 0;
    while (index++ != txt.m_vectxt.size() - 2)
    {
        if (txt.m_vectxt[index].size() != 4)
        {
            abort();
        }
    }

	index = 0;
	while (index++ != txt.m_vectxt.size() - 2)
	{
		int32_t bsState = 0;
		std::string xianshouStr = CStringManager::Format("%d", atoi(txt.m_vectxt[index][2].c_str()));
		if (txt.m_vectxt[index][2].back() == -3)
		{
			bsState = -1;
		}
		else if (txt.m_vectxt[index][2].back() == -4)
		{
			bsState = 1;
		}
		std::string stateStr = CStringManager::Format("%d", bsState);

		auto state = mysql.PreparedStatementCreator(SqlString::insertString("stock", "shijian,chengjiao,xianshou,bishu,maimai"));
		state->setString(1, txt.m_vectxt[index][0]);
		state->setString(2, txt.m_vectxt[index][1]);
		state->setString(3, xianshouStr);
		state->setString(4, txt.m_vectxt[index][3]);
		state->setString(5, stateStr);
		mysql.execute(state);
	}
}

std::string getPrice(const std::vector<std::vector<std::string>>& result, int32_t fundNum)
{
	BigNumber buyNum = 0;
	BigNumber sellNum = 0;
	int32_t rowIndex = -1;
	BigNumber buyxianshou = 0;
	BigNumber sellxianshou = 0;
	while (rowIndex++ != result.size() - 1)
	{
        BigNumber bishu = result[rowIndex][3].c_str();
        if (bishu == 0)
		{
			continue;
		}
        BigNumber maimai = result[rowIndex][4].c_str();
        BigNumber* calcNum = (maimai == 1) ? &buyNum : (maimai == -1) ? &sellNum : nullptr;
        BigNumber* calcXianshou = (maimai == 1) ? &buyxianshou : (maimai == -1) ? &sellxianshou : nullptr;
        if (calcNum == nullptr || calcXianshou == nullptr)
        {
            continue;
        }

        BigNumber jiage = result[rowIndex][1].c_str();
        BigNumber xianshou = result[rowIndex][2].c_str();
        BigNumber zijin = xianshou * jiage;
        if (zijin / bishu > fundNum)
        {
            *calcNum = *calcNum + zijin;
            *calcXianshou = *calcXianshou + xianshou;
        }
	}

    int32_t prec = 4;

    BigNumber buyclass = (buyxianshou == 0) ? BigNumber(0) : buyNum / buyxianshou;
    BigNumber sellclass = (sellxianshou == 0) ? BigNumber(0) : sellNum / sellxianshou;

    std::string buyStr = buyclass.toPrec(prec).toString();
    std::string sellStr = sellclass.toPrec(prec).toString();
    std::string sub = (buyclass == 0 || sellclass == 0) ? "0" : (sellclass - buyclass).toPrec(prec).toString();
    std::string strpresent = (buyclass == 0 || sellclass == 0) ? "0" : ((sellclass - buyclass) / buyclass * 100).toPrec(prec - 2).toString();

    return buyStr + "," + sellStr + "," + sub + "," + strpresent + "%";
}

std::vector<std::string> priceMap(const std::vector<std::vector<std::string>>& result)
{
    std::vector<std::string> pricemap;
	int32_t num = 0;
	while (num != 110000)
	{
        pricemap.push_back(CStringManager::Format("[%d] = %s", num, getPrice(result, num).c_str()));
		num += 10000;
	}
	return pricemap;
}

void Stock::getPriceMap(MysqlCpp& mysql)
{
    //std::map<int32_t, std::string> pricemap;
    //int num = 0;
    //while (num != 110000)
    //{
    //    auto autoBuyState = mysql.PreparedStatementCreator(CStringManager::Format("SELECT SUM(chengjiao*xianshou)/SUM(xianshou) FROM stock WHERE chengjiao*xianshou/bishu > %d AND maimai='1'", num));
    //    auto autoBuyVec = mysql.execute(autoBuyState)->toVector();
    //    std::string buyPrice = autoBuyVec[0][0];
    //
    //    auto autoSellState = mysql.PreparedStatementCreator(CStringManager::Format("SELECT SUM(chengjiao*xianshou)/SUM(xianshou) FROM stock WHERE chengjiao*xianshou/bishu > %d AND maimai='-1'", num));
    //    auto autoSellVec = mysql.execute(autoSellState)->toVector();
    //    std::string sellPrice = autoSellVec[0][0];
    //
    //    pricemap[num] = buyPrice + "," + sellPrice;
    //    num += 10000;
    //}
    //return pricemap;

    auto state = mysql.PreparedStatementCreator(SqlString::selectString("stock", "shijian,chengjiao,xianshou,bishu,maimai"));
    std::vector<std::vector<std::string>> result = mysql.execute(state)->toVector(); //¡ý¡ü--
    int begin = ::GetTickCount();
    auto map = priceMap(result);
    int end = ::GetTickCount();
    printf("time = %dms\n", end - begin);
    CSystem::PrintfVector(map);
}

std::string getFund(const std::vector<std::vector<std::string>>& result, int32_t fundNum)
{
	BigNumber buyNum = 0;
	BigNumber sellNum = 0;
	int32_t rowIndex = -1;
	while (rowIndex++ != result.size() - 1)
	{
		if (BigNumber(result[rowIndex][3].c_str()) == 0)
		{
			continue;
		}
		if (BigNumber(result[rowIndex][4].c_str()) == 1)
		{
			BigNumber buyAdd = BigNumber(result[rowIndex][2].c_str()) * BigNumber(result[rowIndex][1].c_str());
			buyAdd.setPrec(2);
			if ((buyAdd / BigNumber(result[rowIndex][3].c_str())) > fundNum)
			{
				buyNum = buyNum + buyAdd;
			}
		}
		else if (BigNumber(result[rowIndex][4].c_str()) == -1)
		{
			BigNumber sellAdd = BigNumber(result[rowIndex][2].c_str()) * BigNumber(result[rowIndex][1].c_str());
			sellAdd.setPrec(2);
			if (sellAdd / BigNumber(result[rowIndex][3].c_str()) > fundNum)
			{
				sellNum = sellNum + sellAdd;
			}
		}
	}
	std::string buyStr = buyNum.toString();
	std::string sellStr = sellNum.toString();
	return buyStr + "," + sellStr + "," + (buyNum - sellNum).toString();
}

std::map<int32_t, std::string> fundMap(const std::vector<std::vector<std::string>>& result)
{
	std::map<int32_t, std::string> fundmap;
	int32_t num = 0;
	while (num != 110000)
	{
		fundmap[num] = getFund(result, num);
		num += 10000;
	}
	return fundmap;
}

std::map<int32_t, std::string> Stock::getFundMap(MysqlCpp& mysql)
{
	auto state = mysql.PreparedStatementCreator(SqlString::selectString("stock", "shijian,chengjiao,xianshou,bishu,maimai"));
	std::vector<std::vector<std::string>> result = mysql.execute(state)->toVector(); //¡ý¡ü--
	return fundMap(result);
}

void Stock::getPriceFromScreen(const std::string& stock)
{
    CMouse::MoveAbsolute(xyls::Point(500, 500));
    CMouse::LeftClick();
    CKeyboard::InputString(stock + "\n");
    CMouse::MoveAbsolute(xyls::Point(500, 500));
    CMouse::RightClick();
    CMouse::MoveOpposite(xyls::Point(100, 142));
    CMouse::MoveOpposite(xyls::Point(200, 0));
    CMouse::LeftClick();
    Sleep(200);
    CKeyboard::InputString("\n");
    Sleep(200);
    CKeyboard::InputString("\n");
    Sleep(1500);
    CKeyboard::InputString("\n");
}

std::vector<std::string> Stock::getStock()
{
    std::vector<std::string> result;
    result.push_back("000001");
    result.push_back("000002");
    result.push_back("000003");
    result.push_back("000004");
    result.push_back("000005");
    result.push_back("000006");
    result.push_back("000007");
    result.push_back("000008");
    result.push_back("000009");
    result.push_back("000010");
    result.push_back("000011");
    result.push_back("000012");
    result.push_back("000013");
    return result;
}

void Stock::toPrec(std::string& result, int32_t prec)
{
    bool isMinus = (result[0] == '-');
    int32_t countValid = result.size() - (isMinus ? 1 : 0);
    int32_t insertCount = prec - countValid + 1;
    if (insertCount > 0)
    {
        result.insert(isMinus ? 1 : 0, insertCount, '0');
    }
    result.insert(result.size() - prec, 1, '.');
}
