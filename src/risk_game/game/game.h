#pragma once

#include <stdio.h>
#include <thread>
#include <mutex>

#include "../player/alpha_zero/neural_network/alphazero_nn_data.h"
#include "../player/base/player.h"


struct PlayerGameResult
{
	int32_t win;
	int32_t winAndStartedGame;
};

class GameResults {
public:
	std::vector<PlayerGameResult> players;
	int draw;
	int count;

	void addGame(const State& s, uint8_t startingPlayer);
	void add(const GameResults& gr);

	GameResults() : draw(0), count(0), players(std::vector<PlayerGameResult>(PLAYER_COUNT)) {};
};

std::ostream& operator<<(std::ostream& os, const GameResults& dt);

class Counter  // Thread safe class
{
private:
	std::mutex lock;
	int count;
	int i;
	int finished;

	GameResults gr;
	bool showProgress = false;

public:
	Counter() : i(0), count(0), finished(0) {};

	bool hasNext();
	bool hasNext(int next);

	void hasFinished();
	void hasFinished(int finished);

	void setCount(int count);
	void setShowProgress(bool show);
	void addResults(const GameResults& gr);
};

class Game {

private:
	int playerStart = 0;
	std::vector<std::shared_ptr<Player>> players;

public:
	State state;
	State previousStartState;

public:
	Game();

	std::shared_ptr<Player> getPlayer(int index);
	void addPlayer(std::shared_ptr<Player> player);
	void incPlayerStart();

	int playTurn();
	int playThroughSetup();

	GameResults playGames(int numberOfTimes);
		
	void newGame();

private:
	int gameLoop();
	void brodcastGameState(int gameState, int roundCount);
};


class GameGroup
{
private:
	static void threadPlayGame(std::shared_ptr<Player> p1, std::shared_ptr<Player> p2, GameResults* gr, std::shared_ptr<Counter> c);

public:	
	static GameResults playGames(std::shared_ptr<PlayerGroup> pg1, std::shared_ptr<PlayerGroup> pg2, int games);
	static GameResults playGames(std::shared_ptr<PlayerGroup> pg1, std::shared_ptr<PlayerGroup> pg2, int games, NNTrainDataStorage& tds);
};