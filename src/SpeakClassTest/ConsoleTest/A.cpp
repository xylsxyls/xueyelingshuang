#include "A.h"

void A::ReceiveFromOtherClass(Room* room, const int& dataType, void* data, const string& otherClassName){
	printf("A receive message,dataType = %d,data = %s,otherClassName = %s\n", dataType, data, otherClassName.c_str());
}