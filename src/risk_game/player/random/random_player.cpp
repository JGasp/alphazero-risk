#include "random_player.h"

uint64_t RandomPlayer::pickRandomMove(uint64_t availableMoves)
{
	int count = Utility::popcount(availableMoves);
	if (count == 0)
	{
		throw std::invalid_argument("No available moves");
	}

	int random = RNG.rInt() % count;
	uint64_t rMove = Utility::getFirstBitMask(availableMoves);;
	for (int i = 0; i < random; i++)
	{
		availableMoves &= ~rMove;
		rMove = Utility::getFirstBitMask(availableMoves);		
	}

	return rMove;
}

void RandomPlayer::takeTurn(State& state)
{
	while (state.gameStatus() == -1 && state.getCurrentPlayerTurn() == playerIndexTurn)
	{
		if (state.getRoundPhase() == RoundPhase::SETUP)
		{
			uint64_t randomMove = pickRandomMove(state.getCurrentPlayerStatus()->ownedLands);
			addTrainingSample(state, Utility::lm2li(randomMove));
			state.setupReinforcementMove(Utility::lm2li(randomMove));
		}
		else if (state.getRoundPhase() == RoundPhase::SETUP_NEUTRAL)
		{
			uint64_t randomMove = pickRandomMove(LandSet::ALL_LANDS_MASK & ~state.getCurrentPlayerStatus()->ownedLands & ~state.getEnemyPlayerStatus()->ownedLands);
			addTrainingSample(state, Utility::lm2li(randomMove));
			state.setupReinforcementNeutralMove(Utility::lm2li(randomMove));
		}
		else if (state.getRoundPhase() == RoundPhase::REINFORCEMENT)
		{
			GameHelper::playCards(state);

			uint64_t randomMove = pickRandomMove(state.getCurrentPlayerStatus()->ownedLands & ~state.getCurrentPlayerStatus()->ownedFullLands);
			addTrainingSample(state, Utility::lm2li(randomMove));
			state.reinforcementMove(1, Utility::lm2li(randomMove));
		}
		else if (state.getRoundPhase() == RoundPhase::ATTACK)
		{
			uint64_t randomMove = pickRandomMove(state.getCurrentPlayerStatus()->attackLandsWithArmy | Land::SKIP_MOVE_MASK);
			addTrainingSample(state, Utility::lm2li(randomMove));
			if ((Land::SKIP_MOVE_MASK & randomMove) > 0)
			{
				state.gotoFortify();
			}
			else
			{
				LandIndex attackLandTo = Utility::lm2li(randomMove);
				const Land* l = Land::getLand(attackLandTo);
				uint64_t randomAttackLandFrom = pickRandomMove(l->neighboursLandIndexBitMask & state.getCurrentPlayerStatus()->ownedLandsWithArmy);
				LandIndex attackLandFrom = Utility::lm2li(randomAttackLandFrom);
				state.attackMove(attackLandFrom, attackLandTo);
			}
		}
		else if (state.getRoundPhase() == RoundPhase::ATTACK_MOBILIZATION)
		{
			if (RNG.rFloat() > 0.5f)
			{
				land_army_t amount = __MIN(state.getLandArmy(state.getAttackMobilizationFrom()).army - 1, SETTINGS.MIN_UNIT_MOVE);				
				addTrainingSample(state, state.getAttackMobilizationTo());
				state.attackReinforcementMove(amount);
			}
			else
			{
				addTrainingSample(state, state.getAttackMobilizationFrom());
				state.gotoAttack();
			}
		}
		else if (state.getRoundPhase() == RoundPhase::FORTIFY)
		{
			uint64_t randomMoveTo = pickRandomMove(state.getCurrentPlayerStatus()->ownedLands & ~state.getCurrentPlayerStatus()->ownedFullLands | Land::SKIP_MOVE_MASK);
			LandIndex liTo = Utility::lm2li(randomMoveTo);

			addTrainingSample(state, Utility::lm2li(randomMoveTo));
			if (randomMoveTo != Land::SKIP_MOVE_MASK)
			{
				GameHelper::PlayerMovement pm = GameHelper::PlayerMovement(state);

				for (auto lsm : pm.landSetMovements)
				{
					if ((lsm.landSetBitMask & randomMoveTo) > 0)
					{
						uint64_t setLandWithArmy = lsm.landSetBitMask & ~randomMoveTo & state.getCurrentPlayerStatus()->ownedLandsWithArmy;
						if (setLandWithArmy > 0)
						{
							uint64_t randomMoveFrom = pickRandomMove(setLandWithArmy);
							LandIndex liFrom = Utility::lm2li(randomMoveFrom);

							land_army_t amount = state.getLandArmy(liFrom).army - 1;
							land_army_t maxAmount = state.getLandArmySpace(liTo);
							amount = __MIN(maxAmount, amount);
							uint64_t randomAmount = RNG.rInt() % amount;

							state.fortifyMove(randomAmount, liFrom, liTo);
						}						
						break;
					}
				}
			}
			state.nextPlayerGameTurn();
		}
	}
}

void RandomPlayer::gameFinished(int gameStatus, int roundCount)
{
	if (trainStorage != nullptr)
	{
		trainStorage->updateValues(gameStatus, roundCount);
	}
}

RandomPlayerGroup::RandomPlayerGroup(int size)
{
	for (int i = 0; i < size; i++)
	{
		randomPlayers.push_back(std::shared_ptr<RandomPlayer>(new RandomPlayer));
	}
}

size_t RandomPlayerGroup::size()
{
	return randomPlayers.size();
}

std::shared_ptr<Player> RandomPlayerGroup::getPlayer(int index)
{
	return randomPlayers[index];
}
