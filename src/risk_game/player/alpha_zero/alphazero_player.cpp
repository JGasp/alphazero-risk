#include "alphazero_player.h"

void AlphaZeroPlayer::takeTurn(State& state)
{
	mcts.getStorage()->trimNodes();
	while (state.gameStatus() == -1 && state.getCurrentPlayerTurn() == playerIndexTurn)
	{
		mcts.simulate(state, this->nn);

		std::shared_ptr<StateSimulations> ss = mcts.getStorage()->getStateSimulation(state);
		std::vector<float> policy = ss->calculateMoveProbability(1.0f);
		LandIndex li = mcts.pickHigestWeightedMove(policy);

		if (trainStorage != nullptr)
		{
			trainStorage->data.push_back(NNTrainData(state.getCurrentPlayerTurn(), NNInputData(state), NNOutputData(std::move(policy))));
		}

		UtilityNN::makeMove(state, li);
	}
}

void AlphaZeroPlayer::gameFinished(int gameStatus, int roundCount)
{
	if (trainStorage != nullptr)
	{
		trainStorage->updateValues(gameStatus, roundCount);
	}
}

void AlphaZeroPlayer::newGame()
{
	mcts.getStorage()->clearNodes();
}

AlphaZeroPlayerGroup::AlphaZeroPlayerGroup(std::shared_ptr<AlphaZeroNNGroup> nnGroup)
{
	for (int i = 0; i < nnGroup->size(); i++)
	{
		for (int g = 0; g < SETTINGS.NUMBER_OF_CONCURENT_GAMES_PER_GPU; g++)
		{
			alphaZeroPlayers.push_back(std::shared_ptr<AlphaZeroPlayer>(new AlphaZeroPlayer(nnGroup->getNN(i))));
		}		
	}
}

size_t AlphaZeroPlayerGroup::size()
{
	return alphaZeroPlayers.size();
}

std::shared_ptr<Player> AlphaZeroPlayerGroup::getPlayer(int index)
{
	return alphaZeroPlayers[index];
}
