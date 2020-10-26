#include "alphazero_nn.h"

std::string UtilityNN::getCheckpointName(std::string name)
{
	time_t curtime;
	time(&curtime);
	return SETTINGS.DEFAULT_LATEST_CHECKPOINT + "-" + name + "-" + ctime(&curtime);
}

tensorflow::Tensor UtilityNN::buildTensor(std::string value)
{
	tensorflow::Tensor t(tensorflow::DT_STRING, tensorflow::TensorShape());
	t.scalar<tensorflow::tstring>()() = tensorflow::tstring(value);
	return t;
}

tensorflow::Tensor UtilityNN::buildTensor(bool value)
{
	tensorflow::Tensor t(tensorflow::DT_BOOL, tensorflow::TensorShape());
	t.scalar<bool>()() = value;
	return t;
}

tensorflow::Tensor UtilityNN::buildTensor(float value)
{
	tensorflow::Tensor t(tensorflow::DT_FLOAT, tensorflow::TensorShape());
	t.scalar<float>()() = value;
	return t;
}

void setInStateTensor(tensorflow::Tensor& t, int sampleRow, const NNInputData& data) // IMPORTANT, SET ALL FIELDS IN TENSOR
{
	for (int y = 0; y < MAP_Y; y++)
	{		
		for (int x = 0; x < MAP_X; x++)
		{
			const LandArmy& la = data.land[y * MAP_X + x];
			float featureArmy = float(la.army) / LAND_ARMY_MAX;

			int currentPlayer = data.playerIndex;
			int enemyPlayer = currentPlayer == 0 ? 1 : 0;

#if defined(INPUT_VECTOR_TYPE_1) || defined(INPUT_VECTOR_TYPE_2) || defined(INPUT_VECTOR_TYPE_3)
			t.tensor<float, 4>()(sampleRow, y, x, IF_CURRENT_PLAYER) = currentPlayer == la.playerIndex ? featureArmy : 0.0f;
			t.tensor<float, 4>()(sampleRow, y, x, IF_ENEMY_PLAYER) = enemyPlayer == la.playerIndex ? featureArmy : 0.0f;
			t.tensor<float, 4>()(sampleRow, y, x, IF_NEUTRAL_PLAYER) = NEUTRAL_PLAYER == la.playerIndex ? featureArmy : 0.0f;

			t.tensor<float, 4>()(sampleRow, y, x, IF_REINFORCEMENT_SHARE) = data.featureReinforcementShare;
			t.tensor<float, 4>()(sampleRow, y, x, IF_ATTACKS_DURING_TURN) = data.featureAttackFrequency;
			t.tensor<float, 4>()(sampleRow, y, x, IF_CAN_DRAW_CARD) = data.featureCanDrawCard;

			t.tensor<float, 4>()(sampleRow, y, x, IF_PHASE_SETUP) = data.featureIsPhaseSetup;
			t.tensor<float, 4>()(sampleRow, y, x, IF_PHASE_SETUP_NEUTRAL) = data.featureIsPhaseSetupNeutral;
			t.tensor<float, 4>()(sampleRow, y, x, IF_PHASE_REINFORCEMENT) = data.featureIsPhaseReinforcement;
			t.tensor<float, 4>()(sampleRow, y, x, IF_PHASE_ATTACK) = data.featureIsPhaseAttack;
			t.tensor<float, 4>()(sampleRow, y, x, IF_PHASE_ATTACK_MOBILIZATION) = data.featureIsPhaseAttackMobilization;
			t.tensor<float, 4>()(sampleRow, y, x, IF_PHASE_FORTIFY) = data.featureIsPhaseFortify;
#if defined(INPUT_VECTOR_TYPE_2) || defined(INPUT_VECTOR_TYPE_3)
			t.tensor<float, 4>()(sampleRow, y, x, IF_ARMY_SHARE) = data.featureArmyShare;
#if defined(INPUT_VECTOR_TYPE_3)
			t.tensor<float, 4>()(sampleRow, y, x, IF_ROUND) = float(data.round) / SETTINGS.MAX_GAME_ROUNDS;
#endif
#endif
#endif
		}		
	}
}

