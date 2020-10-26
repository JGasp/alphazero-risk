#pragma once

#include <stdint.h>

#include "../alpha_zero/neural_network/alphazero_nn_data.h"


class State;

class Player 
{
public:
	int8_t playerIndexTurn;
	NNTrainDataStorage* trainStorage = nullptr;

	Player() : playerIndexTurn(0) {};
public:
	void setPlayerIndex(int8_t playerIndexTurn);
	void setTrainStorage(NNTrainDataStorage* tds) { trainStorage = tds; };
	void addTrainingSample(const State& state, LandIndex move);


	virtual void newGame() {};
	virtual void takeTurn(State& game) {};
	virtual void gameFinished(int gameStatus, int roundCount) {};
};

class PlayerGroup
{
public:
	virtual size_t size() { return 0; }; // Overried
	virtual std::shared_ptr<Player> getPlayer(int index) { return std::shared_ptr<Player>(new Player); }; // Overried

	void setStorage(std::vector<NNTrainDataStorage>& storages);
	void clearStorage();
};