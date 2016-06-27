/*=====================================================================
程序描述：动态加载动态库
作者：杨楠
创建日期：2015/10/10
修改人：mingl
修改日期：2015/11/30
===================================================================== */
#ifndef __DLL_LOAD_H__
#define __DLL_LOAD_H__

#include <windows.h>

#define DLL_OPEN(DllPath) LoadLibraryEx(DllPath,0,LOAD_WITH_ALTERED_SEARCH_PATH);
#define DLL_LOAD(DllHinstance,DllFunName) (void(__stdcall *)())GetProcAddress(DllHinstance,DllFunName);
#define DLL_CLOSE(DllHinstance) ::FreeLibrary(DllHinstance);

#define V(NewFunName) (void **)(&(NewFunName))

#define FCNNAME(fcn) (FCN((void**)fcn,#fcn))

typedef enum enum_check{
	CHECK_OK=0,			/// 检测成功
	CHECK_ERR_PATH=1,	/// 检测动态库路径失败
	CHECK_ERR_FCN=2		/// 检测函数失败
}ENUM_CHECK;

typedef struct tagCheckFunction{
	int nCheck;			/// 检测结果 ENUM_CHECK
	int nCheckFcnNum;	/// 检测失败个数，尽在检测失败情况下有效
	char szFcnName[128];/// 函数错误名称
	tagCheckFunction()
	{
		nCheck = CHECK_OK;
		nCheckFcnNum = 0;
		memset(szFcnName,0x00,sizeof(szFcnName));
	}
}CHECK_FUNCTION;

typedef struct tagFcn
{
	void** pAddr;
	char szFcnName[128];
	tagFcn(void** _in_pAddr,char* _in_pName)
	{
		pAddr = _in_pAddr;
		memset(szFcnName,0x00,sizeof(szFcnName));
		int i=0,j=0;
		while (_in_pName[i] && (i<sizeof(szFcnName)))
		{
			if(_in_pName[i] == ' ' || _in_pName[i] == '&')
			{
				i++;
				continue;
			}
			szFcnName[j++] = _in_pName[i++];
		}
		szFcnName[sizeof(szFcnName)-1] = 0x00;
	}
}FCN;

class CDLL_LOAD
{
private:
	HINSTANCE m_Hinstance;

public:
	CDLL_LOAD(const char* pPath);
	~CDLL_LOAD(void);

	/************************************************************************
	* 加载动态库 Load_Fcn
	* 功能：加载动态库
	* 返回：CHECK_FUNCTION
	* 参数：
		nMumber 需要检测的函数个数
		...		void**类型，传出获取的函数地址
	************************************************************************/
	CHECK_FUNCTION Load_Fcn(int nMumber,...);
};




#endif // __DLL_LOAD_H__