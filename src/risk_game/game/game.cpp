#include "game.h"
#include "../player/base/player.h"

/////////////
// Counter //
/////////////
bool Counter::hasNext()
{
	return hasNext(1);
}

bool Counter::hasNext(int next)
{
	std::lock_guard<std::mutex> guard(lock);
	auto id = std::this_thread::get_id();
	if (i + next <= count)
	{
		i += next;
		return true;;
	}

	return false;
}

void Counter::hasFinished()
{
	hasFinished(1);
}

void Counter::hasFinished(int finished)
{
	std::lock_guard<std::mutex> guard(lock);
	this->finished += finished;

	if (showProgress && count > 0)
	{
		UtilityFormat::printProgress(this->finished, count);
		if (gr.count > 0)
		{
			std::cout << " [Draw/P1,P2]: " << gr;

			if (this->finished == count)
			{
				std::cout << std::endl;
			}
		}
	}
}

void Counter::setCount(int c)
{
	std::lock_guard<std::mutex> guard(lock);
	count = c;
	i = 0;
	finished = 0;
}

void Counter::setShowProgress(bool show)
{
	showProgress = show;
	if (showProgress && count > 0)
	{
		UtilityFormat::printProgress(i, count);
	}
}

void Counter::addResults(const GameResults& gr)
{
	this->gr.add(gr);
}


//////////
// Game //
//////////
Game::Game()
{
	this->players = {};
}

std::shared_ptr<Player> Game::getPlayer(int index)
{
	return players[index];
}

void Game::addPlayer(std::shared_ptr<Player> player)
{
	player->playerIndexTurn = players.size();
	players.push_back(player);
}

void Game::incPlayerStart()
{
	playerStart++;
	if (playerStart >= players.size())
	{
		playerStart = 0;
	}
}

int Game::gameLoop()
{
	int gameStatus = State::NOT_ENDED;
	while (gameStatus == State::NOT_ENDED) {
		gameStatus = playTurn();
	}
	state.logGameStatus();

	return gameStatus;
}

int Game::playTurn()
{
	int8_t currentPlayer = state.getCurrentPlayerTurn();
	std::shared_ptr<Player>& p = players[currentPlayer];

	if (state.getRoundPhase() == RoundPhase::SETUP)
	{		
		p->takeTurn(state);
		if (currentPlayer == state.getCurrentPlayerTurn()) { throw std::logic_error("Turn was not incremented"); }

		return State::NOT_ENDED;
	}
	else
	{
		p->takeTurn(state);

		int gameStatus = state.gameStatus();
		if (currentPlayer == state.getCurrentPlayerTurn() && gameStatus == State::NOT_ENDED) { throw std::logic_error("Turn was not incremented");}

		return gameStatus;
	}	
}

int Game::playThroughSetup()
{
	while (state.getRoundPhase() != RoundPhase::SETUP)
	{
		playTurn();		
	}

	return -1;
}

void Game::brodcastGameState(int gameStatus, int roundCount)
{
	for (auto p : players)
	{
		p->gameFinished(gameStatus, roundCount);
	}
}

GameResults Game::playGames(int numberOfTimes)
{
	GameResults gr;
		
	for (int i = 0; i < numberOfTimes; i++) 
	{
		newGame();
		int gameStatus = gameLoop();
		brodcastGameState(gameStatus, state.getRound());
		gr.addGame(state, playerStart);

		incPlayerStart();		
	}

	return gr;
}

void Game::newGame()
{
	if (SETTINGS.MIRROR_GAMES && playerStart != 0)
	{
		state = previousStartState;		
		state.invertPlayers();
		state.setCurrentPlayerTurn(playerStart);
	}
	else
	{
		state = State();
		state.setLog(SETTINGS.LOG_STATE);
		state.newGame();
		state.setCurrentPlayerTurn(playerStart);

		previousStartState = state;
	}
	for (auto p : players)
	{
		p->newGame();
	}
}

void GameResults::addGame(const State& s, uint8_t startingPlayer)
{
	count++;
	if (s.gameStatus() == -2)
	{
		draw++;
	}
	for (int i=0; i<players.size(); i++)
	{
		PlayerGameResult& pgr = players[i];
		if (s.gameStatus() == i)
		{
			pgr.win++;

			if (startingPlayer == i)
			{
				pgr.winAndStartedGame++;
			}
		}				
	}
}

void GameResults::add(const GameResults& gr)
{
	draw += gr.draw;
	count += gr.count;

	for (int i = 0; i < players.size(); i++)
	{
		players[i].win += gr.players[i].win;
		players[i].winAndStartedGame += gr.players[i].winAndStartedGame;
	}
}

std::ostream& operator<<(std::ostream& os, const GameResults& dt)
{	
	os << dt.draw;
	for (auto& e : dt.players)
	{
		os << ", " << e.win << '/' << e.winAndStartedGame;
	}
	return os;
}


void GameGroup::threadPlayGame(std::shared_ptr<Player> p1, std::shared_ptr<Player> p2, GameResults* gr, std::shared_ptr<Counter> c)
{
	Game game = Game();
	game.addPlayer(p1);
	game.addPlayer(p2);

	while (c->hasNext(2))
	{
		for (int i = 0; i < 2; i++)
		{
			GameResults singelGr = game.playGames(1);
			gr->add(singelGr);
			c->addResults(singelGr);
			c->hasFinished();
		}
	}
}

GameResults GameGroup::playGames(std::shared_ptr<PlayerGroup> pg1, std::shared_ptr<PlayerGroup> pg2, int games, NNTrainDataStorage& tds)
{
	std::vector<NNTrainDataStorage> storages;
	storages.reserve(pg1->size() + pg2->size());

	pg1->setStorage(storages);
	pg2->setStorage(storages);

	GameResults gr = playGames(pg1, pg2, games);

	for (auto& s : storages)
	{
		tds.extend(s);
	}

	pg1->clearStorage();
	pg2->clearStorage();

	return gr;
}

GameResults GameGroup::playGames(std::shared_ptr<PlayerGroup> pg1, std::shared_ptr<PlayerGroup> pg2, int games)
{
	int groupSize = pg1->size();

	std::vector<std::thread> threads; threads.reserve(groupSize);
	std::vector<GameResults> gameResultsGroup; gameResultsGroup.resize(groupSize);

	printf("Playing games %d\n", games);
	std::shared_ptr<Counter> c(new Counter);
	c->setCount(games);
	c->setShowProgress(true);

	for (int i = 0; i < groupSize; i++) // Spawn threads
	{
		std::shared_ptr<Player> p1 = pg1->getPlayer(i);
		std::shared_ptr<Player> p2 = pg2->getPlayer(i);
		GameResults* gr = &gameResultsGroup[i];

		threads.push_back(std::thread(&GameGroup::threadPlayGame, p1, p2, gr, c));
	}

	for (int i = 0; i < groupSize; i++) // Wait all threads to finish
	{
		threads[i].join();
	}

	GameResults allGr;
	for (auto& gr : gameResultsGroup)
	{
		allGr.add(gr);
	}

	return allGr;	

	return GameResults();
}
