#include "state.h"

State::State()
{
#ifndef SAFE_COMPARE_OPERATION
    memset(&data, 0, sizeof(data));
    data = Data();
#endif
}

void State::gotoSetupNeutral()
{
    resetHash();

    //if (log) printf("[Player %d] Entering SETUP_NEUTRAL\n", getCurrentPlayerTurn());
    if (data.roundPhase != RoundPhase::SETUP) { throw std::invalid_argument("To enter SETUP_NEUTRAL round player must be in SETUP round"); }
    data.roundPhase = RoundPhase::SETUP_NEUTRAL;
}

void State::gotoAttack()
{   
    resetHash();

    if (log) printf("[Player %d] Entering ATTACK, attack lands with army %d\n", getCurrentPlayerTurn(), Utility::popcount(getCurrentPlayerStatus()->attackLandsWithArmy));
    if (data.roundPhase != RoundPhase::REINFORCEMENT && data.roundPhase != RoundPhase::ATTACK_MOBILIZATION){ throw std::invalid_argument("To enter ATTACK round player must be in REINFORCEMENT round"); }
    data.roundPhase = RoundPhase::ATTACK;
    data.attackMobilizationFrom = LandIndex::None;
    data.attackMobilizationTo = LandIndex::None;

    if (data.reinforcements > 0)
    {
        if (log) printf("[Player %d] Can not place reinforcement %d\n", getCurrentPlayerTurn(), data.reinforcements);
        data.reinforcements = 0;
    }

    if (getCurrentPlayerStatus()->attackLandsWithArmy == 0) // Skip attack phase if player can not attack
    {
        gotoFortify();
    }
}

void State::gotoFortify()
{
    resetHash();

    if (log) printf("[Player %d] Entering FORTIFY, attack lands with army %d\n", getCurrentPlayerTurn(), Utility::popcount(getCurrentPlayerStatus()->attackLandsWithArmy));
    if (data.roundPhase != RoundPhase::ATTACK) { throw std::invalid_argument("To enter FORTIFY round player must be in ATTACK round"); }
    data.roundPhase = RoundPhase::FORTIFY;
}

inline void State::resetHash()
{
    hash = 0;
}

size_t State::getHash() const
{
#ifdef SAFE_COMPARE_OPERATION
    return getHashField();
#else
    if (hash == 0)
    {
        hash = XXH64(&data, sizeof(data), 31);
    }
    return hash;
#endif // SAFE_COMPARE_OPERATION
}

size_t State::getHashField() const
{
    if (hash == 0)
    {
        XXH64_state_t* const state = XXH64_createState();
        XXH64_reset(state, 31);

        XXH64_update(state, &data.attackMobilizationFrom, sizeof(data.attackMobilizationFrom));
        XXH64_update(state, &data.attackMobilizationTo, sizeof(data.attackMobilizationTo));
        XXH64_update(state, &data.cardSetsPlayed, sizeof(data.cardSetsPlayed));
        XXH64_update(state, &data.currentPlayerTurn, sizeof(data.currentPlayerTurn));
        XXH64_update(state, &data.drawnCardsBitMask, sizeof(data.drawnCardsBitMask));        
        XXH64_update(state, &data.reinforcements, sizeof(data.reinforcements));
        XXH64_update(state, &data.round, sizeof(data.round));
        XXH64_update(state, &data.roundPhase, sizeof(data.roundPhase));
        XXH64_update(state, &data.playerAllowedDrawCard, sizeof(data.playerAllowedDrawCard));
        XXH64_update(state, &data.attacksDuringTurn, sizeof(data.attacksDuringTurn));

        for (int i = 0; i < PLAYER_COUNT; i++)
        {
            XXH64_update(state, &data.playerStatus[i], sizeof(PlayerStatus));
        }

        for (int i = 0; i < LAND_INDEX_SIZE; i++)
        {
            XXH64_update(state, &data.landArmy[i], sizeof(LandArmy));
        }

        hash = XXH64_digest(state);
    }
    return hash;
}

bool State::equal(const State& other) const
{
#ifdef SAFE_COMPARE_OPERATION
    return equalFields(other);
#else
    return memcmp(&data, &other.data, sizeof(data)) == 0;
#endif // SAFE_COMPARE_OPERATION    
}

bool State::equalFields(const State& other) const
{
    if (data.attackMobilizationFrom != other.data.attackMobilizationFrom) return false;
    if (data.attackMobilizationTo != other.data.attackMobilizationTo) return false;
    if (data.cardSetsPlayed != other.data.cardSetsPlayed) return false;
    if (data.currentPlayerTurn != other.data.currentPlayerTurn) return false;
    if (data.drawnCardsBitMask != other.data.drawnCardsBitMask) return false;
    if (data.playerAllowedDrawCard != other.data.playerAllowedDrawCard) return false;
    if (data.reinforcements != other.data.reinforcements) return false;
    if (data.round != other.data.round) return false;
    if (data.roundPhase != other.data.roundPhase) return false;
    if (data.attacksDuringTurn != other.data.attacksDuringTurn) return false;

    for (int i = 0; i < PLAYER_COUNT; i++)
    {
        if (!(data.playerStatus[i] == other.data.playerStatus[i])) return false;
    }

    for (int i = 0; i < DATA_TERRITORY; i++)
    {
        if (!(data.landArmy[i] == other.data.landArmy[i])) return false;
    }

    return true;
}

void State::newGame()
{
    if (log) printf("#### New game\n");

    uint64_t availableLands = LandSet::ALL_LANDS_MASK;
    while (availableLands != 0)
    {
        uint64_t randomLand = Utility::randomMask(availableLands);
        availableLands &= ~randomLand;
        
        const Land* l = Land::getLand(Utility::lm2li(randomLand));
        setLandArmy(Utility::li2i(l->landIndex), 1);

        if (data.currentPlayerTurn == (PLAYER_COUNT-1))
        {
            randomLand = Utility::randomMask(availableLands);
            availableLands &= ~randomLand;

            l = Land::getLand(Utility::lm2li(randomLand));
            setLandArmy(Utility::li2i(l->landIndex), 1, NEUTRAL_PLAYER);
        }

        nextPlayerTurn();       
    }
    data.reinforcements = (40 - 14) * PLAYER_COUNT; // / 2;

#if defined(_DEBUG) || defined(FORCE_CONSISTENCY_CHECK)
    consistencyCheck();
    consistencyCheckArmyValue();
#endif // !_DEBUG    
}


