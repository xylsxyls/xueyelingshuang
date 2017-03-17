#include "CBrain.h"
#include <windows.h>

//void CBrain::listen(int flag){
//
//}

CBrain::CBrain(){
	ctrl[RUN] = false;
	ctrl[SUSPEND] = false;
	ctrl[END] = false;
}

void CBrain::push_back(const CTask& task, int level){
	mutex.lock();
	mapTask[level].push_back(task);
	mutex.unlock();
}

void CBrain::Run(){
	ctrl[RUN] = true;
}

void CBrain::SuspendRun(){
	ctrl[SUSPEND] = false;
}

void CBrain::EndRun(){
	ctrl[END] = false;
}

void CBrain::ThreadWork(){
	while (ctrl[RUN] != true) Sleep(1);
	while (ctrl[END] != true){
		Sleep(1);
		bool ifHasTask = false;
		mutex.lock();
		for (auto itmapTask = mapTask.begin(); itmapTask != mapTask.end(); itmapTask++){
			currentTask = itmapTask->second.front();
			itmapTask->second.pop_front();
			ifHasTask = true;
			break;
		}
		mutex.unlock();

		//?如果有任务就开始执行
		if (ifHasTask == true){
			bool ifRun = currentTask.CheckRun();
			if (ifRun == true){
				//?把当前任务的后台监测加入列表
				mutex.lock();
				listTaskBack.push_back(currentTask);
				mutex.unlock();

				bool result = currentTask.Run();

				mutex.lock();
				//?如果返回1表示要删除后台监测
				if (result == true){
					for (auto itlistTaskBack = listTaskBack.begin(); itlistTaskBack != listTaskBack.end(); itlistTaskBack++){
						if (itlistTaskBack->uuid == currentTask.uuid){
							listTaskBack.erase(itlistTaskBack);
							break;
						}
					}
				}
				mutex.unlock();
			}
			else{

			}
		}
		else continue;
	}
}

void CBrain::ThreadBack(){
	while (ctrl[RUN] != true) Sleep(1);
	while (ctrl[END] != true){
		Sleep(1);
		mutex.lock();
		for (auto itlistTaskBack = listTaskBack.begin(); itlistTaskBack != listTaskBack.end(); itlistTaskBack++){
			if (itlistTaskBack->ifExceptionRun == true){
				itlistTaskBack->ExceptionRun(itlistTaskBack->CheckException());
			}
		}
		mutex.unlock();
	}
}