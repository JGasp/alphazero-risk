#include "alphazero_mcts.h"



/////////////////////
// SimulationValue //
/////////////////////
void SimulationValue::addValue(float v)
{
	if (N == 0)
	{
		Q = v;
	}
	else
	{
		Q = (N * Q + v) / (N + 1);
	}

	N++;
	active_N--;
}

//////////////////////
// StateSimulations //
//////////////////////
StateSimulations::StateSimulations(NNOutputData out, uint64_t vm)
{
	value = out.value;
	visited = true;
	sumN = 0;

	uint64_t tvm = vm;
	while (tvm > 0)
	{
		uint64_t m = Utility::getFirstBitMask(tvm);
		tvm &= ~m;

		int i = Utility::lm2i(m);
		LandIndex li = Utility::lm2li(m);
		moveValues[li] = SimulationValue(out.policy[i]);
	}
}

bool StateSimulations::getVisited()
{
	return visited;
}

void StateSimulations::setVisited(bool v)
{
	std::lock_guard<std::mutex> guard(lock);
	visited = v;
}

void StateSimulations::addValue(LandIndex li, float value)
{
	std::lock_guard<std::mutex> guard(lock);
	moveValues.at(li).addValue(value);
	sumN++;
}

const std::unordered_map<LandIndex, SimulationValue>& StateSimulations::getMoveValues()
{
	return moveValues;
}

LandIndex StateSimulations::getNextBestMoveAndSetVisited()
{
	std::lock_guard<std::mutex> guard(lock);
	visited = true;

	LandIndex bestMove = LandIndex::None;
	float bestU = -INFINITY;

	LandIndex duplicateBestMove = LandIndex::None;
	float duplicateBestU = -INFINITY;

	for (auto e : moveValues)
	{		
		float P = e.second.P;
		float noiseP = (1 - SETTINGS.DIR_NOISE_EPSI) * P + SETTINGS.DIR_NOISE_EPSI * SETTINGS.DIR_NOISE_VALUE;

		float v = noiseP * SETTINGS.HP_EXPLORATION * sqrtf(1.0f + sumN);
		float n = 1.0f + e.second.N;

		float u = e.second.Q + (v / n);
		if (u > bestU)
		{
			SimulationValue& sv = moveValues.at(e.first);
			// Skip if one thread is already exploring unobserved state to avoid duplicate requests
			if (sv.N == 0 && sv.active_N == 1) 
			{
				/* 
				When there is none unobserved move left, duplicate move request. 
				In case of attack it can split in multiple states
				*/
				if (u > duplicateBestU) 
				{
					duplicateBestU = u;
					duplicateBestMove = e.first;
				}
			}
			else
			{
				bestU = u;
				bestMove = e.first;
			}			
		}	
	}

	if (bestMove == LandIndex::None)
	{
		bestMove = duplicateBestMove;
	}

	SimulationValue& sv = moveValues.at(bestMove);
	sv.active_N++;
	return bestMove;
}

std::vector<float> StateSimulations::calculateMoveProbability(float temp)
{
	std::lock_guard<std::mutex> guard(lock);
	std::vector<float> policy(ALL_MOVES);
	
	float probSum = 0.0f;
	for (int i = 0; i < ALL_MOVES; i++)
	{
		LandIndex li = Utility::i2li(i);
		if (moveValues.contains(li))
		{
			SimulationValue& sv = moveValues.at(li);
			float prob = pow(sv.N, (1.0 / temp));
			policy[i] = prob;
			probSum += prob;
		}
		else
		{
			policy[i] = 0.0f;
		}
	}

	for (int i = 0; i < policy.size(); i++)
	{
		policy[i] /= probSum;
	}

	return policy;

	/*
	if (temp > 0.0f)
	{
	else
	{
		int maxIndex = -1;
		uint32_t maxN = 0;

		for (int i = 0; i < ALL_MOVES; i++)
		{
			LandIndex li = Utility::i2li(i);
			if (moveValues.contains(li))
			{
				SimulationValue& sv = moveValues.at(li);
				if (sv.N > maxN)
				{
					maxIndex = i;
					maxN = sv.N;
				}
			}
			policy[i] = 0.0f;
		}

		policy[maxIndex] = 1.0f;

		return policy;
	}
	*/
}

SimulationValue& StateSimulations::getSimulatedValue(LandIndex li)
{
	std::lock_guard<std::mutex> guard(lock);
	return moveValues.at(li);
}

/////////////////////////////
// StateSimulationsStorage //
/////////////////////////////
bool StateSimulationsStorage::exist(const State& key)
{
	std::lock_guard<std::mutex> guard(lock);
	for (auto i : state_map)
	{
		if (i.first.equalFields(key)) 
		{
			return true;
		}
	}
	return false;
	//return state_map.contains(key);
}

void StateSimulationsStorage::add(const State& key, std::shared_ptr<StateSimulations>& value)
{
	std::lock_guard<std::mutex> guard(lock);
	if (!state_map.contains(key))
	{
		state_map[key] = value;
		statesAdded++;
	}
	else
	{
		duplicatedStatesDropped++;
	}
}

std::shared_ptr<StateSimulations> StateSimulationsStorage::getStateSimulation(const State& state)
{
	std::lock_guard<std::mutex> guard(lock);
	return state_map.at(state);
}

void StateSimulationsStorage::clearNodes()
{
	std::lock_guard<std::mutex> guard(lock);
	state_map.clear();
}

void StateSimulationsStorage::trimNodes()
{
	std::lock_guard<std::mutex> guard(lock);
	for (auto it = state_map.begin(); it != state_map.end();)
	{
		StateSimulations& value = *it->second;
		if (value.getVisited()) // Keep only visited nodes
		{
			value.setVisited(false);
			++it;
		}
		else
		{
			it = state_map.erase(it);
		}
	}
}

