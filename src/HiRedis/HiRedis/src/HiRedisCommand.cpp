#include "HiRedisCommand.h"

HiRedisCommand::HiRedisCommand()
{

}

HiRedisCommand::HiRedisCommand(const std::vector<std::string>& argv) :
m_args(argv)
{

}