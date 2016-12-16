#pragma once
#include <afxwin.h>
#include <list>
#include <map>
using namespace std;
#include "CNoteData.h"

class Cxml{
public:
	enum{
		NOTE,
		CDATA,
		NULLVALUE,
		XML
	};
public:
	double version;
	CString charset;
	CString standalone;
	//存放注释
	list<CNoteData> listNote;
	//存放CDATA
	list<CNoteData> listCDATA;
	//存放单个节点
	CString strOneXmlName;
	CString strOneXmlValue;
	//存放节点属性
	map<CString,CString> mapAttri;
	list<Cxml> listXml;

	//上层指针
	//Cxml* pPreXml;

	//错误
	map<CString,CString> mapError;

	Cxml();
	map<CString,CString> Load(CString strXml);
	//如果自己写需要init，1.0，（UTF-8，GBK），YES
	void init(double version,CString charset,CString standalone);
	//加载文件头
	void LoadHead(CString* strXml);
	//获取一个模块
	bool GetOneModule(CString* pstrXml,int* flag,CString* strXmlName,CString* strXmlAttri,CString* strXmlText);
	//加载一个模块
	void LoadOneModule(int flag,CString strXmlName,CString strXmlAttri,CString strXmlText);
	//合并错误map
	void AddErrorMap(map<CString,CString> mapChildError);
	//加载属性集
	map<CString,CString> LoadAttri(CString strXmlAttri);

	//转换成字符串
	CString toCString(CString NewLineSign = "\r\n",CString FormatSign = "\t",int FormatLength = 0);
	CString AttriToCString(map<CString,CString> mapAttri);
	//添加一整个xml，从nInsert插进去
	void AddXml(CString* strXmlResult,int* nInsert,CString NewLineSign,CString FormatSign,int FormatLength);

	//一开始设为可访问，当访问代码出错是这个设为0，在进行toValue或=之后继续设为1
	bool bVisit;
	list<int> listVisit;
	list<CString> listCStringStorage;
	list<int> listintStorage;

	//增删改查
	Cxml& operator[](CString strNode);//1
	Cxml& operator()(int n);//2
	Cxml& operator()(CString strAttriName);//3
	Cxml& operator()(CString strAttriName,CString strAttriValue);//4
	Cxml& operator()(CString strNOTECDATA,int nFlag);//5
	Cxml operator=(CString strValue);
	Cxml operator=(int nValue);
	Cxml operator=(double dValue);
	CString toValue();
	Cxml* FindPrepXmlFromXml(Cxml* pxml,list<Cxml>* plistXml);
	Cxml* FindPrepXml(list<Cxml>* plistXml);
	Cxml* FindPrepXmlFromXml(Cxml* pxml,Cxml* pFindXml);
	Cxml* FindPrepXml(Cxml* pFindXml);
	

};

//小括号表示具体定位，比如定位到第几个，定位属性，定位注释CDATA
//中括号定位节点
//toValue取数据，不写toValue直接赋值数据，赋值失败不影响原文件
//整形，小数，字符串都会转化成字符串录入，但是取出时一律取出字符串
//如果定位属性失败则不修改，如果定位的个数过大则添在最后一个，如果输入了负数则不修改
//超长录入则录入到最后一个，超长访问则访问失败
//在路径内部直接写不存在的属性为另行添加而不是修改，属性定位时不支持直接修改
//添加时路径中出现找不到的属性的则直接按整条添加在最后，如果是个数则直接定位，个数后不能再用属性定位
//Cxml xml;
//xml["123"](0)["234"](1)["222"](0) = "aaa";//修改，或添加
//xml["123"]["234"]["222"](1) = "bbb";//添加
//xml["123"](0)["234"](1)["222"](0) = "delete";//删除
//CString str = xml["123"](0)["234"](1)["222"](0).toValue();
//CString str = xml["123"](0)["234"](1)["222"](0)("name","aa").toValue();
//CString strAttri = xml["123"](0)["234"](1)["222"](0)("name").toValue();
//xml["123"](0)["234"](1)["222"](0)("name","3") = 5;
//CString strNote = xml["123"](0)["234"](1)["222"](0)("NOTE",0).toValue();//表示获取当前节点下层首个注释，没有返回"-1"
//CString strNote = xml["123"](0)["234"](1)["222"](0)("NOTE",-1).toValue();//表示获取本层上面的第一个注释内容，没有返回"-1"
//CString strNote = xml["123"](0)["234"](1)["222"](0)("NOTE",1).toValue();//表示获取本层下面的第一个注释内容，没有返回"-1"
//CString strCDATA = xml["123"](0)["234"](1)["222"](0)("CDATA",0).toValue();//和注释一样
