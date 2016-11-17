#include <SDKDDKVer.h>
#include "Cxml.h"
#include "CStringManager/CStringManagerAPI.h"
#include "CCharset/CCharsetAPI.h"

Cxml::Cxml(){
	version = 0;
	bVisit = 1;
}

void Cxml::init(double version,CString charset,CString standalone){
	this->version = version;
	this->charset = charset;
	this->standalone = standalone;
}

void Cxml::LoadHead(CString* strXml){
	//如果有文件头先加载文件头
	int nHeadLeft = strXml->Find("<?xml");
	//文件头搜索关键字可能在CDATA中存在
	int nHeadLeftSame = strXml->Find("<");
	//如果没有文件头，找不到关键字或者找到了，但关键字和<不在同一位置
	if(nHeadLeft == -1 || (nHeadLeft != -1 && nHeadLeftSame != nHeadLeft)) return;
	//如果有
	int nHeadRight = strXml->Find(">");
	if(nHeadRight == -1){
		mapError[*strXml] = "无对应>";
		return;
	}
	CString strHead = strXml->Mid(nHeadLeft,nHeadRight - nHeadLeft + 1);
	int nBegin = 0;
	if((nBegin = strHead.Find("version")) != -1 || (nBegin = strHead.Find("VERSION")) != -1){
		int nLeft = strHead.Find("\"",nBegin);
		if(nLeft == -1){
			mapError[strHead] = "version后引号出错";
			return;
		}
		int nRight = strHead.Find("\"",nLeft + 1);
		if(nRight == -1){
			mapError[strHead] = "version后引号出错";
			return;
		}
		CString strVersion = strHead.Mid(nLeft + 1,nRight - nLeft - 1);
		version = atof(strVersion);
	}
	if((nBegin = strHead.Find("encoding")) != -1 || (nBegin = strHead.Find("ENCODING")) != -1){
		int nLeft = strHead.Find("\"",nBegin);
		if(nLeft == -1){
			mapError[strHead] = "encoding后引号出错";
			return;
		}
		int nRight = strHead.Find("\"",nLeft + 1);
		if(nRight == -1){
			mapError[strHead] = "encoding后引号出错";
			return;
		}
		charset = strHead.Mid(nLeft + 1,nRight - nLeft - 1);
	}
	if((nBegin = strHead.Find("standalone")) != -1 || (nBegin = strHead.Find("STANDALONE")) != -1){
		int nLeft = strHead.Find("\"",nBegin);
		if(nLeft == -1){
			mapError[strHead] = "standalone后引号出错";
			return;
		}
		int nRight = strHead.Find("\"",nLeft + 1);
		if(nRight == -1){
			mapError[strHead] = "standalone后引号出错";
			return;
		}
		standalone = strHead.Mid(nLeft + 1,nRight - nLeft - 1);
	}
	strXml->Delete(nHeadLeft,nHeadRight - nHeadLeft + 1);
	return;
}

bool Cxml::GetOneModule(CString* pstrXml,int* flag,CString* strXmlName,CString* strXmlAttri,CString* strXmlText){
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
		*strXmlText = strOne.Mid(3,strOne.GetLength() - 5);
		*flag = NOTE;
		//删掉这个部分
		pstrXml->Delete(nLeft,nRight - nLeft + 1);
		return 1;
	}
	//如果是CDATA <![CDATA[文本内容]]>
	else if(strOne.Mid(0,8) == "![CDATA[" && strOne.Mid(strOne.GetLength() - 2,2) == "]]" && strOne.GetLength() >= 10){
		*strXmlText = strOne.Mid(8,strOne.GetLength() - 10);
		*flag = CDATA;
		//删掉这个部分，然后继续循环
		pstrXml->Delete(nLeft,nRight - nLeft + 1);
		return 1;
	}
	//先当成节点处理
	else{
		//如果是空节点
		if(strOne[strOne.GetLength() - 1] == '/'){
			*flag = NULLVALUE;
			int nAttriLeft = strOne.Find(" ") + 1;
			//说明没有属性值
			if(nAttriLeft == 0){
				*strXmlName = strOne.Mid(0,strOne.GetLength() - 1);
				*strXmlAttri = "";
			}
			else{
				*strXmlName = strOne.Mid(0,nAttriLeft - 1);
				*strXmlAttri = strOne.Mid(nAttriLeft,strOne.GetLength() - nAttriLeft - 1);
			}
			*strXmlText = "null";
			pstrXml->Delete(nLeft,nRight - nLeft + 1);
			return 1;
		}
		else{
			*flag = XML;
			int nAttriLeft = strOne.Find(" ") + 1;
			//说明没有属性值
			if(nAttriLeft == 0){
				*strXmlName = strOne.Mid(0,strOne.GetLength());
				*strXmlAttri = "";
			}
			else{
				*strXmlName = strOne.Mid(0,nAttriLeft - 1);
				*strXmlAttri = strOne.Mid(nAttriLeft,strOne.GetLength() - nAttriLeft);
			}
			//先找到尾节点
			int nTailLeft = pstrXml->Find("</" + *strXmlName + ">",nRight + 1);
			if(nTailLeft == -1){
				mapError["<" + *strXmlName + ">"] = "无法找到当前节点对应的尾节点";
				pstrXml->Delete(nLeft,nRight - nLeft + 1);
				return 0;
			}
			else{
				int nTailRight = nTailLeft + strXmlName->GetLength() + 2;
				*strXmlText = pstrXml->Mid(nRight + 1,nTailLeft - nRight - 1);;
				pstrXml->Delete(nLeft,nTailRight - nLeft + 1);
				return 1;
			}
		}
	}
}

