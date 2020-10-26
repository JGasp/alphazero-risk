#include "player.h"
#include "../../game/game.h"

void Player::setPlayerIndex(int8_t playerIndexTurn)
{
	this->playerIndexTurn = playerIndexTurn;
}

void Player::addTrainingSample(const State& state, LandIndex move)
{
	if (trainStorage != nullptr)
	{
		std::vector<float> policy(TF_OUTPUT_POLICY_TENSOR_SIZE);
		policy[Utility::li2i(move)] = 1.0f;
		trainStorage->data.push_back(NNTrainData(state.getCurrentPlayerTurn(), NNInputData(state), NNOutputData(std::move(policy))));
	}
}

void PlayerGroup::setStorage(std::vector<NNTrainDataStorage>& storages)
{
	for (int i = 0; i < size(); i++)
	{
		auto p = getPlayer(i);
		storages.push_back(NNTrainDataStorage());
		p->setTrainStorage(&storages[storages.size()-1]);
	}
}

void PlayerGroup::clearStorage()
{
	for (int i=0; i<size(); i++)
	{
		auto p = getPlayer(i);
		p->setTrainStorage(nullptr);
	}
}