#include "alphazero_gpu_cluster.h"

std::string AlphaZeroGPU::getDevicePath(int index)
{
	return "/device:GPU:" + std::to_string(index);
}

std::string AlphaZeroGPU::getDeviceTag()
{
	return deviceTag;
}

void AlphaZeroGPU::threadProcessPredictions(int nnIndex)
{
	std::shared_ptr<AlphaZeroNN> nn = neuralNetworks[nnIndex];

#ifdef LOG_PERFORMANCE
	int countPerformanceLog = 0;
	uint64_t totalSamples = 0;
	std::chrono::nanoseconds totalWaiting(0);
	std::chrono::nanoseconds totalProcessing(0);	
	LOG.getNNPerformanceLog() << std::endl; // Open file
#endif // LOG_PERFORMANCE
	while (running)
	{
#ifdef LOG_PERFORMANCE
		auto startWaiting = std::chrono::high_resolution_clock::now();
#endif // LOG_PERFORMANCE

		nn->waitQueueToFill();

#ifdef LOG_PERFORMANCE
		auto endWaiting = std::chrono::high_resolution_clock::now();
		auto startProcessing = std::chrono::high_resolution_clock::now();
#endif // LOG_PERFORMANCE

		std::lock_guard<std::mutex> lock_gpu(lock);
		int samples = nn->processBatchPrediction();
						
#ifdef LOG_PERFORMANCE
		auto endProcessing = std::chrono::high_resolution_clock::now();
		
		if (countPerformanceLog > 10 && countPerformanceLog < 110) // Skip first warmup
		{
			totalSamples += samples;

			auto waitingDuration = endWaiting - startWaiting;
			totalWaiting += waitingDuration;

			auto processingDuration = endProcessing - startProcessing;
			totalProcessing += processingDuration;


			auto nanosecondsPerSample = processingDuration.count() / samples;
			LOG.getNNPerformanceLog() << std::chrono::duration_cast<std::chrono::nanoseconds>(waitingDuration).count() << " ns, "
				<< std::chrono::duration_cast<std::chrono::nanoseconds>(processingDuration).count() << " ns, "
				<< nanosecondsPerSample << " ns/sample, "
				<< samples << " samples" << std::endl;
		}
		else if (countPerformanceLog == 110)
		{
			auto nanosecondsPerSample = std::chrono::duration_cast<std::chrono::nanoseconds>(totalProcessing).count() / totalSamples;
			LOG.getNNPerformanceLog() << "AVG: " << std::chrono::duration_cast<std::chrono::nanoseconds>(totalWaiting).count() / 100 << " ns, "
				<< std::chrono::duration_cast<std::chrono::nanoseconds>(totalProcessing).count() / 100 << " ns, "
				<< nanosecondsPerSample << " ns/sample" << std::endl;
		}

		countPerformanceLog++;
#endif // LOG_PERFORMANCE
	}
}

std::shared_ptr<AlphaZeroNNId> AlphaZeroGPU::addNeuralNetwork(std::string filePath)
{
	printf("Creating NN on gpu %s from graph %s\n", deviceTag.c_str(), filePath.c_str());

	std::lock_guard<std::mutex> guard(lock);

	std::shared_ptr<AlphaZeroNN> nn = std::shared_ptr<AlphaZeroNN>(new AlphaZeroNN());
	nn->loadGraph(filePath, deviceTag);

	neuralNetworks.push_back(nn);
	processingThreads.push_back(std::thread(&AlphaZeroGPU::threadProcessPredictions, this, neuralNetworks.size()-1));

	return std::shared_ptr<AlphaZeroNNId>(new AlphaZeroNNId(cluster, gpuIndex, neuralNetworks.size() - 1));
}

std::shared_ptr<AlphaZeroNN> AlphaZeroGPU::getNN(int nnIndex)
{
	return neuralNetworks[nnIndex];
}

void AlphaZeroGPU::train(int nnIndex, const std::vector<NNTrainData>& trainData, int epochs)
{
	std::lock_guard guard(lock);
	neuralNetworks[nnIndex]->train(trainData, epochs);
}

NNOutputData AlphaZeroGPU::predict(int nnIndex, const NNInputData& state)
{
	std::lock_guard guard(lock);
	return neuralNetworks[nnIndex]->predict(state);
}

void AlphaZeroCluster::initGpus(std::shared_ptr<AlphaZeroCluster> cluster, int numberOfGpus)
{
	printf("Initializing gpus: %d\n", numberOfGpus);
	for (int i = 0; i < numberOfGpus; i++)
	{
		std::shared_ptr<AlphaZeroGPU> gpu(new AlphaZeroGPU(cluster, i));
		gpus.push_back(gpu);
	}
}

