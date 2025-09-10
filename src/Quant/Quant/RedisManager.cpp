#include "RedisManager.h"

RedisManager::RedisManager()
{

}

RedisManager& RedisManager::instance()
{
	static RedisManager s_redisMaanger;
	return s_redisMaanger;
}

bool RedisManager::init()
{
	return true;
}

void RedisManager::uninit()
{

}

