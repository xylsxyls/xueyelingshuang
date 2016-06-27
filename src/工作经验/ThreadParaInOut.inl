#pragma once
#include "stdafx.h"
#include <vector>
using namespace std;

template <typename T1,typename T2 = void*,typename T3 = void*,typename T4 = void*,typename T5 = void*>
class ThreadParaInOut{
public:
	vector<T1> Vect1;
	vector<T2> Vect2;
	vector<T3> Vect3;
	vector<T4> Vect4;
	vector<T5> Vect5;

public:
#define fun(add,Vect,T) \
	void add(int number,...){\
		va_list parameterlist;\
		va_start(parameterlist,number);\
		while(number-- != 0) Vect.push_back(va_arg(parameterlist,T));\
		va_end(parameterlist);\
	}
	fun(add1,Vect1,T1)
	fun(add2,Vect2,T2)
	fun(add3,Vect3,T3)
	fun(add4,Vect4,T4)
	fun(add5,Vect5,T5)
	
	ThreadParaInOut(T1 t1){
		Vect1.push_back(t1);
	}
	ThreadParaInOut(T1 t1,T2 t2){
		Vect1.push_back(t1);
		Vect2.push_back(t2);
	}
	ThreadParaInOut(T1 t1,T2 t2,T3 t3){
		Vect1.push_back(t1);
		Vect2.push_back(t2);
		Vect3.push_back(t3);
	}
	ThreadParaInOut(T1 t1,T2 t2,T3 t3,T4 t4){
		Vect1.push_back(t1);
		Vect2.push_back(t2);
		Vect3.push_back(t3);
		Vect4.push_back(t4);
	}
	ThreadParaInOut(T1 t1,T2 t2,T3 t3,T4 t4,T5 t5){
		Vect1.push_back(t1);
		Vect2.push_back(t2);
		Vect3.push_back(t3);
		Vect4.push_back(t4);
		Vect5.push_back(t5);
	}
	~ThreadParaInOut(){
		Vect1.clear();
		Vect2.clear();
		Vect3.clear();
		Vect4.clear();
		Vect5.clear();
	}
};

