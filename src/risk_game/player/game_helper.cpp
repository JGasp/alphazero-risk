#include "game_helper.h"

void GameHelper::playCards(State& state)
{
#ifdef STATE_SIMPLE_CARDS
	if (state.getPlayerCards() >= 3)
	{
		state.playCards();
	}
#else
	uint64_t cards = GameHelper::getBestCombo(state.getCurrentPlayerStatus());
	if (cards > 0)
	{
		state.playCards(cards);
	}
#endif
}

bool GameHelper::sortLandSet(GameHelper::LandSetPriority* i, GameHelper::LandSetPriority* j)
{
	if (i->notOwnedLands == j->notOwnedLands)
	{
		if (i->notOwnedAttackLands == j->notOwnedAttackLands)
		{
			return i->landSet->landSetIndexBitMask > j->landSet->landSetIndexBitMask;
		}
		else
		{
			return i->notOwnedAttackLands > j->notOwnedAttackLands;
		}
	}
	else
	{
		return i->notOwnedLands < j->notOwnedLands;
	}
}

GameHelper::LandSetPriority::LandSetPriority(const LandSet* lands) : landSet(lands), notOwnedLands(0), notOwnedAttackLands(0) {}

GameHelper::LandSetMovement::LandSetMovement()
{
	landSetBitMask = 0LL;

	landFortifyFrom = NULL;
	landFortifyFromAmount = 0;

	landFortifyTo = NULL;
	landFortifyToNeighbours = 0;
}

void GameHelper::LandSetMovement::add(const Land* l, const State& state, int64_t ownedLands)
{
	if ((l->landIndexBitMask & ownedLands & ~landSetBitMask) > 0) {
		landSetBitMask |= l->landIndexBitMask;
		landSet.push_back(l);

		uint64_t neighbourAttackingLand = ~ownedLands & l->neighboursLandIndexBitMask;
		if (neighbourAttackingLand == 0) // No attacking neighbours
		{			
			LandArmy la = state.getLandArmy(l->landIndex);
			if (la.army > landFortifyFromAmount)
			{
				landFortifyFrom = l;
				landFortifyFromAmount = la.army;
			}
		}
		else
		{
			uint8_t neighboursCount = Utility::popcount(neighbourAttackingLand);
			if (neighboursCount > landFortifyToNeighbours)
			{
				landFortifyToNeighbours = neighboursCount;
				landFortifyTo = l;
			}
		}

		for (int i = 0; i < l->neihboursLandIndex.size(); i++) {
			const Land* nl = Land::getLand(l->neihboursLandIndex[i]);
			add(nl, state, ownedLands);
		}
	}
}

bool sortLandSetMovement(const GameHelper::LandSetMovement& i, const GameHelper::LandSetMovement& j)
{
	return i.landFortifyFromAmount > j.landFortifyFromAmount;
}


GameHelper::PlayerMovement::PlayerMovement(const State& state)
{
	uint64_t ownedLands = state.getCurrentPlayerStatus()->ownedLands;

	landSetMovementBitMask = 0ULL;

	for (int i = 0; i < LAND_INDEX_SIZE; i++) {
		const Land* nl = Land::getLand(i);
		if ((nl->landIndexBitMask & ownedLands & ~landSetMovementBitMask) > 0)
		{
			LandSetMovement lsm = LandSetMovement();
			lsm.add(nl, state, ownedLands);

			landSetMovementBitMask |= lsm.landSetBitMask;
			landSetMovements.push_back(lsm);
		}
	}

	std::sort(landSetMovements.begin(), landSetMovements.end(), sortLandSetMovement);
}

void GameHelper::buildCombo(uint64_t cards, const PlayerStatus* pls, std::vector<CardCombo>& combos)
{
	if (Utility::popcount(cards) >= 3)
	{
		uint64_t owned = cards & pls->ownedLands;
		uint64_t notOwned = cards & ~pls->ownedLands;

		CardCombo cc;
		for (int i = cc.ownedLandCard; i < 3; i++)
		{
			if (owned > 0) // Pick cards that you own land first
			{
				uint64_t notUsedCard = Utility::getFirstBitMask(owned);
				cc.cardMask |= notUsedCard;
				cc.ownedLandCard++;
				owned &= ~notUsedCard;
			}
			else
			{
				uint64_t notUsedCard = Utility::getFirstBitMask(notOwned);
				cc.cardMask |= notUsedCard;
				notOwned &= ~notUsedCard;
			}

		}
		combos.push_back(std::move(cc));
	}
}

bool GameHelper::sortCombo(const CardCombo& c1, const CardCombo& c2)
{
	return c1.ownedLandCard > c2.ownedLandCard;
}

uint64_t GameHelper::getBestCombo(const PlayerStatus* pls)
{
	int cardsCount = Utility::popcount(pls->playerCards);
	if (cardsCount > 3)
	{
		std::vector<CardCombo> combos;

		uint64_t cInf = pls->playerCards & Land::CARD_INFANTRY_BITMASK;
		buildCombo(cInf, pls, combos);

		uint64_t cHorse = pls->playerCards & Land::CARD_HORSE_BITMASK;
		buildCombo(cHorse, pls, combos);

		uint64_t cSige = pls->playerCards & Land::CARD_SIGE_BITMASK;
		buildCombo(cSige, pls, combos);

		if (cInf > 0 && cHorse > 0 && cSige > 0) // Each of one
		{
			CardCombo cc;
			uint64_t ownedCinf = pls->ownedLands & cInf;
			if (ownedCinf > 0)
			{
				cc.cardMask |= Utility::getFirstBitMask(ownedCinf);
				cc.ownedLandCard++;
			}
			else
			{
				cc.cardMask |= Utility::getFirstBitMask(cInf);
			}

			uint64_t ownedCHorse = pls->ownedLands & cHorse;
			if (ownedCHorse > 0)
			{
				cc.cardMask |= Utility::getFirstBitMask(ownedCHorse);
				cc.ownedLandCard++;
			}
			else
			{
				cc.cardMask |= Utility::getFirstBitMask(cHorse);
			}

			uint64_t ownedCSige = pls->ownedLands & cSige;
			if (ownedCSige > 0)
			{
				cc.cardMask |= Utility::getFirstBitMask(ownedCSige);
				cc.ownedLandCard++;
			}
			else
			{
				cc.cardMask |= Utility::getFirstBitMask(cSige);
			}

			combos.push_back(std::move(cc));
		}

		if (combos.size() > 0)
		{
			std::sort(combos.begin(), combos.end(), sortCombo);
			return combos[0].cardMask;
		}
	}

	return 0ULL;
}

