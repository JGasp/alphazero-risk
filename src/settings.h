#pragma once

#define CXXOPTS_NO_EXCEPTIONS
#include <cxxopts/cxxopts.h>

#include <thread>

#include "log.h"
#include "rng.h"


template<typename T1, typename T2>
constexpr auto __MAX(T1 x, T2  y) { return (((x) > (y)) ? (x) : (y)); }

template<typename T1, typename T2>
constexpr auto __MIN(T1 x, T2  y) { return (((x) < (y)) ? (x) : (y)); }


class Settings
{
public:
	std::string MODE = "play";
	std::string DEFAULT_GRAPH_DEF_PB = "model_bin.pb";
	std::string DEFAULT_CHECKPOINT_DIR = "checkpoints";
	std::string DEFAULT_BEST_CHECKPOINT = DEFAULT_CHECKPOINT_DIR + "/best-checkpoint.bin";
	std::string DEFAULT_LATEST_CHECKPOINT = DEFAULT_CHECKPOINT_DIR + "/latest-checkpoint.bin";
	std::string DEFAULT_CHECKPOINT_TEMP = DEFAULT_CHECKPOINT_DIR + "/temp.bin";

	std::string PLAYER_1 = "az";
	std::string GRAPH_DEF_PB_1 = DEFAULT_GRAPH_DEF_PB;
	std::string CHECKPOINT_1 = DEFAULT_LATEST_CHECKPOINT;

	std::string PLAYER_2 = "sp";
	std::string GRAPH_DEF_PB_2 = DEFAULT_GRAPH_DEF_PB;
	std::string CHECKPOINT_2 = DEFAULT_LATEST_CHECKPOINT;

	std::string DEFAULT_DATA = "data";
	std::string DEFAULT_SAMPLES = DEFAULT_DATA + "/training_samples.bin";


	int NUMBER_OF_GPUS = 1;
	int NUMBER_OF_CONCURENT_GAMES_PER_GPU = 4;
	int AVG_PRED_BATCH_SIZE = 32; // 64, 128, 256
	int THREADS_PER_MCTS = 2; // How many concurent thread are doing mcts simulation
	int MCTS_SIMULATIONS = 32; // 32; //300; // How many MCTS simulations for each search step

	bool LOG_STATE = false;
	bool LOG_NN_TRAINING = true;
	bool PERSIST_SAMPLES_DATA = false; // Save all generated data

	int MIN_UNIT_MOVE = 3;
	int MAX_GAME_ROUNDS = 30 + 28;
		
	bool LIMIT_REINFORCEMENT_MOVES = true; // Force alpha zero player to place reinforcement only on lands next to opponents
	bool LIMIT_ATTACK_MOVES = false; // Force alpha zero player to attack as long as he can
	bool MIRROR_GAMES = true; // Map is generaed only once for two games, where both players get to play with same initial conditions
	bool ALLOW_YIELD = true; // AlphaZero player will yield when losing to much

	long TRAIN_ITERATIONS = 10000; // How many times to execute train setp
	int TRAIN_ITERATION_GAMES = 1000; // How many games played each train step to generate train data
	float HP_EXPLORATION = 1.1f; // Exploration parameter cpuct
	float DIR_NOISE_VALUE = 0.3;
	float DIR_NOISE_EPSI = 0.25;
	int TEMPERATURE_TRESHOLD = 15 + 28; // Temperature treshold, encurages exploration in early state of game during training	
		
	int COMPARE_GAMES = 1000; // Number of games during comparions if model is improved
	float COMPARE_TRESHOLD = 0.55f; // percentage of won games
	bool INCLUDE_COMPARE_GAMES_TRAIN_SAMPLES = true; // include compared games into training samples
	int BENCHMARK_GAMES_RANDOM = 10; // Number of games during comparions if model is improved
	int BENCHMARK_GAMES_SCRIPT = 100; // Number of games during comparions if model is improved

	bool TRAINING_REVERT_MODEL = true; // When model fails to improve, revert to best current model
	int EPOCHS = 10;
	int BATCH_SIZE = 512; // Size of training batch samples
	int SAMPLES_STORAGE_MIN = 1024 * BATCH_SIZE; // Keep old samples till min is reached
	int SAMPLES_STORAGE_MAX = 16384 * BATCH_SIZE; // MAX Number of batch samples stored for training

