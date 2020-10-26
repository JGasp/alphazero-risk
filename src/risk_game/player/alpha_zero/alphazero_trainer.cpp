#include "alphazero_trainer.h"

AlphaZeroTrainer::AlphaZeroTrainer()
{
	trainIteration = 0;
	trainStorage.data.reserve(SETTINGS.SAMPLES_STORAGE_MIN);

	randomPlayerGroup = std::shared_ptr<RandomPlayerGroup>(new RandomPlayerGroup(SETTINGS.getNumberOfPlayers()));
	scriptPlayerGroup = std::shared_ptr<ScriptPlayerGroup>(new ScriptPlayerGroup(SETTINGS.getNumberOfPlayers()));
}

void AlphaZeroTrainer::train(std::shared_ptr<AlphaZeroNNGroup> trainGroup, std::shared_ptr<AlphaZeroNNGroup> generateGroup)
{
	trainStorage.loadTrainingSamples(SETTINGS.DEFAULT_SAMPLES);
	trainStorage.registerHandler();

	printf("Started training\n");
	for (trainIteration = 0; trainIteration < SETTINGS.TRAIN_ITERATIONS; trainIteration++)
	{
		printf("Train iteration %d\n", trainIteration);

		generateTrainData(generateGroup);
		trainStorage.trimOldExamples();

		trainGroup->train(trainStorage.data, SETTINGS.EPOCHS);

		if (updateIfImprovement(trainGroup, generateGroup, true))
		{
			trainStorage.updateOldGamesIndex();
		}
	}

	trainStorage.saveTrainingSamples(SETTINGS.DEFAULT_SAMPLES);
}

void AlphaZeroTrainer::generateTrainData(std::shared_ptr<AlphaZeroNNGroup> generate)
{
	int currentSampelCount = trainStorage.data.size();
	printf("Generating training data current sample count %d\n", currentSampelCount);

	std::vector<std::thread> threads; threads.reserve(generate->size());
	std::vector<NNTrainDataStorage> storageGroup; storageGroup.resize(generate->size());

	std::shared_ptr<Counter> c(new Counter);
	c->setCount(SETTINGS.TRAIN_ITERATION_GAMES);
	c->setShowProgress(true);

	for (int i = 0; i < generate->size(); i++) // Spawn threads
	{
		std::shared_ptr<AlphaZeroNNId> nn = generate->getNN(i);
		NNTrainDataStorage* s = &storageGroup[i];
		threads.push_back(std::thread(&AlphaZeroTrainer::threadExecuteTrainingGame, this, nn, s, c));
	}
	for (int i = 0; i < generate->size(); i++) // Wait all threads to finish
	{
		threads[i].join();
	}
	
	for (auto& s : storageGroup)
	{
		trainStorage.extend(s);
	}

	printf("\n");
	if (SETTINGS.PERSIST_SAMPLES_DATA)
	{
		printf("Storing new training samples to disk\n");
		NNTrainDataStorage toSaveExamples;
		for (auto& s : storageGroup)
		{
			toSaveExamples.extend(s);
		}
		toSaveExamples.saveTrainingSamples(SETTINGS.DEFAULT_DATA + "/train_samples_" + std::to_string(trainIteration) + ".bin");
	}	

	int newSamples = trainStorage.data.size() - currentSampelCount;
	printf("Generated %d new samples for total %d\n", newSamples, int(trainStorage.data.size()));
}

void AlphaZeroTrainer::threadExecuteTrainingGame(std::shared_ptr<AlphaZeroNNId> nn, NNTrainDataStorage* nnStorage, std::shared_ptr<Counter> c)
{
	while (c->hasNext())
	{
		AlphaZeroMCTS mcts = AlphaZeroMCTS();

		State rootState = State();
		rootState.setLog(SETTINGS.LOG_STATE);
		rootState.newGame();

		int8_t gameState = -1;
		for (int i = 0; gameState == -1; i++)
		{
			mcts.simulate(rootState, nn);			

			std::shared_ptr<StateSimulations> ss = mcts.getStorage()->getStateSimulation(rootState);
			std::vector<float> policy = ss->calculateMoveProbability(1.0f);

			LandIndex li;
			if (rootState.getRound() > SETTINGS.TEMPERATURE_TRESHOLD) // Temp 0.0f => best move
			{
				li = mcts.pickHigestWeightedMove(policy);
			}
			else
			{
				li = mcts.pickRandomWeightedMove(policy);
			}			

			nnStorage->data.push_back(NNTrainData(rootState.getCurrentPlayerTurn(), NNInputData(rootState), NNOutputData(std::move(policy))));

			UtilityNN::makeMove(rootState, li);
			gameState = rootState.gameStatus();
		}
		rootState.logGameStatus();

		nnStorage->updateValues(gameState, rootState.getRound());

		c->hasFinished();
	}
}

