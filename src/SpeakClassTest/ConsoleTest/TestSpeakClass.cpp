#include "TestSpeakClass.h"
#include "CStringManager/CStringManagerAPI.h"

TestSpeakClass::TestSpeakClass(){
	x = 1;
	room = new Room;
	mutex = new std::mutex;
	a = SpeakClass::CreateClass<A>(room, mutex, "A");
	b = SpeakClass::CreateClass<B>(room, mutex, "B");
	c = SpeakClass::CreateClass<C>(room, mutex, "C");
}

TestSpeakClass::~TestSpeakClass(){
	x = 0;
	thread1->join();
	a->ReleaseClass();
	b->ReleaseClass();
	c->ReleaseClass();
	delete mutex;
	delete room;
}

void TestSpeakClass::run(){
	srand((unsigned int)time(NULL));
	int n = 1;
	while (n-- != 0){
		this->thread1 = new std::thread(std::bind(&TestSpeakClass::Thread1, this));
	}
	
	//this->thread2 = new std::thread(std::bind(&TestSpeakClass::Thread1, this));
	//this->thread3 = new std::thread(std::bind(&TestSpeakClass::Thread1, this));
	//this->thread4 = new std::thread(std::bind(&TestSpeakClass::Thread1, this));
	//this->thread5 = new std::thread(std::bind(&TestSpeakClass::Thread1, this));
}

void TestSpeakClass::Thread1(){
	
	int x = 3;
	
	while (this->x){
		int randNumber = rand() % 15 + 1;

		switch (randNumber)
		{
		case 1:{
			a->SendToOtherClass<A>(room, 11, "MESSAGE_A", "A");
			break;
		}
		case 2:{
			a->SendToOtherClass<B>(room, 12, "MESSAGE_A", "B");
			break;
		}
		case 3:{
			a->SendToOtherClass<C>(room, 13, "MESSAGE_A", "C");
			break;
		}
		case 4:{
			b->SendToOtherClass<A>(room, 21, "MESSAGE_B", "A");
			break;
		}
		case 5:{
			b->SendToOtherClass<B>(room, 22, "MESSAGE_B", "B");
			break;
		}
		case 6:{
			b->SendToOtherClass<C>(room, 23, "MESSAGE_B", "C");
			break;
		}
		case 7:{
			c->SendToOtherClass<A>(room, 31, "MESSAGE_C", "A");
			break;
		}
		case 8:{
			c->SendToOtherClass<B>(room, 32, "MESSAGE_C", "B");
			break;
		}
		case 9:{
			c->SendToOtherClass<C>(room, 33, "MESSAGE_C", "C");
			break;
		}
		case 10:{
			a->AddSpeakClass<A>(room, "A");
			break;
		}
		case 11:{
			b->AddSpeakClass<B>(room, "B");
			break;
		}
		case 12:{
			c->AddSpeakClass<C>(room, "C");
			break;
		}
		case 13:{
			a->RemoveClass<A>(room, "A");
			break;
		}
		case 14:{
			b->RemoveClass<C>(room, "C");
			break;
		}
		case 15:{
			c->RemoveClass<B>(room, "B");
			break;
		}
		default:
			break;
		}
	}
}

void TestSpeakClass::Thread2(){
	while (x){
		int randNumber = rand() % 9 + 7;

		switch (randNumber)
		{
		case 1:{
			a->SendToOtherClass<A>(room, 11, "MESSAGE_A", "A");
			break;
		}
		case 2:{
			a->SendToOtherClass<B>(room, 12, "MESSAGE_A", "B");
			break;
		}
		case 3:{
			a->SendToOtherClass<C>(room, 13, "MESSAGE_A", "C");
			break;
		}
		case 4:{
			b->SendToOtherClass<A>(room, 21, "MESSAGE_B", "A");
			break;
		}
		case 5:{
			b->SendToOtherClass<B>(room, 22, "MESSAGE_B", "B");
			break;
		}
		case 6:{
			b->SendToOtherClass<C>(room, 23, "MESSAGE_B", "C");
			break;
		}
		case 7:{
			c->SendToOtherClass<A>(room, 31, "MESSAGE_C", "A");
			break;
		}
		case 8:{
			c->SendToOtherClass<B>(room, 32, "MESSAGE_C", "B");
			break;
		}
		case 9:{
			c->SendToOtherClass<C>(room, 33, "MESSAGE_C", "C");
			break;
		}
		case 10:{
			a->AddSpeakClass<A>(room, "A");
			break;
		}
		case 11:{
			b->AddSpeakClass<B>(room, "B");
			break;
		}
		case 12:{
			c->AddSpeakClass<C>(room, "C");
			break;
		}
		case 13:{
			a->RemoveClass<A>(room, "A");
			break;
		}
		case 14:{
			b->RemoveClass<C>(room, "C");
			break;
		}
		case 15:{
			c->RemoveClass<B>(room, "B");
			break;
		}
		default:
			break;
		}
	}
}

