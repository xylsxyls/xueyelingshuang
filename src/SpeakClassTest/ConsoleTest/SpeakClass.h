#pragma once
#include <vector>
#include <string>
#include <mutex>
#include <list>
using namespace std;

class Class{
public:
	Class(void* pClass, const string& type){
		this->pClass = pClass;
		this->name = type;
	}
	void* pClass;
	string name;
};

class Room{
public:
	~Room();
public:
	vector<Class> vecClass;

	//?不能出现空地址或空名现象
	void AddClass(void* pClass, const string& className);
	string FindName(void* pClass)const;
	void* FindClass(const string& name)const;
	void* FindClassAndErase(const string& name);
};

class SpeakClass{
public:
	std::mutex* stdmutex;
	list<Room*> listRoom;
public:
	template<typename T>
	static T* CreateClass(Room* room, std::mutex* stdmutex, string className = ""){
		T* pClass = new T;
		pClass->stdmutex = stdmutex;
		if (className == "") className = typeid(T).name();
		bool ifAddSuccess = pClass->AddSpeakClass<T>(room, className, pClass);
		if (ifAddSuccess == true) return pClass;
		else{
			delete pClass;
			return false;
		}
	}

	template<typename T>
	void SendToOtherClass(Room* room, const int& dataType, void* data, string otherClassName = ""){
		//?先判断该指针是否存在
		stdmutex->lock();
		string name = room->FindName(this);
		if (name == "-1") goto RETURN;
		//?判断名字是否为空
		if (otherClassName == "") otherClassName = typeid(T).name();
		void* pClass = room->FindClass(otherClassName);
		if (pClass != NULL) ((T*)pClass)->ReceiveFromOtherClass(room, dataType, data, room->FindName(this));
		RETURN:
		stdmutex->unlock();
	}
	virtual void ReceiveFromOtherClass(Room* room, const int& dataType, void* data, const string& otherClassName) = 0;
	//?如果不给名字则名字为typeid计算，如果不给地址则地址为该子类地址
	template<typename T>
	bool AddSpeakClass(Room* room, string className = "", void* pClass = NULL){
		//?有地址没名字不允许
		if (pClass != NULL && className == "") goto FALSE;
		//?没地址没名字
		else if (pClass == NULL && className == ""){
			pClass = this;
			className = typeid(*this).name();
		}
		//?没地址有名字则说明地址为该子类地址
		else if (pClass == NULL && className != ""){
			pClass = this;
		}
		//?先判断是否重名
		stdmutex->lock();
		void* pClassTemp = room->FindClass(className);
		//?如果有重名
		if (pClassTemp != NULL) goto FALSE;
		//?到这里说明都正确可以添加了
		//?先增加该类的引用计数，再在房间中添加名字
		((T*)pClass)->AddRoom(room);
		room->AddClass(pClass, className);
		goto TRUE;
		TRUE:
		stdmutex->unlock();
		return true;
		FALSE:
		stdmutex->unlock();
		return false;
	}
	//?移除失败则返回空，说明当前状态下已经没有该类了
	template<typename T>
	void* RemoveClass(Room* room, string className = ""){
		//?如果没填说明要移除自己，先根据指针找到自己的名字
		void* voidResult = NULL;
		stdmutex->lock();
		if (className == "") className = room->FindName(this);
		//?如果找不到自己的名字说明已经移除则返回空
		if (className == "-1") goto RETURN;
		//?否则找到并移除，返回移除的指针
		else{
			void* pClass = room->FindClass(className);
			if (pClass == NULL) goto RETURN;
			//?已经找到，先去掉引用计数，再房间名单上去掉自己
			((T*)pClass)->RemoveRoom(room);
			voidResult = room->FindClassAndErase(className);
			goto RETURN;
		}
		RETURN:
		stdmutex->unlock();
		return voidResult;
	}

	//?默认删除自己，在删除之后仍然可以使用该指针调函数？？？？
	void ReleaseClass();

	void AddRoom(Room* room);
	void RemoveRoom(Room* room);
};