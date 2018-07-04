#include "stock.h"
#include <stdint.h>
#include <vector>
#include "Ctxt/CtxtAPI.h"
#include "CStringManager/CStringManagerAPI.h"
#include "BigNumber/BigNumberAPI.h"
#include "CSystem/CSystemAPI.h"
#include "CMouse/CMouseAPI.h"
#include "CKeyboard/CKeyboardAPI.h"
#include "IntDateTime/IntDateTimeAPI.h"
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

std::vector<BigNumber> getPrice(const std::vector<std::vector<std::string>>& result, int32_t fundNum, bool& isMinus)
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

    std::vector<BigNumber> vecResult;
    vecResult.push_back(buyStr.c_str());
    vecResult.push_back(sellStr.c_str());
    vecResult.push_back(sub.c_str());
    vecResult.push_back(BigNumber(strpresent.c_str()).toPrec(4) / 100);
    return vecResult;
}

std::map<std::string, std::vector<BigNumber>> Stock::priceMap(const std::vector<std::vector<std::string>>& result, int32_t& useCount, BigNumber& reserveValue)
{
	useCount = 0;
	BigNumber persent_10000_50000 = 0;
	BigNumber persent_60000_100000 = 0;
    std::map<std::string, std::vector<BigNumber>> pricemap;
	int32_t num = 0;
	while (num != 110000)
	{
		bool isMinus = false;
		std::vector<BigNumber> price = getPrice(result, num, isMinus);
        pricemap[CStringManager::Format("%d", num)] = price;
		num += 10000;
		useCount += (int32_t)(!isMinus);
		if (num == 10000)
		{
			continue;
		}
		BigNumber* persentPtr = (num <= 60000) ? &persent_10000_50000 : &persent_60000_100000;
        *persentPtr = *persentPtr + price.back();
	}
    std::vector<BigNumber> calc;
    calc.push_back(persent_10000_50000);
    pricemap["10000-50000"] = calc;
    calc.clear();
    calc.push_back(persent_60000_100000);
    pricemap["60000-100000"] = calc;
    calc.clear();
    calc.push_back(persent_10000_50000 - persent_60000_100000);
    pricemap["reserveValue"] = calc;
	reserveValue = persent_10000_50000 - persent_60000_100000;
	return pricemap;
}

std::map<std::string, std::vector<BigNumber>> Stock::getPriceMap(MysqlCpp& mysql, int32_t& useCount, BigNumber& reserveValue)
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

std::map<std::string, std::vector<BigNumber>> Stock::getPriceMapFromLocal(int32_t& useCount, BigNumber& reserveValue)
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
    CMouse::MoveAbsolute(xyls::Point(200, 200), 0);
    CMouse::LeftClick(0);
    CKeyboard::InputString(stock + "\n", 50);
    CMouse::MoveAbsolute(xyls::Point(200, 200), 200);
    Sleep(1000);
    CMouse::RightClick(500);
    CMouse::MoveOpposite(xyls::Point(100, 172), 200);
    CMouse::MoveOpposite(xyls::Point(200, 0), 200);
    CMouse::LeftClick(0);
    Sleep(400);
    CKeyboard::InputString("\n", 0);
    Sleep(400);
    CKeyboard::InputString("\n", 0);
    Sleep(1000);
    CKeyboard::InputString("\n", 0);
}