tensorflow::Tensor UtilityNN::buildInTensor(const NNInputData& data)
{
	tensorflow::Tensor t(tensorflow::DT_FLOAT, tensorflow::TensorShape({ 1, MAP_Y, MAP_X, TF_INPUT_FEATURES }));
	setInStateTensor(t, 0, data);
	return t;
}

tensorflow::Tensor UtilityNN::buildInsFutureTensor(const std::vector<FuturePrediction>& values)
{
	tensorflow::Tensor t(tensorflow::DT_FLOAT, tensorflow::TensorShape({ (int) values.size(), MAP_Y, MAP_X, TF_INPUT_FEATURES }));
	for (int i = 0; i < values.size(); i++)
	{
		auto& data = values[i].in;
		setInStateTensor(t, i, data);
	}
	return t;
}

tensorflow::Tensor UtilityNN::buildInsTensor(const std::vector<const NNInputData*>& values)
{
	tensorflow::Tensor t(tensorflow::DT_FLOAT, tensorflow::TensorShape({ (int)values.size(), MAP_Y, MAP_X, TF_INPUT_FEATURES }));
	for (int i = 0; i < values.size(); i++)
	{
		auto& data = *values[i];
		setInStateTensor(t, i, data);
	}
	return t;
}


void setOutValueTensor(tensorflow::Tensor& t, const NNOutputData& value, int row)
{
	t.tensor<float, 2>()(row, 0) = value.value;
}

tensorflow::Tensor UtilityNN::buildOutValueTensor(const NNOutputData& value)
{
	tensorflow::Tensor t(tensorflow::DT_FLOAT, tensorflow::TensorShape({ 1, TF_OUTPUT_VALUE_TENSOR_SIZE }));
	setOutValueTensor(t, value, 0);
	return t;
}

tensorflow::Tensor UtilityNN::buildOutsValueTensor(const std::vector<const NNOutputData*>& values)
{
	tensorflow::Tensor t(tensorflow::DT_FLOAT, tensorflow::TensorShape({ (int)values.size(), TF_OUTPUT_VALUE_TENSOR_SIZE }));

	for (int i = 0; i < values.size(); i++)
	{
		setOutValueTensor(t, *values[i], i);
	}
	return t;
}



void setOutPolicyTensor(tensorflow::Tensor& t, const NNOutputData& value, int row)
{
	for (int j = 0; j < value.policy.size(); j++)
	{
		t.tensor<float, 2>()(row, j) = value.policy[j];
	}
}

tensorflow::Tensor UtilityNN::buildOutPolicyTensor(const NNOutputData& value)
{
	tensorflow::Tensor t(tensorflow::DT_FLOAT, tensorflow::TensorShape({ 1, TF_OUTPUT_POLICY_TENSOR_SIZE }));
	setOutPolicyTensor(t, value, 0);
	return t;
}


tensorflow::Tensor UtilityNN::buildOutsPolicyTensor(const std::vector<const NNOutputData*>& values)
{
	tensorflow::Tensor t(tensorflow::DT_FLOAT, tensorflow::TensorShape({ (int)values.size(), TF_OUTPUT_POLICY_TENSOR_SIZE }));

	for (int i = 0; i < values.size(); i++)
	{
		setOutPolicyTensor(t, *values[i], i);
	}
	return t;
}

std::vector<NNOutputData> UtilityNN::buildOutput(const tensorflow::Tensor& policyTensor, const tensorflow::Tensor& valueTensor)
{	
	int rows = policyTensor.tensor<float, 2>().dimension(0);
	std::vector<NNOutputData> output(rows);

	for (int i = 0; i < rows; i++)
	{
		output[i].policy.reserve(TF_OUTPUT_POLICY_TENSOR_SIZE);
		for (int j = 0; j < TF_OUTPUT_POLICY_TENSOR_SIZE; j++)
		{
			float pVal = policyTensor.tensor<float, 2>()(i, j);
			output[i].policy.push_back(pVal);
		}
		output[i].value = valueTensor.tensor<float, 2>()(i, 0);
	}

	return output;
}



