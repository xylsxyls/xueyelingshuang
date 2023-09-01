#pragma once
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"
#include <vector>
#include <memory>

class TextMessage;
class Semaphore;

class ClearTask : public CTask
{
public:
	ClearTask();

	~ClearTask();

public:
	void DoTask();

	void setParam(std::vector<std::shared_ptr<TextMessage>>* vecTextMessage, Semaphore* semaphore);

private:
	std::vector<std::shared_ptr<TextMessage>>* m_vecTextMessage;
	Semaphore* m_semaphore;
};