#include "land_set.h"

LandSet::LandSet(std::vector<const Land*> lands) : lands(lands)
{
	landSetIndexBitMask = 0ULL;
	for (int i = 0; i < lands.size(); i++) {
		landSetIndexBitMask |= lands[i]->landIndexBitMask;
	}
}


const LandSet LandSet::NORTH_AMERICA = LandSet({ &Land::ALASKA, &Land::NORTHWEST_TERRIOTRY, &Land::GREENLAND, &Land::ALBERTA, &Land::ONTARIO, &Land::QUEBEC,
&Land::WESTERN_UNITED_STATES, &Land::EASTERN_UNITED_STATES, &Land::CENTRAL_AMERICA });

const LandSet LandSet::SOUTH_AMERICA = LandSet({ &Land::VENEZUELA, &Land::PERU, &Land::BRAZIL, &Land::ARGENTINA });

const LandSet LandSet::AFRICA = LandSet({ &Land::NORTH_AFRICA, &Land::EGYPT, &Land::CONGO, &Land::SOUTH_AFRICA, &Land::MADAGASKAR, &Land::EAST_AFRICA });

const LandSet LandSet::EUROPE = LandSet({ &Land::ICELAND, &Land::GREAT_BRITAIN, &Land::SCANDINAVIA, &Land::UKRAINE, &Land::NORTHERN_EUROPE, &Land::WESTERN_EUROPE, &Land::SOUTHERN_EUROPE });

const LandSet LandSet::ASIA = LandSet({ &Land::URAL, &Land::AFGHANISTAN, &Land::MIDDLE_EAST, &Land::INDIA, &Land::SIBERIA, &Land::YAKUTSK, &Land::KAMCHATKA,
	&Land::IRKUTSK, &Land::JAPAN, &Land::MONGOLIA, &Land::CHINA, &Land::SIAM });

const LandSet LandSet::AUSTRALIA = LandSet({ &Land::INDONESIA, &Land::NEW_GUINEA, &Land::WESTERN_AUSTRALIA, &Land::EASTERN_AUSTRALIA });


const uint64_t LandSet::ALL_LANDS_MASK =
LandSet::NORTH_AMERICA.landSetIndexBitMask | 
LandSet::SOUTH_AMERICA.landSetIndexBitMask | 
LandSet::AFRICA.landSetIndexBitMask | 
LandSet::EUROPE.landSetIndexBitMask |
LandSet::ASIA.landSetIndexBitMask |
LandSet::AUSTRALIA.landSetIndexBitMask;

const uint64_t LandSet::ALL_CARD_MASK = LandSet::ALL_LANDS_MASK;