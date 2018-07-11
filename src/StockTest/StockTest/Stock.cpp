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
	mysql.execute(state); //↓↑--

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

std::map<std::string, std::vector<BigNumber>> Stock::getCapitalMapFromDataBase(MysqlCpp& mysql, const std::string& stockNum, const std::string& date)
{
	//pingjunjine，vec, bishu，xianshou，maimai1-1
	std::map<BigNumber, std::vector<std::vector<BigNumber>>> capitalMap;
	auto fenbiVec = getResultVecFromMysql(mysql, stockNum, date);
	BigNumber allzijin = 0;
	int32_t index = -1;
	while (index++ != fenbiVec.size() - 1)
	{
		BigNumber chengjiao = fenbiVec[index][1].c_str();
		BigNumber xianshou = fenbiVec[index][2].c_str();
		BigNumber bishu = fenbiVec[index][3].c_str();
		BigNumber maimai = fenbiVec[index][4].c_str();
        if (bishu == 0 || maimai == 0)
        {
            continue;
        }
		BigNumber zijin = chengjiao * xianshou;
		allzijin = allzijin + zijin;
		BigNumber danbijine = (zijin).toPrec(4) / bishu;
		std::vector<BigNumber> danbivec;
		danbivec.push_back(bishu);
		danbivec.push_back(xianshou);
		danbivec.push_back(maimai);
		capitalMap[danbijine].push_back(danbivec);
	}

	//baifenbi, vec, mairujunjia,maichujunjia,mairubaifenbi,maichubaifenbi,zhangdie,zhangfu
	std::map<std::string, std::vector<BigNumber>> baifenbiCapitalMap;
	BigNumber mairucapitalNum = 0;
	BigNumber maichucapitalNum = 0;
	BigNumber mairuxianshouNum = 0;
	BigNumber maichuxianshouNum = 0;
	int32_t baifenbiNum = 10;
	for (auto itCapital = capitalMap.rbegin(); itCapital != capitalMap.rend(); ++itCapital)
	{
		auto& danbiCapiVec = itCapital->second;
		int32_t index = -1;
		while (index++ != danbiCapiVec.size() - 1)
		{
			if (danbiCapiVec[index][2] == 1)
			{
				mairucapitalNum = mairucapitalNum + itCapital->first * danbiCapiVec[index][0];
				mairuxianshouNum = mairuxianshouNum + danbiCapiVec[index][1];
			}
			else if (danbiCapiVec[index][2] == -1)
			{
				maichucapitalNum = maichucapitalNum + itCapital->first * danbiCapiVec[index][0];
				maichuxianshouNum = maichuxianshouNum + danbiCapiVec[index][1];
			}
		}
		if (mairucapitalNum + maichucapitalNum > allzijin * baifenbiNum / 100)
		{
			std::string baifenbi = CStringManager::Format("%d%%", 100 - baifenbiNum);
			BigNumber mairujunjia = mairucapitalNum.toPrec(4) / mairuxianshouNum.zero();
			BigNumber maichujunjia = maichucapitalNum.toPrec(4) / maichuxianshouNum.zero();
			baifenbiCapitalMap[baifenbi].push_back(mairujunjia);
			baifenbiCapitalMap[baifenbi].push_back(maichujunjia);
			baifenbiCapitalMap[baifenbi].push_back((mairucapitalNum * 100).toPrec(4) / allzijin.zero());
			baifenbiCapitalMap[baifenbi].push_back((maichucapitalNum * 100).toPrec(4) / allzijin.zero());
			baifenbiCapitalMap[baifenbi].push_back(mairujunjia - maichujunjia);
			baifenbiCapitalMap[baifenbi].push_back((mairujunjia.toPrec(4) / maichujunjia.zero() - 1) * 100);
			baifenbiNum += 10;
		}
	}
    std::string baifenbi = CStringManager::Format("%d%%", 100 - baifenbiNum);
    BigNumber mairujunjia = mairucapitalNum.toPrec(4) / mairuxianshouNum.zero();
    BigNumber maichujunjia = maichucapitalNum.toPrec(4) / maichuxianshouNum.zero();
    baifenbiCapitalMap[baifenbi].push_back(mairujunjia);
    baifenbiCapitalMap[baifenbi].push_back(maichujunjia);
    baifenbiCapitalMap[baifenbi].push_back((mairucapitalNum * 100).toPrec(4) / allzijin.zero());
    baifenbiCapitalMap[baifenbi].push_back((maichucapitalNum * 100).toPrec(4) / allzijin.zero());
    baifenbiCapitalMap[baifenbi].push_back(mairujunjia - maichujunjia);
    baifenbiCapitalMap[baifenbi].push_back((mairujunjia.toPrec(4) / maichujunjia.zero() - 1) * 100);

	return baifenbiCapitalMap;
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
    BigNumber persent_110000_150000 = 0;
    BigNumber persent_160000_200000 = 0;
    BigNumber persent_210000_250000 = 0;
    BigNumber persent_260000_300000 = 0;
    std::map<std::string, std::vector<BigNumber>> pricemap;
	int32_t num = -10000;
	while (true)
	{
        num += 10000;
		bool isMinus = false;
		std::vector<BigNumber> price = getPrice(result, num, isMinus);
        pricemap[CStringManager::Format("%d", num)] = price;
		useCount += (int32_t)(!isMinus);
        BigNumber* persentPtr = nullptr;
        if (num >= 10000 && num <= 50000)
        {
            persentPtr = &persent_10000_50000;
        }
        else if (num >= 60000 && num <= 100000)
        {
            persentPtr = &persent_60000_100000;
        }
        else if (num >= 110000 && num <= 150000)
        {
            persentPtr = &persent_110000_150000;
        }
        else if (num >= 160000 && num <= 200000)
        {
            persentPtr = &persent_160000_200000;
        }
        else if (num >= 210000 && num <= 250000)
        {
            persentPtr = &persent_210000_250000;
        }
        else if (num >= 260000 && num <= 300000)
        {
            persentPtr = &persent_260000_300000;
        }
        if (persentPtr != nullptr)
        {
            *persentPtr = *persentPtr + price.back();
        }
        if (num == 300000)
        {
            break;
        }
	}
    std::vector<BigNumber> calc;
    calc.push_back(persent_10000_50000);
    pricemap["10000-50000"] = calc;
    calc.clear();
    calc.push_back(persent_60000_100000);
    pricemap["60000-100000"] = calc;
    calc.clear();
    calc.push_back(persent_110000_150000);
    pricemap["110000-150000"] = calc;
    calc.clear();
    calc.push_back(persent_160000_200000);
    pricemap["160000-200000"] = calc;
    calc.clear();
    calc.push_back(persent_210000_250000);
    pricemap["210000-250000"] = calc;
    calc.clear();
    calc.push_back(persent_260000_300000);
    pricemap["260000-300000"] = calc;
    calc.clear();
    calc.push_back(persent_10000_50000 - persent_60000_100000);
    pricemap["reserveValue"] = calc;
	reserveValue = persent_10000_50000 - persent_60000_100000;
	return pricemap;
}

