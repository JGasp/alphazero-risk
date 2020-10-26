#include "alphazero_moves.h"

uint64_t UtilityNN::getValidMoves(const State& state)
{
	const PlayerStatus* pls = state.getCurrentPlayerStatus();
	const PlayerStatus* epls = state.getEnemyPlayerStatus();

	switch (state.getRoundPhase())
	{
	case RoundPhase::SETUP:
	case RoundPhase::REINFORCEMENT:
	{
		uint64_t ownedLands = pls->ownedLands & ~pls->ownedFullLands;
		if (ownedLands == 0)
		{
			return Land::SKIP_MOVE_MASK;
		}
		else if (SETTINGS.LIMIT_REINFORCEMENT_MOVES)
		{
			uint64_t ownedLandsNeighbours = ownedLands & (epls->attackLands | state.getNeutralPlayerAttackLands());
			if (ownedLandsNeighbours != 0) // Only reinforce bordering lands with enemy or neutral forces
			{
				return ownedLandsNeighbours;
			}
			return ownedLands;
		}
		else
		{
			return ownedLands;
		}
	}
	case RoundPhase::SETUP_NEUTRAL:
		return LandSet::ALL_LANDS_MASK & ~pls->ownedLands & ~epls->ownedLands; // Neutral lands
	case RoundPhase::ATTACK:
	{
		if (SETTINGS.LIMIT_ATTACK_MOVES)
		{
			if (Utility::popcount(pls->attackLandsWithArmy) > 0) // Force attacking
			{
				return pls->attackLandsWithArmy;
			}
			else
			{
				return Land::SKIP_MOVE_MASK;
			}
		}
		else
		{
			return pls->attackLandsWithArmy | Land::SKIP_MOVE_MASK; // Add choice to end attack
		}
	}
	case RoundPhase::ATTACK_MOBILIZATION:
	{
		const Land* from = Land::getLand(state.getAttackMobilizationFrom());
		const Land* to = Land::getLand(state.getAttackMobilizationTo());
		return from->landIndexBitMask | to->landIndexBitMask;
	}
	case RoundPhase::FORTIFY:
		if (SETTINGS.LIMIT_REINFORCEMENT_MOVES)
		{
			return pls->ownedLands & epls->attackLands | Land::SKIP_MOVE_MASK;
		}
		else
		{
			return pls->ownedLands | Land::SKIP_MOVE_MASK;
		}
	default:
		break;
	}
}

