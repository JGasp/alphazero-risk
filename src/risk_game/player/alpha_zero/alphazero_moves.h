#pragma once

#include "../../../settings.h"
#include "../game_helper.h"
#include "../../game/game.h"

namespace UtilityNN
{
	uint64_t getValidMoves(const State& state);
	void makeMove(State& state, LandIndex li);	
}