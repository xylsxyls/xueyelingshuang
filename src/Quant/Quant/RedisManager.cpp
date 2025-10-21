#include "RedisManager.h"

RedisManager::RedisManager()
{

}

RedisManager& RedisManager::instance()
{
	static RedisManager s_redisManager;
	return s_redisManager;
}

bool RedisManager::init()
{
	return true;
}

void RedisManager::uninit()
{

}

