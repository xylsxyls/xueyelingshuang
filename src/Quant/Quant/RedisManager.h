#pragma once
#include "HiRedis/HiRedisAPI.h"

class RedisManager
{
protected:
	RedisManager();

public:
	static RedisManager& instance();

	bool init();

	void uninit();

public:
	HiRedis m_redis;
};