#include "script_player.h"

/*
	Stretegy:
	- Prioritize attacking lands that are part of set where there are least amount of not yet conqured
	- On land capture alwasy move all units, attack and defend with max number possible
	- Attack till you can
	- Play reinforcement card as soon as possible
*/
ScriptPlayer::ScriptPlayer()
{
	attackLandSetPriority = { new GameHelper::LandSetPriority(&LandSet::ASIA), new GameHelper::LandSetPriority(&LandSet::NORTH_AMERICA),
		new GameHelper::LandSetPriority(&LandSet::SOUTH_AMERICA), new GameHelper::LandSetPriority(&LandSet::EUROPE),
		new GameHelper::LandSetPriority(&LandSet::AFRICA), new GameHelper::LandSetPriority(&LandSet::AUSTRALIA) };
}

void ScriptPlayer::updateAttackLandSetPriority(State& state)
{
	for (int i = 0; i < attackLandSetPriority.size(); i++) {
		uint64_t notOwnedLandsMask = attackLandSetPriority[i]->landSet->landSetIndexBitMask & ~playerStatus->ownedLands;
		attackLandSetPriority[i]->notOwnedLands = Utility::popcount(notOwnedLandsMask);
		attackLandSetPriority[i]->notOwnedAttackLands = Utility::popcount(notOwnedLandsMask & attackLandBitMask); // Lands that we can attack
	}
	std::sort(attackLandSetPriority.begin(), attackLandSetPriority.end(), GameHelper::sortLandSet);
}

void ScriptPlayer::updateAttackLandSet(State& state)
{
	for (int i = 0; i < attackLandSetPriority.size(); i++)
	{
		if (attackLandSetPriority[i]->notOwnedAttackLands > 0)
		{
			attackingLandSet = attackLandSetPriority[i];
			break;
		}
	}
}

void ScriptPlayer::updateAttackLandTo(State& state)
{
	for (int i = 0; i < attackingLandSet->landSet->lands.size(); i++)
	{
		const Land* l = attackingLandSet->landSet->lands[i];
		if ((l->landIndexBitMask & attackLandBitMask) > 0)
		{
			landAttackTo = l;
			break;
		}
	}
}

void ScriptPlayer::updateAttackLandFrom(State& state)
{
	attackFromArmy = 0;
	for (int i = 0; i < landAttackTo->neihboursLandIndex.size(); i++)
	{
		LandIndex nl = landAttackTo->neihboursLandIndex[i];
		const Land* alFrom = Land::getLand(nl);

		if (alFrom->landIndexBitMask & ownedAttackLandBitMask)
		{
			LandArmy la = state.getLandArmy(alFrom->landIndex);
			if (la.army > attackFromArmy) {
				attackFromArmy = la.army;
				landAttackFrom = alFrom;
			}
		}
	}
}

void ScriptPlayer::attackLand(State& state)
{
	while (state.getReinforcement() > 0)
	{
		uint64_t ownedNotFullLands = playerStatus->ownedLands & ~playerStatus->ownedFullLands;

		LandIndex reinforcmentTo = landAttackFrom->landIndex;
		if ((landAttackFrom->landIndexBitMask & ownedNotFullLands) == 0) // Current attack from land has max army
		{
			uint64_t neighboursAttackTo = landAttackTo->neighboursLandIndexBitMask & ownedNotFullLands; // Owned neighbors for the attack land to
			if (neighboursAttackTo > 0) // Pick not full neighbor from attack to land
			{
				reinforcmentTo = Utility::lm2li(Utility::getFirstBitMask(neighboursAttackTo));
			}
			else // Pick some other lands that are next to enemy
			{
				neighboursAttackTo = ownedNotFullLands & (state.getEnemyPlayerStatus()->attackLands | state.getNeutralPlayerAttackLands());
				if (neighboursAttackTo > 0)
				{
					reinforcmentTo = Utility::lm2li(Utility::getFirstBitMask(neighboursAttackTo));
				}
				else // Pick first owned not full land
				{
					reinforcmentTo = Utility::lm2li(Utility::getFirstBitMask(ownedNotFullLands));
				}
			}
		}				

		land_army_t maxReinforcement = state.getLandArmySpace(reinforcmentTo);
		land_army_t reinforcement = __MIN(maxReinforcement, state.getReinforcement());
		while (reinforcement > 0)
		{
			land_army_t stepRef = __MIN(reinforcement, SETTINGS.MIN_UNIT_MOVE);

			addTrainingSample(state, reinforcmentTo);

			state.reinforcementMove(stepRef, reinforcmentTo);
			reinforcement -= stepRef;
		}
	}
	attackFromArmy = state.getLandArmy(landAttackFrom->landIndex).army; // Update army if changed
		
	while (attackFromArmy > 1)
	{
		addTrainingSample(state, landAttackTo->landIndex);

		bool landCaptured = state.attackMove(landAttackFrom->landIndex, landAttackTo->landIndex);
		attackFromArmy = state.getLandArmy(landAttackFrom->landIndex).army;

		if (landCaptured && attackFromArmy > 1) // If captured land, move rest of army to new land
		{
			land_army_t maxMove = attackFromArmy - 1;
			while (maxMove > 0)
			{
				addTrainingSample(state, landAttackTo->landIndex);

				land_army_t step = __MIN(maxMove, SETTINGS.MIN_UNIT_MOVE);
				maxMove -= step;

				state.attackReinforcementMove(step);
			}
			break;
		}
	}
}