AlphaZeroNN::AlphaZeroNN()
{
#ifndef _DEBUG
	tensorflow::SessionOptions opts;
	opts.config.mutable_gpu_options()->set_allow_growth(true);
	this->session.reset(tensorflow::NewSession(opts));
#endif
}

void AlphaZeroNN::initWeights()
{
	std::lock_guard<std::mutex> guard(lock);
#ifndef _DEBUG
	TF_CHECK_OK(session->Run({}, {}, { TF_OP_INIT }, nullptr));
#endif
}

void AlphaZeroNN::loadCheckpoint(std::string filePath)
{	
#ifndef _DEBUG
	if (std::filesystem::exists(filePath + ".index"))
	{
		std::lock_guard<std::mutex> guard(lock);
		TF_CHECK_OK(session->Run({ { TF_INPUT_FILE, UtilityNN::buildTensor(filePath)} }, {}, { TF_OP_RESTORE }, nullptr));
	}
	else
	{
		printf("Checkpoint '%s' not found initialized random weights\n", filePath.c_str());
		initWeights();
		saveCheckpoint(filePath);		
	}
#endif
}

void AlphaZeroNN::saveCheckpoint(std::string filePath)
{
	std::lock_guard<std::mutex> guard(lock);
#ifndef _DEBUG	
	std::string dir = filePath.substr(0, filePath.find_last_of("/\\") + 1);
	std::filesystem::create_directories(dir);
	TF_CHECK_OK(session->Run({ { TF_INPUT_FILE, UtilityNN::buildTensor(filePath)} }, {}, { TF_OP_SAVE }, nullptr));
#endif
}

void AlphaZeroNN::loadGraph(std::string filePath, std::string device)
{	
	std::lock_guard<std::mutex> guard(lock);
#ifndef _DEBUG
	TF_CHECK_OK(tensorflow::ReadBinaryProto(tensorflow::Env::Default(), filePath, &graph_def));	
	
	//tensorflow::graph::SetDefaultDevice(device, &this->graph_def);
	for (int i = 0; i < graph_def.node_size(); ++i) 
	{
		auto node = graph_def.mutable_node(i);
		if (node->device() == "/device:GPU:0")
		{
			node->set_device(device);
		}
	}

	TF_CHECK_OK(session->Create(graph_def));
#endif	
}

int AlphaZeroNN::processBatchPrediction()
{
	{ // Sweep queue, shorten lock time
		std::lock_guard<std::mutex> guard(lock);
		predictionsQueueAccepting.swap(predictionsQueueProcessing);
	}
	cvQueueEmpty.notify_one();
	int samples = predictionsQueueProcessing.size();

#ifndef _DEBUG // Start processing predictionsQueueProcessing
	std::vector<tensorflow::Tensor> outTensors;
	TF_CHECK_OK(session->Run({ {TF_INPUT_STATE, UtilityNN::buildInsFutureTensor(predictionsQueueProcessing) }, {TF_INPUT_TRAINING, FALSE_TENSOR} },
		{ TF_OUTPUT_POLICY, TF_OUTPUT_VALUE }, {}, &outTensors));

	tensorflow::Tensor policyTensor = outTensors[0];
	tensorflow::Tensor valueTensor = outTensors[1];

	std::vector<NNOutputData> outs = UtilityNN::buildOutput(policyTensor, valueTensor);

	for (int i = 0; i < samples; i++)
	{
		predictionsQueueProcessing[i].promise.set_value(outs[i]);
	}
#else
	for (auto& it : predictionsQueueProcessing)
	{
		it.promise.set_value(NNOutputData::createRandom());
	}
#endif // !_DEBUG
	predictionsQueueProcessing.clear();
	return samples;
}

