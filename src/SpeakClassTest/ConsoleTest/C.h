#pragma once
#include "SpeakClass.h"

class C : public SpeakClass{
public:
	void ReceiveFromOtherClass(Room* room, const int& dataType, void* data, const string& otherClassName);
};