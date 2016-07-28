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
			//由于在数组中只会出现json和数字，出现json时，在下面的循环中遇到0退出，所以这里只要处理遇到逗号的情况
			CString strTemp = strValue.Mid(begin,i - begin);
			int j = -1;
			while(j++ != strTemp.GetLength() - 1){
				if(strTemp[j] >= '0' && strTemp[j] <= '9') break;
			}
			//如果不相等则有数字出现，默认当数字处理，如果相等说明没有数字出现，等于空字符串，空字符串不处理
			if(j != strTemp.GetLength()){
				vecszValue.push_back((CstrValue)strTemp);
			}
			break;
		}
		//如果先找到了逗号说明遇到了一个字段值
		if(strValue[i] == ','){
			vecszValue.push_back((CstrValue)strValue.Mid(begin,i - begin));
			num++;
			//取逗号后的位置作为开始，因为i会在循环开始做++，所以不需要动
			begin = i + 1;
		}
		//如果找到了一个{说明找到了一个json
		if(strValue[i] == '{'){
			CStringManager manager = strValue;
			int nRight = manager.FindOther('{','}',i);
			Cjson json;
			json.LoadJson(strValue.Mid(i,nRight - i + 1));
			vecszValue.push_back(json);
			num++;
			//取逗号后的位置作为开始
			while(1){
				//防止在结束时，因为没找到逗号导致i还是原来左括号的位置，先把i放到右括号的位置，循环开始的时候会做++，寻找下一位
				i = nRight;
				nRight++;
				if(strValue[nRight] == 0) return;
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

CszValue::~CszValue(){

}

int CszValue::size(){
	return vecszValue.size();
}