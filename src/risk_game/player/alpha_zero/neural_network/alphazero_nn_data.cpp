#include "alphazero_nn_data.h"

void NNOutputData::normalize(uint64_t validMoves)
{
	float sum = 0.0f;
	uint64_t bitMask = 1ULL;
	for (uint8_t i = 0; i < policy.size(); i++)
	{
		if ((validMoves & bitMask) > 0) // Is valid move
		{
			sum += policy[i];
		}
		else
		{
			policy[i] = 0.0f;
		}
		bitMask = bitMask << 1;
	}

	for (int i = 0; i < policy.size(); i++)
	{
		if (policy[i] > 0.0f)
		{
			policy[i] /= sum;
		}
	}
}

NNOutputData NNOutputData::createRandom()
{
	NNOutputData out;

	float sum = 0.0f;
	for (int i = 0; i < TF_OUTPUT_POLICY_TENSOR_SIZE; i++)
	{
		float v = RNG.rFloat() + 0.5f; // Minimal exploration
		sum += v;
		out.policy.push_back(v);
	}

	for (int i = 0; i < TF_OUTPUT_POLICY_TENSOR_SIZE; i++)
	{
		out.policy[i] /= sum; // normalize
	}

	out.value = RNG.rFloat() * 2 - 1; // [-1, 1];

	return std::move(out);
}

void NNTrainDataStorage::updateValues(int gameStatus, int roundCount)
{
	for (size_t i = lastGameIndex; i < data.size(); i++)
	{
#if defined(ROUND_WEIGHTED_VALUE)
		float value = gameStatus == State::DRAW ? 0.0f : data[i].playerIndex == gameStatus ? 1.0f : -1.0f;
		float roundWeight = __MIN(1.0f, float(data[i].in.round) / roundCount);
		data[i].out.value = value * roundWeight;
#else
		data[i].out.value = gameStatus == State::DRAW ? 0.0f : data[i].playerIndex == gameStatus ? 1.0f : -1.0f;
#endif
		
	}
	lastGameIndex = data.size();
}

void NNTrainDataStorage::trimOldExamples()
{	
	if (data.size() > SETTINGS.SAMPLES_STORAGE_MAX)
	{
		size_t excessExamples = data.size() - SETTINGS.SAMPLES_STORAGE_MAX;
		data.erase(data.begin(), data.begin() + excessExamples);
		printf("[MAX] Erased %d oldeset examples\n", int(excessExamples));
	}
	else if(data.size() > SETTINGS.SAMPLES_STORAGE_MIN && oldGameIndex > 0)
	{
		size_t excessExamples = data.size() - SETTINGS.SAMPLES_STORAGE_MIN;
		excessExamples = __MIN(oldGameIndex, excessExamples);
		oldGameIndex -= excessExamples;

		data.erase(data.begin(), data.begin() + excessExamples);
		printf("[MIN] Erased %d oldeset examples\n", int(excessExamples));		
	}
}

void NNTrainDataStorage::loadTrainingSamples(std::string filePath)
{
	if (std::filesystem::exists(filePath))
	{
		std::ifstream in(filePath, std::ios::out | std::ios::binary);

		int size = 0;
		in.read((char*)&size, sizeof(int));

		data.resize(size);
		for (int i = 0; i < size; i++)
		{
			auto& d = data[i];
			in.read((char*)&d.playerIndex, sizeof(d.playerIndex));

			in.read((char*)&d.in, sizeof(d.in));

			in.read((char*)&d.out.value, sizeof(d.out.value));

			d.out.policy.resize(TF_OUTPUT_POLICY_TENSOR_SIZE);
			in.read((char*)&d.out.policy[0], sizeof(float) * TF_OUTPUT_POLICY_TENSOR_SIZE);
		}
	}
	else
	{
		printf("File does note exist: %s\n", filePath.c_str());
	}
}

void NNTrainDataStorage::saveTrainingSamples(std::string filePath)
{
	if (data.size() > 0)
	{
		std::string dir = filePath.substr(0, filePath.find_last_of('/'));
		std::filesystem::create_directories(dir);
		std::ofstream out(filePath, std::ios::out | std::ios::binary);

		size_t size = data.size();
		out.write((char*)&size, sizeof(size_t));
		for (auto& d : data)
		{
			out.write((char*)&d.playerIndex, sizeof(d.playerIndex));
			out.write((char*)&d.in, sizeof(d.in));
			out.write((char*)&d.out.value, sizeof(d.out.value));
			out.write((char*)&d.out.policy[0], sizeof(float) * TF_OUTPUT_POLICY_TENSOR_SIZE);
		}
		printf("Training samples saved %d\n", int(data.size()));
	}
	else
	{
		printf("No training samples\n");
	}
}

static void handleSignalINT(int signum)
{
	printf("\n=> Handling INT signal, saving training samples.\n");
	staticDataStorePtr->saveTrainingSamples(SETTINGS.DEFAULT_SAMPLES);

	exit(signum); // Non clean terimnate
}

void NNTrainDataStorage::registerHandler()
{
	staticDataStorePtr = this;
	signal(SIGINT, handleSignalINT);
}

void NNTrainDataStorage::extend(NNTrainDataStorage& storage)
{
	data.reserve(data.size() + storage.data.size());
	data.insert(data.end(), storage.data.begin(), storage.data.end());
}

void NNTrainDataStorage::updateOldGamesIndex()
{
	oldGameIndex = data.size() - 1;
}

NNInputData::NNInputData(const State& s) // (6 * 7) * 4
{
	const PlayerStatus* ps = s.getCurrentPlayerStatus();
	const PlayerStatus* eps = s.getEnemyPlayerStatus();

#if defined(INPUT_VECTOR_TYPE_1) || defined(INPUT_VECTOR_TYPE_2)
	memcpy(&land, &s.getData().landArmy, sizeof(land));
	playerIndex = s.getCurrentPlayerTurn();
	round = s.getRound();

	float ref = s.calculateReinforcementValue(ps->ownedLands);
	float eref = s.calculateReinforcementValue(eps->ownedLands);

	featureReinforcementShare = ref / (ref + eref);

	featureAttackFrequency = __MIN(s.getAttacksDuringTurn() / 8.0f, 1.0f);
	featureCanDrawCard = s.getPlayerAllowedDrawCard() ? 1.0f : 0.0f;

	featureIsPhaseSetup = s.getRoundPhase() == RoundPhase::SETUP ? 1.0f : 0.0f;
	featureIsPhaseSetupNeutral = (s.getRoundPhase() == RoundPhase::SETUP_NEUTRAL ? 1.0f : 0.0f);
	featureIsPhaseReinforcement = (s.getRoundPhase() == RoundPhase::REINFORCEMENT ? 1.0f : 0.0f);
	featureIsPhaseAttack = (s.getRoundPhase() == RoundPhase::ATTACK ? 1.0f : 0.0f);
	featureIsPhaseAttackMobilization = (s.getRoundPhase() == RoundPhase::ATTACK_MOBILIZATION ? 1.0f : 0.0f);
	featureIsPhaseFortify = (s.getRoundPhase() == RoundPhase::FORTIFY ? 1.0f : 0.0f);

#if defined(INPUT_VECTOR_TYPE_2)
	float ta = ps->totalArmy;
	float eta = eps->totalArmy;
	featureArmyShare = ta / (ta + eta);
#endif
#endif // INPUT_VECTOR_TYPE_1
}
