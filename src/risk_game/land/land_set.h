#pragma once

#include "land.h"

class LandSet {
public:
	static const LandSet NORTH_AMERICA;
	static const LandSet SOUTH_AMERICA;
	static const LandSet AFRICA;
	static const LandSet EUROPE;
	static const LandSet ASIA;
	static const LandSet AUSTRALIA;

	static const uint64_t ALL_LANDS_MASK;
	static const uint64_t ALL_CARD_MASK;

public:
	std::vector<const Land*> lands;
	uint64_t landSetIndexBitMask;

	LandSet(std::vector<const Land*> lands);
};