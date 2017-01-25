#include "B.h"

void B::ReceiveFromOtherClass(Room* room, const int& dataType, void* data, const string& otherClassName){
	printf("B receive message,dataType = %d,data = %s,otherClassName = %s\n",dataType,data,otherClassName.c_str());
}