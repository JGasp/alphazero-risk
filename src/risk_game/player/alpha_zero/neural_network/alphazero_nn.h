#pragma once

#include <cstdio>
#include <functional>
#include <string>
#include <vector>
#include <time.h>
#include <algorithm>
#include <filesystem>
#include <future>
#include <string>

#include "tensorflow/cc/ops/standard_ops.h"
#include "tensorflow/core/framework/graph.pb.h"
#include "tensorflow/core/framework/tensor.h"
#include "tensorflow/core/graph/default_device.h"
#include "tensorflow/core/graph/graph_def_builder.h"
#include "tensorflow/core/lib/core/threadpool.h"
#include "tensorflow/core/lib/strings/str_util.h"
#include "tensorflow/core/lib/strings/stringprintf.h"
#include "tensorflow/core/platform/init_main.h"
#include "tensorflow/core/platform/logging.h"
#include "tensorflow/core/platform/types.h"
#include "tensorflow/core/public/session.h"
#include "tensorflow/core/framework/device_attributes.pb.h"


#include "alphazero_nn_data.h"

static const std::string TF_INPUT_STATE = "input_state";
static const std::string TF_INPUT_TRAINING = "input_training";

static const std::string TF_OUTPUT_POLICY = "output_policy";
static const std::string TF_OUTPUT_VALUE = "output_value";

static const std::string TF_OUTPUT_LOSS_POLICY = "softmax_cross_entropy_loss/value";
static const std::string TF_OUTPUT_LOSS_VALUE = "mean_squared_error/value";

static const std::string TF_TARGET_POLICY = "target_policy";
static const std::string TF_TARGET_VALUE = "target_value";

static const std::string TF_OP_INIT = "init";
static const std::string TF_OP_OPTIMIZE = "optimize";

static const std::string TF_INPUT_FILE = "save/Const";
static const std::string TF_OP_RESTORE = "save/restore_all";
static const std::string TF_OP_SAVE = "save/control_dependency";


class FuturePrediction
{
public:
	NNInputData in;
	std::promise<NNOutputData> promise;

	FuturePrediction(NNInputData in) : in(in) {};
};

namespace UtilityNN 
{
	std::string getCheckpointName(std::string name);

	tensorflow::Tensor buildTensor(std::string value);
	tensorflow::Tensor buildTensor(bool value);
	tensorflow::Tensor buildTensor(float value);

	tensorflow::Tensor buildInTensor(const NNInputData& value);
	tensorflow::Tensor buildInsTensor(const std::vector<const NNInputData*>& value);
	tensorflow::Tensor buildInsFutureTensor(const std::vector<FuturePrediction>& values);

	tensorflow::Tensor buildOutPolicyTensor(const NNOutputData& value);
	tensorflow::Tensor buildOutsPolicyTensor(const std::vector<const NNOutputData*>& value);

	tensorflow::Tensor buildOutValueTensor(const NNOutputData& value);
	tensorflow::Tensor buildOutsValueTensor(const std::vector<const NNOutputData*>& value);

	std::vector<NNOutputData> buildOutput(const tensorflow::Tensor& policy, const tensorflow::Tensor& value);
}

static tensorflow::Tensor TRUE_TENSOR = UtilityNN::buildTensor(true);
static tensorflow::Tensor FALSE_TENSOR = UtilityNN::buildTensor(false);

/*
	Tensorflow dll is build as release and can not be used in debug mode
	- Different memory allocation for std conatiners (std::string)
*/
class AlphaZeroNN
{
private:
	std::mutex lock;
	std::condition_variable cvQueueFull;
	std::condition_variable cvQueueEmpty;
	
	std::unique_ptr<tensorflow::Session> session;
	tensorflow::GraphDef graph_def;
	
	int registeredThreads = 0;
	int queueSize = 1;
	std::vector<FuturePrediction> predictionsQueueAccepting;
	std::vector<FuturePrediction> predictionsQueueProcessing;
	
public:
	AlphaZeroNN();

	void initWeights();
	void loadCheckpoint(std::string filePath);
	void saveCheckpoint(std::string filePath);
	void loadGraph(std::string filePath, std::string device);
	
	void waitQueueToFill();
	bool isQueueFull();

	std::future<NNOutputData> predictFuture(const NNInputData& state); // Thread safe
	int processBatchPrediction();
		
	NNOutputData predict(const NNInputData& state);
	std::vector<NNOutputData> predict(const std::vector<NNInputData>& states);
	void train(const std::vector<NNTrainData>& trainData, int epochs);
	void trainCrossValidation(const std::vector<NNTrainData>& trainData, int k);

	void registerThread(); // Tell NN prediction batch to wait for thread
	void unregisterThread(); // Tell NN prediction batch to stop waiting for thread
};