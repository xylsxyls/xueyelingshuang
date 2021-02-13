#include "AutoPlayGameTask.h"
#include "AttackTask.h"
#include "BuyTask.h"
#include "LevelTask.h"
#include "SayTask.h"
#include "Point/PointAPI.h"

AutoPlayGameTask::AutoPlayGameTask():
m_gameThreadId(0),
m_exit(false)
{
	
}

void AutoPlayGameTask::DoTask()
{
	m_gameThreadId = CTaskThreadManager::Instance().Init();
	Sleep(5);
	if (m_scene == "ÓÎÏ·ÄÚ")
	{
		if (m_hero == "µ¶·æ")
		{
			int32_t count = 3600;
			while (count-- != 0)
			{
				std::shared_ptr<AttackTask> spAttackTask(new AttackTask);
				spAttackTask->setParam((count / 60) % 2 == 1 ? m_teammate1 : m_teammate2);
				CTaskThreadManager::Instance().GetThreadInterface(m_gameThreadId)->PostTask(spAttackTask);
			}
			RCSend("all send");
			int32_t sayCount = 18;
			int32_t sayType = 9;
			int32_t sayIndex = 0;
			while (!m_exit)
			{
				Sleep(10);
				std::shared_ptr<BuyTask> spBuyTask(new BuyTask);
				spBuyTask->setParam(xyls::Point(286, 342), xyls::Point(396, 455), xyls::Point(563, 570));
				CTaskThreadManager::Instance().GetThreadInterface(m_gameThreadId)->PostTask(spBuyTask, 2);
				std::shared_ptr<LevelTask> spLevelTask(new LevelTask);
				spLevelTask->setParam("rwqe");
				CTaskThreadManager::Instance().GetThreadInterface(m_gameThreadId)->PostTask(spLevelTask, 2);
				++sayIndex;
				if (sayIndex % sayCount == 0)
				{
					std::string say;
					if ((sayIndex / sayCount) % sayType == 0)
					{
						say = "haohaowana";
					}
					else if((sayIndex / sayCount) % sayType == 1)
					{
						say = "daiwotangying";
					}
					else if ((sayIndex / sayCount) % sayType == 2)
					{
						say = "wobuhuiwan";
					}
					else if ((sayIndex / sayCount) % sayType == 3)
					{
						say = "buyaolang";
					}
					else if ((sayIndex / sayCount) % sayType == 4)
					{
						say = "woqudaixian";
					}
					else if ((sayIndex / sayCount) % sayType == 5)
					{
						say = "jianchiyixia";
					}
					else if ((sayIndex / sayCount) % sayType == 6)
					{
						say = "woyaoshangcesuo";
					}
					else if ((sayIndex / sayCount) % sayType == 7)
					{
						say = "wodegongjihengao";
					}
					else if ((sayIndex / sayCount) % sayType == 8)
					{
						say = "zuoxiashiye";
					}

					std::shared_ptr<SayTask> spSayTask(new SayTask);
					spSayTask->setParam(say);
					CTaskThreadManager::Instance().GetThreadInterface(m_gameThreadId)->PostTask(spSayTask, 2);
				}
			}
		}
	}

	CTaskThreadManager::Instance().Uninit(m_gameThreadId);
	m_gameThreadId = 0;
	
	RCSend("leave");
}

void AutoPlayGameTask::StopTask()
{
	m_exit = true;
}

void AutoPlayGameTask::setParam(const std::string& hero,
	const std::string& scene,
	const std::string& teammate1,
	const std::string& teammate2)
{
	m_hero = hero;
	m_scene = scene;
	m_teammate1 = teammate1;
	m_teammate2 = teammate2;
}

void AutoPlayGameTask::Sleep(int32_t time)
{
	int32_t count = time * 1000 / 100;
	while (count-- != 0 && !m_exit)
	{
		::Sleep(100);
	}
}