std::map<std::string, std::vector<BigNumber>> Stock::getPriceMapFromDataBase(MysqlCpp& mysql, const std::string& stockNum, int32_t& useCount, BigNumber& reserveValue, const std::string& date)
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
	
    return priceMap(getResultVecFromMysql(mysql, stockNum, date), useCount, reserveValue);
}

std::map<std::string, std::vector<BigNumber>> Stock::getPriceMapFromLocal(int32_t& useCount, BigNumber& reserveValue)
{
	return priceMap(getResultVecFromLocal(), useCount, reserveValue);
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
	std::vector<std::vector<std::string>> result = mysql.execute(state)->toVector(); //↓↑--
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
	auto state = mysql.PreparedStatementCreator(SqlString::selectString("selfstock", "daima", zubie == 0 ? "" : CStringManager::Format("zubie='%d'", zubie).c_str()));
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

std::vector<std::vector<std::string>> Stock::getResultVecFromMysql(MysqlCpp& mysql, const std::string& stockNum, const std::string& date)
{
    std::string todayDate = date;
    if (date == "")
    {
        todayDate = IntDateTime().dateToString();
    }
    auto state = mysql.PreparedStatementCreator(SqlString::selectString(stockNum, "fenbishijian,chengjiao,xianshou,bishu,maimai", "shijian='" + todayDate + "'"));
	return mysql.execute(state)->toVector(); //↓↑--
}

std::vector<std::vector<std::string>> Stock::getResultVecFromLocal()
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
	mysql.execute(state); //↓↑--

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

    mysql.execute(mysql.PreparedStatementCreator(SqlString::deleteString("chooseavg", "shijian='" + preDate + "'")));

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

std::string Stock::getPreDate(MysqlCpp& mysql, std::string* date)
{
    IntDateTime nowTime;
	std::string todayDate;
	if (date == nullptr)
	{
		todayDate = nowTime.dateToString();
	}
	else
	{
		todayDate = *date;
		nowTime.setTime(todayDate + " 00:00:00");
	}
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

void Stock::fenbiInsertDataBase(MysqlCpp& mysql, const std::string& stockNum)
{
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
			return;
		}
	}

	std::string todayDate = IntDateTime().dateToString();
	mysql.execute(mysql.PreparedStatementCreator(SqlString::deleteString(stockNum, "shijian='" + todayDate + "'")));

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

		auto state = mysql.PreparedStatementCreator(SqlString::insertString(stockNum, "shijian,fenbishijian,chengjiao,xianshou,bishu,maimai"));
		state->setString(1, todayDate);
		state->setString(2, txt.m_vectxt[index][0]);
		state->setString(3, txt.m_vectxt[index][1]);
		state->setString(4, xianshouStr);
		state->setString(5, txt.m_vectxt[index][3]);
		state->setString(6, stateStr);
		mysql.execute(state);
	}
	return;
}

