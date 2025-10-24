#include "CompetitionManager.h"
#include "StrategyTask.h"
#include "QuantStrategyManager.h"
#include <algorithm>
#include <iostream>
#include <cmath>

CompetitionManager::CompetitionManager() :
m_pResultQueue(nullptr),
m_completedCount(0),
m_totalTasks(0),
m_stopFlag(false),
m_isRunning(false),
m_isCompleted(false)
{
	m_pResultQueue = new LockFreeQueue<StrategyResult>();
}

CompetitionManager& CompetitionManager::instance()
{
	static CompetitionManager s_competitionManager;
	return s_competitionManager;
}

CompetitionManager::~CompetitionManager()
{
	uninit();
	if (m_pResultQueue)
	{
		delete m_pResultQueue;
		m_pResultQueue = nullptr;
	}
}

bool CompetitionManager::init(uint32_t threadCount)
{
	if (m_isRunning)
	{
		std::cerr << "Competition is already running" << std::endl;
		return false;
	}

	// 清理之前的资源
	cleanup();

	// 初始化线程池
	for (uint32_t i = 0; i < threadCount; ++i)
	{
		uint32_t threadId = CTaskThreadManager::Instance().Init();
		if (threadId == 0)
		{
			std::cerr << "Failed to initialize thread " << i << std::endl;
			uninit();
			return false;
		}
		m_threadIds.push_back(threadId);
	}

	// 初始化无锁队列
	if (!m_pResultQueue->init())
	{
		std::cerr << "Failed to initialize result queue" << std::endl;
		uninit();
		return false;
	}

	std::cout << "CompetitionManager initialized with " << threadCount << " threads" << std::endl;
	return true;
}

void CompetitionManager::uninit()
{
	stopCompetition();

	// 停止所有线程
	for (uint32_t threadId : m_threadIds)
	{
		CTaskThreadManager::Instance().Uninit(threadId);
	}
	m_threadIds.clear();

	// 停止结果处理线程
	if (m_resultThread.joinable())
	{
		m_stopFlag = true;
		// 发送一个空结果来唤醒结果线程
		StrategyResult emptyResult;
		//emptyResult.strategyId = -2; // 特殊ID表示停止信号
		m_pResultQueue->push(emptyResult);

		m_resultThread.join();
	}

	// 清理队列
	if (m_pResultQueue)
	{
		m_pResultQueue->destroy();
	}

	m_isRunning = false;
	m_isCompleted = false;
	std::cout << "CompetitionManager uninitialized" << std::endl;
}

bool CompetitionManager::startCompetition(const CompetitionConfig& config,
	StrategyMode strategyMode,
	const std::vector<std::vector<int32_t>>& allParams)
{
	if (m_isRunning)
	{
		std::cerr << "Competition is already running" << std::endl;
		return false;
	}

	if (m_threadIds.empty())
	{
		std::cerr << "CompetitionManager not initialized" << std::endl;
		return false;
	}

	if (allParams.empty())
	{
		std::cerr << "No strategy parameters provided" << std::endl;
		return false;
	}

	if (!config.marketData)
	{
		std::cerr << "Market data is not provided in config" << std::endl;
		return false;
	}

	if (config.stocks.empty())
	{
		std::cerr << "No stocks provided in config" << std::endl;
		return false;
	}

	// 保存配置和参数
	m_currentConfig = config;
	m_currentStrategyMode = strategyMode;
	m_allParams = allParams;

	// 重置状态
	m_completedCount = 0;
	m_totalTasks = static_cast<uint32_t>(allParams.size());
	m_stopFlag = false;
	m_isRunning = true;
	m_isCompleted = false;
	m_intermediateResults.clear();

	// 重置最终结果
	m_finalResult = CompetitionFinalResult();

	// 启动结果处理线程
	m_resultThread = std::thread(&CompetitionManager::resultProcessingThread, this);

	// 分发任务到线程池
	distributeTasks();

	std::cout << "Competition started with " << allParams.size() << " strategies" << std::endl;
	return true;
}

