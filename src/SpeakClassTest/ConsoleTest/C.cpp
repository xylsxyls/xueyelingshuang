#include "C.h"

void C::ReceiveFromOtherClass(Room* room, const int& dataType, void* data, const string& otherClassName){
	printf("C receive message,dataType = %d,data = %s,otherClassName = %s\n", dataType, data, otherClassName.c_str());
}