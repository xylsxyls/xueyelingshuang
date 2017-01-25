#include "ConsoleTest.h"
#include "Cjson/CjsonAPI.h"
#include <afxwin.h>
#include "CStringManager/CStringManagerAPI.h"
#include "json.h"
#include "CGetPath/CGetPathAPI.h"

int main(){
	string str = "123456";
	CStringManager::Replace(str,"456", "");
	int x = 3;
	return 0;
}