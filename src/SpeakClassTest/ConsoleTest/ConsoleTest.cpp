#include "ConsoleTest.h"
#include "A.h"
#include "B.h"
#include "C.h"
#include "SpeakClass.h"
#include "TestSpeakClass.h"
using namespace std;

int main(){
	TestSpeakClass cls;
	cls.run();
	/*
	A* a = NULL;
	B* b = NULL;
	C* c = NULL;
	Room room;
	std::mutex stdmutex;
	while (1){
		a = SpeakClass::CreateClass<A>(&room, &stdmutex, "A");
		b = SpeakClass::CreateClass<B>(&room, &stdmutex, "B");
		c = SpeakClass::CreateClass<C>(&room, &stdmutex, "C");
		a->RemoveClass<A>(&room, "A");
		b->RemoveClass<B>(&room, "B");
		c->RemoveClass<C>(&room, "C");
		a->AddSpeakClass<A>(&room, "A");
		b->AddSpeakClass<B>(&room, "B");
		c->AddSpeakClass<C>(&room, "C");
		a->ReleaseClass();
		b->ReleaseClass();
		c->ReleaseClass();
		a = 0;
		b = 0;
		c = 0;
	}*/

	/*
	A* a = NULL;
	B* b = NULL;
	C* c = NULL;
	Room room;
	std::mutex stdmutex;
	a = SpeakClass::CreateClass<A>(&room, &stdmutex);
	b = SpeakClass::CreateClass<B>(&room, &stdmutex);
	c = SpeakClass::CreateClass<C>(&room, &stdmutex);
	printf("A send message\n");
	a->SendToOtherClass<B>(&room, 1, "MESSAGEA");
	a->SendToOtherClass<C>(&room, 11, "MESSAGEA");
	printf("B send message\n");
	b->SendToOtherClass<A>(&room, 2, "MESSAGEB");
	b->SendToOtherClass<C>(&room, 22, "MESSAGEB");
	printf("C send message\n");
	c->SendToOtherClass<A>(&room, 3, "MESSAGEC");
	c->SendToOtherClass<B>(&room, 33, "MESSAGEC");
	c->SendToOtherClass<C>(&room, 333, "MESSAGEC");

	b->ReleaseClass();

	printf("A send message\n");
	a->SendToOtherClass<B>(&room, 1, "MESSAGEA");
	a->SendToOtherClass<C>(&room, 11, "MESSAGEA");

	printf("C send message\n");
	c->SendToOtherClass<A>(&room, 3, "MESSAGEC");
	c->SendToOtherClass<B>(&room, 33, "MESSAGEC");
	c->SendToOtherClass<C>(&room, 333, "MESSAGEC");

	printf("B send message\n");
	//?为什么在释放之后依然可以进到函数内
	b->SendToOtherClass<A>(&room, 2, "MESSAGEB");
	b->SendToOtherClass<C>(&room, 22, "MESSAGEB");
	printf("end\n");
	*/
	getchar();
	return 0;
}