bool State::isCurrentPlayerTurnDiffrent(State& other) const
{
	return getCurrentPlayerTurn() != other.getCurrentPlayerTurn();
}

const PlayerStatus* State::getPlayerStatus(uint8_t playerIndex) const
{
    return &data.playerStatus[playerIndex];
}

const PlayerStatus* State::getCurrentPlayerStatus() const
{
	return &data.playerStatus[getCurrentPlayerTurn()];
}

const PlayerStatus* State::getEnemyPlayerStatus() const
{
    return &data.playerStatus[getEnemyPlayerTurn()];
}

LandArmy State::getLandArmy(LandIndex landIndex) const
{
	return getLandArmy(Utility::li2i(landIndex));
}

land_army_t State::getLandArmySpace(LandIndex landIndex) const
{
    return getLandArmySpace(Utility::li2i(landIndex));
}

land_army_t State::getLandArmySpace(uint8_t landIndex) const
{
    return LAND_ARMY_MAX - data.landArmy[landIndex].army;
}

LandArmy State::getLandArmy(uint8_t landIndex) const
{
    return data.landArmy[landIndex];
}

void State::addLandArmy(LandIndex landIndex, land_army_t value, uint8_t playerIndex)
{
    addLandArmy(Utility::li2i(landIndex), value, playerIndex);
}

void State::addLandArmy(uint8_t landIndex, land_army_t value, uint8_t playerIndex)
{
    LandArmy oldValue = getLandArmy(landIndex);    
    if (oldValue.army > 0 && oldValue.playerIndex != playerIndex)
    {
        throw std::logic_error("Can't add army to other player");
    }

    int combinedArmy = ((int)oldValue.army) + value;
	if (combinedArmy > LAND_ARMY_MAX)
	{
        throw std::logic_error("Land amry is over max value");
	}

	setLandArmy(landIndex, combinedArmy, playerIndex);

#ifdef _DEBUG
    consistencyCheckArmyValue();
#endif // _DEBUG
}

void State::addLandArmy(LandIndex landIndex, land_army_t value)
{
    addLandArmy(Utility::li2i(landIndex), value);
}

void State::addLandArmy(uint8_t landIndex, land_army_t value)
{
    LandArmy oldValue = getLandArmy(landIndex);
    if (oldValue.army > 0 && oldValue.playerIndex != getCurrentPlayerTurn())
    {
        throw std::logic_error("Can't add army to other player");
    }

    int combinedArmy = ((int)oldValue.army) + value;
    if (combinedArmy > LAND_ARMY_MAX)
    {
        throw std::logic_error("Land army is over max value");
    }

    setLandArmy(landIndex, combinedArmy, getCurrentPlayerTurn());
}

void updateAttackBitMask(PlayerStatus* p, const Land* l)
{
    if ((p->ownedLands & l->landIndexBitMask) == 0) // Not owning land
    {
        if ((l->neighboursLandIndexBitMask & p->ownedLands) > 0) // Owning neigberhood land
        {
            p->attackLands |= l->landIndexBitMask;
        }

        if ((l->neighboursLandIndexBitMask & p->ownedLandsWithArmy) > 0) // Owning neigberhood land with army
        {
            p->attackLandsWithArmy |= l->landIndexBitMask;
        }
    }	
}

void State::setLandArmy(uint8_t landIndex, land_army_t value)
{
    setLandArmy(landIndex, value, getCurrentPlayerTurn());
}

void State::setLandArmy(uint8_t landIndex, land_army_t value, uint8_t playerIndex) 
{
    if (landIndex > 41)
    {
        throw std::logic_error("Land index out of bound");
    }
    if (!(0 <= playerIndex && playerIndex <= 2)) // 0,1,2
    {
        throw std::logic_error("Player index out of bound");
    }

    resetHash();
    const LandArmy& old = data.landArmy[landIndex];

    land_army_t newValue = value;
    land_army_t oldValue = old.army;

    if (newValue != oldValue || old.playerIndex != playerIndex) // Value changed or owner
    {
        PlayerStatus* oldOwner = old.playerIndex == NEUTRAL_PLAYER ? nullptr : &data.playerStatus[old.playerIndex];
        PlayerStatus* newOwner = playerIndex == NEUTRAL_PLAYER ? nullptr : &data.playerStatus[playerIndex];

        const Land* l = Land::getLand(landIndex);

        if (newOwner != nullptr && newValue == LAND_ARMY_MAX)
        {
            newOwner->ownedFullLands |= l->landIndexBitMask;
        }
        else if (oldOwner != nullptr && oldValue == LAND_ARMY_MAX)
        {
            oldOwner->ownedFullLands &= ~l->landIndexBitMask;
        }
        
        if (old.playerIndex == playerIndex) // Land did not change owner
        {
            if (newOwner != nullptr)
            {
                newOwner->totalArmy += newValue - oldValue;
                if (oldValue == 1 && newValue > 1) // Can attack from this land
                {
                    newOwner->ownedLandsWithArmy |= l->landIndexBitMask;
                    newOwner->attackLandsWithArmy |= l->neighboursLandIndexBitMask;
                    newOwner->attackLandsWithArmy &= ~newOwner->ownedLands;
                }
                else if (oldValue > 1 && newValue == 1) // Can not attack from this land
                {
                    newOwner->ownedLandsWithArmy &= ~l->landIndexBitMask;
                    newOwner->attackLandsWithArmy &= ~l->neighboursLandIndexBitMask;
                    for (int i = 0; i < l->neihboursLandIndex.size(); i++)
                    {
                        const Land* nl = Land::getLand(l->neihboursLandIndex[i]);
                        if ((nl->neighboursLandIndexBitMask & newOwner->ownedLandsWithArmy) > 0)
                        {
                            newOwner->attackLandsWithArmy |= nl->landIndexBitMask;
                        }
                    }

                    newOwner->attackLandsWithArmy &= ~newOwner->ownedLands;
                }
            }            
        }
        else // Land changed owner
        {
            if(newOwner != nullptr) // Update status of new owner
            {
                newOwner->totalArmy += newValue;

                newOwner->ownedLands |= l->landIndexBitMask;
                newOwner->attackLands |= l->neighboursLandIndexBitMask;
                newOwner->attackLands &= ~newOwner->ownedLands;

                if (newValue > 1)
                {
                    newOwner->ownedLandsWithArmy |= l->landIndexBitMask;
                    newOwner->attackLandsWithArmy |= l->neighboursLandIndexBitMask;
                }
                newOwner->attackLandsWithArmy &= ~newOwner->ownedLands;
            }

            if (oldOwner != nullptr) // Update status of old owner
            {
                oldOwner->totalArmy -= oldValue;

                oldOwner->ownedLands &= ~l->landIndexBitMask;
                oldOwner->ownedLandsWithArmy &= ~l->landIndexBitMask;

                oldOwner->attackLands &= ~l->neighboursLandIndexBitMask;
                oldOwner->attackLandsWithArmy &= ~l->neighboursLandIndexBitMask;

                updateAttackBitMask(oldOwner, l); // Check if lost land can be attacked				 
                for (int i = 0; i < l->neihboursLandIndex.size(); i++)
                {
                    const Land* nl = Land::getLand(l->neihboursLandIndex[i]);
                    updateAttackBitMask(oldOwner, nl); // Check if neighbors can still be attacked
                }
            }
        }
        
        data.landArmy[landIndex].army = value;
        data.landArmy[landIndex].playerIndex = playerIndex;
    }

#if defined(_DEBUG) || defined(FORCE_CONSISTENCY_CHECK)
    consistencyCheck();
    consistencyCheckArmyValue();
#endif
}

