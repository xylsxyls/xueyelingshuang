#pragma once
#include <stdint.h>
#include "CBigNumberMacro.h"

//大数运算，不支持负数，出现负数或除数为0等特殊情况一律返回-1
class CBigNumber;
//进制器，基于大数，所有结果和大数相同，输出时需要加输出位数
class CRadixTool;
#define BigNumberLength 10000

//针对系统自带的sprintf_s等函数在使用后会把多余的数变为-2，函数使其变为自定义数字，传入数组名和数组大小，用strlen计算，如果用sizeof计算的话当第二次传参的时候会传入指针，则计算出4，会出错
//_strrev();不需要使用，sprintf_s需要使用
void UnnecessaryToXA(char *arr, int size, int number);
#define UnnecessaryToX(ArrayName,number) UnnecessaryToXA(ArrayName,strlen(ArrayName) + 1,number)
//UnnecessaryToX(temp,0);//宏写法

//brr存入arr，其余全部重置为0
#define strcpy_x(arr,brr,number) strcpy_s(arr,strlen(brr) + 1,brr);UnnecessaryToX(arr,number)

int Power(int BaseNumber, int power);

typedef int32_t MultiBool;

//幂运算^符号必须加括号声明优先级
//只有在确保a和b都不是-1的情况下才能利用a - b == -1来判断是否为负数
//MutilBool值为1时表示判断成立，为0时表示反向成立
//除了-1 == -1的值为1，单边为-1的情况下值为0，其他只要有-1参与的判断都为-1
//在输入正常的情况下，a - b，a / 0，a % 0会得到-1
class CBigNumberAPI CBigNumber
{
public:
	char number[BigNumberLength];

public:
	CBigNumber();
	CBigNumber(int           InitNumber);
	CBigNumber(char*      strInitNumber);

public:
	CBigNumber operator = (int           InitNumber);
	CBigNumber operator = (char*      strInitNumber);

public:
	friend CBigNumber operator + (CBigNumber x, CBigNumber y);
	friend CBigNumber operator - (CBigNumber x, CBigNumber y);
	friend CBigNumber operator * (CBigNumber x, CBigNumber y);
	friend CBigNumber operator ^ (CBigNumber x, CBigNumber y);
	friend CBigNumber operator / (CBigNumber x, CBigNumber y);
	friend CBigNumber operator % (CBigNumber x, CBigNumber y);

	//扩展
public:
	friend CBigNumber operator + (int        x, CBigNumber y);
	friend CBigNumber operator - (int        x, CBigNumber y);
	friend CBigNumber operator * (int        x, CBigNumber y);
	friend CBigNumber operator ^ (int        x, CBigNumber y);
	friend CBigNumber operator / (int        x, CBigNumber y);
	friend CBigNumber operator % (int        x, CBigNumber y);

	friend CBigNumber operator + (CBigNumber x, int        y);
	friend CBigNumber operator - (CBigNumber x, int        y);
	friend CBigNumber operator * (CBigNumber x, int        y);
	friend CBigNumber operator ^ (CBigNumber x, int        y);
	friend CBigNumber operator / (CBigNumber x, int        y);
	friend CBigNumber operator % (CBigNumber x, int        y);

	friend CBigNumber operator + (char*      x, CBigNumber y);
	friend CBigNumber operator - (char*      x, CBigNumber y);
	friend CBigNumber operator * (char*      x, CBigNumber y);
	friend CBigNumber operator ^ (char*      x, CBigNumber y);
	friend CBigNumber operator / (char*      x, CBigNumber y);
	friend CBigNumber operator % (char*      x, CBigNumber y);

	friend CBigNumber operator + (CBigNumber x, char*      y);
	friend CBigNumber operator - (CBigNumber x, char*      y);
	friend CBigNumber operator * (CBigNumber x, char*      y);
	friend CBigNumber operator ^ (CBigNumber x, char*      y);
	friend CBigNumber operator / (CBigNumber x, char*      y);
	friend CBigNumber operator % (CBigNumber x, char*      y);

public:
	friend MultiBool operator == (CBigNumber x, CBigNumber y);
	friend MultiBool operator != (CBigNumber x, CBigNumber y);
	friend MultiBool operator >  (CBigNumber x, CBigNumber y);
	friend MultiBool operator >= (CBigNumber x, CBigNumber y);
	friend MultiBool operator <  (CBigNumber x, CBigNumber y);
	friend MultiBool operator <= (CBigNumber x, CBigNumber y);

	friend MultiBool operator == (CBigNumber x, int        y);
	friend MultiBool operator != (CBigNumber x, int        y);
	friend MultiBool operator >  (CBigNumber x, int        y);
	friend MultiBool operator >= (CBigNumber x, int        y);
	friend MultiBool operator <  (CBigNumber x, int        y);
	friend MultiBool operator <= (CBigNumber x, int        y);

	friend MultiBool operator == (int        x, CBigNumber y);
	friend MultiBool operator != (int        x, CBigNumber y);
	friend MultiBool operator >  (int        x, CBigNumber y);
	friend MultiBool operator >= (int        x, CBigNumber y);
	friend MultiBool operator <  (int        x, CBigNumber y);
	friend MultiBool operator <= (int        x, CBigNumber y);

	friend MultiBool operator == (CBigNumber x, char*      y);
	friend MultiBool operator != (CBigNumber x, char*      y);
	friend MultiBool operator >  (CBigNumber x, char*      y);
	friend MultiBool operator >= (CBigNumber x, char*      y);
	friend MultiBool operator <  (CBigNumber x, char*      y);
	friend MultiBool operator <= (CBigNumber x, char*      y);

	friend MultiBool operator == (char*      x, CBigNumber y);
	friend MultiBool operator != (char*      x, CBigNumber y);
	friend MultiBool operator >  (char*      x, CBigNumber y);
	friend MultiBool operator >= (char*      x, CBigNumber y);
	friend MultiBool operator <  (char*      x, CBigNumber y);
	friend MultiBool operator <= (char*      x, CBigNumber y);

public:
	CBigNumber operator ++ ();//前++
	CBigNumber operator -- ();//前--
	CBigNumber operator ++ (int);//后++
	CBigNumber operator -- (int);//后--

private:
	//把x内的最高位取出压入y.number[0]内，并删除x的最高位，y.number内原有数字向后移
	void HighNumberInto(CBigNumber* x, CBigNumber* y);
public:
	int InvertOut(char* OutString = nullptr);
	int Sequence(char* OutString = nullptr);
};