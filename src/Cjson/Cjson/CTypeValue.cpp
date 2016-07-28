#include <SDKDDKVer.h>
#include "CTypeValue.h"
#include <typeinfo>

#include "CFieldType.h"

vector<void *> vecp;
void freevec(void* freep){
	auto it = vecp.begin();
	for(;it != vecp.end();it++){
		if(*it == freep){
			vecp.erase(it);
			break;
		}
	}
}

CTypeValue::CTypeValue(){
	type = "";
	pClass = NULL;
	pJson = NULL;
	this->strValueError.dValue = -1;
	this->strValueError.nValue = -1;
	this->strValueError.strValue = "-1";
	this->strValueError.type = -1;
}

CTypeValue::CTypeValue(const CstrValue& Class){
	type = typeid(Class).name();
	CstrValue *pNewClass = new CstrValue;
	//vecp.push_back(pNewClass);
	*pNewClass = Class;
	pClass = pNewClass;

	this->strValueError.dValue = -1;
	this->strValueError.nValue = -1;
	this->strValueError.strValue = "-1";
	this->strValueError.type = -1;
}

CTypeValue::CTypeValue(const CszValue& Class){
	type = typeid(Class).name();
	CszValue *pNewClass = new CszValue;
	//vecp.push_back(pNewClass);
	*pNewClass = Class;
	pClass = pNewClass;

	this->strValueError.dValue = -1;
	this->strValueError.nValue = -1;
	this->strValueError.strValue = "-1";
	this->strValueError.type = -1;
}
CTypeValue::CTypeValue(const Cjson& Class){
	type = typeid(Class).name();
	Cjson *pNewClass = new Cjson;
	//vecp.push_back(pNewClass);
	*pNewClass = Class;
	pClass = pNewClass;

	this->strValueError.dValue = -1;
	this->strValueError.nValue = -1;
	this->strValueError.strValue = "-1";
	this->strValueError.type = -1;

	//pJson采取手动传的方式，使用浅拷贝
	pJson = pNewClass;
}

CTypeValue::CTypeValue(const CTypeValue& x){
	type = x.type;

	this->strValueError.dValue = x.strValueError.dValue;
	this->strValueError.nValue = x.strValueError.nValue;
	this->strValueError.strValue = x.strValueError.strValue;
	this->strValueError.type = x.strValueError.type;

	if(type == ""){
		pClass = x.pClass;
	}
	else if(type == "class CstrValue"){
		//pClass必须新new一个
		CstrValue *pNewClass = new CstrValue;
		//vecp.push_back(pNewClass);
		*pNewClass = *((CstrValue *)(x.pClass));
		pClass = pNewClass;
	}
	else if(type == "class CszValue"){
		//pClass必须新new一个
		CszValue *pNewClass = new CszValue();
		//vecp.push_back(pNewClass);
		*pNewClass = *((CszValue *)(x.pClass));
		pClass = pNewClass;
	}
	else if(type == "class Cjson"){
		//pClass必须新new一个
		Cjson *pNewClass = new Cjson();
		//vecp.push_back(pNewClass);
		*pNewClass = *((Cjson *)(x.pClass));
		pClass = pNewClass;
	}
	else{
		AfxMessageBox("拷贝构造传入类型出错，类型为：" + type);
	}
}

CTypeValue& CTypeValue::operator = (const CTypeValue& x){
	type = x.type;

	this->strValueError.dValue = x.strValueError.dValue;
	this->strValueError.nValue = x.strValueError.nValue;
	this->strValueError.strValue = x.strValueError.strValue;
	this->strValueError.type = x.strValueError.type;

	if(type == ""){
		pClass = x.pClass;
	}
	else if(type == "class CstrValue"){
		//pClass必须新new一个
		CstrValue *pNewClass = new CstrValue;
		//vecp.push_back(pNewClass);
		*pNewClass = *((CstrValue *)(x.pClass));
		pClass = pNewClass;
	}
	else if(type == "class CszValue"){
		//pClass必须新new一个
		CszValue *pNewClass = new CszValue();
		//vecp.push_back(pNewClass);
		*pNewClass = *((CszValue *)(x.pClass));
		pClass = pNewClass;
	}
	else if(type == "class Cjson"){
		//pClass必须新new一个
		Cjson *pNewClass = new Cjson();
		//vecp.push_back(pNewClass);
		*pNewClass = *((Cjson *)(x.pClass));
		pClass = pNewClass;
	}
	else{
		AfxMessageBox("拷贝重载等号传入类型出错，类型为：" + type);
	}
	return *this;
}