uint64_t State::getPlayerCards() const
{
	return getCurrentPlayerStatus()->playerCards;
}


uint8_t State::getCardSetsPlayed() const
{
	return data.cardSetsPlayed;
}


bool State::getPlayerAllowedDrawCard() const
{
	return data.playerAllowedDrawCard;
}


land_army_t State::getReinforcement() const
{
	return data.reinforcements;
}

uint8_t State::getAttacksDuringTurn() const
{
    return data.attacksDuringTurn;
}

RoundPhase State::getRoundPhase() const
{
	return data.roundPhase;
}


LandIndex State::getAttackMobilizationFrom() const
{
    return data.attackMobilizationFrom;
}

LandIndex State::getAttackMobilizationTo() const
{
    return data.attackMobilizationTo;
}

int8_t State::getCurrentPlayerTurn() const
{
	return data.currentPlayerTurn;
}

int8_t State::getEnemyPlayerTurn() const
{
    return data.currentPlayerTurn == 0 ? 1 : 0;
}

uint16_t State::getRound() const
{
	return data.round;
}

void State::copy(State& dest)
{
	memcpy(&dest.data, &data, sizeof(data));
}

int8_t State::calculateReinforcementValue() const
{
    uint64_t ownedLand = getCurrentPlayerStatus()->ownedLands;
    return calculateReinforcementValue(ownedLand);
}

int8_t State::calculateReinforcementValue(uint64_t ownedLand) const
{
    int8_t reinforcementCount = Utility::popcount(ownedLand) / 3; // count of owned lands, rounds down

    if ((ownedLand & LandSet::NORTH_AMERICA.landSetIndexBitMask) == LandSet::NORTH_AMERICA.landSetIndexBitMask) {
        reinforcementCount += NORTH_AMERICA_REINFORCEMENT;
    }

    if ((ownedLand & LandSet::SOUTH_AMERICA.landSetIndexBitMask) == LandSet::SOUTH_AMERICA.landSetIndexBitMask) {
        reinforcementCount += SOUTH_AMERICA_REINFORCEMENT;
    }

    if ((ownedLand & LandSet::AFRICA.landSetIndexBitMask) == LandSet::AFRICA.landSetIndexBitMask) {
        reinforcementCount += AFRICA_REINFORCEMENT;
    }

    if ((ownedLand & LandSet::EUROPE.landSetIndexBitMask) == LandSet::EUROPE.landSetIndexBitMask) {
        reinforcementCount += EUROPE_REINFORCEMENT;
    }

    if ((ownedLand & LandSet::ASIA.landSetIndexBitMask) == LandSet::ASIA.landSetIndexBitMask) {
        reinforcementCount += ASIA_REINFORCEMENT;
    }

    if ((ownedLand & LandSet::AUSTRALIA.landSetIndexBitMask) == LandSet::AUSTRALIA.landSetIndexBitMask) {
        reinforcementCount += AUSTRALIA_REINFORCEMENT;
    }

    if (reinforcementCount < 3) // Minimum value of reinforcement is 3
    {
        reinforcementCount = 3;
    }

    return reinforcementCount;
}

void State::invertPlayers()
{
    PlayerStatus pls0 = data.playerStatus[0];
    PlayerStatus pls1 = data.playerStatus[1];
    data.playerStatus[0] = pls1;
    data.playerStatus[1] = pls0;

    for (auto& i : data.landArmy)
    {
        if (i.playerIndex == 0)
        {
            i.playerIndex = 1;
        }
        else if (i.playerIndex == 1)
        {
            i.playerIndex = 0;
        }
    }

#if defined(_DEBUG) || defined(FORCE_CONSISTENCY_CHECK)
    consistencyCheck();
    consistencyCheckArmyValue();
#endif // !_DEBUG
}

int8_t State::gameStatus() const
{
    const PlayerStatus& pls0 = data.playerStatus[0];

    int p0 = Utility::popcount(pls0.ownedLands);
    if (p0 == 0) // Player 0 has no land    
    {
        return 1;
    }

    const PlayerStatus& pls1 = data.playerStatus[1];

    int p1 = Utility::popcount(pls1.ownedLands);
    if (p1 == 0) // Player 1 has no land
    {
        return 0;
    }

    if (SETTINGS.ALLOW_YIELD) // If yeald is allowed
    {
        if (p0 >= 30) // P0 wins with mayority of land aprox 3/4 of all land
        {
            return 0;
        }
        else if (p1 >= 30) // P1 wins with mayority of land aprox 3/4 of all land
        {
            return 1;
        }
    }

    if ((data.round > SETTINGS.MAX_GAME_ROUNDS)) // Game must end
    {
        if (p0 > p1) // Player 0 wins
        {
            return 0;
        }
        else if (p0 < p1) // Player 1 wins
        {
            return 1;
        }
        else // Draw
        {
            return -2;
        }
    }

    return -1; // Game has not ended
}

