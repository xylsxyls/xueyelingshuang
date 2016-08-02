#include <SDKDDKVer.h>
#include "Cjson.h"
#include <typeinfo>

#include "CFieldType.h"

map<CString,CString> Cjson::LoadJson(CString strJson){
	if(type == "class CjsonA") return pJsonA->LoadJson(strJson);
	else return map<CString,CString>();
}

vector<CString> Cjson::GetField(){
	if(type == "class CjsonA") return pJsonA->GetField();
	else return vector<CString>();
}

int Cjson::size(){
	//先获得最原始的Cjson地址
	Cjson *TypeValueTemp = pJson->veCjson.at(0);
	int i = -1;
	while(i++ != pJson->vecField.size() - 1){
		//如果用户输入错误则直接返回-1
		if(pJson->vecField.at(i).field == "" && pJson->vecField.at(i).num == -1) return -1;
		//如果是字段
		if(pJson->vecField.at(i).field != "" && TypeValueTemp->type == "class CjsonA"){
			//如果有这个字段再返回
			auto it = ((CjsonA*)TypeValueTemp->pClass)->mapdata.begin();
			for(;it != ((CjsonA*)TypeValueTemp->pClass)->mapdata.end();it++){
				if(it->first == pJson->vecField.at(i).field){
					//pJson值在[]重载时已经带下去了
					TypeValueTemp =  &((CjsonA*)TypeValueTemp->pClass)->mapdata[pJson->vecField.at(i).field];
					goto rem;
				}
			}
			//如果循环自然退出则表示没找到返回-1
			return -1;
			rem:;
		}
		else if(pJson->vecField.at(i).num != -1 && TypeValueTemp->type == "class CszValue"){
			//如果数组中有这么长再返回
			if(((CszValue*)TypeValueTemp->pClass)->vecszValue.size() > (unsigned int)pJson->vecField.at(i).num){
				//pJson已经在[]重载时带下去了
				TypeValueTemp = &((CszValue*)TypeValueTemp->pClass)->vecszValue.at(pJson->vecField.at(i).num);
			}
			else return -1;
		}
		else return -1;
	}
	if(type == "class CjsonA") return TypeValueTemp->toJson().size();
	else if(type == "class CszValue") return TypeValueTemp->tosz().size();
	else return -1;
}

CString Cjson::toCString(CString NewLineSign,CString FormatSign,int FormatLength){
	if(type == "class CjsonA") return pJsonA->toCString(NewLineSign,FormatSign,FormatLength);
	else return "";
}

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

Cjson::Cjson(){
	pJsonA = new CjsonA;
	type = typeid(CjsonA).name();
	pClass = pJsonA;
	pJson = pJsonA;
	this->strValueError.dValue = -1;
	this->strValueError.nValue = -1;
	this->strValueError.strValue = "-1";
	this->strValueError.type = -1;

	
}

