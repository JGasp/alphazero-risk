#include "alphazero_risk.h"


void executePlay()
{
	std::shared_ptr<AlphaZeroCluster> nnCluster(new AlphaZeroCluster());
	nnCluster->initGpus(nnCluster, SETTINGS.NUMBER_OF_GPUS);

	std::shared_ptr<PlayerGroup> group1;
	std::shared_ptr<PlayerGroup> group2;

	if (SETTINGS.PLAYER_1 == "az")
	{
		auto group = nnCluster->initPlayerGroup("az1", SETTINGS.GRAPH_DEF_PB_1);
		group->loadCheckpoint(SETTINGS.CHECKPOINT_1);

		group1 = std::shared_ptr<PlayerGroup>(new AlphaZeroPlayerGroup(group));
	}
	else if (SETTINGS.PLAYER_1 == "sp")
	{
		group1 = std::shared_ptr<PlayerGroup>(new ScriptPlayerGroup(SETTINGS.getNumberOfPlayers()));
	}
	else if (SETTINGS.PLAYER_1 == "rp")
	{
		group1 = std::shared_ptr<PlayerGroup>(new RandomPlayerGroup(SETTINGS.getNumberOfPlayers()));
	}

	if (SETTINGS.PLAYER_2 == "az")
	{
		auto group = nnCluster->initPlayerGroup("az1", SETTINGS.GRAPH_DEF_PB_2);
		group->loadCheckpoint(SETTINGS.CHECKPOINT_2);

		group2 = std::shared_ptr<PlayerGroup>(new AlphaZeroPlayerGroup(group));
	}
	else if (SETTINGS.PLAYER_2 == "sp")
	{
		group2 = std::shared_ptr<PlayerGroup>(new ScriptPlayerGroup(SETTINGS.getNumberOfPlayers()));
	}
	else if (SETTINGS.PLAYER_2 == "rp")
	{
		group2 = std::shared_ptr<PlayerGroup>(new RandomPlayerGroup(SETTINGS.getNumberOfPlayers()));
	}

	GameResults gr = GameGroup::playGames(group1, group2, SETTINGS.COMPARE_GAMES);

	printf("Games: %d\nDraws:%d\nPlayer 1:%d\nPlayer 2:%d\n", gr.count, gr.draw, gr.players[0].win, gr.players[1].win);
}

void executeTrain()
{
	std::shared_ptr<AlphaZeroCluster> nnCluster(new AlphaZeroCluster());
	nnCluster->initGpus(nnCluster, SETTINGS.NUMBER_OF_GPUS);

	auto trainGroup = nnCluster->initPlayerGroup("az_train", SETTINGS.GRAPH_DEF_PB_1);
	trainGroup->loadCheckpoint(SETTINGS.DEFAULT_LATEST_CHECKPOINT);

	auto generateGroup = nnCluster->initPlayerGroup("az_generate", SETTINGS.GRAPH_DEF_PB_1);
	generateGroup->loadCheckpoint(SETTINGS.DEFAULT_LATEST_CHECKPOINT);

	AlphaZeroTrainer trainer;
	trainer.train(trainGroup, generateGroup);
}

void executeAnalysis()
{
	NNTrainDataStorage storage;

	std::string path = "data";
	for (const auto& entry : std::filesystem::directory_iterator(path))
	{
		NNTrainDataStorage fileStorage;
		fileStorage.loadTrainingSamples(entry.path().string());
		storage.extend(fileStorage);
		break;
	}

	AlphaZeroNN nn = AlphaZeroNN();
	nn.loadGraph(SETTINGS.DEFAULT_GRAPH_DEF_PB, AlphaZeroGPU::getDevicePath(0));
	nn.initWeights();

	nn.trainCrossValidation(storage.data, 10);
}