void State::logGameStatus() const
{
    if (log)
    {
        const PlayerStatus& pls0 = data.playerStatus[0];

        int p0 = Utility::popcount(pls0.ownedLands);
        if (p0 == 0) // Player 0 has no land
        {
            printf("#### Player 1 won // Player 0 has no land left ###\n");
        }

        const PlayerStatus& pls1 = data.playerStatus[1];

        int p1 = Utility::popcount(pls1.ownedLands);
        if (p1 == 0) // Player 1 has no land
        {
            printf("#### Player 0 won // Player 1 has no land left ###\n");           
        }


        if (SETTINGS.ALLOW_YIELD) // If yeald is allowed
        {
            if (p0 >= 30) // P0 wins with mayority of land aprox 3/4 of all land
            {
                printf("#### Player 0 won // Player 1 yield ###\n");
            }
            else if (p1 >= 30) // P1 wins with mayority of land aprox 3/4 of all land
            {
                printf("#### Player 1 won // Player 0 yield ###\n");
            }
        }

        if ((data.round > SETTINGS.MAX_GAME_ROUNDS)) // Game must end
        {
            if (p0 > p1) // Player 0 wins
            {
                printf("#### Game ended // Player 0 won\n");
            }
            else if (p0 < p1) // Player 1 wins
            {
                printf("#### Game ended // Player 1 won\n");               
            }
            else // Draw
            {
                printf("#### Game ended // Draw \n");                
            }
        }
    }
}

void State::drawCard()
{
    if (getPlayerAllowedDrawCard())
    {
#ifdef STATE_SIMPLE_CARDS
        data.playerStatus[data.currentPlayerTurn].playerCards += 1;
        data.playerAllowedDrawCard = false;
        if (log) printf("[Player %d] Drawn card\n", int(getCurrentPlayerTurn()));
#else
        uint64_t availableCards = LandSet::ALL_CARD_MASK & ~data.drawnCardsBitMask;
        if (availableCards == 0) // Reshuffle non drawn cards
        {
            availableCards = LandSet::ALL_CARD_MASK & ~data.playerStatus[0].playerCards & ~data.playerStatus[1].playerCards;
            data.drawnCardsBitMask = availableCards;
        }

        uint64_t drawnCard = Utility::randomMask(availableCards);       

        data.drawnCardsBitMask |= drawnCard;
        data.playerStatus[data.currentPlayerTurn].playerCards |= drawnCard;
        data.playerAllowedDrawCard = false;

        if (log) printf("[Player %d] Drawn card %s\n", int(getCurrentPlayerTurn()), Land::getName(Utility::lm2li(drawnCard)).c_str());
#endif // STATE_SIMPLE_CARDS        
    }
}

DiceRolls State::getDiceRolls(int diceRolls)
{
    DiceRolls rolls = DiceRolls();

    if (diceRolls > 0)
    {
        rolls.roll1 = RNG.rDice();
    }
    if (diceRolls > 1)
    {
        rolls.roll2 = RNG.rDice();

        if (rolls.roll1 < rolls.roll2)
        {
            uint8_t temp = rolls.roll2;
            rolls.roll2 = rolls.roll1;
            rolls.roll1 = temp;
        }
    }
    if (diceRolls > 2)
    {
        rolls.roll3 = RNG.rDice();

        if (rolls.roll1 < rolls.roll3)
        {
            uint8_t temp = rolls.roll3;
            rolls.roll3 = rolls.roll2;
            rolls.roll2 = rolls.roll1;
            rolls.roll1 = temp;
        }
        else if (rolls.roll2 < rolls.roll3)
        {
            uint8_t temp = rolls.roll3;
            rolls.roll3 = rolls.roll2;
            rolls.roll2 = temp;
        }
    }

    return rolls;
}

void State::setLog(bool log)
{
    this->log = log;
}

bool State::getLog()
{
    return this->log;
}

void State::setCurrentPlayerTurn(int8_t currentPlayerTurn)
{
    if (log) printf("#### Starting player %d\n", currentPlayerTurn);
    data.currentPlayerTurn = currentPlayerTurn;
}

void State::nextPlayerTurn()
{
    resetHash();
    uint8_t playerIndexTurn = getCurrentPlayerTurn();
    playerIndexTurn++;
    if (playerIndexTurn >= PLAYER_COUNT)
    {
        playerIndexTurn = 0;
    }
    data.currentPlayerTurn = playerIndexTurn;        
}

void State::logStartingTurn()
{
    if (log) {
        const PlayerStatus* pls = getCurrentPlayerStatus();
            printf("[Player %d] Starting turn %d with Army[%d], Lands[%d], Cards[%d/%d/%d], Reinforcement[%d]\n",
                int(getCurrentPlayerTurn()), data.round, pls->totalArmy, Utility::popcount(pls->ownedLands), 
                Utility::popcount(pls->playerCards), data.cardSetsPlayed, Utility::popcount(data.drawnCardsBitMask),
                data.reinforcements);
    }
}

void State::nextPlayerSetupTurn()
{   
    resetHash();
    data.roundPhase = RoundPhase::SETUP;
    data.round++;
    if (log) printf("=> Round %d\n", data.round);

    uint8_t playerIndexTurn = getCurrentPlayerTurn();
    playerIndexTurn++;
    if (playerIndexTurn >= PLAYER_COUNT) playerIndexTurn = 0;    
    data.currentPlayerTurn = playerIndexTurn;

    if (data.reinforcements == 0)
    {
        data.roundPhase = RoundPhase::REINFORCEMENT;
        data.reinforcements = calculateReinforcementValue();

        if (log) printf("#### Setup phase ended\n");

        logStartingTurn();
    }
}

void State::nextPlayerGameTurn()
{
    resetHash();
    drawCard();

    if (log) printf("[Player %d] Ending turn %d\n------------------------------------------------------------------------\n", 
        int(getCurrentPlayerTurn()), data.round);

    data.round++;
    if (log) printf("=> Round %d\n", data.round);

    nextPlayerTurn();    
    
    data.attacksDuringTurn = 0;
    data.roundPhase = RoundPhase::REINFORCEMENT;
    data.reinforcements = calculateReinforcementValue();

    logStartingTurn();
}

