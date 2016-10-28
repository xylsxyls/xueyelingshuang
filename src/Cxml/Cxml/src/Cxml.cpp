#include <SDKDDKVer.h>
#include "Cxml.h"
#include "CStringManager/CStringManagerAPI.h"


bool Cxml::GetOneModule(CString* pstrXml,CString* strXmlOneModule,int* flag){
	int nLeft = pstrXml->Find("<");
	if(nLeft == -1){
		CString strTemp = *pstrXml;
		//如果全部是无效字符则退出循环
		pstrXml->Replace(" ","");
		pstrXml->Replace("\t","");
		pstrXml->Replace("\n","");
		pstrXml->Replace("\r","");
		if(*pstrXml == "") return 0;
		//如果有字符则出错，中断解析
		else{
			mapError[strTemp] = "存在多余字符，解析中断";
			return 0;
		}
	}
	CStringManager strManager = *pstrXml;
	int nRight = strManager.FindOther('<','>',nLeft);
	if(nRight == -1){
		mapError[*pstrXml] = "找不到对应右侧括号\">\"，解析中断";
		return 0;
	}
	//取出对应括号内的数据，判断时候是注释或者CDATA，否则按节点处理
	CString strOne = pstrXml->Mid(nLeft + 1,nRight - nLeft - 1);
	//如果是注释
	if(strOne.Mid(0,3) == "!--" && strOne.Mid(strOne.GetLength() - 2,2) == "--" && strOne.GetLength() >= 5){
		*strXmlOneModule = strOne;
		*flag = NOTE;
		//删掉这个部分
		pstrXml->Delete(nLeft,nRight - nLeft + 1);
		return 1;
	}
	//如果是CDATA <![CDATA[文本内容]]>
	else if(strOne.Mid(0,8) == "![CDATA[" && strOne.Mid(strOne.GetLength() - 2,2) == "]]" && strOne.GetLength() >= 10){
		*strXmlOneModule = strOne;
		*flag = CDATA;
		//删掉这个部分，然后继续循环
		pstrXml->Delete(nLeft,nRight - nLeft + 2);
		return 1;
	}
	//先当成节点处理
	else{
		//如果是空节点
		if(strOne[0] == '/'){
			*strXmlOneModule = strOne;
			*flag = NULLVALUE;
			pstrXml->Delete(nLeft,nRight - nLeft + 2);
			return 1;
		}
		else{
			//先找到尾节点
			int nTailLeft = pstrXml->Find("</" + strOne + ">",nRight + 1);
			int nTailRight = -1;
			if(nTailLeft != -1){
				nTailRight = nTailLeft + strOne.GetLength() + 2;
			}
			if(nTailLeft == -1){
				mapError["<" + strOne + ">"] = "无法找到当前节点对应的尾节点";
				pstrXml->Delete(nLeft,nRight - nLeft + 2);
				return 0;
			}
			else{
				*strXmlOneModule = pstrXml->Mid(nLeft,nTailRight - nLeft + 1);;
				*flag = XML;
				pstrXml->Delete(nLeft,nTailRight - nLeft + 1);
				return 1;
				/*
				
				}*/
			}
		}
	}
}

void Cxml::LoadOneModule(CString strXmlOneModule,int flag){
	if(flag == NOTE){
		listNote.push_back(strXmlOneModule.Mid(3,strXmlOneModule.GetLength() - 5));
	}
	else if(flag == CDATA){
		listCDATA.push_back(strXmlOneModule.Mid(8,strXmlOneModule.GetLength() - 10));
	}
	else if(flag == NULLVALUE){
		Cxml nullXml;
		nullXml.strOneXmlName = strXmlOneModule.Mid(1,strXmlOneModule.GetLength() - 1);
		nullXml.strOneXmlValue = "null";
		listXml.push_back(nullXml);
	}
	else if(flag == XML){
		//先找到尾节点
		int nLeft = strXmlOneModule.Find("<");
		CStringManager strManager = strXmlOneModule;
		int nRight = strManager.FindOther('<','>',nLeft);
		CString strName = strXmlOneModule.Mid(nLeft + 1,nRight - nLeft - 1);
		int nTailLeft = strXmlOneModule.Find("</" + strName + ">",nRight + 1);
		int	nTailRight = nTailLeft + strName.GetLength() + 2;
		//先取出中间的内容
		CString strText = strXmlOneModule.Mid(nRight + 1,nTailLeft - nRight - 1);
		//如果内容中没有<说明是单个节点
		if(strText.Find('<') == -1){
			Cxml xmlTempOne;
			xmlTempOne.strOneXmlName = strName;
			xmlTempOne.strOneXmlValue = strText;
			listXml.push_back(xmlTempOne);
		}
		//有<则递归
		else{
			Cxml xmlTemp;
			xmlTemp.strOneXmlName = strName;
			map<CString,CString> mapChildError = xmlTemp.Load(strText);
			//合并错误map
			AddErrorMap(mapChildError);
			listXml.push_back(xmlTemp);
			/*
			while(1){
				CString strXmlModuleTemp;
				int flag = 0;
				bool x = GetOneModule(&strText,&strXmlModuleTemp,&flag);
				LoadOneModule(strXmlModuleTemp,flag);
				if(x == 0) break;
			}*/
			/*
			strOneXmlName = strOne;
			map<CString,CString> mapChildError;
			Cxml xmlTemp;
			mapChildError = xmlTemp.Load(strText);
			//合并错误map
			AddErrorMap(mapChildError);
			listXml.push_back(xmlTemp);
			strXml.Delete(nLeft,nTailRight - nLeft + 2);
			continue;*/
		}
	}
	
}

map<CString,CString> Cxml::Load(CString strXml){
	while(1){
		CString strXmlOneModule;
		int flag = 0;
		bool x = GetOneModule(&strXml,&strXmlOneModule,&flag);
		if(x == 0) break;
		LoadOneModule(strXmlOneModule,flag);
	}
	return mapError;
}

void Cxml::AddErrorMap(map<CString,CString> mapChildError){
	auto it = mapChildError.begin();
	for(;it != mapChildError.end();it++){
		mapError[it->first] = it->second;
	}
	return;
}

int main(){
	CString str = "<!---Students grades are uploaded by months---->\n <class_list>  		<student>  		<name>Tanmay</name>  		<grade>A</grade>  		</student>  		</class_list>";
	Cxml xml;
	xml.Load(str);
	int x = 3;
	return 0;
}