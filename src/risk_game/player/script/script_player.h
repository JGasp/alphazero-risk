#pragma once

#include <algorithm>

#include "../base/player.h"
#include "../game_helper.h"
#include "../../game/game.h"
#include "../alpha_zero/neural_network/alphazero_nn_data.h"

class ScriptPlayer : public Player
{
private:
	const PlayerStatus* playerStatus;

	std::vector<GameHelper::LandSetPriority*> attackLandSetPriority;
	uint64_t ownedAttackLandBitMask;
	uint64_t attackLandBitMask;

	GameHelper::LandSetPriority* attackingLandSet;
	const Land* landAttackTo;
	const Land* landAttackFrom;
	land_army_t attackFromArmy;

public:
	ScriptPlayer();
	void takeTurn(State& game) override;
	void gameFinished(int gameStatus, int roundCount) override;

	static std::vector<std::shared_ptr<ScriptPlayer>> buildGroup(int size);
	static std::vector<std::shared_ptr<Player>> wrapGroup(std::vector<std::shared_ptr<ScriptPlayer>> scPlayers);

private:
	void updateAttackLandSetPriority(State& game);
	void updateAttackLandSet(State& game);
	void updateAttackLandTo(State& game);
	void updateAttackLandFrom(State& game);
	void attackLand(State& game);
	void fortify(State& game);
};

class ScriptPlayerGroup : public PlayerGroup
{
private:
	std::vector<std::shared_ptr<ScriptPlayer>> scriptPlayers;

public:
	ScriptPlayerGroup(int size);

	size_t size() override;
	std::shared_ptr<Player> getPlayer(int index) override;
};