// Assum attacking and defending with max numbers available
bool State::attackMove(LandIndex from, LandIndex to)
{
    data.attacksDuringTurn += 1;

#ifdef _DEBUG
    consistencyCheckArmyValue();
#endif // _DEBUG
    bool occupiedNewLand = false;

    if (data.roundPhase != RoundPhase::ATTACK) 
    { 
        throw std::invalid_argument("For attack player must be in round state attack"); 
    }
    if (from == LandIndex::None) 
    { 
        throw std::invalid_argument("Attacking from land index not specified"); 
    }
    if (to == LandIndex::None) 
    { 
        throw std::invalid_argument("Attacking to land index not specified"); 
    }

    uint8_t landIndexFrom = Utility::li2i(from);
    LandArmy attackingLand = getLandArmy(landIndexFrom);
    
    uint8_t landIndexTo = Utility::li2i(to);
    LandArmy defendingLand = getLandArmy(landIndexTo);

    int8_t attacker = attackingLand.playerIndex;
    int8_t defender = defendingLand.playerIndex;

    if (attacker != getCurrentPlayerTurn())
    {
        throw std::invalid_argument("Can not attack from not owned land");
    }
    if (attacker == defender)
    {
        throw std::invalid_argument("Can not attack yourself");
    }    

    if (attackingLand.playerIndex == defendingLand.playerIndex)
    { 
        printf("[Player %d] Attacking from %s[%d] to %s[%d]\n",
            int(attacker),
            Land::getName(from).c_str(), int(attackingLand.army),
            Land::getName(to).c_str(), int(defendingLand.army));

        throw std::invalid_argument("Attacking own land"); 
    }

    if (attackingLand.army <= 1) { throw std::invalid_argument("AttackMove, must have army value greater than 1"); }
    
    int attackingUnits = 1;
    uint8_t attackLandAmount = attackingLand.army;
    uint8_t defendLandAmount = defendingLand.army;

    DiceRolls attackingRolls, defendingRolls;
    if (defendingLand.army > 0)
    {
        int attackingAmount = attackLandAmount >= 4 ? 3 : attackLandAmount == 3 ? 2 : 1;
        attackingUnits = attackingAmount;
        int defendingAmount = defendLandAmount >= 2 ? 2 : 1;

        attackingRolls = getDiceRolls(attackingAmount);
        defendingRolls = getDiceRolls(defendingAmount);

        if (attackingRolls.roll1 > defendingRolls.roll1)
        {
            defendLandAmount--;           
        }
        else
        {
            attackLandAmount--;
            attackingUnits--;
        }

        if (attackingAmount >= 2 && defendingAmount == 2)
        {
            if (attackingRolls.roll2 > defendingRolls.roll2)
            {
                defendLandAmount--;
            }
            else
            {
                attackLandAmount--;
                attackingUnits--;
            }
        }
    }

    if (defendLandAmount == 0)
    {
        if (log) printf("[Player %d -> %d] Attacking from %s[%d -> %d] to %s[%d -> %d] \t Rolls: [%d/%d] [%d/%d] [%d/%d]\n",
            int(attacker), int(defender),
            Land::getName(from).c_str(), int(attackingLand.army), int(attackLandAmount),
            Land::getName(to).c_str(), int(defendingLand.army), int(defendLandAmount),
            unsigned(attackingRolls.roll1), unsigned(defendingRolls.roll1),
            unsigned(attackingRolls.roll2), unsigned(defendingRolls.roll2),
            unsigned(attackingRolls.roll3), unsigned(defendingRolls.roll3)
        );                    

        if (log) printf("[Player %d -> %d] Occupied land from %s[%d -> %d] to %s[%d -> %d]\n",
            int(attacker), int(defender),
            Land::getName(from).c_str(), int(attackLandAmount), int(attackLandAmount - attackingUnits),
            Land::getName(to).c_str(), int(defendLandAmount), int(defendLandAmount + attackingUnits)
        );

        attackLandAmount -= attackingUnits;

        if (attackLandAmount > 1) // Go to mobilization state
        {
            data.roundPhase = RoundPhase::ATTACK_MOBILIZATION;
            data.attackMobilizationFrom = from;
            data.attackMobilizationTo = to;

            if (log) printf("[Player %d] Entering ATTACK_MOBILIZATION\n", getCurrentPlayerTurn());
        }       

        data.playerAllowedDrawCard = true;                        

        setLandArmy(landIndexFrom, attackLandAmount, attacker); // Attacker units without attacking units
        setLandArmy(landIndexTo, attackingUnits, attacker); // Attacker surviving units occupy new territory            

        occupiedNewLand = true;
    }
    else
    {
        setLandArmy(landIndexFrom, attackLandAmount, attacker); // Attacker remaining units
        setLandArmy(landIndexTo, defendLandAmount, defender); // Defender remaining units
            
        if (log) printf("[Player %d -> %d] Attacking from %s[%d -> %d] to %s[%d -> %d] \t Rolls: [%d/%d] [%d/%d] [%d/%d]\n",
            int(attacker), int(defender),
            Land::getName(from).c_str(), int(attackingLand.army), int(attackLandAmount),
            Land::getName(to).c_str(), int(defendingLand.army), int(defendLandAmount),
            unsigned(attackingRolls.roll1), unsigned(defendingRolls.roll1),
            unsigned(attackingRolls.roll2), unsigned(defendingRolls.roll2),
            unsigned(attackingRolls.roll3), unsigned(defendingRolls.roll3)
        );
    }

    if(getRoundPhase() == RoundPhase::ATTACK && getCurrentPlayerStatus()->attackLandsWithArmy == 0)
    {
        gotoFortify();
    }

#ifdef _DEBUG
    consistencyCheckArmyValue();
#endif // _DEBUG
    return occupiedNewLand;
}

void State::attackReinforcementMove(land_army_t amount)
{
    if (data.roundPhase != RoundPhase::ATTACK_MOBILIZATION) { throw std::invalid_argument("For attack reinforcement player must be in round state ATTACK_MOBILIZATION"); }

    uint8_t landIndexFrom = Utility::li2i(data.attackMobilizationFrom);
    LandArmy currentAmountFrom = getLandArmy(landIndexFrom);

    land_army_t afterMoveFromArmy = currentAmountFrom.army - amount;
    if (afterMoveFromArmy < 1) { throw std::invalid_argument("For attack reinforcement player must leave 1 army behind"); }

    uint8_t landIndexTo = Utility::li2i(data.attackMobilizationTo);
    LandArmy currentAmountTo = getLandArmy(landIndexTo);

    setLandArmy(landIndexFrom, currentAmountFrom.army - amount);
    setLandArmy(landIndexTo, currentAmountTo.army + amount);

    LandArmy afterAmountFrom = getLandArmy(landIndexFrom);
    LandArmy afterAmountTo = getLandArmy(landIndexTo);

    if (log) printf("[Player %d] Attack reinforceing from %s[%d -> %d] to %s[%d -> %d]\n", int(getCurrentPlayerTurn()),
        Land::getName(data.attackMobilizationFrom).c_str(), int(currentAmountFrom.army), int(afterAmountFrom.army),
        Land::getName(data.attackMobilizationTo).c_str(), int(currentAmountTo.army), int(afterAmountTo.army));

    if (afterAmountFrom.army == 1)
    {
        gotoAttack();
    }
}