CTypeValue& CTypeValue::operator[] (CString field){
	//不论当前是什么类型都先把字段存进去，因为存的时候之前的类型是没有意义的
	pJson->vecField.push_back(CFieldType(field,-1));
	//把本层的地址传入pvec并带下去，方便后面取之前的地址和清空vector
	pJson->vecTypeValue.push_back(this);

	if(type == "class CstrValue"){
		
	}
	else if(type == "class CszValue"){
		
	}
	else if(type == "class Cjson"){
		//如果有这个字段再返回
		auto it = ((Cjson*)pClass)->mapdata.begin();
		for(;it != ((Cjson*)pClass)->mapdata.end();it++){
			if(it->first == field){
				//返回之前先把pJson的值带下去，因为如果出现不存在的情况，return *this能保证pJson肯定有值
				((Cjson*)pClass)->mapdata[field].pJson = this->pJson;
				return ((Cjson*)pClass)->mapdata[field];
			}
		}
	}
	else{
		AfxMessageBox("重载[field]时本身类型出错，类型为：" + type);
	}
	return *this;
}

CTypeValue& CTypeValue::operator[] (int num){
	//不论是什么类型先把数字存进去，因为在存的时候最初定义的类型都是Cjson
	pJson->vecField.push_back(CFieldType("",num));
	//把本层的地址传入pvec并带下去，方便后面取之前的地址和清空vector
	pJson->vecTypeValue.push_back(this);

	if(type == "class CstrValue"){
		
	}
	else if(type == "class CszValue"){
		//如果数组中有这么长再返回
		if(((CszValue*)pClass)->vecszValue.size() > (unsigned int)num){
			((CszValue*)pClass)->vecszValue.at(num).pJson = this->pJson;
			return ((CszValue*)pClass)->vecszValue.at(num);
		}
	}
	else if(type == "class Cjson"){
		
	}
	else{
		AfxMessageBox("重载[num]时本身类型出错，类型为：" + type);
	}
	return *this;
}

