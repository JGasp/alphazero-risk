#pragma once

#include "../base/player.h"
#include "../game_helper.h"
#include "../../game/game.h"
#include "../game_helper.h"
#include "../alpha_zero/neural_network/alphazero_nn_data.h"


class RandomPlayer : public Player
{
private:
	uint64_t pickRandomMove(uint64_t availableMoves);
public:
	void takeTurn(State& game) override;
	void gameFinished(int gameStatus, int roundCount) override;
};


class RandomPlayerGroup : public PlayerGroup
{
private:
	std::vector<std::shared_ptr<RandomPlayer>> randomPlayers;

public:
	RandomPlayerGroup(int size);

	size_t size() override;
	std::shared_ptr<Player> getPlayer(int index) override;	
};