void State::fortifyMove(land_army_t amount, LandIndex from, LandIndex to)
{
    if (data.roundPhase != RoundPhase::FORTIFY) { throw std::invalid_argument("For reinforcement player must be in round state FORTIFY"); }

    uint8_t landIndexFrom = static_cast<uint8_t>(from);
    LandArmy currentAmountFrom = getLandArmy(landIndexFrom);

    land_army_t afterMoveFromArmy = currentAmountFrom.army - amount;
    if (afterMoveFromArmy < 1) { throw std::invalid_argument("For fortify player must leave 1 army behind"); }

    uint8_t landIndexTo = static_cast<uint8_t>(to);
    LandArmy currentAmountTo = getLandArmy(landIndexTo);

    int afterMoveToArmy = ((int)currentAmountTo.army) + amount;
    if (afterMoveToArmy > LAND_ARMY_MAX) 
    { 
        throw std::invalid_argument("For fortify player must not owerflow amry"); 
    }

    setLandArmy(landIndexFrom, afterMoveFromArmy);
    setLandArmy(landIndexTo, afterMoveToArmy);

    if (log) printf("[Player %d] Fortify from %s[%d -> %d] to %s[%d -> %d]\n", int(getCurrentPlayerTurn()),
        Land::getName(from).c_str(), int(currentAmountFrom.army), int(afterMoveFromArmy),
        Land::getName(to).c_str(), int(currentAmountTo.army), int(afterMoveToArmy));
}

void State::reinforcementMove(land_army_t amount, LandIndex to)
{
    if (data.roundPhase != RoundPhase::REINFORCEMENT) { throw std::invalid_argument("For reinforcement player must be in round state REINFORCEMENT"); }
    if (data.reinforcements < amount)
    {
        printf("[Player %d] Reinforcement move invalid amount %d // %d", int(getCurrentPlayerTurn()), amount, data.reinforcements);
        throw std::invalid_argument("Reinforcement move exceeded amount");
    }
    LandArmy la = getLandArmy(to);

    data.reinforcements -= amount;
    addLandArmy(to, amount);

    LandArmy laAfter = getLandArmy(to);

    if (log) printf("[Player %d] Reinforcement placed amount %d [%d -> %d]to %s\n", 
        int(getCurrentPlayerTurn()), amount, la.army, laAfter.army, Land::getName(to).c_str());
    
    if (data.reinforcements == 0)
    {
        gotoAttack();
    }
}

void State::cardReinforcementMove(LandIndex to)
{
    if (data.roundPhase != RoundPhase::REINFORCEMENT) { throw std::invalid_argument("For reinforcement player must be in round state REINFORCEMENT"); }
    
    addLandArmy(to, 2);

    if (log) printf("[Player %d] Card reinforcement placed amount %d to %s\n", int(getCurrentPlayerTurn()), 2, Land::getName(to).c_str());
}

void State::setupReinforcementMove(LandIndex to)
{
    if (data.roundPhase != RoundPhase::SETUP) { throw std::invalid_argument("For setup reinforcement player must be in round state SETUP"); }    
    if (data.reinforcements <= 0) 
    { 
        throw std::invalid_argument("No reinforcement left"); 
    }
    data.reinforcements -= 2;

    uint64_t ownedLand = getCurrentPlayerStatus()->ownedLands;
    const Land* l = Land::getLand(to);
    if ((ownedLand & l->landIndexBitMask) == 0)
    {
        throw std::invalid_argument("Setup reinforcement move placed on not owned land");
    }

    if (log) printf("[Player %d] Setup reinforcement placed amount %d to %s, left reinforcement %d\n", int(getCurrentPlayerTurn()), 2, Land::getName(to).c_str(), int(data.reinforcements));
    
    addLandArmy(to, 2);

    gotoSetupNeutral();
}

void State::setupReinforcementNeutralMove(LandIndex to)
{
    if (data.roundPhase != RoundPhase::SETUP_NEUTRAL) { throw std::invalid_argument("For setup neutral reinforcement player must be in round state SETUP_NEUTRAL"); }
    uint64_t neutralLands = ~getCurrentPlayerStatus()->ownedLands & ~getEnemyPlayerStatus()->ownedLands;
    const Land* l = Land::getLand(to);
    if ((neutralLands & l->landIndexBitMask) == 0)
    {
        throw std::invalid_argument("Setup neutral reinforcement move placed on not neutral land");
    }

    if (log) printf("[Player %d] Setup neutral reinforcement placed amount %d to %s\n", int(getCurrentPlayerTurn()), 1, Land::getName(to).c_str());

    LandArmy la = getLandArmy(to);
    if (la.playerIndex != NEUTRAL_PLAYER)
    {
        throw std::invalid_argument("Setup neutral reinforcement move placed on not neutral land");
    }

    setLandArmy(Utility::li2i(to), la.army + 1, NEUTRAL_PLAYER);

    nextPlayerSetupTurn();
}

void State::setupLandOccupation(LandIndex to)
{    
    if (data.roundPhase != RoundPhase::SETUP) { throw std::invalid_argument("For setup reinforcement player must be in round state SETUP"); }
    uint64_t ownedLand = data.playerStatus[0].ownedLands | data.playerStatus[1].ownedLands;
    if ((ownedLand & Utility::li2i(to)) > 0)
    {
        throw std::invalid_argument("Setup reinforcement move placed on already owned land");
    }

    addLandArmy(to, 1);
}

uint64_t State::getNeutralPlayerAttackLands() const
{
    uint64_t neutralPlayerLands = LandSet::ALL_LANDS_MASK & ~getCurrentPlayerStatus()->ownedLands & ~getEnemyPlayerStatus()->ownedLands; // All lands that don't belong to any of players
    uint64_t neutralPlayerAttackLands = 0ULL;

    uint64_t neutralPlayerLandsIter = neutralPlayerLands;
    while (neutralPlayerLandsIter > 0)
    {
        uint64_t neutralLand = Utility::getFirstBitMask(neutralPlayerLandsIter);
        neutralPlayerLandsIter = neutralPlayerLandsIter & ~neutralLand;

        neutralPlayerAttackLands |= Land::getLand(Utility::lm2i(neutralLand))->neighboursLandIndexBitMask; // Add all neighbours
    }
    neutralPlayerAttackLands &= ~neutralPlayerLands; // Removed owned lands

    return neutralPlayerAttackLands;
}