#ifdef LOG_PERFORMANCE
std::mutex logLock;
int countPerformanceLog = 1000;
#endif // LOG_PERFORMANCE

///////////////////
// AlphaZeroMCTS //
///////////////////
void AlphaZeroMCTS::simulate(const State& state, std::shared_ptr<AlphaZeroNNId> nn)
{
	std::vector<std::thread> threads;

#ifdef LOG_PERFORMANCE
	auto startProcessing = std::chrono::high_resolution_clock::now();
#endif // LOG_PERFORMANCE

	setRootState(state, nn);

	int count = SETTINGS.MCTS_SIMULATIONS - (SETTINGS.MCTS_SIMULATIONS % SETTINGS.THREADS_PER_MCTS); // Make sure that count is multiple of number of threads, else it will cause deadlock at predictFuture()
	std::shared_ptr<Counter> c(new Counter);
	c->setCount(count);

	for (int i = 0; i < SETTINGS.THREADS_PER_MCTS; i++) // Spawn threads
	{
		threads.push_back(std::thread(&AlphaZeroMCTS::threadSimulateJob, this, state, nn, c));
	}
	for (int i = 0; i < SETTINGS.THREADS_PER_MCTS; i++) // Wait all threads to finish
	{
		threads[i].join();
	}

#ifdef LOG_PERFORMANCE
	auto endProcessing = std::chrono::high_resolution_clock::now();
	if (countPerformanceLog > 0)
	{
		std::lock_guard guard(logLock);
		countPerformanceLog--;
		LOG.getMCTSPerformanceLog() << std::chrono::duration_cast<std::chrono::nanoseconds>(endProcessing - startProcessing).count() << " ns\n";
	}
#endif // LOG_PERFORMANCE
}

void AlphaZeroMCTS::setRootState(const State& state, std::shared_ptr<AlphaZeroNNId> nn)
{
	store.trimNodes();
	if (!store.exist(state))
	{
		uint64_t validMoves = UtilityNN::getValidMoves(state);
		NNOutputData out = nn->predict(NNInputData(state));
		out.normalize(validMoves);

		std::shared_ptr<StateSimulations> ss_ptr(new StateSimulations(out, validMoves));
		store.add(state, ss_ptr);

		missCouter++;
	}
	else
	{
		hitCouter++;
	}
}


void AlphaZeroMCTS::threadSimulateJob(State state, std::shared_ptr<AlphaZeroNNId> nn, std::shared_ptr<Counter> c)
{
	nn->registerThread();
	while (c->hasNext()) 
	{
		State copyState = state;
		copyState.setLog(false);
		search(copyState, nn);
	}
	nn->unregisterThread();
}

float AlphaZeroMCTS::search(State& state, std::shared_ptr<AlphaZeroNNId> nn)
{
	int8_t gameStatus = state.gameStatus();
	if (gameStatus != State::NOT_ENDED)
	{
		if (gameStatus == State::DRAW)
		{
			return 0.0f;
		}
		state.logGameStatus();
		return gameStatus == state.getCurrentPlayerTurn() ? 1.0f : -1.0f;
	}

	uint64_t validMoves = UtilityNN::getValidMoves(state);
	if (validMoves == 0)
	{
		throw std::invalid_argument("Valid moves can not be 0");
	}

#ifdef _DEBUG
	if (state.getRoundPhase() == RoundPhase::ATTACK && Utility::popcount(validMoves) <= 1) // If only one 1 automaticaly pick it no simulation
	{
		throw std::invalid_argument("Valid moves can not be 1 or less");
	}
#endif // DEBUG	

	if (!store.exist(state))
	{
		std::future<NNOutputData> fout = nn->predictFuture(NNInputData(state)); // Queue thread for prediction
		NNOutputData out = fout.get();
		out.normalize(validMoves);

		std::shared_ptr<StateSimulations> ss_ptr(new StateSimulations(out, validMoves));
		store.add(state, ss_ptr);
		return out.value;
	}
	else
	{
		std::shared_ptr<StateSimulations> ss = store.getStateSimulation(state);
		LandIndex bestMove = ss->getNextBestMoveAndSetVisited();				

		int currentPlayer = state.getCurrentPlayerTurn();
		UtilityNN::makeMove(state, bestMove); // State changed
 		int nextMovePlayer = state.getCurrentPlayerTurn();

		float newValue = search(state, nn); // State can changed

		if (currentPlayer != nextMovePlayer) 
		{
			newValue = -newValue; // Players change swap value
		}
		ss->addValue(bestMove, newValue);

		return newValue;
	}
}

LandIndex AlphaZeroMCTS::pickRandomWeightedMove(const std::vector<float>& probs)
{
	float probSum = std::accumulate(probs.begin(), probs.end(), 0.0f);
	float randomA = probSum * RNG.rFloat();
	float iterSumProb = 0.0f;

	for (int i = 0; i < probs.size(); i++)
	{
		iterSumProb += probs[i];
		if (iterSumProb >= randomA)
		{
			return Utility::i2li(i);
		}				
	}

	throw std::invalid_argument("All probabilities can't be zero");
}

LandIndex AlphaZeroMCTS::pickHigestWeightedMove(const std::vector<float>& probs)
{	
	float bestProb = 0.0f;
	LandIndex bestLi = LandIndex::None;

	for (int i = 0; i < probs.size(); i++)
	{
		if (probs[i] > bestProb)
		{
			bestProb = probs[i];
			bestLi = Utility::i2li(i);
		}
	}

	return bestLi;
}

StateSimulationsStorage* AlphaZeroMCTS::getStorage()
{
	return &store;
}