void CompetitionManager::stopCompetition()
{
	if (!m_isRunning)
	{
		return;
	}

	m_stopFlag = true;

	// 停止所有线程中的任务
	for (uint32_t threadId : m_threadIds)
	{
		auto threadInterface = CTaskThreadManager::Instance().GetThreadInterface(threadId);
		if (threadInterface)
		{
			threadInterface->StopAllTask();
		}
	}

	std::cout << "Competition stopped" << std::endl;
}

bool CompetitionManager::isRunning() const
{
	return m_isRunning;
}

int CompetitionManager::getProgress() const
{
	if (m_totalTasks == 0)
	{
		return 0;
	}
	return static_cast<int>((m_completedCount * 100) / m_totalTasks);
}

CompetitionFinalResult CompetitionManager::getFinalResult() const
{
	std::lock_guard<std::mutex> lock(m_resultsMutex);
	return m_finalResult;
}

bool CompetitionManager::isCompleted() const
{
	return m_isCompleted;
}

void CompetitionManager::resultProcessingThread()
{
	Semaphore waitSemaphore;
	bool finalSignalReceived = false;
	uint32_t processedCount = 0;

	std::cout << "Result processing thread started" << std::endl;

	while (!m_stopFlag && !finalSignalReceived)
	{
		StrategyResult result;
		if (m_pResultQueue->pop(&result))
		{
			if (result.strategyMode == StrategyMode::COUNT)
			{
				// 完成信号
				finalSignalReceived = true;
				std::cout << "Received completion signal" << std::endl;
				break;
			}
			else if (result.strategyMode == StrategyMode::COUNT)
			{
				// 停止信号
				std::cout << "Received stop signal" << std::endl;
				break;
			}
			else
			{
				processStrategyResult(result);
				processedCount++;

				if (processedCount % 10 == 0)
				{
					std::cout << "Processed " << processedCount << " results, progress: "
						<< getProgress() << "%" << std::endl;
				}
			}
		}
		else
		{
			// 队列为空，等待一段时间
			waitSemaphore.wait(100);
		}
	}

	// 处理剩余的结果
	if (!m_stopFlag)
	{
		StrategyResult result;
		while (m_pResultQueue->pop(&result))
		{
			if ((int32_t)result.strategyMode >= 0)
			{
				processStrategyResult(result);
				processedCount++;
			}
		}

		// 计算最终排名
		calculateFinalRanking();

		m_isCompleted = true;
		std::cout << "Competition completed. Total results processed: " << processedCount << std::endl;
	}
	else
	{
		std::cout << "Competition stopped by user. Processed " << processedCount << " results" << std::endl;
	}

	m_isRunning = false;
}

void CompetitionManager::processStrategyResult(const StrategyResult& result)
{
	std::lock_guard<std::mutex> lock(m_resultsMutex);
	m_intermediateResults.push_back(result);
}