std::future<NNOutputData> AlphaZeroNN::predictFuture(const NNInputData& state)
{
	std::future<NNOutputData> f;
	{
		std::unique_lock ul(lock);
		cvQueueEmpty.wait(ul, [this] { return !isQueueFull(); });
		predictionsQueueAccepting.push_back(FuturePrediction(state));
		f = predictionsQueueAccepting.back().promise.get_future();	
	}

	if (isQueueFull())
	{
		cvQueueFull.notify_one();
	}
	else 
	{
		cvQueueEmpty.notify_one();
	}
	
	return f;
}

void AlphaZeroNN::registerThread()
{
	{
		std::lock_guard guard(lock);
		registeredThreads++;
		queueSize = __MAX(1, registeredThreads / 2);
	}
	cvQueueEmpty.notify_one();
}

void AlphaZeroNN::unregisterThread()
{
	{
		std::lock_guard guard(lock);
		registeredThreads--;
		queueSize = __MAX(1, registeredThreads / 2);
	}
	cvQueueFull.notify_one();
}

void AlphaZeroNN::waitQueueToFill()
{
	std::unique_lock ul(lock);
	cvQueueFull.wait(ul, [this] { return isQueueFull(); });
}

bool AlphaZeroNN::isQueueFull()
{
	return predictionsQueueAccepting.size() >= queueSize;
}

std::vector<NNOutputData> AlphaZeroNN::predict(const std::vector<NNInputData>& states)
{
	std::vector<NNOutputData> outputs;
	for (auto& s : states)
	{
		outputs.push_back(predict(s));
	}

	return outputs;
}

NNOutputData AlphaZeroNN::predict(const NNInputData& state)
{
	std::lock_guard<std::mutex> guard(lock);
#ifndef _DEBUG
	std::vector<tensorflow::Tensor> outTensors;

	TF_CHECK_OK(session->Run({ {TF_INPUT_STATE, UtilityNN::buildInTensor(state) }, {TF_INPUT_TRAINING, FALSE_TENSOR} },
		{ TF_OUTPUT_POLICY, TF_OUTPUT_VALUE }, {}, &outTensors));

	tensorflow::Tensor policyTensor = outTensors[0];
	tensorflow::Tensor valueTensor = outTensors[1];

	return UtilityNN::buildOutput(policyTensor, valueTensor)[0];
#else	
	return NNOutputData::createRandom();
#endif
}

void AlphaZeroNN::train(const std::vector<NNTrainData>& trainData, int epochs)
{
	std::lock_guard<std::mutex> guard(lock);
#ifndef _DEBUG
	std::vector<const NNTrainData*> shuffleTrainData(trainData.size());
	std::vector<const NNInputData*> input(SETTINGS.BATCH_SIZE);
	std::vector<const NNOutputData*> output(SETTINGS.BATCH_SIZE);

	for (int i = 0; i < trainData.size(); i++)
	{
		shuffleTrainData[i] = &trainData[i];
	}

	printf("Started training\n");
	for (int e = 0; e < epochs; e++)
	{
		int epochCount = 0;
		float epochLossV = 0.0;
		float epochLossPi = 0.0;

		printf("EPOCH %d\n", e);
		std::shuffle(shuffleTrainData.begin(), shuffleTrainData.end(), RNG.getEngine());

		int batchCount = trainData.size() / SETTINGS.BATCH_SIZE;
		int count = 0;

		for (int c = 0; c < batchCount; c++)
		{
			for (int i = 0; i < SETTINGS.BATCH_SIZE; i++)
			{
				const NNTrainData* td = shuffleTrainData[count + i];
				input[i] = &td->in;
				output[i] = &td->out;
			}
			count += SETTINGS.BATCH_SIZE;

			tensorflow::Tensor inputTensor = UtilityNN::buildInsTensor(input);
			tensorflow::Tensor policyTensor = UtilityNN::buildOutsPolicyTensor(output);
			tensorflow::Tensor valueTensor = UtilityNN::buildOutsValueTensor(output);

			std::vector<tensorflow::Tensor> outTensors;
			TF_CHECK_OK(session->Run({ {TF_INPUT_STATE, inputTensor}, {TF_TARGET_POLICY, policyTensor}, {TF_TARGET_VALUE, valueTensor}, {TF_INPUT_TRAINING, TRUE_TENSOR} },
				{ TF_OUTPUT_LOSS_POLICY, TF_OUTPUT_LOSS_VALUE }, { TF_OP_OPTIMIZE }, &outTensors));

			epochLossPi += outTensors[0].flat<float>()(0);
			epochLossV += outTensors[1].flat<float>()(0);
			epochCount++;
				
			UtilityFormat::printProgress(c+1, batchCount);				
		}

		float avgEpochLossPI = epochLossPi / epochCount;
		float avgEpochLossV = epochLossV / epochCount;
		printf("\nLoss Policy / Value: %f / %f\n", avgEpochLossPI, avgEpochLossV);

		if(SETTINGS.LOG_NN_TRAINING) LOG.getNNTrainingLog() << avgEpochLossPI << ", " << avgEpochLossV << ", ";
	}
	if (SETTINGS.LOG_NN_TRAINING) LOG.getNNTrainingLog() << std::endl;
#endif
}

