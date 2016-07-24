#pragma once
#include <afxwin.h>
#include <typeinfo>

class CTypeValue{
public:
	CString type;
	void *pClass;

public:
	CTypeValue();
	template<typename T>
	void fill(T x){
		type = typeid(x).name();
		T *px = new T;
		*px = x;
		pClass = px;
	}
	void free();
	~CTypeValue();
};