void UtilityNN::makeMove(State& state, LandIndex li)
{
	if (li == LandIndex::None)
	{
		throw std::invalid_argument("Move was not picked");
	}

	if (li == LandIndex::Count) // End turn
	{
		switch (state.getRoundPhase())
		{
		case RoundPhase::REINFORCEMENT:
			state.gotoAttack(); break;
		case RoundPhase::ATTACK:
			state.gotoFortify(); break;
		case RoundPhase::FORTIFY: 
			state.nextPlayerGameTurn(); break;
		default:
			throw std::logic_error("Not possible to skip current round status");
		}
	}
	else
	{
		const PlayerStatus* pls = state.getCurrentPlayerStatus();
		if (state.getRoundPhase() == RoundPhase::SETUP)
		{
			state.setupReinforcementMove(li);
		}
		else if (state.getRoundPhase() == RoundPhase::SETUP_NEUTRAL)
		{
			state.setupReinforcementNeutralMove(li);
		}
		else if (state.getRoundPhase() == RoundPhase::REINFORCEMENT) // Reinforcement placement
		{
			GameHelper::playCards(state);

#if defined(FAST_ATTACK_MOBILIZATION)
			land_army_t reinforcement = state.getReinforcement() / 2;
			if (reinforcement < SETTINGS.MIN_UNIT_MOVE)
			{
				reinforcement = __MIN(SETTINGS.MIN_UNIT_MOVE, state.getReinforcement());
			}
#else
			land_army_t reinforcement = __MIN(SETTINGS.MIN_UNIT_MOVE, state.getReinforcement());
#endif
			land_army_t maxValue = state.getLandArmySpace(li);
			reinforcement = __MIN(maxValue, reinforcement);

			state.reinforcementMove(reinforcement, li);
		}
		else if (state.getRoundPhase() == RoundPhase::ATTACK) // Attacking land
		{
			const Land* l = Land::getLand(li);
			bool occupiedTerritory = false;

			land_army_t bestArmy = 0;
			LandIndex bestAttackFrom = LandIndex::None;

			for (int i = 0; i < l->neihboursLandIndex.size(); i++)
			{
				const Land* nl = Land::getLand(l->neihboursLandIndex[i]);
				if ((nl->landIndexBitMask & pls->ownedLandsWithArmy) > 0)
				{
					land_army_t attackArmy = state.getLandArmy(nl->landIndex).army - 1;
					if (attackArmy > bestArmy)
					{
						bestArmy = attackArmy;
						bestAttackFrom = nl->landIndex;
					}
				}
			}

			state.attackMove(bestAttackFrom, li);
		}
		else if (state.getRoundPhase() == RoundPhase::ATTACK_MOBILIZATION) // Move or don't move units
		{
			if (li == state.getAttackMobilizationFrom()) // End reinforcement
			{
				state.gotoAttack();
			}
			else if (li == state.getAttackMobilizationTo())
			{
				land_army_t value = state.getLandArmy(state.getAttackMobilizationFrom()).army - 1;
#if defined(FAST_ATTACK_MOBILIZATION)
				land_army_t reinforcement = value / 2;
				if (reinforcement < SETTINGS.MIN_UNIT_MOVE)
				{
					reinforcement = __MIN(SETTINGS.MIN_UNIT_MOVE, value);
				}
#else
				land_army_t reinforcement = __MIN(SETTINGS.MIN_UNIT_MOVE, value);
#endif				
				state.attackReinforcementMove(reinforcement);
			}
			else
			{
				throw std::invalid_argument("Attack mobilization to is invalid");
			}

		}
		else if (state.getRoundPhase() == RoundPhase::FORTIFY) // Priority move to land next to enemy, from land not next to enemy
		{
			const Land* landTo = Land::getLand(li);
			land_army_t value = state.getLandArmy(landTo->landIndex).army;
			if (value != LAND_ARMY_MAX)
			{
				GameHelper::PlayerMovement pm = GameHelper::PlayerMovement(state);
				for (int i = 0; i < pm.landSetMovements.size(); i++)
				{
					GameHelper::LandSetMovement lsm = pm.landSetMovements[i];
					if ((lsm.landSetBitMask & landTo->landIndexBitMask) > 0)
					{						
						land_army_t bestValueNoNeighbours = 0;
						LandIndex bestLandToMoveFromNoNeighbours = LandIndex::None;
						land_army_t bestValue = 0;
						LandIndex bestLandToMoveFrom = LandIndex::None;						

						for (int j = 0; j < lsm.landSet.size(); j++)
						{
							const Land* landFrom = lsm.landSet[j];
							if (landFrom->landIndex != landTo->landIndex)
							{
								land_army_t value = state.getLandArmy(landFrom->landIndex).army - 1;

								uint64_t ownedNeighbours = landFrom->neighboursLandIndexBitMask & pls->ownedLands;
								if (ownedNeighbours == landFrom->neighboursLandIndexBitMask) // All neihbours are owned lands
								{
									if (value > bestValueNoNeighbours)
									{
										bestValueNoNeighbours = value;
										bestLandToMoveFromNoNeighbours = landFrom->landIndex;
									}	
								}
								else
								{
									if (value > bestValue)
									{
										bestValue = value;
										bestLandToMoveFrom = landFrom->landIndex;
									}
								}								
							}
						}

						if (bestLandToMoveFromNoNeighbours != LandIndex::None)
						{
							bestLandToMoveFrom = bestLandToMoveFromNoNeighbours;
							bestValue = bestValueNoNeighbours;
						}
						if (bestLandToMoveFrom != LandIndex::None)
						{
							land_army_t maxValue = state.getLandArmySpace(li);
							state.fortifyMove(__MIN(maxValue, bestValue), bestLandToMoveFrom, li);
						}
						break;
					}
				}
			}
			state.nextPlayerGameTurn();
		}
	}
}