void Cxml::LoadOneModule(int flag,CString strXmlName,CString strXmlAttri,CString strXmlText){
	if(flag == NOTE){
		listNote.push_back(CNoteData(listXml.size(),strXmlText));
	}
	else if(flag == CDATA){
		listCDATA.push_back(CNoteData(listXml.size(),strXmlText));
	}
	else if(flag == NULLVALUE){
		Cxml nullXml;
		nullXml.strOneXmlName = strXmlName;
		nullXml.strOneXmlValue = strXmlText;
		if(strXmlAttri != "") nullXml.mapAttri = nullXml.LoadAttri(strXmlAttri);
		listXml.push_back(nullXml);
	}
	else if(flag == XML){
		//如果内容中没有<说明是单个节点
		if(strXmlText.Find('<') == -1){
			Cxml xmlTempOne;
			xmlTempOne.strOneXmlName = strXmlName;
			xmlTempOne.strOneXmlValue = strXmlText;
			if(strXmlAttri != "") xmlTempOne.mapAttri = xmlTempOne.LoadAttri(strXmlAttri);
			listXml.push_back(xmlTempOne);
		}
		//有<则递归
		else{
			Cxml xmlTemp;
			xmlTemp.strOneXmlName = strXmlName;
			if(strXmlAttri != "") xmlTemp.mapAttri = xmlTemp.LoadAttri(strXmlAttri);
			map<CString,CString> mapChildError = xmlTemp.Load(strXmlText);
			//合并错误map
			AddErrorMap(mapChildError);
			listXml.push_back(xmlTemp);
		}
	}
	return;
}

map<CString,CString> Cxml::LoadAttri(CString strXmlAttri){
	map<CString,CString> mapAttri;
	CStringManager strManager = strXmlAttri;
	vector<CString> vecHead = strManager.split(" ");
	int i = -1;
	while(i++ != vecHead.size() - 1){
		CStringManager strOneManager = vecHead.at(i);
		vector<CString> vecOneAttri = strOneManager.split("=");
		if(vecOneAttri.size() == 1){
			mapError[strXmlAttri] = "属性之间有多个空格或出现无等号属性";
		}
		else if(vecOneAttri.size() > 2){
			mapError[strXmlAttri] = "属性之间有多个等号";
		}
		else{
			CString strAttriValue = vecOneAttri.at(1);
			if((strAttriValue[0] == '\"' && strAttriValue[strAttriValue.GetLength() - 1] == '\"') || (strAttriValue[0] == '\'' && strAttriValue[strAttriValue.GetLength() - 1] == '\'')){
				mapAttri[vecOneAttri.at(0)] = strAttriValue.Mid(1,strAttriValue.GetLength() - 2);
			}
			else{
				mapError[strXmlAttri] = "属性值周围无引号，或有空格";
			}
		}
	}
	return mapAttri;
}

