#include "CBrain.h"
#include <windows.h>
#include <assert.h>

CBrain::CBrain(){
	ctrl[RUN] = false;
	ctrl[SUSPEND] = false;
	ctrl[END] = false;
}

void CBrain::push_back(const CTask& task, int level){
	mutex.lock();
	//?如果当前有任务在执行并且优先级高于当前执行任务则先暂停该任务
	if (level > currentLevel){
		SuspendRun();
	}
	mapTask[level].push_back(task);
	mutex.unlock();
}

void CBrain::Run(){
	ctrl[RUN] = true;
}

void CBrain::SuspendRun(){
	//?暂停前台任务
	currentTask.SuspendRun();
	//?暂停
	ctrl[SUSPEND] = true;
}

void CBrain::RecoveryRun(){
	ctrl[SUSPEND] = false;
}

void CBrain::EndRun(){
	ctrl[END] = false;
}

void CBrain::ThreadWork(){
	while (ctrl[RUN] == false) Sleep(1);
	while (ctrl[END] == false){
		while (ctrl[SUSPEND] == true) Sleep(1);
		Sleep(1);
		//?首先获取最优先任务到成员变量currentTask中
		GetCurrentTask();
		//?如果有任务就开始执行
		if (currentTask.isValid() == true){
			//?初始化运行
			int intiResult = currentTask.initRun();
			//?如果是1表示可以运行
			if (intiResult == CTask::INITSUCCESS){
				//?把当前任务的后台监测加入列表
				AddBack(currentTask);
				int runResult = currentTask.Run();
				assert(runResult == CTask::RUNSUCCESS || runResult == CTask::RUNBACK || runResult == CTask::RUNSUSPEND);
				//?如果返回1表示要删除前台执行和后台监测，返回0则表示该任务被暂停或者是长期后台任务，则不作处理
				if (runResult == CTask::RUNSUCCESS){
					DeleteTask(currentTask);
					DeleteBack(currentTask);
				}
				//?如果是长期后台任务，需要把该任务移动到同层优先级的末尾
				else if (runResult == CTask::RUNBACK) DeleteTask(currentTask);
				//?如果是被手动暂停则删除后台
				else if (runResult == CTask::RUNSUSPEND) DeleteBack(currentTask);
			}
			//?如果初始化失败则进入任务的异常处理，并且丢弃该任务
			else{
				currentTask.ExceptionRun(intiResult);
				DeleteTask(currentTask);
				DeleteBack(currentTask);
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

void CBrain::DeleteTask(const CTask& task){
	std::unique_lock<std::mutex> lock(mutex);
	for (auto itmapTask = mapTask.begin(); itmapTask != mapTask.end(); itmapTask++){
		for (auto itlist = itmapTask->second.begin(); itlist != itmapTask->second.end(); itlist++){
			//?如果发现相同的task
			if (itlist->uuid == task.uuid){
				//?如果出现0个任务的树节点则说明出错
				assert(itmapTask->second.size() != 0);
				//?如果该任务层只有一个任务，那么直接移除一整个map节点
				if (itmapTask->second.size() == 1) mapTask.erase(itmapTask);
				//?如果有多个任务那么移除一个
				else itmapTask->second.erase(itlist);
				return;
			}
		}
	}
}

void CBrain::DeleteBack(const CTask& task){
	std::unique_lock<std::mutex> lock(mutex);
	for (auto itlistTaskBack = listTaskBack.begin(); itlistTaskBack != listTaskBack.end(); itlistTaskBack++){
		if (itlistTaskBack->uuid == task.uuid){
			listTaskBack.erase(itlistTaskBack);
			break;
		}
	}
}

void CBrain::GetCurrentTask(){
	std::unique_lock<std::mutex> lock(mutex);
	//?取出任务树种优先级最高排在最前的任务
	for (auto itmapTask = mapTask.begin(); itmapTask != mapTask.end(); itmapTask++){
		currentLevel = itmapTask->first;
		currentTask = itmapTask->second.front();
		return;
	}
	currentTask.SetEmpty();
}

void CBrain::AddBack(const CTask& task){
	std::unique_lock<std::mutex> lock(mutex);
	listTaskBack.push_back(currentTask);
}

void CBrain::MoveToEnd(const CTask& task){
	std::unique_lock<std::mutex> lock(mutex);
	for (auto itmapTask = mapTask.begin(); itmapTask != mapTask.end(); itmapTask++){
		for (auto itlist = itmapTask->second.begin(); itlist != itmapTask->second.end(); itlist++){
			if (task.uuid == itlist->uuid){
				itmapTask->second.erase(itlist);
				itmapTask->second.push_back(task);
				return;
			}
		}
	}
}

int main(){
	return 0;
}