void CompetitionManager::calculateFinalRanking()
{
	std::lock_guard<std::mutex> lock(m_resultsMutex);

	if (m_intermediateResults.empty())
	{
		std::cout << "No results to rank" << std::endl;
		return;
	}

	// 按总收益率排序
	std::sort(m_intermediateResults.begin(), m_intermediateResults.end(),
		[](const StrategyResult& a, const StrategyResult& b) {
		return a.totalReturn > b.totalReturn;
	});

	// 填充最终结果
	m_finalResult.rankedResults = m_intermediateResults;
	m_finalResult.totalStrategies = m_totalTasks;
	m_finalResult.completedStrategies = m_completedCount;

	// 计算统计指标
	if (!m_intermediateResults.empty())
	{
		m_finalResult.bestReturn = m_intermediateResults.front().totalReturn;
		m_finalResult.worstReturn = m_intermediateResults.back().totalReturn;

		BigNumber sumReturn = 0;
		BigNumber sumAnnualReturn = 0;
		BigNumber sumMaxDrawdown = 0;
		BigNumber sumWinRate = 0;
		BigNumber sumProfitArea = 0;
		BigNumber sumHealthScore = 0;

		for (const auto& result : m_intermediateResults)
		{
			sumReturn = sumReturn + result.totalReturn;
			sumAnnualReturn = sumAnnualReturn + result.annualReturn;
			sumMaxDrawdown = sumMaxDrawdown + result.maxDrawdown;
			sumWinRate = sumWinRate + result.winRate;
			sumProfitArea = sumProfitArea + result.profitArea;
			sumHealthScore = sumHealthScore + result.healthScore;
		}

		BigNumber count = (int32_t)m_intermediateResults.size();
		m_finalResult.averageReturn = sumReturn / count;
		m_finalResult.averageAnnualReturn = sumAnnualReturn / count;
		m_finalResult.averageMaxDrawdown = sumMaxDrawdown / count;
		m_finalResult.averageWinRate = sumWinRate / count;
		m_finalResult.averageProfitArea = sumProfitArea / count;
		m_finalResult.averageHealthScore = sumHealthScore / count;

		// 计算中位数收益率
		size_t midIndex = m_intermediateResults.size() / 2;
		if (m_intermediateResults.size() % 2 == 0)
		{
			m_finalResult.medianReturn = (m_intermediateResults[midIndex - 1].totalReturn +
				m_intermediateResults[midIndex].totalReturn) / 2;
		}
		else
		{
			m_finalResult.medianReturn = m_intermediateResults[midIndex].totalReturn;
		}

		// 计算标准差
		BigNumber variance = 0;
		for (const auto& result : m_intermediateResults)
		{
			BigNumber diff = result.totalReturn - m_finalResult.averageReturn;
			variance = variance + (diff * diff);
		}
		variance = variance / count;
		m_finalResult.stdDevReturn = variance.sqrt();
	}

	std::cout << "Final ranking calculated. Total strategies: "
		<< m_finalResult.completedStrategies << "/" << m_finalResult.totalStrategies << std::endl;
}

void CompetitionManager::distributeTasks()
{
	// 分配任务到线程
	size_t tasksPerThread = (m_allParams.size() + m_threadIds.size() - 1) / m_threadIds.size();

	for (size_t i = 0; i < m_threadIds.size(); ++i)
	{
		auto threadInterface = CTaskThreadManager::Instance().GetThreadInterface(m_threadIds[i]);
		if (!threadInterface)
		{
			std::cerr << "Failed to get thread interface for thread " << m_threadIds[i] << std::endl;
			continue;
		}

		// 计算该线程的任务范围
		size_t startIdx = i * tasksPerThread;
		size_t endIdx = (std::min)(startIdx + tasksPerThread, m_allParams.size());

		for (size_t j = startIdx; j < endIdx; ++j)
		{
			// 创建策略实例
			auto spStrategy = QuantStrategyManager::instance().createStrategy(m_currentStrategyMode);
			if (!spStrategy)
			{
				std::cerr << "Failed to create strategy for parameters set " << j << std::endl;
				continue;
			}

			// 设置策略参数
			spStrategy->setStrategyParam(m_allParams[j]);

			// 添加股票
			for (const auto& stock : m_currentConfig.stocks)
			{
				spStrategy->addStock(stock);
			}

			// 设置市场数据
			spStrategy->setMarket(m_currentConfig.marketData);

			// 初始化策略
			spStrategy->init(m_currentConfig.beginTime, m_currentConfig.endTime);

			// 创建任务
			int32_t taskId = static_cast<int32_t>(j + 1);
			std::shared_ptr<StrategyTask> spStrategyTask(new StrategyTask);
			spStrategyTask->setParam(m_currentConfig.beginTime, m_currentConfig.endTime, m_currentConfig.stocks,
				spStrategy, m_currentConfig.marketData, m_currentConfig.initialFund, m_pResultQueue);
			threadInterface->PostTask(spStrategyTask, 1);

			if (j % 10 == 0)
			{
				std::cout << "Distributed " << (j - startIdx + 1) << " tasks to thread " << i << std::endl;
			}
		}

		std::cout << "Thread " << i << " received " << (endIdx - startIdx) << " tasks" << std::endl;
	}
}

void CompetitionManager::cleanup()
{
	// 清理中间状态
	{
		std::lock_guard<std::mutex> lock(m_resultsMutex);
		m_intermediateResults.clear();
	}

	m_completedCount = 0;
	m_totalTasks = 0;
	m_stopFlag = false;
	m_isRunning = false;
	m_isCompleted = false;
	m_allParams.clear();
}