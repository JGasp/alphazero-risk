#pragma once

#include "../land/land_set.h"
#include "../../settings.h"
#include <xxhash/xxhash.h>

#include <stdint.h>

static constexpr int DATA_TERRITORY = static_cast<int>(LandIndex::Count);

static constexpr int DATA_PLAYER_DRAWS_CARD = 1;

static constexpr int PLAYER_COUNT = 2;
static constexpr int DATA_PLAYER_CARDS = sizeof(uint64_t) / sizeof(int8_t);
static constexpr int DATA_USED_CARDS = DATA_PLAYER_CARDS;
static constexpr int CARD_SETS_PLAYED = 1;

static constexpr int MAP_Y = 7;
static constexpr int MAP_X = 6;

typedef uint8_t land_army_t;
static constexpr land_army_t LAND_ARMY_MAX = 32; //64;

struct LandArmy
{
	land_army_t army : 6 = 0;
	uint8_t playerIndex : 2 = 2;

	bool operator==(const LandArmy& other) const
	{
		return army == other.army &&
			playerIndex == other.playerIndex;
	}
};

static constexpr int LandArmySize = sizeof(LandArmy);

static constexpr uint8_t NEUTRAL_PLAYER = 2;

class DiceRolls
{
public:
	uint8_t roll1;
	uint8_t roll2;
	uint8_t roll3;
	DiceRolls() : roll1(0), roll2(0), roll3(0) {}
};

enum class RoundPhase : uint8_t
{
	SETUP,
	SETUP_NEUTRAL,
	REINFORCEMENT,
	ATTACK,
	ATTACK_MOBILIZATION,
	FORTIFY
};

struct PlayerStatus
{
	uint64_t ownedLands: 48 = 0;
	uint64_t ownedLandsWithArmy: 48 = 0;
	uint64_t ownedFullLands: 48 = 0;
	uint64_t attackLands: 48 = 0;
	uint64_t attackLandsWithArmy: 48 = 0;	
	int16_t totalArmy = 0;

#ifdef STATE_SIMPLE_CARDS
	uint8_t playerCards = 0; // Just count cards as equals
#else
	uint64_t playerCards : 48 = 0; // Seperate cards
#endif

	bool operator==(const PlayerStatus& other) const
	{
		return ownedLands == other.ownedLands &&
			ownedLandsWithArmy == other.ownedLandsWithArmy &&
			ownedFullLands == other.ownedFullLands &&
			attackLands == other.attackLands &&
			attackLandsWithArmy == other.attackLandsWithArmy &&
			totalArmy == other.totalArmy &&
			playerCards == other.playerCards;
	}
};

struct Data
{
	LandArmy landArmy[DATA_TERRITORY];
	PlayerStatus playerStatus[PLAYER_COUNT];		
	uint16_t round = 1;
	int8_t currentPlayerTurn = 0;
	uint8_t cardSetsPlayed = 0;	
	uint8_t reinforcements = 0;
	RoundPhase roundPhase = RoundPhase::SETUP;
	LandIndex attackMobilizationFrom = LandIndex::None;
	LandIndex attackMobilizationTo = LandIndex::None;	
	bool playerAllowedDrawCard = false;
	uint8_t attacksDuringTurn = 0;

#ifdef STATE_SIMPLE_CARDS
	uint16_t drawnCardsBitMask = 0; // Just count cards as equals
#else
	uint64_t drawnCardsBitMask = 0; // Seperate cards
#endif
};

#define SAFE_COMPARE_OPERATION

class State
{
private:
	Data data;
	mutable size_t hash = 0;
	bool log = false;
	bool yield = false;

private:
	inline void resetHash();	

	static DiceRolls getDiceRolls(int rolls);
	void logStartingTurn();
public:
	static const int DRAW = -2;
	static const int NOT_ENDED = -1;

	State();	
	void newGame();

	bool equal(const State& other) const;
	bool equalFields(const State& other) const;

	bool operator==(const State& other) const
	{
		return getHash() == other.getHash() && equal(other);
	}
	size_t getHash() const;
	size_t getHashField() const;

	bool getLog();
	void setLog(bool log);

	void copy(State& state);

	void nextPlayerTurn();
	void nextPlayerSetupTurn();
	void nextPlayerGameTurn();	

	void drawCard();
	void setCurrentPlayerTurn(int8_t currentPlayerTurn);

	void setLandArmy(uint8_t landIndex, land_army_t value);
	void setLandArmy(uint8_t landIndex, land_army_t value, uint8_t playerIndex);
	
	bool isCurrentPlayerTurnDiffrent(State& other) const;
	int8_t calculateReinforcementValue() const;
	int8_t calculateReinforcementValue(uint64_t ownedLand) const;

	void invertPlayers();
public: // Player exposed methods
	int8_t gameStatus() const;
	void logGameStatus() const;

	const PlayerStatus* getPlayerStatus(uint8_t playerIndex) const;
	const PlayerStatus* getCurrentPlayerStatus() const;
	const PlayerStatus* getEnemyPlayerStatus() const;

	land_army_t getLandArmySpace(LandIndex landIndex) const;
	land_army_t getLandArmySpace(uint8_t landIndex) const;

	LandArmy getLandArmy(LandIndex landIndex) const;
	LandArmy getLandArmy(uint8_t landIndex) const;

	uint64_t getPlayerCards() const;
	uint8_t getCardSetsPlayed() const;
	bool getPlayerAllowedDrawCard() const;
	int8_t getCurrentPlayerTurn() const;
	int8_t getEnemyPlayerTurn() const;

	RoundPhase getRoundPhase() const;
	LandIndex getAttackMobilizationFrom() const;
	LandIndex getAttackMobilizationTo() const;

	land_army_t getReinforcement() const;
	uint8_t getAttacksDuringTurn() const;
	uint16_t getRound() const;
	
	void gotoSetupNeutral();

	void gotoAttack();
	void gotoFortify();

	void addLandArmy(LandIndex landIndex, land_army_t value, uint8_t playerIndex);
	void addLandArmy(uint8_t landIndex, land_army_t value, uint8_t playerIndex);

	void addLandArmy(LandIndex landIndex, land_army_t value);
	void addLandArmy(uint8_t landIndex, land_army_t value);	

	bool attackMove(LandIndex from, LandIndex to);
	void attackReinforcementMove(land_army_t amount);
	void fortifyMove(land_army_t amount, LandIndex from, LandIndex to);
	void reinforcementMove(land_army_t amount, LandIndex to);
	void cardReinforcementMove(LandIndex to);
	void setupReinforcementMove(LandIndex to);
	void setupReinforcementNeutralMove(LandIndex to);
	void setupLandOccupation(LandIndex to);		

	uint64_t getNeutralPlayerAttackLands() const; // Used to get lands from which we can attack neutral player
	
	const Data& getData() const;

#ifdef STATE_SIMPLE_CARDS
	void playCards();
#else
	void playCards(uint64_t cardsPlayed);
#endif
	void consistencyCheck();
	void consistencyCheckArmyValue();
};

namespace std {
	template <>
	struct hash<State>
	{
		std::size_t operator()(const State& k) const
		{
			return k.getHash();
		}
	};
}