std::shared_ptr<AlphaZeroNNId> AlphaZeroCluster::addNeuralNetwork(int gpuIndex, std::string grouprName, std::string filePath)
{
	printf("Creating NN for group %s\n", grouprName.c_str());
	std::shared_ptr<AlphaZeroNNId> id = gpus[gpuIndex]->addNeuralNetwork(filePath);

	if (!grouprName.empty())
	{		
		if (groups.contains(grouprName))
		{
			groups[grouprName] = std::shared_ptr<AlphaZeroNNGroup>(new AlphaZeroNNGroup(grouprName));
		}

		auto& group = groups.at(grouprName);
		group->add(id);
	}

	return id;
}

std::shared_ptr<AlphaZeroGPU> AlphaZeroCluster::getGPU(int gpuIndex)
{
	return gpus[gpuIndex];
}

std::shared_ptr<AlphaZeroNNGroup> AlphaZeroCluster::getGroup(std::string groupName)
{
	return groups[groupName];
}

std::shared_ptr<AlphaZeroNNGroup> AlphaZeroCluster::initPlayerGroup(std::string groupName, std::string graphFilePath)
{
	if (!groups.contains(groupName))
	{
		std::shared_ptr<AlphaZeroNNGroup> group(new AlphaZeroNNGroup(groupName));
		groups[groupName] = group;

		for (auto& gpu : gpus)
		{
			printf("Creating NN for group %s\n", groupName.c_str());
			std::shared_ptr<AlphaZeroNNId> nnId = gpu->addNeuralNetwork(graphFilePath);
			group->add(nnId);
		}

		return group;
	}
	else
	{
		throw std::invalid_argument("Duplicated player group");
	}	
}

void AlphaZeroNNId::loadCheckpoint(std::string filePath)
{
	printf("NN on gpuIndex %d with id %d loading checkpoint %s\n", gpuIndex, nnId, filePath.c_str());
	cluster->getGPU(gpuIndex)->getNN(nnId)->loadCheckpoint(filePath);
}

void AlphaZeroNNGroup::loadCheckpoint(std::string filePath)
{
	for (auto& id : neuralNetworkIds)
	{
		id->loadCheckpoint(filePath);
	}
}

void AlphaZeroNNId::saveCheckpoint(std::string filePath)
{
	printf("NN on gpuIndex %d with id %d saving checkpoint %s\n", gpuIndex, nnId, filePath.c_str());
	cluster->getGPU(gpuIndex)->getNN(nnId)->saveCheckpoint(filePath);
}

void AlphaZeroNNGroup::saveCheckpoint(std::string filePath)
{
	neuralNetworkIds[0]->saveCheckpoint(filePath);
}

void AlphaZeroNNId::train(const std::vector<NNTrainData>& trainData, int epochs)
{
	cluster->getGPU(gpuIndex)->train(nnId, trainData, epochs);
}

void AlphaZeroNNId::registerThread()
{
	cluster->getGPU(gpuIndex)->getNN(nnId)->registerThread();
}

void AlphaZeroNNId::unregisterThread()
{
	cluster->getGPU(gpuIndex)->getNN(nnId)->unregisterThread();
}

std::future<NNOutputData> AlphaZeroNNId::predictFuture(const NNInputData& state)
{
	return cluster->getGPU(gpuIndex)->getNN(nnId)->predictFuture(state);
}

NNOutputData AlphaZeroNNId::predict(const NNInputData& state)
{
	return cluster->getGPU(gpuIndex)->predict(nnId, state);
}

void AlphaZeroNNGroup::add(std::shared_ptr<AlphaZeroNNId> instance)
{
	neuralNetworkIds.push_back(instance);
}

void AlphaZeroNNGroup::train(const std::vector<NNTrainData>& trainData, int epochs)
{
	auto& nn = neuralNetworkIds[0];
	nn->train(trainData, epochs);
	nn->saveCheckpoint(SETTINGS.DEFAULT_CHECKPOINT_TEMP);

	for (int i=1; i<neuralNetworkIds.size(); i++)
	{
		neuralNetworkIds[i]->loadCheckpoint(SETTINGS.DEFAULT_CHECKPOINT_TEMP);
	}
}

int AlphaZeroNNGroup::size()
{
	return neuralNetworkIds.size();
}

std::shared_ptr<AlphaZeroNNId> AlphaZeroNNGroup::getNN(int index)
{
	return neuralNetworkIds[index];
}
