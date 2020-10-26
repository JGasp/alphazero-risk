# alphazero-risk
AlphaZero implementatoin for board game Risk in C++

## Building Tensorflow
To build from sources follow [official guide](https://www.tensorflow.org/install).

> NOTE: All builded files are found in `~/tensorflow/bazel-bin/tensorflow/`
### Windows
>NOTE: On windows to build usable .dll follow [guide](https://medium.com/@ashley.tharp/btw-if-you-enjoy-my-tutorial-i-always-appreciate-endorsements-on-my-linkedin-https-www-linkedin-a6d6fcba1e44). There is problem with exporting symbols. In order to fix it for our program to work we need to add `TF_EXPORT` to `TF_EXPORT struct SessionOptions` in file `~\tensorflow\core\public\session_options.h` and `TF_EXPORT Session`, `TF_EXPORT Status NewSession`, `TF_EXPORT Session* NewSession` in file `~\tensorflow\core\public\session.h`  

Build .dll `bazel build --config=opt --config=cuda --define=no_tensorflow_py_deps=true tensorflow/tensorflow.dll`  
Build .lib `bazel build --config=opt --config=cuda --define=no_tensorflow_py_deps=true tensorflow/tensorflow.lib`  

Copy file `libtensorflow_cc.so.2` and `libtensorflow_framework.so.2` into project folder `~/lib/tensorflow/` and rename them to `tensorflow.dll` and `tensorflow.lib`

### Linux 
Build .so `bazel build --config=opt --config=cuda --define=no_tensorflow_py_deps=true tensorflow/libtensorflow_cc.so`

Copy file `libtensorflow_cc.so.2` and `libtensorflow_framework.so.2` into project folder `~/lib/tensorflow/`

### Header files
Build header files  
`bazel build --config=opt --config=cuda --define=no_tensorflow_py_deps=true //tensorflow:install_headers`

Copy headers folder `~/tensorflow/bazel-bin/tensorflow/include` into project `~/lib/tensorflow/includes`

## Compile program
Use cmake build script.

## Using program
For detailed options available examine file [settings.h](https://github.com/JGasp/alphazero-risk/blob/master/src/settings.h). Some options/macros are present in CMakeLists.txt that are related to version of input vector and changes/optimizations to Risk game implementation. In order to apply those changes you need to recompile project.

Simple play command:  
`AlphaZero_risk -m play --mcts=16 --cg=1000` Play 1000 games using 16 MCTS searches per move (default: AlphaZero vs ScriptPlayer)

## Changing NN
In order to build graph we used python. Script is located in [~python/src/build_model.py](https://github.com/JGasp/alphazero-risk/blob/master/python/src/build_graph.py). If you change graph it is recommended to rename it to something uniqe and update CMakeLists.txt file to copy correct graph into working directory.

## Trained models
On [release page](https://github.com/JGasp/alphazero-risk/releases/tag/1.0) we uploaded our pretrained models. In order to use them place them into `{CWD}/checkpoints/`