const Data& State::getData() const
{
    return data;
}

#ifdef STATE_SIMPLE_CARDS
void State::playCards()
{
    resetHash();

    uint64_t playerCards = getPlayerCards(); // Number of cards
    if (playerCards >= 3)
    {
        data.playerStatus[getCurrentPlayerTurn()].playerCards -= 3;
        data.cardSetsPlayed += 1;

        uint16_t gainedReinforcement = 0;
        switch (data.cardSetsPlayed)
        {
        case 1: gainedReinforcement = 4; break;
        case 2: gainedReinforcement = 6; break;
        case 3: gainedReinforcement = 8; break;
        case 4: gainedReinforcement = 10; break;
        case 5: gainedReinforcement = 12; break;
        case 6: gainedReinforcement = 15; break;
        default: gainedReinforcement = 15 + (data.cardSetsPlayed - 6) * 5; break;
        }

        data.reinforcements += gainedReinforcement;

        if (log) printf("[Player %d] Playing cards. New reinforcement count %d\n", getCurrentPlayerTurn(), int(data.reinforcements));
    }    
}
#else
void State::playCards(uint64_t cardsPlayed)
{
    if (Utility::popcount(cardsPlayed) != 3)
    {
        throw std::invalid_argument("Can't play more than 3 cards");
    }

    resetHash();

    uint64_t ownedLands = getCurrentPlayerStatus()->ownedLands;
    uint64_t reinforcementLand = cardsPlayed & ownedLands;

    if (log)
    {
        printf("[Player %d] Playing cards ", getCurrentPlayerTurn());

        uint64_t iterCardsPlayed = cardsPlayed;
        while (iterCardsPlayed > 0)
        {
            uint64_t landMask = Utility::getFirstBitMask(iterCardsPlayed);
            printf("%s[%c], ", Land::getName(Utility::lm2li(landMask)).c_str(), Land::getCardType(landMask));
            iterCardsPlayed &= ~landMask;
        }
        printf("\n");
    }

    while (reinforcementLand > 0) // Max 2 units per set played
    {
        uint64_t landMask = Utility::getFirstBitMask(reinforcementLand);
        reinforcementLand &= ~landMask;

        LandIndex li = Utility::lm2li(landMask);
        land_army_t value = getLandArmy(li).army;
        if (value + 2 <= LAND_ARMY_MAX)
        {
            cardReinforcementMove(li);
            if (log) printf("[Player %d] Gained 2 units on land %s[%c] \n", getCurrentPlayerTurn(), Land::getName(Utility::lm2li(landMask)).c_str(), Land::getCardType(landMask));
            break;
        }
    }

    uint64_t playerCards = getPlayerCards();
    data.playerStatus[getCurrentPlayerTurn()].playerCards = playerCards & ~cardsPlayed;
    data.cardSetsPlayed += 1;

    uint16_t gainedReinforcement = 0;
    switch (data.cardSetsPlayed)
    {
    case 1: gainedReinforcement = 4; break;
    case 2: gainedReinforcement = 6; break;
    case 3: gainedReinforcement = 8; break;
    case 4: gainedReinforcement = 10; break;
    case 5: gainedReinforcement = 12; break;
    case 6: gainedReinforcement = 15; break;
    default: gainedReinforcement = 15 + (data.cardSetsPlayed - 6) * 5; break;
    }

    data.reinforcements += gainedReinforcement;
}
#endif


void State::consistencyCheckArmyValue()
{
    int player0Army = 0;
    int player1Army = 0;

    for (int i = 0; i < LAND_INDEX_SIZE; i++)
    {
        LandArmy landArmy = data.landArmy[i];
        if (landArmy.playerIndex == 0)
        {
            player0Army += landArmy.army;
        }
        if (landArmy.playerIndex == 1)
        {
            player1Army += landArmy.army;
        }
    }

    if (player0Army != data.playerStatus[0].totalArmy)
    {
        printf("Player 0 [totalArmy] %d (%d)\n", data.playerStatus[0].totalArmy, player0Army);
    }
    if (player1Army != data.playerStatus[1].totalArmy)
    {
        printf("Player 1 [totalArmy] %d (%d)\n", data.playerStatus[1].totalArmy, player1Army);
    }
}

