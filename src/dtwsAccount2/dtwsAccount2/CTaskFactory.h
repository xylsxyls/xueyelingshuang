#pragma once
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"
#include "Rect/RectAPI.h"

//任务分为两部分，准备内容和执行内容
//准备内容是先决条件，移动到某处点击，移动到某处则为准备条件，点击为具体执行内容
//一旦被打断，准备内容必须也跟着重做
//准备内容和执行内容是不可分的

/** 登录任务
*/
class CLoginTask : public CTask
{
public:
	virtual void DoTask();
};

/** 寻找某张图，移动到该图片中心并单击
*/
class CClickPicTask : public CTask
{
public:
	CClickPicTask(const std::string& picPath,const xyls::Rect& rect);
	virtual void DoTask();

private:
	std::string m_picPath;
	xyls::Rect m_picRect;

private:
	/** 任务集群
	*/
	std::list<std::shared_ptr<CTask>> m_taskList;
};

class CFindPicTask : public CTask
{
public:
	virtual void DoTask();
	xyls::Point& GetPoint();
private:
	xyls::Point m_pointResult;
};

class CMoveMouseTask : public CTask
{
public:
	CMoveMouseTask(const xyls::Point& point);
	virtual void DoTask();

private:
	xyls::Point m_point;
};

class CClickTask : public CTask
{
public:
	virtual void DoTask();
};