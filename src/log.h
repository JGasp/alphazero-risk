#pragma once

#include <fstream>
#include <string>
#include <iostream>
#include <filesystem>


namespace UtilityFormat
{
	static void printProgress(long current, long end)
	{
		double percentage = ((double)current) / end;
		int paddedPercentage = (int)(percentage * 100);

		int lpad = (int)(percentage * 60);
		int rpad = 60 - lpad;

		printf("\r%d/%d %3d%% [%.*s%*s]", current, end, paddedPercentage, lpad, "||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||", rpad, "");
		fflush(stdout);
	}
}

class Log
{
private:
	std::ofstream improvementLog;
	std::ofstream benchmarkLog;
	std::ofstream nnTrainingLog;
	std::ofstream nnPerformanceLog;
	std::ofstream mctsPerformanceLog;

public:
	void init()
	{
		std::filesystem::create_directories("log");
	}

	std::ofstream& getImprovementLog()
	{
		if (!improvementLog.is_open())
		{
			improvementLog = std::ofstream("log/azr-improvement-log.txt", std::ofstream::out | std::ofstream::app);
		}
		return improvementLog;
	}

	std::ofstream& getNNTrainingLog()
	{
		if (!nnTrainingLog.is_open())
		{
			nnTrainingLog = std::ofstream("log/azr-nn-training-log.txt", std::ofstream::out | std::ofstream::app);
		}
		return nnTrainingLog;
	}

	std::ofstream& getBenchmarkLog()
	{
		if (!benchmarkLog.is_open())
		{
			benchmarkLog = std::ofstream("log/azr-benchmark-log.txt", std::ofstream::out | std::ofstream::app);
		}
		return benchmarkLog;
	}

	std::ofstream& getNNPerformanceLog()
	{
		if (!nnPerformanceLog.is_open())
		{
			nnPerformanceLog = std::ofstream("log/nn-performance-log.txt", std::ofstream::out);
		}
		return nnPerformanceLog;
	}

	std::ofstream& getMCTSPerformanceLog()
	{
		if (!mctsPerformanceLog.is_open())
		{
			mctsPerformanceLog = std::ofstream("log/mcts-performance-log.txt", std::ofstream::out);
		}
		return mctsPerformanceLog;
	}

	static Log& getInstance()
	{
		static Log INSTANCE;
		return INSTANCE;
	}
};

static Log& LOG = Log::getInstance();