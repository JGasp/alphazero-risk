#pragma once

#include "alphazero_moves.h"
#include "neural_network/alphazero_gpu_cluster.h"

#include <math.h>
#include <stdint.h>
#include <unordered_map>
#include <numeric>
#include <algorithm>
#include <chrono>


static const int ALL_MOVES = DATA_TERRITORY + 1;

class SimulationValue
{
public:
	float Q;
	float P;
	uint32_t N;
	uint8_t active_N;

	void addValue(float v);

	SimulationValue() : Q(0.0f), N(0), P(0.0f), active_N(0) {};
	SimulationValue(float p) : Q(0.0f), N(0), P(p), active_N(0) {};
};


class StateSimulations // Thread safe class
{
private:	
	std::mutex lock;
	std::unordered_map<LandIndex, SimulationValue> moveValues;	

	float value;
	bool visited;
	uint32_t sumN;

public:
	StateSimulations(NNOutputData out, uint64_t validMoves);

	SimulationValue& getSimulatedValue(LandIndex li);

	void setVisited(bool v);
	bool getVisited();
	void addValue(LandIndex li, float value);
	const std::unordered_map<LandIndex, SimulationValue>& getMoveValues();

	LandIndex getNextBestMoveAndSetVisited();
	std::vector<float> calculateMoveProbability(float temp);	
};


class StateSimulationsStorage // Thread safe class
{
private:
	std::mutex lock;
	std::unordered_map<State, std::shared_ptr<StateSimulations>> state_map;

	uint64_t statesAdded = 0;
	uint64_t duplicatedStatesDropped = 0;
public:
	std::shared_ptr<StateSimulations> getStateSimulation(const State& state);

	bool exist(const State& state);
	void add(const State& key, std::shared_ptr<StateSimulations>& value);

	void trimNodes();
	void clearNodes();
};


class AlphaZeroMCTS
{
private:	
	StateSimulationsStorage store;

	float search(State& state, std::shared_ptr<AlphaZeroNNId> nn);
	void threadSimulateJob(State state, std::shared_ptr<AlphaZeroNNId> nn, std::shared_ptr<Counter> c);
	void setRootState(const State& state, std::shared_ptr<AlphaZeroNNId> nn);

public:
	AlphaZeroMCTS() {};

	void simulate(const State& state, std::shared_ptr<AlphaZeroNNId> nn);
	
	StateSimulationsStorage* getStorage();
	LandIndex pickRandomWeightedMove(const std::vector<float>& probs);
	LandIndex pickHigestWeightedMove(const std::vector<float>& probs);

	uint64_t hitCouter = 0;
	uint64_t missCouter = 0;
};