void State::consistencyCheck()
{
    for (int i = 0; i < LAND_INDEX_SIZE; i++)
    {
        const Land* l = Land::LAND_MAP[i];
        LandArmy landArmy = data.landArmy[i];
        if (!(data.roundPhase == RoundPhase::SETUP || data.roundPhase == RoundPhase::SETUP_NEUTRAL) && landArmy.army == 0)
        {
            printf("! Empty land: %s", Land::getName(l->landIndex).c_str());
        }
        else
        {
            int pOwner = landArmy.playerIndex;
            if (pOwner == NEUTRAL_PLAYER)
            {
                PlayerStatus& pls0 = data.playerStatus[0];
                PlayerStatus& pls1 = data.playerStatus[1];

                if ((pls0.ownedLands & l->landIndexBitMask) != 0)
                {
                    printf("Player %d [ownedLands](-) / %s(%d)\n", 0, Land::getName(l->landIndex).c_str(), i);
                }
                if ((pls1.ownedLands & l->landIndexBitMask) != 0)
                {
                    printf("Player %d [ownedLands](-) / %s(%d)\n", 1, Land::getName(l->landIndex).c_str(), i);
                }

                if ((pls0.ownedLandsWithArmy & l->landIndexBitMask) != 0)
                {
                    printf("Player %d [ownedLandsWithArmy](-) / %s(%d)\n", 0, Land::getName(l->landIndex).c_str(), i);
                }
                if ((pls1.ownedLandsWithArmy & l->landIndexBitMask) != 0)
                {
                    printf("Player %d [ownedLandsWithArmy](-) / %s(%d)\n", 1, Land::getName(l->landIndex).c_str(), i);
                }

                bool attackLand0 = false;
                bool attackLandWithArmy0 = false;
                bool attackLand1 = false;
                bool attackLandWithArmy1 = false;
                for (int j = 0; j < l->neihboursLandIndex.size(); j++)
                {
                    LandIndex li = l->neihboursLandIndex[j];
                    LandArmy value = data.landArmy[Utility::li2i(li)];

                    if (value.playerIndex == 0)
                    {
                        if (value.army >= 1)
                        {
                            attackLand0 = true;
                            if (value.army > 1)
                            {
                                attackLandWithArmy0 = true;
                            }
                        }
                    }
                    else if (value.playerIndex == 1)
                    {
                        if (value.army >= 1)
                        {
                            attackLand1 = true;
                            if (value.army > 1)
                            {
                                attackLandWithArmy1 = true;
                            }
                        }
                    }
                }

                bool attackLandPls = (pls0.attackLands & l->landIndexBitMask) != 0;
                if (attackLandPls != attackLand0)
                {
                    if (attackLand0)
                    {
                        printf("Player %d [attackLands](+) / %s(%d)\n", 0, Land::getName(l->landIndex).c_str(), i);
                    }
                    else
                    {
                        printf("Player %d [attackLands](-) / %s(%d)\n", 0, Land::getName(l->landIndex).c_str(), i);
                    }
                }

                bool attackLandWithArmyPls = (pls0.attackLandsWithArmy & l->landIndexBitMask) != 0;
                if (attackLandWithArmyPls != attackLandWithArmy0)
                {
                    if (attackLandWithArmy0)
                    {
                        printf("Player %d [attackLandsWithArmy](+) / %s(%d)\n", 0, Land::getName(l->landIndex).c_str(), i);
                    }
                    else
                    {
                        printf("Player %d [attackLandsWithArmy](-) / %s(%d)\n", 0, Land::getName(l->landIndex).c_str(), i);
                    }

                }

                attackLandPls = (pls1.attackLands & l->landIndexBitMask) != 0;
                if (attackLandPls != attackLand1)
                {
                    if (attackLand1)
                    {
                        printf("Player %d [attackLands](+) / %s(%d)\n", 1, Land::getName(l->landIndex).c_str(), i);
                    }
                    else
                    {
                        printf("Player %d [attackLands](-) / %s(%d)\n", 1, Land::getName(l->landIndex).c_str(), i);
                    }
                }

                attackLandWithArmyPls = (pls1.attackLandsWithArmy & l->landIndexBitMask) != 0;
                if (attackLandWithArmyPls != attackLandWithArmy1)
                {
                    if (attackLandWithArmy1)
                    {
                        printf("Player %d [attackLandsWithArmy](+) / %s(%d)\n", 1, Land::getName(l->landIndex).c_str(), i);
                    }
                    else
                    {
                        printf("Player %d [attackLandsWithArmy](-) / %s(%d)\n", 1, Land::getName(l->landIndex).c_str(), i);
                    }

                }

            }
            else
            {
                int pNotOwner = pOwner == 0 ? 1 : 0;

                PlayerStatus& owner = data.playerStatus[pOwner];
                PlayerStatus& notOwner = data.playerStatus[pNotOwner];

                if ((owner.ownedLands & l->landIndexBitMask) == 0)
                {
                    printf("Player %d [ownedLands](+) / %s(%d)\n", pOwner, Land::getName(l->landIndex).c_str(), i);
                }

                if ((owner.attackLands & l->landIndexBitMask) != 0)
                {
                    printf("Player %d [attackLands](-) / %s(%d)\n", pOwner, Land::getName(l->landIndex).c_str(), i);
                }

                if ((owner.attackLandsWithArmy & l->landIndexBitMask) != 0)
                {
                    printf("Player %d [attackLandsWithArmy](-) / %s(%d)\n", pOwner, Land::getName(l->landIndex).c_str(), i);
                }

                if (landArmy.army > 1)
                {
                    if ((owner.ownedLandsWithArmy & l->landIndexBitMask) == 0)
                    {
                        printf("Player %d [ownedLandsWithArmy](+) / %s(%d)\n", pOwner, Land::getName(l->landIndex).c_str(), i);
                    }
                }
                else
                {
                    if ((owner.ownedLandsWithArmy & l->landIndexBitMask) != 0)
                    {
                        printf("Player %d [ownedLandsWithArmy](-) / %s(%d)\n", pOwner, Land::getName(l->landIndex).c_str(), i);
                    }
                }

                if ((notOwner.ownedLands & l->landIndexBitMask) != 0)
                {
                    printf("Player %d [ownedLands](-) / %s(%d)\n", pNotOwner, Land::getName(l->landIndex).c_str(), i);
                }

                if ((notOwner.ownedLandsWithArmy & l->landIndexBitMask) != 0)
                {
                    printf("Player %d [ownedLandsWithArmy](-) / %s(%d)\n", pNotOwner, Land::getName(l->landIndex).c_str(), i);
                }

                bool attackLand = false;
                bool attackLandWithArmy = false;
                for (int j = 0; j < l->neihboursLandIndex.size(); j++)
                {
                    LandIndex li = l->neihboursLandIndex[j];
                    LandArmy value = data.landArmy[Utility::li2i(li)];

                    if (value.playerIndex == pNotOwner)
                    {
                        if (value.army >= 1)
                        {
                            attackLand = true;
                            if (value.army > 1)
                            {
                                attackLandWithArmy = true;
                            }
                        }
                    }
                }

                bool attackLandPls = (notOwner.attackLands & l->landIndexBitMask) != 0;
                if (attackLandPls != attackLand)
                {
                    if (attackLand)
                    {
                        printf("Player %d [attackLands](+) / %s(%d)\n", pNotOwner, Land::getName(l->landIndex).c_str(), i);
                    }
                    else
                    {
                        printf("Player %d [attackLands](-) / %s(%d)\n", pNotOwner, Land::getName(l->landIndex).c_str(), i);
                    }
                }

                bool attackLandWithArmyPls = (notOwner.attackLandsWithArmy & l->landIndexBitMask) != 0;
                if (attackLandWithArmyPls != attackLandWithArmy)
                {
                    if (attackLandWithArmy)
                    {
                        printf("Player %d [attackLandsWithArmy](+) / %s(%d)\n", pNotOwner, Land::getName(l->landIndex).c_str(), i);
                    }
                    else
                    {
                        printf("Player %d [attackLandsWithArmy](-) / %s(%d)\n", pNotOwner, Land::getName(l->landIndex).c_str(), i);
                    }

                }
            }
        }
    }
}
