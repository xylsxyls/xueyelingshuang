#ifndef _HEADER_H_
#define _HEADER_H_

#include<iostream>
#include<map>
#include<string>

using std::cout;
using std::endl;
typedef void* (*CreateFuntion)(void);

//保存类和对应创建函数之类的映射

class ClassFactory{
public:
	static void* GetClassByName(std::string name){
		std::map<std::string,CreateFuntion>::const_iterator find ;
		find = clsMap().find(name);
		if(find==clsMap().end()){
			return NULL;
		}
		else{
			return find->second();
		}
	}
	static void RegistClass(std::string name,CreateFuntion method){
		//for debug
		cout << "Regist Class "<< name << endl ;
		clsMap().insert(std::make_pair(name,method));
	}
private:
	//static std::map<std::string,CreateFuntion> m_clsMap;
	static std::map<std::string, CreateFuntion>& clsMap(); 
};

std::map<std::string, CreateFuntion>& ClassFactory::clsMap()
{
	static std::map<std::string, CreateFuntion> *s_clsMap = new std::map<std::string, CreateFuntion>();  //只进行一次初始化
	return *s_clsMap;
}

class RegistyClass
{
public:
	RegistyClass(std::string name, CreateFuntion method){
		//for debug
		cout << "RegistyClass Construct\n"  ;
		ClassFactory::RegistClass(name, method);
	}
};

template<class T, const char name[]>
class Register
{
public:
	Register(){
		const RegistyClass tmp=rc;
	}
	static void* CreateInstance(){
		return new T;
	}
public:
	static const RegistyClass rc;
};

#define DEFINE_CLASS(class_name) \
	char NameArray##class_name[]=#class_name;\
	class class_name:public Register<class_name,NameArray##class_name>

#define DEFINE_CLASS_EX(class_name,father_class) \
	char NameArray##class_name[]=#class_name;\
	class class_name:public Register<class_name,NameArray##class_name>,public father_class

#endif
