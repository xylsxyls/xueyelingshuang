#include "CookStorageSyncTask.h"
#include "CookStorageService.h"
#include "CSystem/CSystemAPI.h"
#include "LogManager/LogManagerAPI.h"

CookStorageSyncTask::CookStorageSyncTask() :
CTask(72001),
m_version(0),
m_stopped(false)
{

}

void CookStorageSyncTask::setParam(const std::string& stateText, int64_t version)
{
	m_stateText = stateText;
	m_version = version;
	m_stopped.store(false);
}

void CookStorageSyncTask::DoTask()
{
	for (int32_t attempt = 1; attempt <= 3; ++attempt)
	{
		if (m_stopped.load())
		{
			LOGWARNING("CookStorageSyncTask stopped before mysql sync version=%lld attempt=%d",
			           static_cast<long long>(m_version),
			           attempt);
			return;
		}
		if (CookStorageService::instance().syncStateSnapshotToMysql(m_stateText, m_version))
		{
			return;
		}
		LOGERROR("CookStorageSyncTask mysql sync retry version=%lld attempt=%d",
		         static_cast<long long>(m_version),
		         attempt);
		CSystem::Sleep(500);
	}
	LOGERROR("CookStorageSyncTask mysql sync failed finally version=%lld bytes=%d",
	         static_cast<long long>(m_version),
	         static_cast<int32_t>(m_stateText.size()));
}

void CookStorageSyncTask::StopTask()
{
	m_stopped.store(true);
}

bool CookStorageSyncTask::ReExecute()
{
	return true;
}

CTask* CookStorageSyncTask::Clone()
{
	CookStorageSyncTask* task = new CookStorageSyncTask();
	if (task != nullptr)
	{
		task->setParam(m_stateText, m_version);
	}
	return task;
}
