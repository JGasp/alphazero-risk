#pragma once

#include <chrono>

#include "alphazero_nn.h"

static const std::string NEW_NN_GROUP = "new_group";
static const std::string OLD_NN_GROUP = "old_group";
static const int CUDA_DEFAULT_DEVICE = 0;


class AlphaZeroCluster;

class AlphaZeroNNId
{
private:
	std::shared_ptr<AlphaZeroCluster> cluster;
	int gpuIndex;
	int nnId;

public:
	AlphaZeroNNId(std::shared_ptr<AlphaZeroCluster> cluster, int gpuIndex, int nnId): cluster(cluster), gpuIndex(gpuIndex), nnId(nnId) 
	{
		printf("Created NN on gpuIndex %d with id %d\n", gpuIndex, nnId);
	};

	void loadCheckpoint(std::string filePath); // Thread safe
	void saveCheckpoint(std::string filePath); // Thread safe
	void train(const std::vector<NNTrainData>& trainData, int epochs); // Thread safe

	void registerThread(); // Tell NN prediction batch to wait for thread
	void unregisterThread(); // Tell NN prediction batch to stop waiting for thread

	std::future<NNOutputData> predictFuture(const NNInputData& state); // Thread safe
	NNOutputData predict(const NNInputData& state); // Thread safe
};

class AlphaZeroNNGroup
{
private:
	std::string name;
	std::vector<std::shared_ptr<AlphaZeroNNId>> neuralNetworkIds;

public:
	AlphaZeroNNGroup(std::string name): name(name) 
	{
		printf("Created NN Group %s\n", name.c_str());
	};

	void add(std::shared_ptr<AlphaZeroNNId> instance);

	void loadCheckpoint(std::string filePath);
	void saveCheckpoint(std::string filePath);
	void train(const std::vector<NNTrainData>& trainData, int epochs);

	int size();
	std::shared_ptr<AlphaZeroNNId> getNN(int nnIndex);
};


class AlphaZeroGPU
{
private:
	std::shared_ptr<AlphaZeroCluster> cluster;

	std::mutex lock; // Is gpu in use
	int gpuIndex;
	std::string deviceTag;	

	bool running = true;

	std::vector<std::shared_ptr<AlphaZeroNN>> neuralNetworks;
	std::vector<std::thread> processingThreads;	

	void threadProcessPredictions(int nnIndex); // Thread safe
public:
	AlphaZeroGPU(std::shared_ptr<AlphaZeroCluster> cluster, int gpuIndex): cluster(cluster), gpuIndex(gpuIndex), deviceTag(getDevicePath(gpuIndex)) 
	{ 
		printf("Initialized GPU %s\n", deviceTag.c_str());
	};

	std::shared_ptr<AlphaZeroNNId> addNeuralNetwork(std::string filePath);
	std::shared_ptr<AlphaZeroNN> getNN(int nnIndex);
	
	void train(int nnIndex, const std::vector<NNTrainData>& trainData, int epochs); // Thread safe
	NNOutputData predict(int nnIndex, const NNInputData& state); // Thread safe

	static std::string getDevicePath(int index);

	std::string getDeviceTag();
};


class AlphaZeroCluster
{
	std::vector<std::shared_ptr<AlphaZeroGPU>> gpus;
	std::unordered_map<std::string, std::shared_ptr<AlphaZeroNNGroup>> groups;

public:
	AlphaZeroCluster() 
	{
		printf("Creating AZ Cluster\n");
	};
	void initGpus(std::shared_ptr<AlphaZeroCluster> cluster, int numberOfGpus);

	std::shared_ptr<AlphaZeroNNId> addNeuralNetwork(int gpuIndex, std::string clusterName, std::string filePath);
	std::shared_ptr<AlphaZeroGPU> getGPU(int gpuIndex);
	std::shared_ptr<AlphaZeroNNGroup> getGroup(std::string groupName);

	std::shared_ptr<AlphaZeroNNGroup> initPlayerGroup(std::string groupName, std::string graphFilePath);
};