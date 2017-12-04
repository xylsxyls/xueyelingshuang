#include "ConsoleTest.h"

class QBase
{
public:
	int GetQBasebase(){ return base; }
	int base;
};

class ControlBase : public QBase
{
public:
	ControlBase(){}
	
	void setBase(){}
protected:
	
};

class ControlFont : public ControlBase
{
public:
	ControlFont(){}
	void setFont(){}
};

class ControlSelf : public ControlBase
{
public:
	ControlSelf(){}
	void setSelf(){}
};

class ComboBox : public ControlFont, ControlSelf
{
public: 
	ComboBox(){ }


};


int main(){
	ComboBox* c = new ComboBox;
	QBase* sssss = nullptr;
	//S* s = new S;
	//s->GetQBasebase();
	ComboBox d;
	ComboBox *_t = static_cast<ComboBox *>(sssss);
	//c->getQBase()->GetQBasebase();
	return 0;
}