void AlphaZeroNN::trainCrossValidation(const std::vector<NNTrainData>& trainData, int k)
{
	std::lock_guard<std::mutex> guard(lock);
#ifndef _DEBUG
	std::vector<const NNTrainData*> shuffleTrainData(trainData.size());
	std::vector<const NNInputData*> input(SETTINGS.BATCH_SIZE);
	std::vector<const NNOutputData*> output(SETTINGS.BATCH_SIZE);

	for (int i = 0; i < trainData.size(); i++)
	{
		shuffleTrainData[i] = &trainData[i];
	}

	int crossValidationGroupSize = shuffleTrainData.size() / k;
	float previousBestEpochLossV = 0.0f;

	printf("Started cross-validation training\n");
	for (int vi = 0; vi < k; vi++)
	{
		printf("Cross-validation step: %d/%d\n", vi, k);
		initWeights();

		if (vi % k == 0)
		{
			std::shuffle(shuffleTrainData.begin(), shuffleTrainData.end(), RNG.getEngine());
		}

		std::vector<const NNTrainData*> trainingSamples;
		std::vector<const NNTrainData*> validationSamples;

		int start = vi * crossValidationGroupSize; // Validation batch start index
		int end = (vi + 1) * crossValidationGroupSize; // Validation batch end index

		for (int i = 0; i < shuffleTrainData.size(); i++)
		{
			const NNTrainData* td = shuffleTrainData[i];
			if (start < i && i < end)
			{
				validationSamples.push_back(td);
			}
			else
			{
				trainingSamples.push_back(td);
			}
		}
		
		bool modelLearning = true;
		int failedImprovement = 0;

		for (int e = 0; e < SETTINGS.EPOCHS || modelLearning; e++)
		{
			printf("EPOCH %d\n", e);

			std::shuffle(trainingSamples.begin(), trainingSamples.end(), RNG.getEngine());
			std::shuffle(validationSamples.begin(), validationSamples.end(), RNG.getEngine());

			{ // Training phase
				int epochCount = 0;
				float epochLossV = 0.0;
				float epochLossPi = 0.0;

				int batchCount = trainingSamples.size() / SETTINGS.BATCH_SIZE; // Round down some samples are lost
				int count = 0;

				for (int c = 0; c < batchCount; c++)
				{
					for (int i = 0; i < SETTINGS.BATCH_SIZE; i++)
					{
						const NNTrainData* td = trainingSamples[count + i];
						input[i] = &td->in;
						output[i] = &td->out;
					}
					count += SETTINGS.BATCH_SIZE;

					tensorflow::Tensor inputTensor = UtilityNN::buildInsTensor(input);
					tensorflow::Tensor policyTensor = UtilityNN::buildOutsPolicyTensor(output);
					tensorflow::Tensor valueTensor = UtilityNN::buildOutsValueTensor(output);

					std::vector<tensorflow::Tensor> outTensors;
					TF_CHECK_OK(session->Run({ {TF_INPUT_STATE, inputTensor}, {TF_TARGET_POLICY, policyTensor}, {TF_TARGET_VALUE, valueTensor}, {TF_INPUT_TRAINING, TRUE_TENSOR} },
						{ TF_OUTPUT_LOSS_POLICY, TF_OUTPUT_LOSS_VALUE }, { TF_OP_OPTIMIZE }, &outTensors));

					epochLossPi += outTensors[0].flat<float>()(0);
					epochLossV += outTensors[1].flat<float>()(0);
					epochCount++;

					UtilityFormat::printProgress(c + 1, batchCount);
				}


				float avgEpochLossPI = epochLossPi / epochCount;
				float avgEpochLossV = epochLossV / epochCount;

				printf("\nTraining Loss Policy %f Value: %f\n", avgEpochLossPI, avgEpochLossV);
				saveCheckpoint(SETTINGS.DEFAULT_CHECKPOINT_DIR + "/cross-validation-" + std::to_string(vi) + "-" + std::to_string(e) + " .bin");

				LOG.getNNTrainingLog() << avgEpochLossPI << "," << avgEpochLossV;
			}


			{ // Validation phase
				int validationCount = 0;
				float validationLossPi = 0.0;
				float validationLossV = 0.0;

				int batchCount = validationSamples.size() / SETTINGS.BATCH_SIZE; // Round down some samples are lost
				int count = 0;

				for (int c = 0; c < batchCount; c++)
				{
					for (int i = 0; i < SETTINGS.BATCH_SIZE; i++)
					{
						const NNTrainData* td = validationSamples[count + i];
						input[i] = &td->in;
						output[i] = &td->out;
					}
					count += SETTINGS.BATCH_SIZE;

					tensorflow::Tensor inputTensor = UtilityNN::buildInsTensor(input);
					tensorflow::Tensor policyTensor = UtilityNN::buildOutsPolicyTensor(output);
					tensorflow::Tensor valueTensor = UtilityNN::buildOutsValueTensor(output);

					std::vector<tensorflow::Tensor> outTensors;
					TF_CHECK_OK(session->Run({ {TF_INPUT_STATE, inputTensor}, {TF_TARGET_POLICY, policyTensor}, {TF_TARGET_VALUE, valueTensor}, {TF_INPUT_TRAINING, FALSE_TENSOR} },
						{ TF_OUTPUT_LOSS_POLICY, TF_OUTPUT_LOSS_VALUE }, { }, &outTensors));

					validationLossPi += outTensors[0].flat<float>()(0);
					validationLossV += outTensors[1].flat<float>()(0);
					validationCount++;

					UtilityFormat::printProgress(c + 1, batchCount);
				}

				float avgValidationLossPI = validationLossPi / validationCount;
				float avgValidationLossV = validationLossV / validationCount;
				printf("\nValidation Loss Policy %f Value: %f\n", avgValidationLossPI, avgValidationLossV);
				LOG.getNNTrainingLog() << "," << avgValidationLossPI << "," << avgValidationLossV << std::endl;

				if (e == 0)
				{
					previousBestEpochLossV = avgValidationLossV;
				}
				else
				{
					float diffAvgLoss = previousBestEpochLossV - avgValidationLossV;
					if (diffAvgLoss > 0 && diffAvgLoss > previousBestEpochLossV * SETTINGS.DYNAMIC_EPOCH_THRESHOLD) // Improved model on validation
					{
						previousBestEpochLossV = avgValidationLossV;
						failedImprovement = 0;
					}
					else
					{
						failedImprovement++;
						if (failedImprovement == 3)
						{
							printf("=> Stopped training model is not learning\n");
							modelLearning = false;
						}
					}
				}
			}
		}

	}	
#endif
}