void AlphaZeroTrainer::benchmark(std::shared_ptr<AlphaZeroPlayerGroup> azpg)
{
	printf("Playing benchmark games with random\n");
	GameResults rGR = GameGroup::playGames(azpg, randomPlayerGroup, SETTINGS.BENCHMARK_GAMES_RANDOM);
	printf("Model benchmark games played: %d \t Model: %d/%d \t Random: %d/%d\n", rGR.count, rGR.players[0].win, rGR.players[0].winAndStartedGame, rGR.players[1].win, rGR.players[1].winAndStartedGame);

	printf("Playing benchmark games with script\n");
	GameResults sGR = GameGroup::playGames(azpg, scriptPlayerGroup, SETTINGS.BENCHMARK_GAMES_SCRIPT);
	printf("Model benchmark games played: %d \t Model: %d/%d \t Script: %d/%d\n", sGR.count, sGR.players[0].win, sGR.players[0].winAndStartedGame, sGR.players[1].win, sGR.players[1].winAndStartedGame);

	LOG.getBenchmarkLog() << trainIteration << ',' << rGR << ", " << sGR << std::endl;
}

bool AlphaZeroTrainer::updateIfImprovement(std::shared_ptr<AlphaZeroNNGroup> trainGroup, std::shared_ptr<AlphaZeroNNGroup> generateGroup, bool doBenchmark)
{
	if (SETTINGS.COMPARE_GAMES > 0)
	{
		int samples = trainStorage.data.size();				

		std::shared_ptr<AlphaZeroPlayerGroup> trainAZPG(new AlphaZeroPlayerGroup(trainGroup));
		std::shared_ptr<AlphaZeroPlayerGroup> generateAZPG(new AlphaZeroPlayerGroup(generateGroup));

		printf("Playing comparison games betweean new and old model\n");
		GameResults gr;
		if (SETTINGS.INCLUDE_COMPARE_GAMES_TRAIN_SAMPLES)
		{			
			gr = GameGroup::playGames(trainAZPG, generateAZPG, SETTINGS.COMPARE_GAMES, trainStorage);
			printf("New samples generated from compare games %d\n", int(trainStorage.data.size() - samples));
		}
		else
		{
			gr = GameGroup::playGames(trainAZPG, generateAZPG, SETTINGS.COMPARE_GAMES);
		}		

		LOG.getImprovementLog() << trainIteration << ',' << gr << std::endl;

		if (isModelImproved(gr))
		{
			printf("Model improved\n");
			trainGroup->saveCheckpoint(SETTINGS.DEFAULT_BEST_CHECKPOINT);
			trainGroup->saveCheckpoint(SETTINGS.DEFAULT_CHECKPOINT_DIR + "/checkpoint-iter-" + std::to_string(trainIteration) + ".bin");
			generateGroup->loadCheckpoint(SETTINGS.DEFAULT_BEST_CHECKPOINT);
			
			if(doBenchmark) benchmark(generateAZPG);
			return true;
		}
		else
		{
			printf("Model did not improve\n");
			if (SETTINGS.TRAINING_REVERT_MODEL)
			{
				printf("Model reverted back old\n");
				trainGroup->loadCheckpoint(SETTINGS.DEFAULT_LATEST_CHECKPOINT);
			}
			return false;
		}
	}
	else
	{
		printf("Model improved (No compare games set)\n");
		trainGroup->saveCheckpoint(SETTINGS.DEFAULT_BEST_CHECKPOINT);
		trainGroup->saveCheckpoint(SETTINGS.DEFAULT_CHECKPOINT_DIR + "/checkpoint-iter-" + std::to_string(trainIteration) + ".bin");
		
		generateGroup->loadCheckpoint(SETTINGS.DEFAULT_BEST_CHECKPOINT);

		std::shared_ptr<AlphaZeroPlayerGroup> generateAZPG(new AlphaZeroPlayerGroup(generateGroup));
		if (doBenchmark) benchmark(generateAZPG);
		return true;
	}
}

bool AlphaZeroTrainer::isModelImproved(const GameResults& gr)
{
	PlayerGameResult newModel = gr.players[0];
	PlayerGameResult oldModel = gr.players[1];

	return newModel.win >= ((newModel.win + oldModel.win) * SETTINGS.COMPARE_TRESHOLD);		
}

void AlphaZeroTrainer::trainOnScript(std::shared_ptr<AlphaZeroNNGroup> nnGroup, std::shared_ptr<AlphaZeroNNGroup> oldNNGroup)
{
	trainStorage.loadTrainingSamples(SETTINGS.DEFAULT_SAMPLES);
	trainStorage.registerHandler();

	std::shared_ptr<AlphaZeroPlayerGroup> azpGroup(new AlphaZeroPlayerGroup(nnGroup));
	std::shared_ptr<AlphaZeroPlayerGroup> oldAzpGroup(new AlphaZeroPlayerGroup(oldNNGroup));

	printf("Started training on script player\n");
	for (trainIteration = 0; trainIteration < SETTINGS.TRAIN_ITERATIONS; trainIteration++)
	{
		printf("Train iteration %d\n", trainIteration);
		GameResults gr = GameGroup::playGames(azpGroup, scriptPlayerGroup, SETTINGS.TRAIN_ITERATION_GAMES * 2, trainStorage);
		LOG.getBenchmarkLog() << trainIteration << ",,,," << gr << std::endl;

		trainStorage.trimOldExamples();
		nnGroup->train(trainStorage.data, SETTINGS.EPOCHS);

		if (updateIfImprovement(nnGroup, oldNNGroup, false))
		{
			trainStorage.updateOldGamesIndex();
		}
	}

	trainStorage.saveTrainingSamples(SETTINGS.DEFAULT_SAMPLES);
}

