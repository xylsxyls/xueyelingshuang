#pragma once
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"

class AutoPlayGameTask : public CTask
{
public:
	AutoPlayGameTask();

public:
	void DoTask();

	void StopTask();

	void setParam(const std::string& hero,
		const std::string& scene,
		const std::string& teammate1,
		const std::string& teammate2);

private:
	void Sleep(int32_t time);

private:
	uint32_t m_gameThreadId;
	std::string m_hero;
	std::string m_scene;
	std::string m_teammate1;
	std::string m_teammate2;
	std::atomic<bool> m_exit;
};