#include "LevelDBManager.h"
#include "leveldb/db.h"
#include <mutex>

LevelDBManager::LevelDBManager():
m_defaultDBFile(nullptr)
{

}

LevelDBManager& LevelDBManager::instance()
{
	static LevelDBManager s_levelDBManager;
	return s_levelDBManager;
}

void LevelDBManager::openDBFile(const std::string& path, int32_t fileId)
{
	std::unique_lock<std::mutex> lock(m_mutex);
	if (fileId == 0 && m_defaultDBFile != nullptr)
	{
		return;
	}

	if (fileId != 0 && (m_dbFile.find(fileId) != m_dbFile.end()))
	{
		return;
	}

	leveldb::DB* dbFile = nullptr;
    leveldb::Options options;
    options.create_if_missing = true;
    leveldb::Status status = leveldb::DB::Open(options, path, &dbFile);
	if (!status.ok() || dbFile == nullptr)
	{
        printf("openDBFile error = %s, dbFile = %p\n", status.ToString().c_str(), dbFile);
		return;
    }
	if (fileId == 0)
	{
		m_defaultDBFile = dbFile;
		return;
	}
	
	m_dbFile[fileId] = dbFile;
}

void LevelDBManager::setConfig(const std::string& key, const std::string& value, int32_t fileId)
{
	leveldb::DB* dbFile = findDBFile(fileId);
	if (dbFile == nullptr)
	{
		return;
	}

	leveldb::Status status = dbFile->Put(leveldb::WriteOptions(), key, value);
    if (!status.ok())
	{
		printf("setConfig error = %s\n", status.ToString().c_str());
		return;
    }
}

bool LevelDBManager::getConfig(const std::string& key, std::string& value, int32_t fileId)
{
	leveldb::DB* dbFile = findDBFile(fileId);
	if (dbFile == nullptr)
	{
		return false;
	}

	leveldb::Status status = dbFile->Get(leveldb::ReadOptions(), key, &value);
	if (!status.ok())
	{
		return false;
    }
	return true;
}

void LevelDBManager::findConfig(const std::string& key, const std::function<void(const std::string&, const std::string&)>& findConfigFun, int32_t fileId)
{
	leveldb::DB* dbFile = findDBFile(fileId);
	if (dbFile == nullptr)
	{
		return;
	}

	leveldb::Iterator* it = dbFile->NewIterator(leveldb::ReadOptions());
	if (it == nullptr)
	{
		return;
	}

	for (it->SeekToFirst(); it->Valid(); it->Next())
	{
		leveldb::Status status = it->status();
		if (!status.ok())
		{
			printf("findConfig error = %s\n", status.ToString().c_str());
		}
		else
		{
			const std::string&& keyString = it->key().ToString();
			if (keyString.find(key) != std::string::npos)
			{
				findConfigFun(keyString, it->value().ToString());
			}
		}
	}
	delete it;
}

void LevelDBManager::deleteConfig(const std::string& key, int32_t fileId)
{
	leveldb::DB* dbFile = findDBFile(fileId);
	if (dbFile == nullptr)
	{
		return;
	}

	leveldb::Status status = dbFile->Delete(leveldb::WriteOptions(), key);
	if (!status.ok())
	{
		printf("deleteConfig error = %s\n", status.ToString().c_str());
		return;
    }
}

void LevelDBManager::closeDBFile(int32_t fileId)
{
	std::unique_lock<std::mutex> lock(m_mutex);
	if (fileId == 0 && m_defaultDBFile == nullptr)
	{
		return;
	}

	auto it = m_dbFile.find(fileId);
	if (fileId != 0 && (it == m_dbFile.end()))
	{
		return;
	}

	if (fileId == 0)
	{
		delete m_defaultDBFile;
		m_defaultDBFile = nullptr;
		return;
	}

	delete it->second;
	m_dbFile.erase(it);
}

leveldb::DB* LevelDBManager::findDBFile(int32_t fileId)
{
	leveldb::DB* dbFile = nullptr;
	if (fileId == 0)
	{
		dbFile = m_defaultDBFile;
	}
	else
	{
		auto it = m_dbFile.find(fileId);
		if (it == m_dbFile.end())
		{
			return nullptr;
		}
		dbFile = it->second;
	}
	return dbFile;
}