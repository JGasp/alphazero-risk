#pragma once

#include <stdio.h>
#include <string>
#include "alphazero_player.h"
#include "../random/random_player.h"
#include "../script/script_player.h"

class AlphaZeroTrainer
{
private:
	int trainIteration;

	void generateTrainData(std::shared_ptr<AlphaZeroNNGroup> nnModel);
	void threadExecuteTrainingGame(std::shared_ptr<AlphaZeroNNId> nn, NNTrainDataStorage* nnStorage, std::shared_ptr<Counter> c);	
		
	bool isModelImproved(const GameResults& gr);
	bool updateIfImprovement(std::shared_ptr<AlphaZeroNNGroup> newModel, std::shared_ptr<AlphaZeroNNGroup> oldModel, bool doBenchmark);
	void benchmark(std::shared_ptr<AlphaZeroPlayerGroup> nnModel);

public:
	NNTrainDataStorage trainStorage;

	std::shared_ptr<RandomPlayerGroup> randomPlayerGroup;
	std::shared_ptr<ScriptPlayerGroup> scriptPlayerGroup;

	AlphaZeroTrainer();
	void train(std::shared_ptr<AlphaZeroNNGroup> nnGroup, std::shared_ptr<AlphaZeroNNGroup> oldNNGroup);
	void trainOnScript(std::shared_ptr<AlphaZeroNNGroup> nnGroup, std::shared_ptr<AlphaZeroNNGroup> oldNNGroup);
	void trainOnGeneratedData(std::shared_ptr<AlphaZeroNNGroup> nn, std::shared_ptr<AlphaZeroNNGroup> nnOld);
};
