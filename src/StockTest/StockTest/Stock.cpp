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

bool Stock::insertDatabase(MysqlCpp& mysql)
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
            return false;
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
	return true;
}

std::string getPrice(const std::vector<std::vector<std::string>>& result, int32_t fundNum, bool& isMinus)
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
	isMinus = (strpresent[0] == '-');

    return buyStr + "," + sellStr + "," + sub + "," + strpresent + "%";
}

std::vector<std::string> Stock::priceMap(const std::vector<std::vector<std::string>>& result, int32_t& useCount, BigNumber& reserveValue)
{
	useCount = 0;
	BigNumber persent_10000_50000 = 0;
	BigNumber persent_60000_100000 = 0;
    std::vector<std::string> pricemap;
	int32_t num = 0;
	while (num != 110000)
	{
		bool isMinus = false;
		std::string price = getPrice(result, num, isMinus).c_str();
		pricemap.push_back(CStringManager::Format("[%d] = %s", num, price.c_str()));
		num += 10000;
		useCount += (int32_t)(!isMinus);
		if (num == 10000)
		{
			continue;
		}
		std::string persent = CStringManager::split(price, ",").back();
		persent.pop_back();
		BigNumber* persentPtr = (num <= 60000) ? &persent_10000_50000 : &persent_60000_100000;
		*persentPtr = *persentPtr + persent.c_str();
	}
	pricemap.push_back(CStringManager::Format("[10000-50000] = %s", persent_10000_50000.toString().c_str()));
	pricemap.push_back(CStringManager::Format("[60000-100000] = %s", persent_60000_100000.toString().c_str()));
	pricemap.push_back(CStringManager::Format("[reserveValue] = %s", (persent_10000_50000 - persent_60000_100000).toString().c_str()));
	reserveValue = persent_10000_50000 - persent_60000_100000;
	return pricemap;
}

std::vector<std::string> Stock::getPriceMap(MysqlCpp& mysql, int32_t& useCount, BigNumber& reserveValue)
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
	
	return priceMap(getResultVecFromMysql(mysql), useCount, reserveValue);
}

std::vector<std::string> Stock::getPriceMapFromLocal(int32_t& useCount, BigNumber& reserveValue)
{
	return priceMap(getResultVec(), useCount, reserveValue);
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
    CMouse::MoveAbsolute(xyls::Point(500, 500), 0);
    CMouse::LeftClick(0);
    CKeyboard::InputString(stock + "\n", 50);
    CMouse::MoveAbsolute(xyls::Point(500, 500), 100);
    CMouse::RightClick(400);
    CMouse::MoveOpposite(xyls::Point(100, 142), 100);
    CMouse::MoveOpposite(xyls::Point(200, 0), 100);
    CMouse::LeftClick(0);
    Sleep(200);
    CKeyboard::InputString("\n", 0);
    Sleep(400);
    CKeyboard::InputString("\n", 0);
    Sleep(1000);
    CKeyboard::InputString("\n", 0);
}

std::vector<std::vector<std::string>> Stock::getSelfStock(MysqlCpp& mysql)
{
	auto state = mysql.PreparedStatementCreator(SqlString::selectString("selfstock", "daima"));
	return mysql.execute(state)->toVector();
}

std::vector<std::vector<std::string>> Stock::getDefineStock(const std::string& define)
{
	std::vector<std::vector<std::string>> result;
	auto vecDefine = CStringManager::split(define, ",");
	int32_t index = -1;
	while (index++ != vecDefine.size() - 1)
	{
		std::vector<std::string> stock;
		stock.push_back(vecDefine[index]);
		result.push_back(stock);
	}
	return result;
}

std::vector<std::vector<std::string>> Stock::getResultVecFromMysql(MysqlCpp& mysql)
{
	auto state = mysql.PreparedStatementCreator(SqlString::selectString("stock", "shijian,chengjiao,xianshou,bishu,maimai"));
	return mysql.execute(state)->toVector(); //¡ý¡ü--
}

std::vector<std::vector<std::string>> Stock::getResultVec()
{
	std::vector<std::vector<std::string>> result;

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
			return result;
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

		std::vector<std::string> line;
		line.push_back(txt.m_vectxt[index][0]);
		line.push_back(txt.m_vectxt[index][1]);
		line.push_back(xianshouStr);
		line.push_back(txt.m_vectxt[index][3]);
		line.push_back(stateStr);
		result.push_back(line);
	}
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

void Stock::printReserveMap(const std::map<BigNumber, std::vector<std::string>>& reserveMap)
{
	Ctxt txt("D:\\stock.txt");//stockReserveMap.txt
	//txt.ClearFile();
	for (auto itData = reserveMap.begin(); itData != reserveMap.end(); ++itData)
	{
		auto& dataVec = itData->second;
		std::string strDataVec;
		for (auto itStock = dataVec.begin(); itStock != dataVec.end(); ++itStock)
		{
			strDataVec += *itStock + ",";
		}
		strDataVec.pop_back();
		txt.AddLine("[%s] = %s", itData->first.toString().c_str(), strDataVec.c_str());
	}
}
