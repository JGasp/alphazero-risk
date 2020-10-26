#pragma once

#include "../base/player.h"
#include "alphazero_mcts.h"


class AlphaZeroPlayer : public Player 
{
private:
	std::shared_ptr<AlphaZeroNNId> nn;
	AlphaZeroMCTS mcts;

public:
	AlphaZeroPlayer(std::shared_ptr<AlphaZeroNNId> nnId) : nn(nnId) {};

	void takeTurn(State& game) override;
	void gameFinished(int gameStatus, int roundCount) override;
	void newGame() override;
};


class AlphaZeroPlayerGroup : public PlayerGroup
{
private:
	std::vector<std::shared_ptr<AlphaZeroPlayer>> alphaZeroPlayers;

public:
	AlphaZeroPlayerGroup(std::shared_ptr<AlphaZeroNNGroup> nnGroup);

	size_t size() override;
	std::shared_ptr<Player> getPlayer(int index) override;
};