#include "CMouse.h"
#include "DmSoft/DmSoftAPI.h"
#include "CRandom/CRandomAPI.h"

bool CMouse::LeftDown(){
	return DmSoft::LeftDown() == 1;
}

bool CMouse::RightDown(){
	return DmSoft::RightDown() == 1;
}

bool CMouse::LeftUp(){
	return DmSoft::LeftUp() == 1;
}

bool CMouse::RightUp(){
	return DmSoft::RightUp() == 1;
}

bool CMouse::LeftClick(int sleepTime){
	bool result = true;
	result = result && (DmSoft::LeftDown() == 1);
	if (sleepTime == -1) sleepTime = CRandom::Int(100, 500);
	Sleep(sleepTime);
	result = result && (DmSoft::LeftUp() == 1);
	return result;
}

bool CMouse::RightClick(int sleepTime){
	bool result = true;
	result = result && (DmSoft::RightDown() == 1);
	if (sleepTime == -1) sleepTime = CRandom::Int(100, 500);
	Sleep(sleepTime);
	result = result && (DmSoft::RightUp() == 1);
	return result;
}

bool CMouse::LeftDoubleClick(int sleepTime1, int sleepTime2, int sleepTime3){
	bool result = true;
	result = result && LeftClick(sleepTime1);
	if (sleepTime2 == -1) sleepTime2 = CRandom::Int(100, 500);
	Sleep(sleepTime2);
	result = result && LeftClick(sleepTime3);
	return result;
}

bool CMouse::RightDoubleClick(int sleepTime1, int sleepTime2, int sleepTime3){
	bool result = true;
	result = result && RightClick(sleepTime1);
	if (sleepTime2 == -1) sleepTime2 = CRandom::Int(100, 500);
	Sleep(sleepTime2);
	result = result && RightClick(sleepTime3);
	return result;
}

bool CMouse::LeftManyClick(int times, vector<int> vecSleepTime){
	if (vecSleepTime.size() != 0 && times * 2 - 1 != vecSleepTime.size()) return false;
	if (vecSleepTime.size() == 0){
		int timesTemp = times * 2 - 1;
		while (timesTemp-- != 0){
			vecSleepTime.push_back(-1);
		}
	}
	bool result = true;
	int i = -1;
	while (i++ != vecSleepTime.size() - 2){
		result = result && LeftClick(vecSleepTime.at(i));
		i++;
		if (vecSleepTime.at(i) == -1) vecSleepTime.at(i) = CRandom::Int(100, 500);
		Sleep(vecSleepTime.at(i));
	}
	result = result && LeftClick(vecSleepTime.at(i));
	return result;
}

bool CMouse::RightManyClick(int times, vector<int> vecSleepTime){
	if (vecSleepTime.size() != 0 && times * 2 - 1 != vecSleepTime.size()) return false;
	if (vecSleepTime.size() == 0){
		int timesTemp = times * 2 - 1;
		while (timesTemp-- != 0){
			vecSleepTime.push_back(-1);
		}
	}
	bool result = true;
	int i = -1;
	while (i++ != vecSleepTime.size() - 2){
		result = result && RightClick(vecSleepTime.at(i));
		i++;
		if (vecSleepTime.at(i) == -1) vecSleepTime.at(i) = CRandom::Int(100, 500);
		Sleep(vecSleepTime.at(i));
	}
	result = result && RightClick(vecSleepTime.at(i));
	return result;
}

bool CMouse::MoveAbsolute(Point point, int sleepTime){
	return DmSoft::MoveTo(point.x, point.y) == 1;
}

bool CMouse::MoveOpposite(Point point, int sleepTime){
	return false;
}

bool CMouse::MidWheelDown(){
	return DmSoft::WheelDown() == 1;
}

bool CMouse::MidWheelUp(){
	return DmSoft::WheelUp() == 1;
}

Point CMouse::GetCurrentPos(){
	VARIANT x, y;
	DmSoft::GetCursorPos(&x, &y);
	return Point(x.intVal,y.intVal);
}

int main(){
	Sleep(2000);
	Point po = CMouse::GetCurrentPos();
	int x = 3;
	return 0;
}