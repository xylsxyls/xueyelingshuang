#include "dtwsAccount.h"
#include <afx.h>
#include "CBrain/CBrainAPI.h"
#include "TaskLogin.h"
#include "TaskCreateAccount.h"
#include "TaskIntoGame.h"
#include "CScreenEx.h"
#include "ConfigInfo.h"
#include "DmSoft/DmSoftAPI.h"


int main(){
	CBrain brain;
	Sleep(2000);
	brain.Run();
	brain.push_back(CTask::CreateApi<TaskLogin>(), 1);
	brain.push_back(CTask::CreateApi<TaskCreateAccount>(), 1);
	brain.push_back(CTask::CreateApi<TaskIntoGame>(), 1);
	
	printf("111");
	getchar();
	return 0;
}