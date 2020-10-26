#pragma once

#include <signal.h>
#include <stdlib.h>

#include "../../../state/state.h"
#include "../../../../settings.h"


static const int TF_INPUT_MOVE_SIZE = DATA_TERRITORY;


static const int IF_CURRENT_PLAYER = 0;
static const int IF_ENEMY_PLAYER = IF_CURRENT_PLAYER + 1;
static const int IF_NEUTRAL_PLAYER = IF_ENEMY_PLAYER + 1;


#if defined(INPUT_VECTOR_TYPE_1) || defined(INPUT_VECTOR_TYPE_2) || defined(INPUT_VECTOR_TYPE_3)
#if defined(INPUT_VECTOR_TYPE_3)
static const int IF_ROUND = IF_NEUTRAL_PLAYER + 1;
static const int IF_ARMY_SHARE = IF_ROUND + 1;
static const int IF_REINFORCEMENT_SHARE = IF_ARMY_SHARE + 1;
#elif defined(INPUT_VECTOR_TYPE_2)
static const int IF_ARMY_SHARE = IF_NEUTRAL_PLAYER + 1;
static const int IF_REINFORCEMENT_SHARE = IF_ARMY_SHARE + 1;
#else
static const int IF_REINFORCEMENT_SHARE = IF_NEUTRAL_PLAYER + 1;
#endif
static const int IF_ATTACKS_DURING_TURN = IF_REINFORCEMENT_SHARE + 1;
static const int IF_CAN_DRAW_CARD = IF_ATTACKS_DURING_TURN + 1;

static const int IF_PHASE_SETUP = IF_CAN_DRAW_CARD + 1;
static const int IF_PHASE_SETUP_NEUTRAL = IF_PHASE_SETUP + 1;
static const int IF_PHASE_REINFORCEMENT = IF_PHASE_SETUP_NEUTRAL + 1;
static const int IF_PHASE_ATTACK = IF_PHASE_REINFORCEMENT + 1;
static const int IF_PHASE_ATTACK_MOBILIZATION = IF_PHASE_ATTACK + 1;
static const int IF_PHASE_FORTIFY = IF_PHASE_ATTACK_MOBILIZATION + 1;

static const int TF_INPUT_FEATURES = IF_PHASE_FORTIFY + 1;

static const auto FEATURES = {
	IF_CURRENT_PLAYER,
	IF_ENEMY_PLAYER,
	IF_NEUTRAL_PLAYER,
#if defined(INPUT_VECTOR_TYPE_2) || defined(INPUT_VECTOR_TYPE_3)
	IF_ARMY_SHARE,
#if defined(INPUT_VECTOR_TYPE_3)
	IF_ROUND,
#endif
#endif
	IF_REINFORCEMENT_SHARE,
	IF_ATTACKS_DURING_TURN,
	IF_CAN_DRAW_CARD,

	IF_PHASE_SETUP,
	IF_PHASE_SETUP_NEUTRAL,
	IF_PHASE_REINFORCEMENT,
	IF_PHASE_ATTACK,
	IF_PHASE_ATTACK_MOBILIZATION,
	IF_PHASE_FORTIFY
};
#endif

static const int TF_INPUT_TENSOR_SIZE = TF_INPUT_MOVE_SIZE * TF_INPUT_FEATURES;

static const int TF_OUTPUT_POLICY_TENSOR_SIZE = TF_INPUT_MOVE_SIZE + 1;
static const int TF_OUTPUT_VALUE_TENSOR_SIZE = 1;


class NNInputData
{
public:
#if defined(INPUT_VECTOR_TYPE_1) || defined(INPUT_VECTOR_TYPE_2) || defined(INPUT_VECTOR_TYPE_3)
	LandArmy land[DATA_TERRITORY];
	uint8_t playerIndex = 0;
	uint16_t round = 0;

	float featureReinforcementShare = 0.0f;
	float featureAttackFrequency = 0.0f;
	float featureCanDrawCard = 0.0f;

	float featureIsPhaseSetup = 0.0f;
	float featureIsPhaseSetupNeutral = 0.0f;
	float featureIsPhaseReinforcement = 0.0f;
	float featureIsPhaseAttack = 0.0f;
	float featureIsPhaseAttackMobilization = 0.0f;
	float featureIsPhaseFortify = 0.0f;

#if defined(INPUT_VECTOR_TYPE_2) || defined(INPUT_VECTOR_TYPE_3)
	float featureArmyShare;
#endif
#endif

	NNInputData() {};
	NNInputData(const State& s);
};

class NNOutputData
{
public:
	std::vector<float> policy;
	float value;
	NNOutputData() : value(0.0f) {};
	NNOutputData(std::vector<float>&& policy) : policy(policy), value(0.0f) {};

	void normalize(uint64_t validMoves);

	static NNOutputData createRandom();
};

class NNTrainData
{
public:
	int8_t playerIndex;
	NNInputData in;
	NNOutputData out;

	NNTrainData() : playerIndex(0) {};
	NNTrainData(uint8_t playerIndex, NNInputData&& in, NNOutputData&& out) : playerIndex(playerIndex), in(in), out(out) {};
};

class NNTrainDataStorage
{
public:
	NNTrainDataStorage() : lastGameIndex(0) {};

	std::vector<NNTrainData> data;
	size_t lastGameIndex;
	size_t oldGameIndex;

	void updateValues(int gameStatus, int roundCount);
	void trimOldExamples();

	void loadTrainingSamples(std::string filePath);
	void saveTrainingSamples(std::string filePath);

	void registerHandler();

	void extend(NNTrainDataStorage& storage);
	void updateOldGamesIndex();
};

static NNTrainDataStorage* staticDataStorePtr = nullptr;