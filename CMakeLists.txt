cmake_minimum_required(VERSION 3.17)
project ("AlphaZero_Risk")

# cmake . -DCMAKE_BUILD_TYPE=Release
# make VERBOSE=1

# bazel build --config=opt --config=cuda --define=no_tensorflow_py_deps=true //tensorflow/tools/lib_package:libtensorflow
# bazel build --config=opt --config=cuda --define=no_tensorflow_py_deps=true //tensorflow:install_headers
# bazel build --config=opt --config=cuda --define=no_tensorflow_py_deps=true tensorflow/libtensorflow_cc.so
# bazel build --config=opt --config=cuda --define=no_tensorflow_py_deps=true tensorflow/tensorflow.dll
# bazel build --config=opt --config=cuda --define=no_tensorflow_py_deps=true tensorflow/tensorflow.lib

# Initial configuration
#set(GUI true)
set(INPUT_VECTOR_TYPE_2 true)
set(BLOCKS 20)

set(STATE_SIMPLE_CARDS true)
set(FAST_ATTACK_MOBILIZATION true)
set(FAST_REINFORCEMENT true)
set(ROUND_WEIGHTED_VALUE false)

#add_compile_definitions(LOG_PERFORMANCE)

#Inlude lib folders
include_directories("libs/tensorflow/include/src")
include_directories("libs/tensorflow/include")
include_directories("libs")


# Add source to this project's executable.
set(SOURCE_FILES 
  "src/alphazero_risk.h"
  "src/alphazero_risk.cpp"
  "src/risk_game/land/land.cpp" 
  "src/risk_game/state/state.cpp"
  "src/risk_game/player/base/player.cpp"   
  "src/risk_game/player/alpha_zero/alphazero_player.cpp" 
  "src/risk_game/player/alpha_zero/alphazero_trainer.cpp"
  "src/risk_game/player/alpha_zero/alphazero_mcts.cpp"
  "src/risk_game/player/alpha_zero/neural_network/alphazero_nn.cpp"
  "src/risk_game/player/script/script_player.cpp" 
  "src/risk_game/player/random/random_player.cpp"
  "src/risk_game/player/alpha_zero/alphazero_moves.cpp"
  "src/risk_game/game/game.cpp"
  "src/risk_game/player/game_helper.cpp" 
  "src/risk_game/land/land_set.cpp"
  "src/risk_game/player/alpha_zero/neural_network/alphazero_gpu_cluster.cpp"
  "src/risk_game/player/alpha_zero/neural_network/alphazero_nn_data.cpp"
  "libs/xxhash/xxhash.c"    
)


# Add source for gui
if(WIN32 AND GUI)
    add_compile_definitions(GUI)
    set(SOURCE_FILES ${SOURCE_FILES}
          "src/risk_game/board/board_gui.cpp"
          "libs/imgui/imgui.cpp"
          "libs/imgui/imgui_demo.cpp"
          "libs/imgui/imgui_draw.cpp"
          "libs/imgui/imgui_widgets.cpp"
          "libs/imgui/imgui_impl_win32.cpp"
          "libs/imgui/imgui_impl_dx11.cpp")
endif()

# Define executable entry
add_executable (AlphaZero_Risk ${SOURCE_FILES} "src/log.h" "src/rng.h")


if(STATE_SIMPLE_CARDS)
    add_compile_definitions(STATE_SIMPLE_CARDS)
endif()
if(FAST_ATTACK_MOBILIZATION)
    add_compile_definitions(FAST_ATTACK_MOBILIZATION)
endif()
if(FAST_REINFORCEMENT)
    add_compile_definitions(FAST_REINFORCEMENT)
endif()
if(ROUND_WEIGHTED_VALUE)
    add_compile_definitions(ROUND_WEIGHTED_VALUE)
endif()


# Add link libraries for different platforms
if(INPUT_VECTOR_TYPE_1)
    add_compile_definitions(INPUT_VECTOR_TYPE_1)
    set(GRAPH_FILE ${CMAKE_CURRENT_SOURCE_DIR}/python/model/model_bin_V1_${BLOCKS}.pb)
elseif(INPUT_VECTOR_TYPE_2)
    add_compile_definitions(INPUT_VECTOR_TYPE_2)
    set(GRAPH_FILE ${CMAKE_CURRENT_SOURCE_DIR}/python/model/model_bin_V2_${BLOCKS}.pb)
elseif(INPUT_VECTOR_TYPE_3)
    add_compile_definitions(INPUT_VECTOR_TYPE_3)
    set(GRAPH_FILE ${CMAKE_CURRENT_SOURCE_DIR}/python/model/model_bin_V3_${BLOCKS}.pb)
endif()

add_custom_command(
            TARGET AlphaZero_Risk POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    ${GRAPH_FILE}
                    ${CMAKE_BINARY_DIR}/model_bin.pb)

if(WIN32)
    add_compile_definitions(NOMINMAX)
    add_compile_definitions(__cpp_lib_bitops)    
    add_compile_definitions(_HAS_DEPRECATED_RESULT_OF=1)
    set(TF_LIB tensorflow.lib)
    set(TF_DLL tensorflow.dll)

    add_custom_command(
            TARGET AlphaZero_Risk POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    ${CMAKE_CURRENT_SOURCE_DIR}/libs/tensorflow/${TF_DLL}
                    ${CMAKE_BINARY_DIR}/${TF_DLL})

    target_compile_features(AlphaZero_Risk PUBLIC cxx_std_20)
    target_link_libraries(AlphaZero_Risk ${CMAKE_CURRENT_SOURCE_DIR}/libs/tensorflow/${TF_LIB})

    if(GUI)
        add_custom_command(
                TARGET AlphaZero_Risk POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                        ${CMAKE_CURRENT_SOURCE_DIR}/docs/risk_map_clear.jpg
                        ${CMAKE_BINARY_DIR}/risk_map_clear.jpg)

        target_link_libraries(AlphaZero_Risk C:/Program\ Files\ \(x86\)/Windows\ Kits/10/Lib/10.0.18362.0/um/x64/d3d11.lib)  
    endif()
elseif(UNIX)
    set(CMAKE_CXX_FLAGS "-std=gnu++2a -w -pthread -lncurses -Wall -Wextra")
    set(CMAKE_CXX_FLAGS_DEBUG "-g")
    set(CMAKE_CXX_FLAGS_RELEASE "-O3")

    target_link_options(AlphaZero_Risk PUBLIC "LINKER:--allow-multiple-definition")
    target_link_options(AlphaZero_Risk PUBLIC "LINKER:--whole-archive")
    target_link_options(AlphaZero_Risk PUBLIC "LINKER:--no-as-needed")

    set(TF_LIB libtensorflow_framework.so.2)
    set(TF_DLL libtensorflow_cc.so.2)

    target_link_libraries(AlphaZero_Risk ${CMAKE_CURRENT_SOURCE_DIR}/libs/tensorflow/${TF_LIB})
    target_link_libraries(AlphaZero_Risk ${CMAKE_CURRENT_SOURCE_DIR}/libs/tensorflow/${TF_DLL})
endif()