void CTypeValue::TypeEqual(CString strTemp){
	//把传进来的field循环查看，如果有则修改，无则创建
	int level = -1;
	CTypeValue *pTypeValueTemp = pJson->vecTypeValue.at(0);
	int i = -1;
	while(i++ != pJson->vecField.size() - 1){
		//表示当前层数
		level++;
		//如果field是空并且数字小于0说明用户填错
		if(pJson->vecField.at(i).field == "" && pJson->vecField.at(i).num < 0){
			//跳到结尾
			goto rem;
		}
		//如果field不是空说明此处是字段
		if(pJson->vecField.at(i).field != ""){
			//如果当前层是最后一层则说明现在应该创建或覆盖字段
			if(i == pJson->vecField.size() - 1){
				//退回上一层添加或覆盖字段
				pTypeValueTemp->toJson().mapdata[pJson->vecField.at(i).field] = (CstrValue)strTemp;
				break;
			}
			//如果当前层根本不是json说明用户改变了当前层的性质，且不会是最后一层，因为如果是则在上面跳出循环了
			if(pTypeValueTemp->type != "class Cjson"){
				//如果下一层是field说明需要创建一个json，以便下面使用
				if(pJson->vecField.at(i + 1).field != ""){
					//重新给上一层赋值新的空数组，以便下面使用
					pJson->vecTypeValue.at(level - 1)->toJson().mapdata[pJson->vecField.at(i - 1).field] = Cjson();
				}
				//如果下一层是数字说明需要创建一个数组，以便下面使用
				else{
					pJson->vecTypeValue.at(level - 1)->toJson().mapdata[pJson->vecField.at(i - 1).field] = CszValue();
				}
			}
			//如果当前层能找到
			auto it = pTypeValueTemp->toJson().mapdata.begin();
			for(;it != pTypeValueTemp->toJson().mapdata.end();it++){
				//如果能找到，直接跳出，因为只是取下一层指针所以此处不需要操作
				if(it->first == pJson->vecField.at(i).field) break;
			}
			if(it == pTypeValueTemp->toJson().mapdata.end()){
				//如果下一层是field说明需要创建一个json
				if(pJson->vecField.at(i + 1).field != ""){
					pTypeValueTemp->toJson().mapdata[pJson->vecField.at(i).field] = Cjson();
				}
				//如果下一层是数字说明需要创建一个数组
				else{
					pTypeValueTemp->toJson().mapdata[pJson->vecField.at(i).field] = CszValue();
				}
			}
			//把找到的下一层CTypeValue传给指针
			pTypeValueTemp = &(pTypeValueTemp->toJson().mapdata[pJson->vecField.at(i).field]);
		}
		//否则是数字
		else{
			//如果当前层数组根本不存在则先创建数组
			if(pTypeValueTemp->type != "class CszValue"){
				//map在改数据的时候会先执行这个类的析构，所以不用管节点内存泄漏
				//重新给上一层赋值新的空数组，以便下面使用
				pJson->vecTypeValue.at(level - 1)->toJson().mapdata[pJson->vecField.at(i - 1).field] = CszValue();
			}
			//如果vec中的数字比数组实际大小大则追加在末尾
			if(pJson->vecField.at(i).num >= (int)pTypeValueTemp->tosz().vecszValue.size()){
				//如果本层是最后一层则追加一个字段
				if(i == pJson->vecField.size() - 1){
					pTypeValueTemp->tosz().vecszValue.push_back(CstrValue(strTemp));
					break;
				}
				//如果下一层是field则追加一个json
				else if(pJson->vecField.at(i + 1).field != ""){
					pTypeValueTemp->tosz().vecszValue.push_back(Cjson());
				}
				//如果下一层是数字则追加一个数组
				else{
					pTypeValueTemp->tosz().vecszValue.push_back(CszValue());
				}
				//写size - 1的原因是当用户输入的数字过大时也采用最后一个
				pTypeValueTemp = &(pTypeValueTemp->tosz().vecszValue.at(pTypeValueTemp->tosz().vecszValue.size() - 1));
			}
			//如果有则覆盖
			else{
				//如果本层是最后一层则覆盖字段
				if(i == pJson->vecField.size() - 1){
					pTypeValueTemp->tosz().vecszValue.at(pJson->vecField.at(i).num) = (CstrValue(strTemp));
					break;
				}
				//如果下一层是field则覆盖一个空json
				else if(pJson->vecField.at(i + 1).field != ""){
					pTypeValueTemp->tosz().vecszValue.at(pJson->vecField.at(i).num) = Cjson();
				}
				//如果下一层是数字则覆盖一个空数组
				else{
					pTypeValueTemp->tosz().vecszValue.at(pJson->vecField.at(i).num) = CszValue();
				}
				//给下一层指针
				pTypeValueTemp = &(pTypeValueTemp->tosz().vecszValue.at(pJson->vecField.at(i).num));
			}
		}
	}

	rem:
	//使用过一次就清空
	pJson->vecField.clear();
	pJson->vecTypeValue.clear();
	return;
}

CTypeValue& CTypeValue::operator = (int nNum){
	CString strTemp = "";
	strTemp.Format("%d",nNum);

	TypeEqual(strTemp);
	
	return *this;
}

CTypeValue& CTypeValue::operator = (double dNum){
	CString strTemp = "";
	strTemp.Format("%lf",dNum);

	TypeEqual(strTemp);

	return *this;
}

CTypeValue& CTypeValue::operator = (CString str){
	CString strTemp = "";
	strTemp.Format("\"%s\"",str);

	TypeEqual(strTemp);
	
	return *this;
}

CstrValue& CTypeValue::toValue(){
	if(type == "class CstrValue"){
		return *((CstrValue *)pClass);
	}
	//如果要访问的类型不是本身的类型则直接返回错误
	return strValueError;
}

CszValue& CTypeValue::tosz(){
	if(type == "class CszValue"){
		return *((CszValue *)pClass);
	}
	//如果要访问的类型不是本身的类型则直接返回空
	return szValueError;
}

Cjson& CTypeValue::toJson(){
	if(type == "class Cjson"){
		return *((Cjson *)pClass);
	}
	//如果要访问的类型不是本身的类型则直接返回空
	return jsonError;
}

CTypeValue::~CTypeValue(){
	if(type == ""){
		if(pClass != NULL){
			AfxMessageBox("出错");
		}
	}
	else if(type == "class CstrValue"){
		//freevec(pClass);
		delete (CstrValue *)pClass;
	}
	else if(type == "class CszValue"){
		//freevec(pClass);
		delete (CszValue *)pClass;
	}
	else if(type == "class Cjson"){
		//freevec(pClass);
		delete (Cjson *)pClass;
	}
	else{
		AfxMessageBox("释放类型出错，类型为：" + type);
	}
}