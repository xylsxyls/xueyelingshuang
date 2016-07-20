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

	int main()
{
	Object* p = Object::CreateObject("A");

	delete p;
	C* pC = (C*)Object::CreateObject("C");
	cout<<"x = "<<pC->x<<endl;
	cout<<"y = "<<pC->y<<endl;
	system("pause");
	return 0;
}