void executeTrainScript()
{
	std::shared_ptr<AlphaZeroCluster> nnCluster(new AlphaZeroCluster());
	nnCluster->initGpus(nnCluster, SETTINGS.NUMBER_OF_GPUS);

	auto trainGroup = nnCluster->initPlayerGroup("az_train", SETTINGS.DEFAULT_GRAPH_DEF_PB);
	trainGroup->loadCheckpoint(SETTINGS.DEFAULT_LATEST_CHECKPOINT);

	auto generateGroup = nnCluster->initPlayerGroup("az_generate", SETTINGS.DEFAULT_GRAPH_DEF_PB);
	generateGroup->loadCheckpoint(SETTINGS.DEFAULT_LATEST_CHECKPOINT);

	AlphaZeroTrainer trainer;
	trainer.trainOnScript(trainGroup, generateGroup);
}

void executeTrinData()
{
	std::shared_ptr<AlphaZeroCluster> nnCluster(new AlphaZeroCluster());
	nnCluster->initGpus(nnCluster, SETTINGS.NUMBER_OF_GPUS);

	auto trainGroup = nnCluster->initPlayerGroup("az_train", SETTINGS.DEFAULT_GRAPH_DEF_PB);
	trainGroup->loadCheckpoint(SETTINGS.DEFAULT_LATEST_CHECKPOINT);

	auto generateGroup = nnCluster->initPlayerGroup("az_generate", SETTINGS.DEFAULT_GRAPH_DEF_PB);
	generateGroup->loadCheckpoint(SETTINGS.DEFAULT_LATEST_CHECKPOINT);

	AlphaZeroTrainer trainer;
	trainer.trainOnGeneratedData(trainGroup, generateGroup);
}

//void testBatchNormalization()
//{
//	std::shared_ptr<AlphaZeroNN> nn(new AlphaZeroNN());
//	nn->loadGraph(SETTINGS.DEFAULT_GRAPH_DEF_PB, AlphaZeroGPU::getDevicePath(0));
//	nn->loadCheckpoint(SETTINGS.DEFAULT_LATEST_CHECKPOINT);
//
//	State s; s.newGame();
//	NNInputData in(s);
//	NNOutputData out = nn->predict(in);
//
//	std::vector<NNInputData> inputs;
//	inputs.push_back(in);
//
//	for (int i = 0; i < 3; i++)
//	{
//		State sn;
//		sn.newGame();
//		inputs.push_back(NNInputData(sn));
//		sn.invertPlayers();
//		inputs.push_back(NNInputData(sn));
//
//		for (int i = 0; i < LAND_INDEX_SIZE; i++)
//		{
//			LandArmy la = sn.getLandArmy(i);
//			if (la.playerIndex == sn.getCurrentPlayerTurn())
//			{
//				land_army_t s = sn.getLandArmySpace(i);
//				sn.addLandArmy(i, s, la.playerIndex);
//			}			
//		}
//		inputs.push_back(NNInputData(sn));
//	}
//
//	std::vector<NNOutputData> outputs = nn->predict(inputs);
//
//	for (auto& out : outputs)
//	{
//		printf("Value %f\n", out.value);
//	}
//
//	if (outputs[0].value != out.value)
//	{
//		printf("Error");
//	}
//}

void executeProgram()
{
	printf("===> Starting program with GPUs: %d, Games per GPU %d, MCTS threads: %d, MCTS simulations %d\n",
		SETTINGS.NUMBER_OF_GPUS, SETTINGS.NUMBER_OF_CONCURENT_GAMES_PER_GPU,
		SETTINGS.THREADS_PER_MCTS, SETTINGS.MCTS_SIMULATIONS);

	if (SETTINGS.MODE == "train")
	{
		executeTrain();
	}
	else if (SETTINGS.MODE == "play")
	{
		executePlay();
	}
	else if (SETTINGS.MODE == "analysis")
	{
		executeAnalysis();
	}
	else if (SETTINGS.MODE == "train-script")
	{
		executeTrainScript();
	}
	else if (SETTINGS.MODE == "train-data")
	{
		executeTrinData();
	}
}

int main(int argc, char* argv[])
{
	LOG.init();
	SETTINGS.init(argc, argv);

	tensorflow::port::InitMain(argv[0], &argc, &argv);	

	executeProgram();
	//testBatchNormalization();

	return 0;
}