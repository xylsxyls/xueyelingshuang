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
};

//Cxml xml;
//xml["123"][0]["234"][1]["222"][0] = "aaa";//修改，或添加
//xml["123"]["234"]["222"][1] = "bbb";//添加
//xml["123"][0]["234"][1]["222"][0] = "delete";//删除
//CString str = xml["123"][0]["234"][1]["222"][0].toValue();
//CString strAttri = xml["123"][0]["234"][1]["222"][0]("name");
//CString strNote = xml["123"][0]["234"][1]["222"][0].GetNote(0);//表示获取当前节点下层首个注释，没有返回"-1"
//CString strNote = xml["123"][0]["234"][1]["222"][0].GetNote(-1);//表示获取本层上面的第一个注释内容，没有返回"-1"
//CString strNote = xml["123"][0]["234"][1]["222"][0].GetNote(1);//表示获取本层下面的第一个注释内容，没有返回"-1"
//CString strCDATA = xml["123"][0]["234"][1]["222"][0].GetCDATA(0);//和注释一样