map<CString,CString> Cxml::Load(CString strXml){
	//加载文件头
	LoadHead(&strXml);
	while(1){
		CString strXmlName,strXmlAttri,strXmlText;
		int flag = 0;
		bool x = GetOneModule(&strXml,&flag,&strXmlName,&strXmlAttri,&strXmlText);
		if(x == 0) break;
		LoadOneModule(flag,strXmlName,strXmlAttri,strXmlText);
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

CString Cxml::toCString(CString NewLineSign,CString FormatSign,int FormatLength){
	//文件头
	CString strHead;
	if(version != 0 || charset != "" || standalone != ""){
		strHead = "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>";
		if(version != 0){
			CString strTemp;
			strTemp.Format("%1.1lf",version);
			strHead.Replace("1.0",strTemp);
		}
		else strHead.Replace("version=\"1.0\" ","");
		if(charset != "") strHead.Replace("UTF-8",charset);
		else strHead.Replace("encoding=\"UTF-8\" ","");
		if(standalone != "") strHead.Replace("yes",standalone);
		else strHead.Replace("standalone=\"yes\" ","");
	}

	//得到strFormatSignTemp，带格式个数的格式标识位
	int nFormatLengthTemp = FormatLength;
	CString strFormatSignTemp;
	while(nFormatLengthTemp-- != 0) strFormatSignTemp = strFormatSignTemp + FormatSign;
	
	CString strXmlResult;
	int nInsert = 0;
	//先把名字和值转换成字符串，如果无名则表示在最外层
	//有名
	if(strOneXmlName != ""){
		//空值
		if(strOneXmlValue == "null"){
			strXmlResult = NewLineSign + strFormatSignTemp + "<" + strOneXmlName + AttriToCString(mapAttri) + "/>";
			nInsert = nInsert + strXmlResult.GetLength();
		}
		//实值
		else if(strOneXmlValue != ""){
			strXmlResult = NewLineSign + strFormatSignTemp + "<" + strOneXmlName + AttriToCString(mapAttri) + ">" + strOneXmlValue + "</" + strOneXmlName + ">";
			nInsert = nInsert + strXmlResult.GetLength();
		}
		//无值，从listXml里进入下一层
		else{
			strXmlResult = NewLineSign + strFormatSignTemp + "<" + strOneXmlName + AttriToCString(mapAttri) + ">" + strFormatSignTemp + "</" + strOneXmlName + ">";
			nInsert = strXmlResult.Find(strFormatSignTemp + "</" + strOneXmlName + ">");
			//因为进入到下一层，所以需要格式 + 1
			AddXml(&strXmlResult,&nInsert,NewLineSign,FormatSign,FormatLength + 1);
		}
	}
	//无名
	else{
		//无名表示最外层，所以格式不加
		AddXml(&strXmlResult,&nInsert,NewLineSign,FormatSign,FormatLength);
	}
	//如果无文件头去掉最开头的回车符
	if(strHead == ""){
		return strXmlResult.Mid(NewLineSign.GetLength(),strXmlResult.GetLength() - NewLineSign.GetLength());
	}
	return strHead + strXmlResult;
}

CString Cxml::AttriToCString(map<CString,CString> mapAttri){
	CString strResult;
	for(auto it = mapAttri.begin();it != mapAttri.end();it++){
		strResult = strResult + " " + it->first + "=";
		int nFind = it->second.Find("\"");
		//说明值的中间没有双引号，则在左右加上双引号
		if(nFind == -1) strResult = strResult + "\"" + it->second + "\"";
		else strResult = strResult + "\'" + it->second + "\'";
	}
	return strResult;
}

void Cxml::AddXml(CString* strXmlResult,int* pInsert,CString NewLineSign,CString FormatSign,int FormatLength){
	auto itNote = listNote.begin();
	auto itData = listCDATA.begin();
	auto itXml = listXml.begin();
	int i = -1;
	//循环次数根据listXml决定
	while(itXml != listXml.end()){
		//第几次循环的标识位
		i++;
		//得到strFormatSignTemp，带格式个数的格式标识位
		int nFormatLengthTemp = FormatLength;
		CString strFormatSignTemp;
		while(nFormatLengthTemp-- != 0) strFormatSignTemp = strFormatSignTemp + FormatSign;
		//如果到了对应的注释或CDATA点则加进去
		if(itNote != listNote.end() && itNote->nPosition == i){
			CString strNote = NewLineSign + strFormatSignTemp + "<!--" + itNote->strNoteData + "-->";
			strXmlResult->Insert(*pInsert,strNote);
			*pInsert = *pInsert + strNote.GetLength();
			itNote++;
		}
		if(itData != listCDATA.end() && itData->nPosition == i){
			CString strData = NewLineSign + strFormatSignTemp + "<![CDATA[" + itData->strNoteData + "]]>";
			strXmlResult->Insert(*pInsert,strData);
			*pInsert = *pInsert + strData.GetLength();
			itData++;
		}
		CString strXmlTemp = NewLineSign + itXml->toCString(NewLineSign,FormatSign,FormatLength);
		strXmlResult->Insert(*pInsert,strXmlTemp);
		*pInsert = *pInsert + strXmlTemp.GetLength();
		itXml++;
	}
	strXmlResult->Insert(*pInsert,NewLineSign);
	*pInsert = *pInsert + NewLineSign.GetLength();
	return;
}

Cxml& Cxml::operator[](CString strNode){
	//如果最后一个元素出错
	if(listVisit.size() > 0){
		int nLast = listVisit.back();
		if(nLast == 3 || nLast == 5){
			bVisit = 0;
			return *this;
		}
	}
	listVisit.push_back(1);
	listCStringStorage.push_back(strNode);
	return *this;
}

Cxml& Cxml::operator()(int n){
	if(n < 0){
		bVisit = 0;
		return *this;
	}
	//如果访问路径为空则出错
	if(listVisit.size() == 0){
		bVisit = 0;
		return *this;
	}
	//如果最后一个元素出错
	int nLast = listVisit.back();
	if(nLast == 2 || nLast == 3 || nLast == 5){
		bVisit = 0;
		return *this;
	}
	listVisit.push_back(2);
	listintStorage.push_back(n);
	return *this;
}

Cxml& Cxml::operator()(CString strAttriName){
	//如果访问路径为空则出错
	if(listVisit.size() == 0){
		bVisit = 0;
		return *this;
	}
	//如果最后一个元素出错
	int nLast = listVisit.back();
	if(nLast == 3 || nLast == 5){
		bVisit = 0;
		return *this;
	}
	listVisit.push_back(3);
	listCStringStorage.push_back(strAttriName);
	return *this;
}

Cxml& Cxml::operator()(CString strAttriName,CString strAttriValue){
	//如果访问路径为空则出错
	if(listVisit.size() == 0){
		bVisit = 0;
		return *this;
	}
	//如果最后一个元素出错
	int nLast = listVisit.back();
	if(nLast == 2 || nLast == 3 || nLast == 5){
		bVisit = 0;
		return *this;
	}
	listVisit.push_back(4);
	listCStringStorage.push_back(strAttriName);
	listCStringStorage.push_back(strAttriValue);
	return *this;
}

Cxml& Cxml::operator()(CString strNOTECDATA,int nFlag){
	//如果访问路径为空则出错
	if(listVisit.size() == 0){
		bVisit = 0;
		return *this;
	}
	//如果最后一个元素出错
	int nLast = listVisit.back();
	if(nLast == 3 || nLast == 5){
		bVisit = 0;
		return *this;
	}
	listVisit.push_back(5);
	listCStringStorage.push_back(strNOTECDATA);
	listintStorage.push_back(nFlag);
	return *this;
}

Cxml Cxml::operator=(CString strValue){
	//如果访问路径出错则直接返回
	if(bVisit == 0) return *this;
	//建立一个指针池，存放符合可能的所有指针，先存入第一层中listXml的指针
	list<list<Cxml>*> listplistXml;
	listplistXml.push_back(&listXml);
	//建立两个临时变量，一个存储上一个节点路径，一个存储CString
	list<int> listVisitTemp;
	list<CString> listCStringStorageTemp;
	list<Cxml> listXmlTemp;
	listXmlTemp.push_back(*this);
	//上层指针
	list<Cxml>* plistXmlTemp = &listXmlTemp;
	//取出listVisit中最后一个，用于最后赋值判断
	int nLast = listVisit.back();
	//循环到路径没有为止
	while(listVisit.size() != 0){
		//如果是节点
		if(listVisit.front() == 1){
			//取出节点名存入临时变量并弹出节点名
			CString strNode = *listCStringStorage.begin();
			listCStringStorage.pop_front();
			listVisitTemp.clear();
			listVisitTemp.push_back(1);
			listCStringStorageTemp.clear();
			listCStringStorageTemp.push_back(strNode);

			//无论是写入还是修改该层的值必须先清空
			list<Cxml>* plistXmlFirst = (*listplistXml.begin());
			for(auto itplistXmlTemp = plistXmlTemp->begin();itplistXmlTemp != plistXmlTemp->end();itplistXmlTemp++){
				if(&(itplistXmlTemp->listXml) == plistXmlFirst){
					itplistXmlTemp->strOneXmlValue = "";
					break;
				}
			}

			//取出指针list中第一个指针，循环从第一个指针指的list中查找
			plistXmlTemp = *listplistXml.begin();
			//取出第一个之后就清空，方便后面存入下层的多个指针
			listplistXml.clear();
			for(auto itXml = plistXmlTemp->begin();itXml != plistXmlTemp->end();itXml++){
				//如果能找到该节点
				if(itXml->strOneXmlName == strNode){
					//把下一个Cxml指针赋值给临时变量
					listplistXml.push_back(&(itXml->listXml));
				}
			}
			//如果为空说明是写入
			if(listplistXml.size() == 0){
				Cxml xmlTemp;
				xmlTemp.strOneXmlName = strNode;
				xmlTemp.strOneXmlValue = "AddNewTemp";
				plistXmlTemp->push_back(xmlTemp);
				//必须进入list才能给到真正的指针，把新开的空list指针传给指针池
				listplistXml.push_back(&(plistXmlTemp->back().listXml));
			}
		}
		else if(listVisit.front() == 2){
			//取出并弹出存储数据
			int n = *listintStorage.begin();
			listintStorage.pop_front();
			//如果只有一个，并且是上层添加进去的
			if(listplistXml.size() == 1 && ((*listplistXml.begin())->begin()->strOneXmlValue == "AddNewTemp")){
				(*listplistXml.begin())->begin()->strOneXmlValue = "";
				goto rem;
			}
			//如果指针池内没有该位置的指针
			if(listplistXml.size() <= (unsigned int)n){
				//添加一个新的Cxml并给出其中listXml的指针
				Cxml xmlTemp;
				xmlTemp.strOneXmlName = *listCStringStorageTemp.begin();
				xmlTemp.strOneXmlValue = "AddNewTemp";
				listCStringStorageTemp.pop_front();
				while(listCStringStorageTemp.size() != 0){
					xmlTemp.mapAttri[*listCStringStorageTemp.begin()] = *(++listCStringStorageTemp.begin());
					listCStringStorageTemp.pop_front();
					listCStringStorageTemp.pop_front();
				}
				//存入临时Cxml，清空指针池，并把list中的指针放入指针池中
				plistXmlTemp->push_back(xmlTemp);
				listplistXml.clear();
				listplistXml.push_back(&(plistXmlTemp->back().listXml));
			}
			else{
				int nTemp = n;
				auto itlistplistXml = listplistXml.begin();
				while(nTemp-- != 0){
					itlistplistXml++;
				}
				//取出指针池中指定的那个指针，再清空指针池，再放入指定的那个
				list<Cxml>* plistXmlTemp = *itlistplistXml;
				listplistXml.clear();
				listplistXml.push_back(plistXmlTemp);
			}
		}
		else if(listVisit.front() == 3){
			//取出属性名，并弹出
			CString strAttriName = *listCStringStorage.begin();
			listCStringStorage.pop_front();
			//默认按照第一个存
			(*listplistXml.begin())->begin()->mapAttri[strAttriName] = strValue;
		}
		else if(listVisit.front() == 4){
			//取出属性名和属性值
			CString strAttriName = *listCStringStorage.begin();
			CString strAttriValue = *(++listCStringStorage.begin());
			listCStringStorage.pop_front();
			listCStringStorage.pop_front();
			//从指针池中寻找是否有符合这个条件的节点，如果没有则全部重新建立
			//先把属性名和属性值存入临时list中
			listCStringStorageTemp.push_back(strAttriName);
			listCStringStorageTemp.push_back(strAttriValue);
			//如果指针池中只有一个指针，先判断是不是新添加的，如果是，那么直接添加一条属性即可
			if(listplistXml.size() == 1 && ((*listplistXml.begin())->begin()->strOneXmlValue == "AddNewTemp")){
				(*listplistXml.begin())->begin()->mapAttri[strAttriName] = strAttriValue;
				goto rem;
			}
			//建立一个临时指针池
			list<list<Cxml>*> listplistXmlTemp;
			for(auto itlistplistXml = listplistXml.begin();itlistplistXml != listplistXml.end();itlistplistXml++){
				map<CString,CString>* pmapAttriTemp = &((*itlistplistXml)->begin()->mapAttri);
				//如果找到这条属性则把当前的指针放到临时指针池中
				if(pmapAttriTemp->find(strAttriName) != pmapAttriTemp->end() && (*pmapAttriTemp)[strAttriName] == strAttriValue){
					listplistXmlTemp.push_back(*itlistplistXml);
				}
			}
			//将临时指针池赋值给指针池
			listplistXml = listplistXmlTemp;
			//如果此时指针池依然为空，说明没有指定的指针
			if(listplistXml.size() == 0){
				Cxml xmlTemp;
				xmlTemp.strOneXmlName = *listCStringStorageTemp.begin();
				xmlTemp.strOneXmlValue = "AddNewTemp";
				listCStringStorageTemp.pop_front();
				while(listCStringStorageTemp.size() != 0){
					xmlTemp.mapAttri[*listCStringStorageTemp.begin()] = *(++listCStringStorageTemp.begin());
					listCStringStorageTemp.pop_front();
					listCStringStorageTemp.pop_front();
				}
				//存入临时Cxml，清空指针池，并把list中的指针放入指针池中
				plistXmlTemp->push_back(xmlTemp);
				listplistXml.clear();
				listplistXml.push_back(&(plistXmlTemp->back().listXml));
			}
		}
		else if(listVisit.front() == 5){
			CString strNOTECDATA = *listCStringStorage.begin();
			int nFlag = *listintStorage.begin();
			if(strNOTECDATA == "NOTE"){
				if(nFlag == -1){
					//先看指针池中的第一个指针是上层指针下listXml中的第几个，然后添加在上层指针下
					list<Cxml>* plistXmlFirst = *listplistXml.begin();
					int nPositionTemp = 0;
					for(auto itlistXml = plistXmlTemp->begin();itlistXml != plistXmlTemp->end();itlistXml++){
						if(plistXmlFirst == &(itlistXml->listXml)){
							CNoteData NoteDataTemp;
							NoteDataTemp.nPosition = nPositionTemp;
							NoteDataTemp.strNoteData = strValue;
							itlistXml->listNote.push_back(NoteDataTemp);
							goto rem;
						}
						nPositionTemp++;
					}
				}
				else if(nFlag == 1){
					//先看指针池中的第一个指针是上层指针下listXml中的第几个，然后添加在上层指针下
					list<Cxml>* plistXmlFirst = *listplistXml.begin();
					int nPositionTemp = 0;
					for(auto itlistXml = plistXmlTemp->begin();itlistXml != plistXmlTemp->end();itlistXml++){
						if(plistXmlFirst == &(itlistXml->listXml)){
							CNoteData NoteDataTemp;
							NoteDataTemp.nPosition = nPositionTemp + 1;
							NoteDataTemp.strNoteData = strValue;
							itlistXml->listNote.push_back(NoteDataTemp);
							goto rem;
						}
						nPositionTemp++;
					}
				}
				else if(nFlag == 0){
					//先看指针池中的第一个指针是上层指针下listXml中的第几个，然后添加在本层指针下
					list<Cxml>* plistXmlFirst = *listplistXml.begin();
					int nPositionTemp = 0;
					for(auto itlistXml = plistXmlTemp->begin();itlistXml != plistXmlTemp->end();itlistXml++){
						if(plistXmlFirst == &(itlistXml->listXml)){
							CNoteData NoteDataTemp;
							NoteDataTemp.nPosition = 0;
							NoteDataTemp.strNoteData = strValue;
							plistXmlFirst->begin()->listNote.push_back(NoteDataTemp);
							goto rem;
						}
						nPositionTemp++;
					}
				}
				else AfxMessageBox("标识出错，分别为-1，0，1");
			}
			else if(strNOTECDATA == "CDATA"){
				if(nFlag == -1){
					//先看指针池中的第一个指针是上层指针下listXml中的第几个，然后添加在上层指针下
					list<Cxml>* plistXmlFirst = *listplistXml.begin();
					int nPositionTemp = 0;
					for(auto itlistXml = plistXmlTemp->begin();itlistXml != plistXmlTemp->end();itlistXml++){
						if(plistXmlFirst == &(itlistXml->listXml)){
							CNoteData NoteDataTemp;
							NoteDataTemp.nPosition = nPositionTemp;
							NoteDataTemp.strNoteData = strValue;
							itlistXml->listCDATA.push_back(NoteDataTemp);
							goto rem;
						}
						nPositionTemp++;
					}
				}
				else if(nFlag == 1){
					//先看指针池中的第一个指针是上层指针下listXml中的第几个，然后添加在上层指针下
					list<Cxml>* plistXmlFirst = *listplistXml.begin();
					int nPositionTemp = 0;
					for(auto itlistXml = plistXmlTemp->begin();itlistXml != plistXmlTemp->end();itlistXml++){
						if(plistXmlFirst == &(itlistXml->listXml)){
							CNoteData NoteDataTemp;
							NoteDataTemp.nPosition = nPositionTemp + 1;
							NoteDataTemp.strNoteData = strValue;
							itlistXml->listCDATA.push_back(NoteDataTemp);
							goto rem;
						}
						nPositionTemp++;
					}
				}
				else if(nFlag == 0){
					//先看指针池中的第一个指针是上层指针下listXml中的第几个，然后添加在本层指针下
					list<Cxml>* plistXmlFirst = *listplistXml.begin();
					int nPositionTemp = 0;
					for(auto itlistXml = plistXmlTemp->begin();itlistXml != plistXmlTemp->end();itlistXml++){
						if(plistXmlFirst == &(itlistXml->listXml)){
							CNoteData NoteDataTemp;
							NoteDataTemp.nPosition = 0;
							NoteDataTemp.strNoteData = strValue;
							plistXmlFirst->begin()->listCDATA.push_back(NoteDataTemp);
							goto rem;
						}
						nPositionTemp++;
					}
				}
				else AfxMessageBox("标识出错，分别为-1，0，1");
			}
			else AfxMessageBox("注释或CDATA存入标识出错，分别为NOTE和CDATA");
		}
		else AfxMessageBox("访问路径被篡改");
		rem:
		//取出一个路径弹出一个路径
		listVisit.pop_front();
	}
	//取出指针池中的第一个进行赋值
	if(nLast == 1 || nLast == 2 || nLast == 4){
		//把指针池中第一个指针所指list清空，然后从上层指针中循环寻找第一个指针所在的Cxml，然后修改赋值
		list<Cxml>* plistXmlFirst = (*listplistXml.begin());
		plistXmlFirst->clear();
		for(auto itplistXmlTemp = plistXmlTemp->begin();itplistXmlTemp != plistXmlTemp->end();itplistXmlTemp++){
			if(&(itplistXmlTemp->listXml) == plistXmlFirst){
				itplistXmlTemp->strOneXmlValue = strValue;
				break;
			}
		}
	}
	else if(nLast == 3 || nLast == 5);
	else AfxMessageBox("访问路径被篡改");
	return *this;
}

Cxml Cxml::operator=(int nValue){
	CString strTemp;
	strTemp.Format("%d",nValue);
	*this = strTemp;
	return *this;
}

Cxml Cxml::operator=(double dValue){
	CString strTemp;
	strTemp.Format("%lf",dValue);
	*this = strTemp;
	return *this;
}

CString Cxml::toValue(){
	return "";
}


int main(){
	//double a = 0;
	//if(a != 0) AfxMessageBox("1");
	//CString str = "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?><!--by months--> <class_list a=\'1\' b=\"2\">  		<student a=\"3\" b=\'\"6\"\'>  		<!--Shdds--><name/>  <!--Shs--><![CDATA[123456<<<>>>789]]><grade>A</grade>  		</student>  		</class_list>";
	Cxml xml;
	xml["aaa"]["bbb"] = 1;
	xml["aaa"]["ccc"] = 1;
	//xml.Load(str);
	CString strXml = xml.toCString();
	AfxMessageBox(strXml);
	int x = 3;
	return 0;
}