std::vector<std::vector<std::string>> Stock::getSelfStock(MysqlCpp& mysql, int32_t zubie)
{
    auto state = mysql.PreparedStatementCreator(SqlString::selectString("selfstock", "daima", CStringManager::Format("zubie='%d'", zubie).c_str()));
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

void Stock::insertQuoteDataBase(MysqlCpp& mysql)
{
	std::vector<std::string> vec;
	vec.push_back("shijian varchar(20)");
	vec.push_back("daima varchar(20)");
	vec.push_back("zuigaozhangfu varchar(20)");
	vec.push_back("zuizhongzhangfu varchar(20)");
	vec.push_back("kaipan varchar(20)");
	vec.push_back("zuigao varchar(20)");
	vec.push_back("zuidi varchar(20)");
	vec.push_back("xianjia varchar(20)");
	vec.push_back("zuoshou varchar(20)");
	vec.push_back("huanshou varchar(20)");
	vec.push_back("sanhushuliang varchar(20)");
	vec.push_back("shijinglv varchar(20)");
	auto state = mysql.PreparedStatementCreator(SqlString::createTableString("stockquote", vec));
	mysql.execute(state); //¡ý¡ü--

	Ctxt quotetxt("D:\\Table.txt");
	quotetxt.LoadTxt(2, "\t");

	int index = 0;
	while (index++ != quotetxt.m_vectxt.size() - 2)
	{
		auto& lineVec = quotetxt.m_vectxt[index];
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
	while (index++ != quotetxt.m_vectxt.size() - 2)
	{
		if (quotetxt.m_vectxt[index].size() != 11)
		{
			return;
		}
	}

	std::string time = IntDateTime().dateToString();

	mysql.execute(mysql.PreparedStatementCreator(SqlString::deleteString("stockquote", "shijian='" + time + "'")));

	index = 0;
	while (index++ != quotetxt.m_vectxt.size() - 2)
	{
		std::string& kaipan = quotetxt.m_vectxt[index][3] == "--" ? quotetxt.m_vectxt[index][7] : quotetxt.m_vectxt[index][3];
		std::string& zuigao = quotetxt.m_vectxt[index][4] == "--" ? quotetxt.m_vectxt[index][7] : quotetxt.m_vectxt[index][4];
		std::string& zuidi = quotetxt.m_vectxt[index][5] == "--" ? quotetxt.m_vectxt[index][7] : quotetxt.m_vectxt[index][5];
		std::string& xianjia = quotetxt.m_vectxt[index][6] == "--" ? quotetxt.m_vectxt[index][7] : quotetxt.m_vectxt[index][6];
		std::string& zuoshou = quotetxt.m_vectxt[index][7];
		auto state = mysql.PreparedStatementCreator(SqlString::insertString("stockquote", "shijian,daima,zuigaozhangfu,zuizhongzhangfu,kaipan,zuigao,zuidi,xianjia,zuoshou,huanshou,sanhushuliang,shijinglv"));
		state->setString(1, time.c_str());
		state->setString(2, CStringManager::Mid(quotetxt.m_vectxt[index][0], 2, 6));
		state->setString(3, ((BigNumber(zuigao.c_str()) / BigNumber(zuoshou.c_str()) - 1) * 100).toPrec(4).toString());
		state->setString(4, ((BigNumber(xianjia.c_str()) / BigNumber(zuoshou.c_str()) - 1) * 100).toPrec(4).toString());
		state->setString(5, kaipan);
		state->setString(6, zuigao);
		state->setString(7, zuidi);
		state->setString(8, xianjia);
		state->setString(9, zuoshou);
		state->setString(10, quotetxt.m_vectxt[index][8] == "--" ? "0" : quotetxt.m_vectxt[index][8]);
		state->setString(11, quotetxt.m_vectxt[index][9] == "--" ? "0" : quotetxt.m_vectxt[index][9]);
		state->setString(12, quotetxt.m_vectxt[index][10] == "--" ? "0" : quotetxt.m_vectxt[index][10]);
		mysql.execute(state);
	}
}

void Stock::chooseTest(MysqlCpp& mysql, const std::string& preDate)
{
    std::string todayDate = IntDateTime().dateToString();
    std::string whereString = CStringManager::Format("shijian='%s'", preDate.c_str());
    auto preChooseVec = mysql.execute(mysql.PreparedStatementCreator(SqlString::selectString("choose", "daima", whereString)))->toVector();

    int32_t stockCount = atoi(mysql.execute(mysql.PreparedStatementCreator(CStringManager::Format("select count(*) from stockquote where shijian='%s'", preDate.c_str())))->toVector()[0][0].c_str());

    auto preQuoteVec = mysql.execute(mysql.PreparedStatementCreator(SqlString::selectString("stockquote", "daima", whereString)))->toVector();

    whereString = "shijian='" + todayDate + "' and (";
    int32_t index = -1;
    while (index++ != preQuoteVec.size() - 1)
    {
        whereString += "daima='" + preQuoteVec[index][0] + "' or ";
    }
    whereString.pop_back();
    whereString.pop_back();
    whereString.pop_back();
    whereString.pop_back();
    whereString.push_back(')');

    auto todayQuoteVec = mysql.execute(mysql.PreparedStatementCreator(SqlString::selectString("stockquote", "zuigaozhangfu,zuizhongzhangfu", whereString)))->toVector();
    
    BigNumber zuigaozhangfuAll = 0;
    BigNumber zuizhongzhangfuAll = 0;
    BigNumber zuigaozhangfumingciAll = 0;
    BigNumber zuizhongzhangfumingciAll = 0;
    BigNumber zuigaozhangfubaifenbiAll = 0;
    BigNumber zuizhongzhangfubaifenbiAll = 0;
    index = -1;
    while (index++ != preChooseVec.size() - 1)
    {
        std::string& chooseDaima = preChooseVec[index][0];
        whereString = "shijian = '" + todayDate + "' and daima='" + chooseDaima + "'";
        auto veczhangfu = mysql.execute(mysql.PreparedStatementCreator(SqlString::selectString("stockquote", "zuigaozhangfu,zuizhongzhangfu", whereString)))->toVector();
        std::string zuigaozhangfu = veczhangfu[0][0];
        zuigaozhangfuAll = zuigaozhangfuAll + zuigaozhangfu.c_str();
        std::string zuizhongzhangfu = veczhangfu[0][1];
        zuizhongzhangfuAll = zuizhongzhangfuAll + zuizhongzhangfu.c_str();
        int32_t zuigaozhangfumingci = 1;
        int32_t zuizhongzhangfumingci = 1;
        int32_t quoteIndex = -1;
        while (quoteIndex++ != todayQuoteVec.size() - 1)
        {
            if (BigNumber(todayQuoteVec[quoteIndex][0].c_str()) > BigNumber(zuigaozhangfu.c_str()))
            {
                ++zuigaozhangfumingci;
            }
            if (BigNumber(todayQuoteVec[quoteIndex][1].c_str()) > BigNumber(zuizhongzhangfu.c_str()))
            {
                ++zuizhongzhangfumingci;
            }
        }
        zuigaozhangfumingciAll = zuigaozhangfumingciAll + zuigaozhangfumingci;
        zuizhongzhangfumingciAll = zuizhongzhangfumingciAll + zuizhongzhangfumingci;
        BigNumber zuigaozhangfubaifenbi = BigNumber(zuigaozhangfumingci).toPrec(6) / stockCount;
        BigNumber zuizhongzhangfubaifenbi = BigNumber(zuizhongzhangfumingci).toPrec(6) / stockCount;
        zuigaozhangfubaifenbiAll = zuigaozhangfubaifenbiAll + zuigaozhangfubaifenbi;
        zuizhongzhangfubaifenbiAll = zuizhongzhangfubaifenbiAll + zuizhongzhangfubaifenbi;
        std::string setString = CStringManager::Format("zuigaozhangfu='%s',zuizhongzhangfu='%s',zuigaozhangfumingci='%d',zuizhongzhangfumingci='%d',zuigaozhangfubaifenbi='%s',zuizhongzhangfubaifenbi='%s'",
            zuigaozhangfu.c_str(), zuizhongzhangfu.c_str(), zuigaozhangfumingci, zuizhongzhangfumingci, (zuigaozhangfubaifenbi * 100).toPrec(2).toString().c_str(), (zuizhongzhangfubaifenbi * 100).toPrec(2).toString().c_str());
        whereString = CStringManager::Format("shijian='%s' and daima='%s'", preDate.c_str(), chooseDaima.c_str());
        mysql.execute(mysql.PreparedStatementCreator(SqlString::updateString("choose", setString, whereString)))->toVector();
    }
    int32_t chooseCount = preChooseVec.size();
    auto state = mysql.PreparedStatementCreator(SqlString::insertString("chooseavg", "shijian,zuigaozhangfu,zuizhongzhangfu,zuigaozhangfumingci,zuizhongzhangfumingci,zuigaozhangfubaifenbi,zuizhongzhangfubaifenbi"));
    state->setString(1, preDate.c_str());
    state->setString(2, (zuigaozhangfuAll.toPrec(4) / chooseCount).toPrec(2).toString().c_str());
    state->setString(3, (zuizhongzhangfuAll.toPrec(4) / chooseCount).toPrec(2).toString().c_str());
    state->setString(4, (zuigaozhangfumingciAll.toPrec(4) / chooseCount).toPrec(2).toString().c_str());
    state->setString(5, (zuizhongzhangfumingciAll.toPrec(4) / chooseCount).toPrec(2).toString().c_str());
    state->setString(6, ((zuigaozhangfubaifenbiAll * 100).toPrec(4) / chooseCount).toPrec(2).toString().c_str());
    state->setString(7, ((zuizhongzhangfubaifenbiAll * 100).toPrec(4) / chooseCount).toPrec(2).toString().c_str());
    mysql.execute(state);
}

std::string Stock::getPreDate(MysqlCpp& mysql)
{
    IntDateTime nowTime;
    std::string todayDate = nowTime.dateToString();
    int32_t count = 15;
    while (count-- != 0)
    {
        nowTime = nowTime - 86400;
        std::string preDate = nowTime.dateToString();
        std::string whereString = CStringManager::Format("shijian='%s'", preDate.c_str());
        if (atoi(mysql.execute(mysql.PreparedStatementCreator(SqlString::selectString("stockquote", "count(*)", whereString)))->toVector()[0][0].c_str()) != 0)
        {
            return preDate;
        }
    }
    return todayDate;
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
    Ctxt txt("D:\\stock" + IntDateTime().dateToString() + ".txt");//stockReserveMap.txt
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
		txt.AddLine("[%s] = %s", (itData->first * 100).toPrec(2).toString().c_str(), strDataVec.c_str());
	}
    txt.AddLine("");
}

void Stock::printChooseMap(const std::map<BigNumber, std::vector<BigNumber>>& chooseMap)
{
    Ctxt txt("D:\\stock" + IntDateTime().dateToString() + ".txt");
    txt.AddLine("choose");
    for (auto itData = chooseMap.begin(); itData != chooseMap.end(); ++itData)
    {
        auto& dataVec = itData->second;
        std::string strDataVec;
		std::string num = dataVec[0].toString();
		while (num.size() < 6)
		{
			num = "0" + num;
		}
		strDataVec += num + ",";
        int32_t index = 0;
        while (index++ != dataVec.size() - 1)
        {
            strDataVec += (dataVec[index] * 100).toPrec(2).toString() + ",";
        }
        strDataVec.pop_back();
        txt.AddLine("[%s] = %s", (itData->first * 100).toPrec(2).toString().c_str(), strDataVec.c_str());
    }
}

void Stock::saveChooseToDataBase(MysqlCpp& mysql, std::map<BigNumber, std::vector<BigNumber>>& chooseMap, int32_t zubie)
{
    mysql.execute(mysql.PreparedStatementCreator(SqlString::deleteString("choose", CStringManager::Format("shijian='%s' and zubie='%d'", IntDateTime().dateToString().c_str(), zubie))));
    for (auto itChoose = chooseMap.begin(); itChoose != chooseMap.end(); ++itChoose)
    {
        auto state = mysql.PreparedStatementCreator(SqlString::insertString("choose", "shijian,daima,zubie,xiangcha,xiaodan,dadan"));
        state->setString(1, IntDateTime().dateToString());
		std::string num = itChoose->second[0].toString();
		while (num.size() < 6)
		{
			num = "0" + num;
		}
		state->setString(2, num);
        state->setString(3, CStringManager::Format("%d", zubie));
        state->setString(4, (itChoose->first * 100).toPrec(2).toString());
        state->setString(5, (itChoose->second[1] * 100).toPrec(2).toString());
        state->setString(6, (itChoose->second[2] * 100).toPrec(2).toString());
        mysql.execute(state);
    }
}

void Stock::printPriceMap(const std::map<std::string, std::vector<BigNumber>>& priceMap)
{
    Ctxt txt("D:\\stock" + IntDateTime().dateToString() + ".txt");
    std::vector<std::string> vecPrint;
    vecPrint.push_back("0");
    vecPrint.push_back("10000");
    vecPrint.push_back("20000");
    vecPrint.push_back("30000");
    vecPrint.push_back("40000");
    vecPrint.push_back("50000");
    vecPrint.push_back("60000");
    vecPrint.push_back("70000");
    vecPrint.push_back("80000");
    vecPrint.push_back("90000");
    vecPrint.push_back("100000");
    int32_t index = -1;
    while (index++ != vecPrint.size() - 1)
    {
        auto& vecBigNumber = priceMap.find(vecPrint[index])->second;
        std::string strBigNumber = vecBigNumber[0].toString() + "," +
            vecBigNumber[1].toString() + "," +
            vecBigNumber[2].toString() + "," +
            (vecBigNumber[3] * 100).toPrec(2).toString() + "%";
        txt.AddLine("[%s] = %s", vecPrint[index].c_str(), strBigNumber.c_str());
    }
    vecPrint.clear();
    vecPrint.push_back("10000-50000");
    vecPrint.push_back("60000-100000");
    vecPrint.push_back("reserveValue");
    index = -1;
    while (index++ != vecPrint.size() - 1)
    {
        auto& vecBigNumber = priceMap.find(vecPrint[index])->second;
        txt.AddLine("[%s] = %s", vecPrint[index].c_str(), (vecBigNumber[0] * 100).toPrec(2).toString().c_str());
    }
}

void Stock::addChooseMap(std::map<BigNumber, std::vector<BigNumber>>& chooseMap, const std::map<std::string, std::vector<BigNumber>>& priceMap, const std::string& stockNum)
{
    if ((priceMap.find("reserveValue")->second)[0] * 100 > "3" && (priceMap.find("10000-50000")->second)[0] * 100 > "-2")
    {
        bool isUsed = true;
        std::vector<std::string> calc;
        calc.push_back("0");
        calc.push_back("10000");
        calc.push_back("20000");
        calc.push_back("30000");
        calc.push_back("40000");
        calc.push_back("50000");
        calc.push_back("60000");
        calc.push_back("70000");
        calc.push_back("80000");
        calc.push_back("90000");
        calc.push_back("100000");
        int32_t index = -1;
        while (index++ != calc.size() - 1)
        {
            if (((priceMap.find(calc[index])->second)[2] == 0) && ((priceMap.find(calc[index])->second)[3] == 0))
            {
                isUsed = false;
                break;
            }
        }
        if (isUsed)
        {
            std::vector<BigNumber> vecBigNumber;
            vecBigNumber.push_back(stockNum.c_str());
            vecBigNumber.push_back((priceMap.find("10000-50000")->second)[0]);
            vecBigNumber.push_back((priceMap.find("60000-100000")->second)[0]);
            chooseMap[(priceMap.find("reserveValue")->second)[0]] = vecBigNumber;
        }
    }
}
