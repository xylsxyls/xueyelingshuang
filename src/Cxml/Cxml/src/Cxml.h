#pragma once
#include <afxwin.h>
#include <list>
#include <map>
using namespace std;

class Cxml{
public:
	enum{
		NOTE,
		CDATA,
		NULLVALUE,
		XML
	};
public:
	map<CString,CString> Load(CString strXml);

	//存放注释
	list<CString> listNote;
	//存放CDATA
	list<CString> listCDATA;
	//存放单个节点
	CString strOneXmlName;
	CString strOneXmlValue;
	list<Cxml> listXml;

	//错误
	map<CString,CString> mapError;
	//合并错误map
	void AddErrorMap(map<CString,CString> mapChildError);

	
	//获取一个模块
	bool GetOneModule(CString* pstrXml,CString* strXmlOneModule,int* flag);
	//加载一个模块
	void LoadOneModule(CString strXmlOneModule,int flag);
};

//Cxml xml;
//xml["123"](0)["234"](1)["222"](0) = "aaa";//修改，或添加
//xml["123"](0)["234"](1)["222"](0)[1] = "bbb";//添加
//xml["123"](0)["234"](1)["222"](0) = "delete";//删除
//CString str = xml["123"](0)["234"](1)["222"](0).toValue();
//CString strAttri = xml["123"](0)["234"](1)["222"](0).GetAttri("name");
//CString strNote = xml["123"](0)["234"](1)["222"](0).GetNote(0);//表示获取当前节点下层注释，没有返回空
//CString strNote = xml["123"](0)["234"](1)["222"](0).GetNote(-1);//表示获取本层上面的第一个注释内容，没有返回空
//CString strCDATA = xml["123"](0)["234"](1)["222"](0).GetCDATA(0);//和注释一样