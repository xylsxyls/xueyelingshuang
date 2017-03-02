#include "SpeakClass.h"
#include <typeinfo>

void SpeakClass::ReleaseClass(){
	stdmutex->lock();
	for (auto itlistRoom = listRoom.begin(); itlistRoom != listRoom.end();){
		string className = (*itlistRoom)->FindName(sonThis);
		//?如果找不到自己的名字说明已经移除则返回空
		if (className == "-1"){
			printf("出错，已经被移除");
			continue;
		}
		//?否则找到并移除，返回移除的指针
		else{
			void* pClass = (*itlistRoom)->FindClass(className);
			if (pClass == NULL){
				printf("出错，已经被移除");
				continue;
			}
			//?已经找到，先去掉引用计数，再房间名单上去掉自己
			Room* room = *itlistRoom;
			auto itTemp = itlistRoom;
			itlistRoom++;
			RemoveRoom(*itTemp);
			room->FindClassAndErase(className);
			continue;
		}
		itlistRoom++;
	}
	stdmutex->unlock();
	//?删除内存，调试时用typeid看过这里的this是子类指针
	delete this;
	return;
}

void SpeakClass::AddRoom(Room* room){
	listRoom.push_back(room);
}

void SpeakClass::RemoveRoom(Room* room){
	for (auto itlistRoom = listRoom.begin(); itlistRoom != listRoom.end(); itlistRoom++){
		if (*itlistRoom == room){
			listRoom.erase(itlistRoom);
			break;
		}
	}
}

Room::~Room(){
	if (vecClass.size() != 0) printf("房间内有成员未退出，出现内存泄漏");
}

void Room::AddClass(void* pClass,const string& className){
	vecClass.push_back(Class(pClass, className));
}

string Room::FindName(void* pClass)const{
	for (auto itvecClass = vecClass.begin(); itvecClass != vecClass.end(); itvecClass++){
		if (itvecClass->pClass == pClass){
			return itvecClass->name;
		}
	}
	return "-1";
}

void* Room::FindClass(const string& name)const{
	for (auto itvecClass = vecClass.begin(); itvecClass != vecClass.end(); itvecClass++){
		if (itvecClass->name == name){
			return itvecClass->pClass;
		}
	}
	return NULL;
}

void* Room::FindClassAndErase(const string& name){
	for (auto itvecClass = vecClass.begin(); itvecClass != vecClass.end(); itvecClass++){
		if (itvecClass->name == name){
			void* result = itvecClass->pClass;
			vecClass.erase(itvecClass);
			return result;
		}
	}
	return NULL;
}

int main(){ return 0; }