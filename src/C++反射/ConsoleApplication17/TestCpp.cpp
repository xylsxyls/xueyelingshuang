#include<iostream>
#include<cstring>
#include "DynBase.h"
using namespace std;

class A : public Object
{
	DECLARE_CLASS(A)
public :
	A(){cout<<hex<<(long)this<<" A constructor!"<<endl;}
	~A(){cout<<hex<<(long)this<<" A destructor!"<<endl;}
};
IMPLEMENT_CLASS(A)

class B : public Object
{
	DECLARE_CLASS(B)
public :
	B(){cout<<hex<<(long)this<<" B constructor!"<<endl;}
	~B(){cout<<hex<<(long)this<<" B destructor!"<<endl;}
};
IMPLEMENT_CLASS(B)

class C : public Object
{
	DECLARE_CLASS(C)
public:
	int x;
	int y;
public :
	C(){
		cout<<hex<<(long)this<<" C constructor!"<<endl;
		x = 1;
		y = 2;
	}
	~C(){cout<<hex<<(long)this<<" C destructor!"<<endl;}
};
IMPLEMENT_CLASS(C)

#define COCreateMacro(COTYPE,CTYPE,object,...) \
class COTYPE : public Object\
{\
	DECLARE_CLASS(COTYPE)\
public:\
	CTYPE *object;\
	COTYPE(){\
		object = new CTYPE(##__VA_ARGS__);\
	}\
	~COTYPE(){\
		delete object;\
	}\
};\
IMPLEMENT_CLASS(COTYPE)

#define CONew(COTYPE,strTYPE,object) (*((COTYPE*)(Object::CreateObject(strTYPE)))->object);
//COCreateMacro(COjson,Cjson,json);

int main(){

	system("pause");
	return 0;
}