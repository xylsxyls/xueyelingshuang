#include <SDKDDKVer.h>
#include "CszValue.h"
#include "CStringManagerAPI.h"

CszValue::CszValue(){

}

CszValue::CszValue(CString strValue){
	int num = 0;
	int begin = 0;
	int i = -1;
	while(1){
		i++;
		//如果到了末尾就退出
		if(strValue[i] == 0) break;
		//如果先找到了逗号说明遇到了一个字段值
		if(strValue[i] == ','){
			CstrValue strValueTemp = strValue.Mid(begin,i - begin);
			CszOneValue szOneValueTemp(num,strValueTemp);
			vecszOneValue.push_back(szOneValueTemp);
			//
			//CstrValue strValueTemp = new CstrValue(strValue.Mid(begin,i - begin));
			CTypeValue strTypeValue;
			strTypeValue.fill(strValueTemp);
			vecszValue.push_back(strTypeValue);
			strTypeValue.free();
			num++;
			//取逗号后的位置作为开始，因为i会在循环开始做++，所以不需要动
			begin = i + 1;
		}
		//如果找到了一个{说明找到了一个json
		if(strValue[i] == '{'){
			CStringManager manager = strValue;
			int nRight = manager.FindOther('{','}',i);
			CString strJsonTemp = strValue.Mid(i,nRight - i + 1);
			Cjson json;
			json.LoadJson(strJsonTemp);
			vecjson.push_back(json);
			//
			CTypeValue jsonTypeValue;
			jsonTypeValue.fill(json);
			vecszValue.push_back(jsonTypeValue);
			jsonTypeValue.free();
			num++;
			//取逗号后的位置作为开始
			while(1){
				//防止在结束时，因为没找到逗号导致i还是原来左括号的位置，先把i放到右括号的位置，循环开始的时候会做++，寻找下一位
				i = nRight;
				nRight++;
				if(strValue[nRight] == 0) break;
				if(strValue[nRight] == ','){
					begin = nRight + 1;
					//i现在取逗号的位置，循环开始的时候会先做++
					i = nRight;
					break;
				}
			}
		}
	}
}