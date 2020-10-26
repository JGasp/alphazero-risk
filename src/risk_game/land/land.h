#pragma once

#include <iostream>
#include <vector>
#include <bitset>
#include <bit>

#include "land_index.h"
#include "../../settings.h"


namespace Utility
{
	int popcount(uint64_t x);

	uint64_t getFirstBitMask(uint64_t bitMask);

	uint8_t li2i(LandIndex index);
	uint8_t lm2i(uint64_t bitMask);	
	LandIndex lm2li(uint64_t bitMask);
	LandIndex i2li(uint8_t index);

	uint64_t li2lm(LandIndex index);
	uint64_t lis2lms(const std::vector<LandIndex>& neihboursIndex);	

	uint64_t randomMask(uint64_t masks);
}


class Land
{
public:
	static const Land ALASKA;
	static const Land NORTHWEST_TERRIOTRY;
	static const Land GREENLAND;
	static const Land ALBERTA;
	static const Land ONTARIO;
	static const Land QUEBEC;
	static const Land WESTERN_UNITED_STATES;
	static const Land EASTERN_UNITED_STATES;
	static const Land CENTRAL_AMERICA;	

	static const Land VENEZUELA;
	static const Land PERU;
	static const Land BRAZIL;
	static const Land ARGENTINA;	

	static const Land NORTH_AFRICA;
	static const Land EGYPT;
	static const Land CONGO;
	static const Land SOUTH_AFRICA;
	static const Land MADAGASKAR;
	static const Land EAST_AFRICA;	

	static const Land ICELAND;
	static const Land GREAT_BRITAIN;
	static const Land SCANDINAVIA;
	static const Land UKRAINE;
	static const Land NORTHERN_EUROPE;
	static const Land WESTERN_EUROPE;
	static const Land SOUTHERN_EUROPE;

	static const Land URAL;
	static const Land AFGHANISTAN;
	static const Land MIDDLE_EAST;
	static const Land INDIA;
	static const Land SIBERIA;
	static const Land YAKUTSK;
	static const Land KAMCHATKA;
	static const Land IRKUTSK;
	static const Land JAPAN;
	static const Land MONGOLIA;
	static const Land CHINA;
	static const Land SIAM;

	static const Land INDONESIA;
	static const Land NEW_GUINEA;
	static const Land WESTERN_AUSTRALIA;
	static const Land EASTERN_AUSTRALIA;

	static const Land* LAND_MAP[LAND_INDEX_SIZE];
	static const Land* getLand(uint8_t landIndex);
	static const Land* getLand(LandIndex landIndex);

	static const LandIndex SKIP_MOVE;
	static const uint64_t SKIP_MOVE_MASK;	

	static const uint64_t CARD_INFANTRY_BITMASK;
	static const uint64_t CARD_HORSE_BITMASK;
	static const uint64_t CARD_SIGE_BITMASK;
	
	static std::string getName(LandIndex landIndex);
	static char getCardType(LandIndex landIndex);
	static char getCardType(uint64_t landIndex);
	static void print(const Land* l);

public:
	const LandIndex landIndex;
	uint64_t landIndexBitMask;
	const std::vector<LandIndex> neihboursLandIndex;
	uint64_t neighboursLandIndexBitMask;

	Land(LandIndex index, const std::vector<LandIndex>& neihboursIndex);	
};