Cjson::Cjson(const CstrValue& Class){
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

Cjson::Cjson(const CszValue& Class){
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
Cjson::Cjson(const CjsonA& Class){
	type = typeid(Class).name();
	CjsonA *pNewClass = new CjsonA;
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

Cjson::Cjson(const Cjson& x){
	if(type != "") AfxMessageBox("拷贝构造时原type不为空，type=" + type);
	type = x.type;

	this->strValueError.dValue = x.strValueError.dValue;
	this->strValueError.nValue = x.strValueError.nValue;
	this->strValueError.strValue = x.strValueError.strValue;
	this->strValueError.type = x.strValueError.type;

	if(type == ""){
		pClass = x.pClass;
	}
	if(type == "class CstrValue"){
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
	else if(type == "class CjsonA"){
		//pClass必须新new一个
		CjsonA *pNewClass = new CjsonA();
		//vecp.push_back(pNewClass);
		*pNewClass = *((CjsonA *)(x.pClass));
		pClass = pNewClass;
	}
	else{
		AfxMessageBox("拷贝构造传入类型出错，类型为：" + type);
	}
}

Cjson& Cjson::operator = (const Cjson& x){
	if(type == ""){

	}
	else if(type == "class CstrValue"){
		delete (CstrValue *)pClass;
	}
	else if(type == "class CszValue"){
		delete (CszValue *)pClass;
	}
	else if(type == "class CjsonA"){
		delete (CjsonA *)pClass;
	}
	else{
		AfxMessageBox("重载等于是原类型出错，type=" + type);
	}
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
	else if(type == "class CjsonA"){
		//pClass必须新new一个
		CjsonA *pNewClass = new CjsonA();
		//vecp.push_back(pNewClass);
		*pNewClass = *((CjsonA *)(x.pClass));
		pClass = pNewClass;
	}
	else{
		AfxMessageBox("拷贝重载等号传入类型出错，类型为：" + type);
	}
	return *this;
}

Cjson& Cjson::operator[] (CString field){
	//不论当前是什么类型都先把字段存进去，因为存的时候之前的类型是没有意义的
	pJson->vecField.push_back(CFieldType(field,-1));
	//把本层的地址传入pvec并带下去，方便后面取之前的地址和清空vector
	pJson->veCjson.push_back(this);

	if(type == "class CstrValue"){
		
	}
	else if(type == "class CszValue"){
		
	}
	else if(type == "class CjsonA"){
		//如果有这个字段再返回
		auto it = ((CjsonA*)pClass)->mapdata.begin();
		for(;it != ((CjsonA*)pClass)->mapdata.end();it++){
			if(it->first == field){
				//返回之前先把pJson的值带下去，因为如果出现不存在的情况，return *this能保证pJson肯定有值
				((CjsonA*)pClass)->mapdata[field].pJson = this->pJson;
				return ((CjsonA*)pClass)->mapdata[field];
			}
		}
	}
	else{
		AfxMessageBox("重载[field]时本身类型出错，类型为：" + type);
	}
	return *this;
}

Cjson& Cjson::operator[] (int num){
	//不论是什么类型先把数字存进去，因为在存的时候最初定义的类型都是CjsonA
	pJson->vecField.push_back(CFieldType("",num));
	//把本层的地址传入pvec并带下去，方便后面取之前的地址和清空vector
	pJson->veCjson.push_back(this);

	if(type == "class CstrValue"){
		
	}
	else if(type == "class CszValue"){
		//如果数组中有这么长再返回
		if(((CszValue*)pClass)->vecszValue.size() > (unsigned int)num){
			((CszValue*)pClass)->vecszValue.at(num).pJson = this->pJson;
			return ((CszValue*)pClass)->vecszValue.at(num);
		}
	}
	else if(type == "class CjsonA"){
		
	}
	else{
		AfxMessageBox("重载[num]时本身类型出错，类型为：" + type);
	}
	return *this;
}

void Cjson::TypeEqual(CString strTemp){
	//把传进来的field循环查看，如果有则修改，无则创建
	int level = -1;
	Cjson *pTypeValueTemp = pJson->veCjson.at(0);
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
				//如果发现是"\"delete\""则把当前字段删除
				if(strTemp == "\"delete\""){
					auto it = pTypeValueTemp->toJson().mapdata.begin();
					for(;it!=pTypeValueTemp->toJson().mapdata.end();it++){
						if(it->first == pJson->vecField.at(i).field){
							//在释放之后pJson值会丢失，因为释放的是本层Cjson，所以pJson会被重置，所以先取出来
							CjsonA *pJsonTemp = pJson;
							pTypeValueTemp->toJson().mapdata.erase(it);
							//这里使用pJsonTemp来清空
							pJsonTemp->vecField.clear();
							pJsonTemp->veCjson.clear();
							return;
						}
					}
				}
				else{
					//退回上一层添加或覆盖字段
					pTypeValueTemp->toJson().mapdata[pJson->vecField.at(i).field] = (CstrValue)strTemp;
				}
				break;
			}
			//如果当前层根本不是json说明用户改变了当前层的性质，且不会是最后一层，因为如果是则在上面跳出循环了
			if(pTypeValueTemp->type != "class CjsonA"){
				//如果下一层是field说明需要创建一个json，以便下面使用
				if(pJson->vecField.at(i + 1).field != ""){
					//重新给上一层赋值新的空数组，以便下面使用
					pJson->veCjson.at(level - 1)->toJson().mapdata[pJson->vecField.at(i - 1).field] = CjsonA();
				}
				//如果下一层是数字说明需要创建一个数组，以便下面使用
				else{
					pJson->veCjson.at(level - 1)->toJson().mapdata[pJson->vecField.at(i - 1).field] = CszValue();
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
					pTypeValueTemp->toJson().mapdata[pJson->vecField.at(i).field] = CjsonA();
				}
				//如果下一层是数字说明需要创建一个数组
				else{
					pTypeValueTemp->toJson().mapdata[pJson->vecField.at(i).field] = CszValue();
				}
			}
			//把找到的下一层Cjson传给指针
			pTypeValueTemp = &(pTypeValueTemp->toJson().mapdata[pJson->vecField.at(i).field]);
		}
		//否则是数字
		else{
			//如果当前层数组根本不存在则先创建数组
			if(pTypeValueTemp->type != "class CszValue"){
				//map在改数据的时候会先执行这个类的析构，所以不用管节点内存泄漏
				//重新给上一层赋值新的空数组，以便下面使用
				pJson->veCjson.at(level - 1)->toJson().mapdata[pJson->vecField.at(i - 1).field] = CszValue();
			}
			//如果vec中的数字比数组实际大小大则追加在末尾
			if(pJson->vecField.at(i).num >= (int)pTypeValueTemp->tosz().vecszValue.size()){
				//如果本层是最后一层则追加一个字段
				if(i == pJson->vecField.size() - 1){
					//如果是"\"delete\""
					if(strTemp == "\"delete\""){
						pTypeValueTemp->tosz().vecszValue.pop_back();
					}
					//如果是"\"insert\""
					else if(strTemp == "\"insert\""){
						pTypeValueTemp->tosz().vecszValue.push_back(CstrValue(strTemp));
					}
					else{
						pTypeValueTemp->tosz().vecszValue.push_back(CstrValue(strTemp));
					}
					break;
				}
				//如果下一层是field则追加一个json
				else if(pJson->vecField.at(i + 1).field != ""){
					pTypeValueTemp->tosz().vecszValue.push_back(CjsonA());
				}
				//如果下一层是数字说明用户填错，因为数组中是不可以直接添加数组的
				else{
					goto rem;
				}
				//写size - 1的原因是当用户输入的数字过大时也采用最后一个
				pTypeValueTemp = &(pTypeValueTemp->tosz().vecszValue.at(pTypeValueTemp->tosz().vecszValue.size() - 1));
			}
			//如果有则覆盖
			else{
				//如果本层是最后一层则覆盖字段
				if(i == pJson->vecField.size() - 1){
					//如果是"\"delete\""
					if(strTemp == "\"delete\""){
						pTypeValueTemp->tosz().vecszValue.erase(pTypeValueTemp->tosz().vecszValue.begin() + pJson->vecField.at(i).num);
					}
					//如果是"\"insert\""
					else if(strTemp == "\"insert\""){
						pTypeValueTemp->tosz().vecszValue.insert(pTypeValueTemp->tosz().vecszValue.begin() + pJson->vecField.at(i).num,CstrValue(strTemp));
					}
					else{
						pTypeValueTemp->tosz().vecszValue.at(pJson->vecField.at(i).num) = (CstrValue(strTemp));
					}
					break;
				}
				//如果下一层是field则覆盖一个空json
				else if(pJson->vecField.at(i + 1).field != ""){
					pTypeValueTemp->tosz().vecszValue.at(pJson->vecField.at(i).num) = CjsonA();
				}
				//如果下一层是数字说明用户填错，因为数组中是不可以直接添加数组的
				else{
					goto rem;
				}
				//给下一层指针
				pTypeValueTemp = &(pTypeValueTemp->tosz().vecszValue.at(pJson->vecField.at(i).num));
			}
		}
	}

	rem:
	//使用过一次就清空
	pJson->vecField.clear();
	pJson->veCjson.clear();
	return;
}

Cjson& Cjson::operator = (int nNum){
	CString strTemp = "";
	strTemp.Format("%d",nNum);

	TypeEqual(strTemp);
	
	return *this;
}

Cjson& Cjson::operator = (double dNum){
	CString strTemp = "";
	strTemp.Format("%lf",dNum);

	TypeEqual(strTemp);

	return *this;
}

Cjson& Cjson::operator = (CString str){
	CString strTemp = "";
	strTemp.Format("\"%s\"",str);

	TypeEqual(strTemp);
	
	return *this;
}

CstrValue& Cjson::toValue(){
	if(type == "class CstrValue"){
		return *((CstrValue *)pClass);
	}
	//如果要访问的类型不是本身的类型则直接返回错误
	return strValueError;
}

CszValue& Cjson::tosz(){
	if(type == "class CszValue"){
		return *((CszValue *)pClass);
	}
	//如果要访问的类型不是本身的类型则直接返回空
	return szValueError;
}

CjsonA& Cjson::toJson(){
	if(type == "class CjsonA"){
		return *((CjsonA *)pClass);
	}
	//如果要访问的类型不是本身的类型则直接返回空
	return jsonError;
}

Cjson::~Cjson(){
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
	else if(type == "class CjsonA"){
		//freevec(pClass);
		delete (CjsonA *)pClass;
	}
	else{
		AfxMessageBox("释放类型出错，类型为：" + type);
	}
}