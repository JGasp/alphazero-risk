# alphazero-risk
AlphaZero implementatoin for board game Risk in C++

# Getting started

## Building Tensorflow
To build from sources follow [official guide](https://www.tensorflow.org/install).

NOTE: All builded files are found in `~/tensorflow/bazel-bin/tensorflow/`
### Windows
NOTE: On windwos to build usable .dll follow [guide](https://medium.com/@ashley.tharp/btw-if-you-enjoy-my-tutorial-i-always-appreciate-endorsements-on-my-linkedin-https-www-linkedin-a6d6fcba1e44). There is problem with exporting symbols. In order to fix it for our program to work we need to add `TF_EXPORT` to `TF_EXPORT struct SessionOptions` in file `~\tensorflow\core\public\session_options.h` and `TF_EXPORT Session`, `TF_EXPORT Status NewSession` and `TF_EXPORT Session* NewSession` in file `~\tensorflow\core\public\session.h`

Build .dll (tensorflow/bazel-bin/tensorflow/libtensorflow.dll)
`bazel build --config=opt --config=cuda --define=no_tensorflow_py_deps=true tensorflow/tensorflow.dll`

Build .lib (tensorflow/bazel-bin/tensorflow/)
`bazel build --config=opt --config=cuda --define=no_tensorflow_py_deps=true tensorflow/tensorflow.lib`

Copy file `libtensorflow_cc.so.2` and `libtensorflow_framework.so.2` into project folder `~/lib/tensorflow/` and rename them to `tensorflow.dll` and `tensorflow.lib`

### Linux 
Build .so
`bazel build --config=opt --config=cuda --define=no_tensorflow_py_deps=true tensorflow/libtensorflow_cc.so`

Copy file `libtensorflow_cc.so.2` and `libtensorflow_framework.so.2` into project folder `~/lib/tensorflow/`

### Header files
Build header files
`bazel build --config=opt --config=cuda --define=no_tensorflow_py_deps=true //tensorflow:install_headers`

Copy headers folder `~/tensorflow/bazel-bin/tensorflow/include` into project `~/lib/tensorflow/includes`

## Compile program
Use cmake build script.