// Mobilizaion, move units to lands to where they can attack most lands
void ScriptPlayer::fortify(State& state)
{	
	if (Utility::popcount(playerStatus->ownedLandsWithArmy) > 0)
	{
		GameHelper::PlayerMovement pm = GameHelper::PlayerMovement(state);
		GameHelper::LandSetMovement lsm = pm.landSetMovements[0];

		if (lsm.landFortifyFromAmount > 0 && lsm.landFortifyTo != NULL)
		{
			land_army_t amount = state.getLandArmy(lsm.landFortifyFrom->landIndex).army - 1;
			land_army_t maxAmount = state.getLandArmySpace(lsm.landFortifyTo->landIndex);
			amount = __MIN(amount, maxAmount);
			
			addTrainingSample(state, lsm.landFortifyTo->landIndex);

			state.fortifyMove(amount, lsm.landFortifyFrom->landIndex, lsm.landFortifyTo->landIndex);
		}
		else
		{
			addTrainingSample(state, Land::SKIP_MOVE);
		}
	}	
}

void ScriptPlayer::takeTurn(State& state)
{	
	if (state.getRoundPhase() == RoundPhase::SETUP)
	{
		playerStatus = state.getCurrentPlayerStatus();
		ownedAttackLandBitMask = playerStatus->ownedLands;
		attackLandBitMask = playerStatus->attackLands;

		updateAttackLandSetPriority(state);
		updateAttackLandSet(state);
		updateAttackLandTo(state);
		updateAttackLandFrom(state);

		{			
			addTrainingSample(state, landAttackFrom->landIndex);
			state.setupReinforcementMove(landAttackFrom->landIndex);
		}

		{
			uint64_t neutralLands = LandSet::ALL_LANDS_MASK & ~state.getCurrentPlayerStatus()->ownedLands & ~state.getEnemyPlayerStatus()->ownedLands;
			uint64_t neutralLandsNextToEnemy = neutralLands & state.getEnemyPlayerStatus()->attackLands & ~state.getCurrentPlayerStatus()->attackLands; // Neutrala lands bordering on enemy lands and none frendly land
			if (neutralLandsNextToEnemy == 0)
			{
				neutralLandsNextToEnemy = neutralLands & state.getEnemyPlayerStatus()->attackLands; // Neutrala lands bordering on enemy lands
			}

			uint64_t neutralLand = 0ULL;
			if (Utility::popcount(neutralLandsNextToEnemy) > 0) // Prefer to place neutral army next to enemy player
			{
				neutralLand = Utility::randomMask(neutralLandsNextToEnemy);
			}
			else
			{
				neutralLand = Utility::randomMask(neutralLands);
			}

			addTrainingSample(state, Utility::lm2li(neutralLand));
			state.setupReinforcementNeutralMove(Utility::lm2li(neutralLand));
		}
	}
	else
	{
		playerStatus = state.getCurrentPlayerStatus();

		ownedAttackLandBitMask = playerStatus->ownedLands;
		attackLandBitMask = playerStatus->attackLands;

		GameHelper::playCards(state);

		while (attackLandBitMask > 0 || state.getReinforcement() > 0) // Attacking
		{
			updateAttackLandSetPriority(state);
			updateAttackLandSet(state);
			updateAttackLandTo(state);
			updateAttackLandFrom(state);

			attackLand(state);

			ownedAttackLandBitMask = playerStatus->ownedLandsWithArmy;
			attackLandBitMask = playerStatus->attackLandsWithArmy;
		}

		fortify(state);
		state.nextPlayerGameTurn();
	}			
}

void ScriptPlayer::gameFinished(int gameStatus, int roundCount)
{
	if (trainStorage != nullptr)
	{
		trainStorage->updateValues(gameStatus, roundCount);
	}
}

std::vector<std::shared_ptr<ScriptPlayer>> ScriptPlayer::buildGroup(int size)
{
	std::vector<std::shared_ptr<ScriptPlayer>> group(size);

	for (int i = 0; i < size; i++)
	{
		group[i] = std::shared_ptr<ScriptPlayer>(new ScriptPlayer);
	}

	return group;
}

std::vector<std::shared_ptr<Player>> ScriptPlayer::wrapGroup(std::vector<std::shared_ptr<ScriptPlayer>> scPlayers)
{
	std::vector<std::shared_ptr<Player>> players;

	for (auto& scp : scPlayers)
	{
		players.push_back(scp);
	}

	return players;
}

ScriptPlayerGroup::ScriptPlayerGroup(int size)
{
	for (int i = 0; i < size; i++)
	{
		scriptPlayers.push_back(std::shared_ptr<ScriptPlayer>(new ScriptPlayer()));
	}
}

size_t ScriptPlayerGroup::size()
{
	return scriptPlayers.size();
}

std::shared_ptr<Player> ScriptPlayerGroup::getPlayer(int index)
{
	return scriptPlayers[index];
}