std::map<std::string, BigNumber> Stock::getLastPriceFromStockQuote(MysqlCpp& mysql, const std::string& date)
{
	std::map<std::string, BigNumber> result;
	auto vec = mysql.execute(mysql.PreparedStatementCreator(SqlString::selectString("stockquote", "daima,xianjia", "shijian='" + date + "'")))->toVector();
	int32_t index = -1;
	while (index++ != vec.size() - 1)
	{
		result[vec[index][0]] = vec[index][1].c_str();
	}
	return result;
}

std::map<std::string, BigNumber> Stock::getLastPriceFromStockTradeQuote(MysqlCpp& mysql, MysqlCpp& mysqlfenbi, const std::string& date)
{
	std::map<std::string, BigNumber> result;
	auto vecDaima = mysql.execute(mysql.PreparedStatementCreator(SqlString::selectString("stockquote", "daima", "shijian='" + date + "'")))->toVector();
	int32_t index = -1;
	while (index++ != vecDaima.size() - 1)
	{
        auto vec = mysqlfenbi.execute(mysqlfenbi.PreparedStatementCreator(SqlString::selectString(vecDaima[index][0], "chengjiao", "shijian='" + date + "'")))->toVector();
        result[vecDaima[index][0]] = vec.size() == 0 ? "0" : vec.back().back().c_str();
	}
	return result;
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
    int32_t price = 0;
    int32_t count = 31;
    while (count-- != 0)
    {
        vecPrint.push_back(CStringManager::Format("%d", price));
        price += 10000;
    }
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
    vecPrint.push_back("110000-150000");
    vecPrint.push_back("160000-200000");
    vecPrint.push_back("210000-250000");
    vecPrint.push_back("260000-300000");
    vecPrint.push_back("reserveValue");
    index = -1;
    while (index++ != vecPrint.size() - 1)
    {
        auto& vecBigNumber = priceMap.find(vecPrint[index])->second;
        txt.AddLine("[%s] = %s", vecPrint[index].c_str(), (vecBigNumber[0] * 100).toPrec(2).toString().c_str());
    }
}

