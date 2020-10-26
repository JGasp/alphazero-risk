#pragma once

#include <algorithm>
#include <mutex>

#include "base/player.h"
#include "../game/game.h"

namespace GameHelper
{
	class LandSetPriority
	{
	public:
		const LandSet* landSet;
		uint8_t notOwnedLands;
		uint8_t notOwnedAttackLands;

		LandSetPriority(const LandSet* set);
	};

	class LandSetMovement
	{
	public:
		uint64_t landSetBitMask;
		std::vector<const Land*> landSet;

		const Land* landFortifyFrom;
		land_army_t landFortifyFromAmount;

		const Land* landFortifyTo;
		uint8_t landFortifyToNeighbours;

		LandSetMovement();
	public:
		void add(const Land* l, const State& game, int64_t ownedLands);
	};

	class PlayerMovement
	{
	public:
		uint64_t landSetMovementBitMask;
		std::vector<LandSetMovement> landSetMovements;

	public:
		PlayerMovement(const State& state);
	};

	struct CardCombo
	{
		int ownedLandCard;
		uint64_t cardMask;
		CardCombo() : ownedLandCard(0), cardMask(0ULL) {};
	};

	bool sortLandSet(LandSetPriority* i, LandSetPriority* j);

	void buildCombo(uint64_t cards, const PlayerStatus* pls, std::vector<CardCombo>& combos);
	bool sortCombo(const CardCombo& c1, const CardCombo& c2);
	uint64_t getBestCombo(const PlayerStatus* pls);

	void playCards(State& state);
}