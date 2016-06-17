#pragma once
#include <vector>
using namespace std;

template<typename T>void FreeVec(vector<T> *pVec){
	unsigned int i = 0;
	//将向量中的每个指针释放，长度不为0就删除第一个元素
	while(pVec->size() != 0){
		//在删除对应类的时候，在类的析构函数里有对应释放该元素的函数，所有这里不需要再次释放了
		delete pVec->at(0);
		//但是如果只有这句，应该发生内存泄漏，但是在这里没有，可能是因为mysql底层存在内存管理
		//VecErase(pVec,pVec->at(0));
	}
	return;
}

template<typename T>void VecErase(vector<T> *pVec,T p){
	vector<T>::iterator it = pVec->begin();
	for(it = pVec->begin();it <= pVec->end();it++){
		if(*it == p){
			pVec->erase(it);
			break;
		}
	}
}