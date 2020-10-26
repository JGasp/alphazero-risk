// AlphaZero-Risk.h : Include file for standard system include files,
// or project specific include files.

#pragma once

#include <tensorflow/core/platform/init_main.h>

#include "risk_game/game/game.h"
#include "risk_game/board/board_gui.h"
#include "risk_game/player/alpha_zero/alphazero_player.h"
#include "risk_game/player/alpha_zero/alphazero_trainer.h"
#include "risk_game/player/script/script_player.h"
#include "settings.h"

#include <filesystem>
#include <memory>
#include <iostream>
#include <stdio.h>
#include <time.h>
#include <thread>
