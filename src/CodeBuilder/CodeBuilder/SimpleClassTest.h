#pragma once
#addHeadFile

/** SimpleClassTest操作类
*/
class SimpleClassTest : public SimpleFatherClassTest
{
public:
	/** 构造函数
	*/
	SimpleClassTest();

	/** 析构函数
	*/
	virtual ~SimpleClassTest();

public:
#addFun

protected:
	/** 初始化
	*/
	void init();

	/** 检测内部指针是否有效
	@return 返回内部指针是否有效
	*/
	bool check();

private:
#addVar
};