void TestSpeakClass::Thread3(){
	while (1){
		int randNumber = rand() % 9 + 7;

		switch (randNumber)
		{
		case 1:{
			a->SendToOtherClass<A>(room, 11, "MESSAGE_A", "A");
			break;
		}
		case 2:{
			a->SendToOtherClass<B>(room, 12, "MESSAGE_A", "B");
			break;
		}
		case 3:{
			a->SendToOtherClass<C>(room, 13, "MESSAGE_A", "C");
			break;
		}
		case 4:{
			b->SendToOtherClass<A>(room, 21, "MESSAGE_B", "A");
			break;
		}
		case 5:{
			b->SendToOtherClass<B>(room, 22, "MESSAGE_B", "B");
			break;
		}
		case 6:{
			b->SendToOtherClass<C>(room, 23, "MESSAGE_B", "C");
			break;
		}
		case 7:{
			c->SendToOtherClass<A>(room, 31, "MESSAGE_C", "A");
			break;
		}
		case 8:{
			c->SendToOtherClass<B>(room, 32, "MESSAGE_C", "B");
			break;
		}
		case 9:{
			c->SendToOtherClass<C>(room, 33, "MESSAGE_C", "C");
			break;
		}
		case 10:{
			a->AddSpeakClass<A>(room, "A");
			break;
		}
		case 11:{
			b->AddSpeakClass<B>(room, "B");
			break;
		}
		case 12:{
			c->AddSpeakClass<C>(room, "C");
			break;
		}
		case 13:{
			a->RemoveClass<A>(room, "A");
			break;
		}
		case 14:{
			b->RemoveClass<C>(room, "C");
			break;
		}
		case 15:{
			c->RemoveClass<B>(room, "B");
			break;
		}
		default:
			break;
		}
	}
}

void TestSpeakClass::Thread4(){
	while (1){
		int randNumber = rand() % 9 + 7;

		switch (randNumber)
		{
		case 1:{
			a->SendToOtherClass<A>(room, 11, "MESSAGE_A", "A");
			break;
		}
		case 2:{
			a->SendToOtherClass<B>(room, 12, "MESSAGE_A", "B");
			break;
		}
		case 3:{
			a->SendToOtherClass<C>(room, 13, "MESSAGE_A", "C");
			break;
		}
		case 4:{
			b->SendToOtherClass<A>(room, 21, "MESSAGE_B", "A");
			break;
		}
		case 5:{
			b->SendToOtherClass<B>(room, 22, "MESSAGE_B", "B");
			break;
		}
		case 6:{
			b->SendToOtherClass<C>(room, 23, "MESSAGE_B", "C");
			break;
		}
		case 7:{
			c->SendToOtherClass<A>(room, 31, "MESSAGE_C", "A");
			break;
		}
		case 8:{
			c->SendToOtherClass<B>(room, 32, "MESSAGE_C", "B");
			break;
		}
		case 9:{
			c->SendToOtherClass<C>(room, 33, "MESSAGE_C", "C");
			break;
		}
		case 10:{
			a->AddSpeakClass<A>(room, "A");
			break;
		}
		case 11:{
			b->AddSpeakClass<B>(room, "B");
			break;
		}
		case 12:{
			c->AddSpeakClass<C>(room, "C");
			break;
		}
		case 13:{
			a->RemoveClass<A>(room, "A");
			break;
		}
		case 14:{
			b->RemoveClass<C>(room, "C");
			break;
		}
		case 15:{
			c->RemoveClass<B>(room, "B");
			break;
		}
		default:
			break;
		}
	}
}

void TestSpeakClass::Thread5(){
	while (1){
		int randNumber = rand() % 9 + 7;

		switch (randNumber)
		{
		case 1:{
			a->SendToOtherClass<A>(room, 11, "MESSAGE_A", "A");
			break;
		}
		case 2:{
			a->SendToOtherClass<B>(room, 12, "MESSAGE_A", "B");
			break;
		}
		case 3:{
			a->SendToOtherClass<C>(room, 13, "MESSAGE_A", "C");
			break;
		}
		case 4:{
			b->SendToOtherClass<A>(room, 21, "MESSAGE_B", "A");
			break;
		}
		case 5:{
			b->SendToOtherClass<B>(room, 22, "MESSAGE_B", "B");
			break;
		}
		case 6:{
			b->SendToOtherClass<C>(room, 23, "MESSAGE_B", "C");
			break;
		}
		case 7:{
			c->SendToOtherClass<A>(room, 31, "MESSAGE_C", "A");
			break;
		}
		case 8:{
			c->SendToOtherClass<B>(room, 32, "MESSAGE_C", "B");
			break;
		}
		case 9:{
			c->SendToOtherClass<C>(room, 33, "MESSAGE_C", "C");
			break;
		}
		case 10:{
			a->AddSpeakClass<A>(room, "A");
			break;
		}
		case 11:{
			b->AddSpeakClass<B>(room, "B");
			break;
		}
		case 12:{
			c->AddSpeakClass<C>(room, "C");
			break;
		}
		case 13:{
			a->RemoveClass<A>(room, "A");
			break;
		}
		case 14:{
			b->RemoveClass<C>(room, "C");
			break;
		}
		case 15:{
			c->RemoveClass<B>(room, "B");
			break;
		}
		default:
			break;
		}
	}
}