	float DYNAMIC_EPOCH_THRESHOLD = 0.01f; // 1% improvement minimum	
	int DATA_GAMES_SS = 5000; // 5000;
	int DATA_GAMES_SR = 5000; // 5000;
	int DATA_TRAIN_LOOPS = 1000;

	int getNumberOfPlayers()
	{
		return NUMBER_OF_GPUS * NUMBER_OF_CONCURENT_GAMES_PER_GPU;
	}

public:
	void init(int argc, char* argv[]) // https://github.com/jarro2783/cxxopts
	{
		cxxopts::Options options("AlphaZero-Risk", "AlphaZero implementation for game Risk");
		options.add_options()
			("m", "Mode [train/play]", cxxopts::value<std::string>()->default_value(MODE))
			("g", "Default graph file path", cxxopts::value<std::string>()->default_value(DEFAULT_GRAPH_DEF_PB))
			("c", "Checkpoint file path", cxxopts::value<std::string>()->default_value(DEFAULT_LATEST_CHECKPOINT))

			("p1", "Player 1 [az/sp]", cxxopts::value<std::string>()->default_value(PLAYER_1))
			("g1", "Graph file path for player 1", cxxopts::value<std::string>()->default_value(GRAPH_DEF_PB_1))
			("c1", "Checkpoint file path for player 1", cxxopts::value<std::string>()->default_value(CHECKPOINT_1))

			("p2", "Player 2 [az/sp]", cxxopts::value<std::string>()->default_value(PLAYER_2))
			("g2", "Graph file path for player 2", cxxopts::value<std::string>()->default_value(GRAPH_DEF_PB_2))
			("c2", "Checkpoint file path for player 2", cxxopts::value<std::string>()->default_value(CHECKPOINT_2))

			("gpus", "Number of gpu units", cxxopts::value<int>()->default_value(std::to_string(NUMBER_OF_GPUS)))
			("gpu-games", "Number of concurent games per gpu", cxxopts::value<int>()->default_value(std::to_string(NUMBER_OF_CONCURENT_GAMES_PER_GPU)))
			("t", "Number of threads", cxxopts::value<int>()->default_value(std::to_string(THREADS_PER_MCTS)))		
			("apbs", "Set number of games per gpu to get avg. prediction batch size", cxxopts::value<int>()->default_value(std::to_string(AVG_PRED_BATCH_SIZE)))

			("lnt", "Log nn training", cxxopts::value<bool>()->default_value(std::to_string(LOG_NN_TRAINING)))
			("ls", "Log state", cxxopts::value<bool>()->default_value(std::to_string(LOG_STATE)))

			("dgss", "Number of data games for trin-data script vs script", cxxopts::value<int>()->default_value(std::to_string(DATA_GAMES_SS)))
			("dgsr", "Number of data games for trin-data script vs random", cxxopts::value<int>()->default_value(std::to_string(DATA_GAMES_SR)))
			("dtl", "Number of train loops for data games", cxxopts::value<int>()->default_value(std::to_string(DATA_TRAIN_LOOPS)))

			("allow-yield", "Allow yield when enemy ownes 3/4 of lands", cxxopts::value<bool>()->default_value(std::to_string(ALLOW_YIELD)))
			("limit-reinforcement", "Limit reinforcement moves", cxxopts::value<bool>()->default_value(std::to_string(LIMIT_REINFORCEMENT_MOVES)))
			("limit-attack", "Limit attack moves", cxxopts::value<bool>()->default_value(std::to_string(LIMIT_ATTACK_MOVES)))
			("mirror-games", "Play games in pair with mirrored initial position", cxxopts::value<bool>()->default_value(std::to_string(MIRROR_GAMES)))
			
			("ti", "Number of train iterations", cxxopts::value<long>()->default_value(std::to_string(TRAIN_ITERATIONS)))
			("tg", "Games played per train iteration", cxxopts::value<int>()->default_value(std::to_string(TRAIN_ITERATION_GAMES)))
			("mcts", "Number of MCTS simulations", cxxopts::value<int>()->default_value(std::to_string(MCTS_SIMULATIONS)))
			
			("hp", "Exploration factor", cxxopts::value<float>()->default_value(std::to_string(HP_EXPLORATION)))
			("dnv", "Dirchlet noise value", cxxopts::value<float>()->default_value(std::to_string(DIR_NOISE_VALUE)))
			("dne", "Dirchlet noise epsi", cxxopts::value<float>()->default_value(std::to_string(DIR_NOISE_EPSI)))
			("temp", "Temperature trehsold", cxxopts::value<int>()->default_value(std::to_string(TEMPERATURE_TRESHOLD)))

			("e", "Number of epochs per train iteration", cxxopts::value<int>()->default_value(std::to_string(EPOCHS)))
			("bs", "Batch size", cxxopts::value<int>()->default_value(std::to_string(BATCH_SIZE)))
			("cg", "Number of games for comparison", cxxopts::value<int>()->default_value(std::to_string(COMPARE_GAMES)))
			("ct", "Treshold for accepted improvement", cxxopts::value<float>()->default_value(std::to_string(COMPARE_TRESHOLD)))
			("s", "Number of stored samples", cxxopts::value<int>()->default_value(std::to_string(SAMPLES_STORAGE_MIN)))
			
			("h,help", "Display help", cxxopts::value<bool>()->default_value(std::to_string(false)));


		cxxopts::ParseResult result = options.parse(argc, argv);		

		if (result.count("help"))
		{
			std::cout << options.help() << std::endl;
			exit(0);
		}

		MODE = result["m"].as<std::string>();
		DEFAULT_GRAPH_DEF_PB = result["g"].as<std::string>();
		DEFAULT_LATEST_CHECKPOINT = result["c"].as<std::string>();

		PLAYER_1 = result["p1"].as<std::string>();
		GRAPH_DEF_PB_1 = result["g1"].as<std::string>();
		CHECKPOINT_1 = result["c1"].as<std::string>();

		PLAYER_2 = result["p2"].as<std::string>();
		GRAPH_DEF_PB_2 = result["g2"].as<std::string>();
		CHECKPOINT_2 = result["c2"].as<std::string>();
		
		THREADS_PER_MCTS = result["t"].as<int>();
		NUMBER_OF_GPUS = result["gpus"].as<int>();

		if (result["gpu-games"].count() > 0)
		{
			NUMBER_OF_CONCURENT_GAMES_PER_GPU = result["gpu-games"].as<int>(); // Override;		
		}
		else
		{
			NUMBER_OF_CONCURENT_GAMES_PER_GPU = AVG_PRED_BATCH_SIZE / THREADS_PER_MCTS * 2;
			// NUMBER_OF_CONCURENT_GAMES_PER_GPU = __MAX(1, std::thread::hardware_concurrency() * 2 / NUMBER_OF_GPUS / THREADS_PER_MCTS);
		}		

		DATA_GAMES_SS = result["dgss"].as<int>();
		DATA_GAMES_SR = result["dgsr"].as<int>();
		DATA_TRAIN_LOOPS = result["dtl"].as<int>();
		LOG_STATE = result["ls"].as<bool>();

		ALLOW_YIELD = result["allow-yield"].as<bool>();
		LIMIT_REINFORCEMENT_MOVES = result["limit-reinforcement"].as<bool>();
		LIMIT_ATTACK_MOVES = result["limit-attack"].as<bool>();
		MIRROR_GAMES = result["mirror-games"].as<bool>();
		
		TRAIN_ITERATIONS = result["ti"].as<long>();
		TRAIN_ITERATION_GAMES = result["tg"].as<int>();
		MCTS_SIMULATIONS = result["mcts"].as<int>();

		HP_EXPLORATION = result["hp"].as<float>();
		DIR_NOISE_VALUE = result["dnv"].as<float>();
		DIR_NOISE_EPSI = result["dne"].as<float>();
		TEMPERATURE_TRESHOLD = result["temp"].as<int>();

		EPOCHS = result["e"].as<int>();
		BATCH_SIZE = result["bs"].as<int>();
		COMPARE_GAMES = result["cg"].as<int>();
		COMPARE_TRESHOLD = result["ct"].as<float>();
		SAMPLES_STORAGE_MIN = result["s"].as<int>();

		std::ofstream outSettigns("log/settings.txt", std::ofstream::out);
		auto it = options.m_options->begin();
		while (it != options.m_options->end())
		{
			std::string value = it->second->value().get_default_value(); // Default value
			if (result[it->first].count() > 0) // Non default value
			{
				value = result[it->first].m_raw_value;
			}

			outSettigns << it->first << "(" << it->second->description() << ")" << "=" << value << std::endl;
			it++;
		}
	}

	static Settings& getInstance()
	{
		static Settings INSTANCE;
		return INSTANCE;
	}
};

static Settings& SETTINGS = Settings::getInstance();
