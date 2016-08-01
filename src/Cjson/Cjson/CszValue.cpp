#include <SDKDDKVer.h>
#include "CszValue.h"
#include "CStringManagerAPI.h"
#include "CTypeValue.h"
#include "CstrValue.h"
#include "Cjson.h"

CszValue::CszValue(){

}

CszValue::CszValue(CString strValue){
	int num = 0;
	int begin = 0;
	int i = -1;
	while(1){
		i++;
		//如果到了末尾先取出最后一个再退出
		if(strValue[i] == 0){
			//由于在数组中只会出现json和值，出现json时，在下面的循环中遇到0退出，所以这里只要处理遇到逗号的情况
			CString strTemp = strValue.Mid(begin,i - begin);
			CstrValue strValueTemp = strTemp;
			if(strValueTemp.type == -1){
				//如果是空数组，那么传进来的字符串为无效字符
				strTemp.Replace("\t","");
				strTemp.Replace("\r\n","");
				strTemp.Replace("\n","");
				strTemp.Replace(" ","");
				//说明是空数组
				if(strTemp == "") break;
				else mapszError[strTemp] = "不可识别的数组值";
			}
			vecszValue.push_back(strValueTemp);
			break;
		}
		//如果先找到了逗号说明遇到了一个字段值
		if(strValue[i] == ','){
			CstrValue strValueTemp = strValue.Mid(begin,i - begin);
			if(strValueTemp.type == -1){
				mapszError[strValue.Mid(begin,i - begin)] = "不可识别的数组值";
			}
			vecszValue.push_back(strValueTemp);
			num++;
			//取逗号后的位置作为开始，因为i会在循环开始做++，所以不需要动
			begin = i + 1;
		}
		//如果找到了一个{说明找到了一个json
		if(strValue[i] == '{'){
			CString strValueTemp = strValue.Mid(0,i);
			int nPre = strValueTemp.ReverseFind(',');
			strValueTemp = strValueTemp.Mid(nPre + 1,i - nPre - 1);
			strValueTemp.Replace("\t","");
			strValueTemp.Replace("\r\n","");
			strValueTemp.Replace("\n","");
			strValueTemp.Replace(" ","");
			if(strValueTemp != "") mapszError[strValue.Mid(nPre + 1,i - nPre - 1)] = "在数组中json元素前有多余字符";

			CStringManager manager = strValue;
			int nRight = manager.FindOther('{','}',i);
			if(nRight == -1) mapszError[strValue] = "{}不匹配";
			Cjson json;
			map<CString,CString> mapJsonError = json.LoadJson(strValue.Mid(i,nRight - i + 1));

			if(mapJsonError.size() != 0){
				auto it = mapJsonError.begin();
				for(;it != mapJsonError.end();it++){
					mapszError[it->first] = it->second;
				}
			}

			vecszValue.push_back(json);
			num++;
			//取逗号后的位置作为开始
			while(1){
				//防止在结束时，因为没找到逗号导致i还是原来左括号的位置，先把i放到右括号的位置，循环开始的时候会做++，寻找下一位
				i = nRight;
				nRight++;
				if(strValue[nRight] == 0) return;
				if(strValue[nRight] == '\t' || (strValue[nRight] == '\r' && strValue[nRight + 1] == '\n') || strValue[nRight] == '\n' || strValue[nRight] == ' ') continue;
				else if(strValue[nRight] == ','){
					begin = nRight + 1;
					//i现在取逗号的位置，循环开始的时候会先做++
					i = nRight;
					break;
				}
				else mapszError[strValue] = "数组中的json后有多余字符";
			}
		}
	}
}

CszValue::~CszValue(){

}

int CszValue::size(){
	return vecszValue.size();
}