void Stock::bestAnalyzeDataBase(MysqlCpp& mysql, MysqlCpp& mysqlfenbi)
{
	//取出今天的总数据
	std::string todayDate = IntDateTime().dateToString();
	std::string preDate = Stock::getPreDate(mysql);
	std::string prepreDate = Stock::getPreDate(mysql, &preDate);
	auto vec = mysql.execute(mysql.PreparedStatementCreator(SqlString::selectString("stockquote", "daima,zuigaozhangfu", "shijian='" + todayDate + "'")))->toVector();
	std::map<BigNumber, std::vector<std::string>> bestMap;
	int32_t index = -1;
	while (index++ != vec.size() - 1)
	{
		bestMap[vec[index][1].c_str()].push_back(vec[index][0]);
	}
	//获取最好的前20个，可能超过
	std::vector<std::string> bestStockVec;
	for (auto itStock = bestMap.rbegin(); itStock != bestMap.rend(); ++itStock)
	{
		if (bestStockVec.size() >= 20)
		{
			break;
		}
		auto& vecbestitem = itStock->second;
		int32_t indexItem = -1;
		while (indexItem++ != vecbestitem.size() - 1)
		{
			BigNumber prezuizhongzhangfu = mysql.execute(mysql.PreparedStatementCreator(SqlString::selectString("stockquote", "zuizhongzhangfu", "daima='" + vecbestitem[indexItem] + "' and shijian='" + preDate + "'")))->toVector()[0][0].c_str();
			if (prezuizhongzhangfu < BigNumber("9.95"))
			{
				bestStockVec.push_back(vecbestitem[indexItem]);
			}
		}
	}
	//添加到best
	mysql.execute(mysql.PreparedStatementCreator(SqlString::deleteString("best", "shijian='" + todayDate + "'")));
	index = -1;
	while (index++ != bestStockVec.size() - 1)
	{
		auto state = mysql.PreparedStatementCreator(SqlString::insertString("best", "shijian,daima,zuigaozhangfu,zuizhongzhangfu,huanshoubianhua,sanhushuliang,shijinglvbianhua"));
		state->setString(1, todayDate);
		state->setString(2, bestStockVec[index]);
		auto todayQuoteVec = mysql.execute(mysql.PreparedStatementCreator(SqlString::selectString("stockquote", "zuigaozhangfu,zuizhongzhangfu", "shijian='" + todayDate + "' and daima='" + bestStockVec[index] + "'")))->toVector();
		auto preQuoteVec = mysql.execute(mysql.PreparedStatementCreator(SqlString::selectString("stockquote", "huanshou,sanhushuliang,shijinglv", "shijian='" + preDate + "' and daima='" + bestStockVec[index] + "'")))->toVector();
		auto prepreQuoteVec = mysql.execute(mysql.PreparedStatementCreator(SqlString::selectString("stockquote", "huanshou,shijinglv", "shijian='" + prepreDate + "' and daima='" + bestStockVec[index] + "'")))->toVector();
		state->setString(3, todayQuoteVec[0][0]);
		state->setString(4, todayQuoteVec[0][1]);
		state->setString(5, (BigNumber(preQuoteVec[0][0].c_str()) - BigNumber(prepreQuoteVec[0][0].c_str())).toPrec(2).toString());
		state->setString(6, preQuoteVec[0][1]);
		state->setString(7, (BigNumber(preQuoteVec[0][2].c_str()) - BigNumber(prepreQuoteVec[0][1].c_str())).toPrec(2).toString());
		mysql.execute(state);
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