void AlphaZeroTrainer::trainOnGeneratedData(std::shared_ptr<AlphaZeroNNGroup> trainGroup, std::shared_ptr<AlphaZeroNNGroup> generatorGroup)
{
	printf("Started training on generated data\n");
	for (int e = 0; e < SETTINGS.DATA_TRAIN_LOOPS; e++)
	{
		printf("===> Train loop %d\n", e);

		NNTrainDataStorage storage;

#ifdef LOG_PERFORMANCE
		auto generationStartTime = std::chrono::high_resolution_clock::now();
#endif // LOG_PERFORMANCE

		if (SETTINGS.DATA_GAMES_SS > 0)
		{
			printf("Generating training data with Script vs Script games %d\n", SETTINGS.DATA_GAMES_SS);
						
			std::shared_ptr<ScriptPlayer> p1 = std::shared_ptr<ScriptPlayer>(new ScriptPlayer());
			p1->setTrainStorage(&storage);
			std::shared_ptr<ScriptPlayer> p2 = std::shared_ptr<ScriptPlayer>(new ScriptPlayer());
			p2->setTrainStorage(&storage);

			Game game;
			game.addPlayer(p1);
			game.addPlayer(p2);

			size_t beforeSamples = storage.data.size();
			game.playGames(SETTINGS.DATA_GAMES_SS);
			size_t afterSamples = storage.data.size();

			printf("Samples generated %d\n", int(afterSamples-beforeSamples));
		}

		if (SETTINGS.DATA_GAMES_SR > 0)
		{
			printf("Generating training data with Script vs Random games: %d\n", SETTINGS.DATA_GAMES_SR);

			std::shared_ptr<ScriptPlayer> p1 = std::shared_ptr<ScriptPlayer>(new ScriptPlayer());
			p1->setTrainStorage(&storage);
			std::shared_ptr<RandomPlayer> p2 = std::shared_ptr<RandomPlayer>(new RandomPlayer());
			p2->setTrainStorage(&storage);

			Game game;
			game.addPlayer(p1);
			game.addPlayer(p2);

			size_t beforeSamples = storage.data.size();
			game.playGames(SETTINGS.DATA_GAMES_SR);
			size_t afterSamples = storage.data.size();

			printf("Samples generated %d\n", int(afterSamples - beforeSamples));
		}

#ifdef LOG_PERFORMANCE
		auto generationEndTime = std::chrono::high_resolution_clock::now();
		auto generationDuration = generationEndTime - generationStartTime;
		auto seconds = float(std::chrono::duration_cast<std::chrono::nanoseconds>(generationDuration).count()) / powf(10.0f, 9);

		auto samples = SETTINGS.DATA_GAMES_SS + SETTINGS.DATA_GAMES_SR;
		float sps = samples / seconds;

		//printf("%f samples per second", sps);
#endif // LOG_PERFORMANCE

		trainGroup->train(storage.data, 3);

		{
			printf("Playing comparison games betweean new and old model\n");
			std::shared_ptr<AlphaZeroPlayerGroup> azpGroup(new AlphaZeroPlayerGroup(trainGroup));
			std::shared_ptr<AlphaZeroPlayerGroup> oldAzpGroup(new AlphaZeroPlayerGroup(generatorGroup));

			GameResults grc = GameGroup::playGames(azpGroup, oldAzpGroup, SETTINGS.COMPARE_GAMES);			

			LOG.getImprovementLog() << grc << std::endl;

			if (isModelImproved(grc))
			{
				printf("Model improved\n");
				trainGroup->saveCheckpoint(SETTINGS.DEFAULT_CHECKPOINT_DIR + "/checkpoint-data-epoch-" + std::to_string(e) + ".bin");
				trainGroup->saveCheckpoint(SETTINGS.DEFAULT_LATEST_CHECKPOINT);
				generatorGroup->loadCheckpoint(SETTINGS.DEFAULT_LATEST_CHECKPOINT);

				printf("Playing benchmark games\n");
				std::shared_ptr<ScriptPlayerGroup> spg = std::shared_ptr<ScriptPlayerGroup>(new ScriptPlayerGroup(azpGroup->size()));
				GameResults grb = GameGroup::playGames(azpGroup, spg, SETTINGS.BENCHMARK_GAMES_SCRIPT);

				LOG.getBenchmarkLog() << grb << std::endl;
			}

			trainGroup->saveCheckpoint(SETTINGS.DEFAULT_CHECKPOINT_TEMP); // Keep latest model saved on HDD at all times
		}
	}
}