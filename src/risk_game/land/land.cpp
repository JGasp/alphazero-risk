#include "land.h"

#define ABM_OPTZ

int popcount(uint32_t x)
{
	x = x - ((x >> 1) & 0x55555555);
	x = (x & 0x33333333) + ((x >> 2) & 0x33333333);
	x = (x + (x >> 4)) & 0x0F0F0F0F;
	x = x + (x >> 8);
	x = x + (x >> 16);
	return x & 0x0000003F;
}

int Utility::popcount(uint64_t x)
{	
#ifdef  ABM_OPTZ
	return std::popcount(x); // C++20 https://en.cppreference.com/w/cpp/numeric/popcount
#else
	uint32_t* ptr = (uint32_t*)&x;
	return popcount(ptr[0]) + popcount(ptr[1]);
#endif //  ABM_OPTZ	
}

uint64_t Utility::getFirstBitMask(uint64_t bitMask)
{
#ifdef  ABM_OPTZ
	int leadingZeros = std::countr_zero(bitMask);
	uint64_t firstMask = 1ULL << leadingZeros;
	return firstMask;
#else
	 uint64_t firstBitMask = 1ULL;
	for (int i = 0; i < 64; i++)
	{
		if (bitMask & firstBitMask)
		{
			break;
		}
		firstBitMask = firstBitMask << 1;
	}

	return firstBitMask;
#endif
}


uint8_t Utility::li2i(LandIndex landIndex)
{
	return static_cast<uint8_t>(landIndex);
}

LandIndex Utility::lm2li(uint64_t bitMask)
{
	return i2li(lm2i(bitMask));
}

LandIndex Utility::i2li(uint8_t index)
{
	return static_cast<LandIndex>(index);
}

uint8_t Utility::lm2i(uint64_t bitMask)
{
#ifdef ABM_OPTZ
	return std::countr_zero(bitMask);
#else
	for (uint8_t i = 0; i < 64; i++)
	{
		if ((bitMask & 1) > 0)
		{
			return i;
		}
		bitMask = bitMask >> 1;
	}

	return 0;
#endif
}

uint64_t Utility::li2lm(LandIndex landIndex)
{
	uint8_t index = Utility::li2i(landIndex);
	uint64_t indexBitMask = 1ULL;
	indexBitMask = indexBitMask << index;	

	return indexBitMask;
}

uint64_t Utility::lis2lms(const std::vector<LandIndex>& neihboursIndex)
{
	uint64_t neighboursBitMask = 0ULL;

	for (int i = 0; i < neihboursIndex.size(); i++) {
		neighboursBitMask |= Utility::li2lm(neihboursIndex[i]);
	}

	return neighboursBitMask;
}

uint64_t Utility::randomMask(uint64_t masks)
{
	int count = popcount(masks);
	int rIndex = RNG.rInt() % count;

	uint64_t mask = getFirstBitMask(masks);
	for (int i = 0; i < rIndex; i++)
	{
		masks &= ~mask;
		mask = getFirstBitMask(masks);
	}
	return mask;
}

Land::Land(LandIndex index, const std::vector<LandIndex>& neihboursIndex) :
	landIndex(index), neihboursLandIndex(neihboursIndex) 
{
	this->landIndexBitMask = Utility::li2lm(index);
	this->neighboursLandIndexBitMask = Utility::lis2lms(neihboursIndex);

	Land::LAND_MAP[Utility::li2i(index)] = this;

#ifdef _DEBUG
	LandIndex li = Utility::lm2li(this->landIndexBitMask);
	if (li != index)
	{
		throw std::invalid_argument("Error");
	}
#endif // _DEBUG
};

const Land* Land::getLand(uint8_t landIndex) 
{
	return Land::LAND_MAP[landIndex];
}

const Land* Land::getLand(LandIndex landIndex) 
{
	return Land::LAND_MAP[Utility::li2i(landIndex)];
}

void Land::print(const Land* l) 
{
	std::bitset<64> indexMask(l->landIndexBitMask);
	printf("Index: %d \n Index mask: %s\n", Utility::li2i(l->landIndex), indexMask.to_string().c_str());
	
	printf("Neighbours index: ");
	for (const auto& value : l->neihboursLandIndex) {
		printf("%d,", Utility::li2i(value));
	}

	std::bitset<64> neighboursMask(l->neighboursLandIndexBitMask);
	printf("\nNeighbours index mask: %s", neighboursMask.to_string().c_str());

}

std::string Land::getName(LandIndex landIndex) 
{
	switch (landIndex)
	{
	case LandIndex::ALASKA: return "ALASKA";
	case LandIndex::NORTHWEST_TERRIOTRY: return "NORTHWEST TERRIOTRY";
	case LandIndex::GREENLAND: return "GREENLAND";
	case LandIndex::ALBERTA: return "ALBERTA";
	case LandIndex::ONTARIO: return "ONTARIO";
	case LandIndex::QUEBEC: return "QUEBEC";
	case LandIndex::WESTERN_UNITED_STATES: return "WESTERN_UNITED_STATES";
	case LandIndex::EASTERN_UNITED_STATES: return "EASTERN_UNITED_STATES";
	case LandIndex::CENTRAL_AMERICA: return "CENTRAL_AMERICA";

	case LandIndex::VENEZUELA: return "VENEZUELA";
	case LandIndex::PERU: return "PERU";
	case LandIndex::BRAZIL: return "BRAZIL";
	case LandIndex::ARGENTINA: return "ARGENTINA";

	case LandIndex::NORTH_AFRICA: return "NORTH_AFRICA";
	case LandIndex::EGYPT: return "EGYPT";
	case LandIndex::CONGO: return "CONGO";
	case LandIndex::SOUTH_AFRICA: return "SOUTH_AFRICA";
	case LandIndex::MADAGASKAR: return "MADAGASKAR";
	case LandIndex::EAST_AFRICA: return "EAST_AFRICA";

	case LandIndex::ICELAND: return "ICELAND";
	case LandIndex::GREAT_BRITAIN: return "GREAT_BRITAIN";
	case LandIndex::SCANDINAVIA: return "SCANDINAVIA";
	case LandIndex::UKRAINE: return "UKRAINE";
	case LandIndex::NORTHERN_EUROPE: return "NORTHERN_EUROPE";
	case LandIndex::WESTERN_EUROPE: return "WESTERN_EUROPE";
	case LandIndex::SOUTHERN_EUROPE: return "SOUTHERN_EUROPE";

	case LandIndex::URAL: return "URAL";
	case LandIndex::AFGHANISTAN: return "AFGHANISTAN";
	case LandIndex::MIDDLE_EAST: return "MIDDLE_EAST";
	case LandIndex::INDIA: return "INDIA";
	case LandIndex::SIBERIA: return "SIBERIA";
	case LandIndex::YAKUTSK: return "YAKUTSK";
	case LandIndex::KAMCHATKA: return "KAMCHATKA";
	case LandIndex::IRKUTSK: return "IRKUTSK";
	case LandIndex::JAPAN: return "JAPAN";
	case LandIndex::MONGOLIA: return "MONGOLIA";
	case LandIndex::CHINA: return "CHINA";
	case LandIndex::SIAM: return "SIAM";

	case LandIndex::INDONESIA: return "INDONESIA";
	case LandIndex::NEW_GUINEA: return "NEW_GUINEA";
	case LandIndex::WESTERN_AUSTRALIA: return "WESTERN_AUSTRALIA";
	case LandIndex::EASTERN_AUSTRALIA: return "EASTERN_AUSTRALIA";
	case LandIndex::Count: return "END GAME CARD";

	default:
		return "Undefined";		
	}
}

char Land::getCardType(LandIndex landIndex)
{
	return getCardType(Utility::li2lm(landIndex));
}

char Land::getCardType(uint64_t landMask)
{

	char cardType = 'N';
	if ((Land::CARD_HORSE_BITMASK & landMask) > 0)
	{
		cardType = 'H';
	}
	else if ((Land::CARD_SIGE_BITMASK & landMask) > 0)
	{
		cardType = 'S';
	}
	else if ((Land::CARD_INFANTRY_BITMASK & landMask) > 0)
	{
		cardType = 'I';
	}
	else
	{
		throw std::invalid_argument("Card has no defiend type");
	}

	return cardType;
}

const Land* Land::LAND_MAP[LAND_INDEX_SIZE] = {};


const Land Land::ALASKA = Land(LandIndex::ALASKA, { LandIndex::NORTHWEST_TERRIOTRY, LandIndex::ALBERTA, LandIndex::KAMCHATKA });
const Land Land::NORTHWEST_TERRIOTRY = Land(LandIndex::NORTHWEST_TERRIOTRY, { LandIndex::ALASKA, LandIndex::ALBERTA, LandIndex::ONTARIO, LandIndex::GREENLAND });
const Land Land::GREENLAND = Land(LandIndex::GREENLAND, { LandIndex::NORTHWEST_TERRIOTRY, LandIndex::ONTARIO, LandIndex::QUEBEC, LandIndex::ICELAND });
const Land Land::ALBERTA = Land(LandIndex::ALBERTA, { LandIndex::ALASKA, LandIndex::NORTHWEST_TERRIOTRY, LandIndex::ONTARIO, LandIndex::WESTERN_UNITED_STATES });
const Land Land::ONTARIO = Land(LandIndex::ONTARIO, { LandIndex::NORTHWEST_TERRIOTRY, LandIndex::ALBERTA, LandIndex::WESTERN_UNITED_STATES, LandIndex::EASTERN_UNITED_STATES, LandIndex::QUEBEC, LandIndex::GREENLAND });
const Land Land::QUEBEC = Land(LandIndex::QUEBEC, { LandIndex::ONTARIO, LandIndex::EASTERN_UNITED_STATES, LandIndex::GREENLAND });
const Land Land::WESTERN_UNITED_STATES = Land(LandIndex::WESTERN_UNITED_STATES, { LandIndex::ALBERTA, LandIndex::ONTARIO, LandIndex::EASTERN_UNITED_STATES, LandIndex::CENTRAL_AMERICA });
const Land Land::EASTERN_UNITED_STATES = Land(LandIndex::EASTERN_UNITED_STATES, { LandIndex::CENTRAL_AMERICA, LandIndex::WESTERN_UNITED_STATES, LandIndex::ONTARIO, LandIndex::QUEBEC });
const Land Land::CENTRAL_AMERICA = Land(LandIndex::CENTRAL_AMERICA, { LandIndex::WESTERN_UNITED_STATES, LandIndex::EASTERN_UNITED_STATES, LandIndex::VENEZUELA });


const Land Land::VENEZUELA = Land(LandIndex::VENEZUELA, { LandIndex::CENTRAL_AMERICA, LandIndex::PERU, LandIndex::BRAZIL });
const Land Land::PERU = Land(LandIndex::PERU, { LandIndex::VENEZUELA, LandIndex::BRAZIL, LandIndex::ARGENTINA });
const Land Land::BRAZIL = Land(LandIndex::BRAZIL, { LandIndex::VENEZUELA, LandIndex::PERU, LandIndex::ARGENTINA, LandIndex::NORTH_AFRICA });
const Land Land::ARGENTINA = Land(LandIndex::ARGENTINA, { LandIndex::PERU, LandIndex::BRAZIL });


const Land Land::NORTH_AFRICA = Land(LandIndex::NORTH_AFRICA, { LandIndex::BRAZIL, LandIndex::WESTERN_EUROPE, LandIndex::SOUTHERN_EUROPE, LandIndex::EGYPT, LandIndex::EAST_AFRICA, LandIndex::CONGO });
const Land Land::EGYPT = Land(LandIndex::EGYPT, { LandIndex::SOUTHERN_EUROPE, LandIndex::NORTH_AFRICA, LandIndex::EAST_AFRICA, LandIndex::MIDDLE_EAST });
const Land Land::CONGO = Land(LandIndex::CONGO, { LandIndex::NORTH_AFRICA, LandIndex::EAST_AFRICA, LandIndex::SOUTH_AFRICA });
const Land Land::SOUTH_AFRICA = Land(LandIndex::SOUTH_AFRICA, { LandIndex::CONGO, LandIndex::EAST_AFRICA, LandIndex::MADAGASKAR });
const Land Land::MADAGASKAR = Land(LandIndex::MADAGASKAR, { LandIndex::SOUTH_AFRICA, LandIndex::EAST_AFRICA });
const Land Land::EAST_AFRICA = Land(LandIndex::EAST_AFRICA, { LandIndex::EGYPT, LandIndex::NORTH_AFRICA, LandIndex::CONGO, LandIndex::SOUTH_AFRICA, LandIndex::MADAGASKAR, LandIndex::MIDDLE_EAST });


const Land Land::ICELAND = Land(LandIndex::ICELAND, { LandIndex::GREENLAND, LandIndex::GREAT_BRITAIN, LandIndex::SCANDINAVIA });
const Land Land::GREAT_BRITAIN = Land(LandIndex::GREAT_BRITAIN, { LandIndex::ICELAND, LandIndex::WESTERN_EUROPE, LandIndex::SCANDINAVIA, LandIndex::NORTHERN_EUROPE });
const Land Land::SCANDINAVIA = Land(LandIndex::SCANDINAVIA, { LandIndex::ICELAND, LandIndex::GREAT_BRITAIN, LandIndex::UKRAINE, LandIndex::NORTHERN_EUROPE });
const Land Land::UKRAINE = Land(LandIndex::UKRAINE, { LandIndex::SCANDINAVIA, LandIndex::NORTHERN_EUROPE, LandIndex::SOUTHERN_EUROPE, LandIndex::MIDDLE_EAST, LandIndex::AFGHANISTAN, LandIndex::URAL });
const Land Land::NORTHERN_EUROPE = Land(LandIndex::NORTHERN_EUROPE, { LandIndex::SCANDINAVIA, LandIndex::GREAT_BRITAIN, LandIndex::SOUTHERN_EUROPE, LandIndex::WESTERN_EUROPE, LandIndex::UKRAINE });
const Land Land::WESTERN_EUROPE = Land(LandIndex::WESTERN_EUROPE, { LandIndex::NORTH_AFRICA, LandIndex::GREAT_BRITAIN, LandIndex::SOUTHERN_EUROPE, LandIndex::NORTHERN_EUROPE });
const Land Land::SOUTHERN_EUROPE = Land(LandIndex::SOUTHERN_EUROPE, { LandIndex::WESTERN_EUROPE, LandIndex::NORTHERN_EUROPE, LandIndex::UKRAINE, LandIndex::NORTH_AFRICA, LandIndex::EGYPT, LandIndex::MIDDLE_EAST });


const Land Land::URAL = Land(LandIndex::URAL, { LandIndex::UKRAINE, LandIndex::AFGHANISTAN, LandIndex::CHINA, LandIndex::SIBERIA });
const Land Land::AFGHANISTAN = Land(LandIndex::AFGHANISTAN, { LandIndex::UKRAINE, LandIndex::URAL, LandIndex::CHINA, LandIndex::INDIA, LandIndex::MIDDLE_EAST });
const Land Land::MIDDLE_EAST = Land(LandIndex::MIDDLE_EAST, { LandIndex::EGYPT, LandIndex::EAST_AFRICA, LandIndex::SOUTHERN_EUROPE, LandIndex::UKRAINE, LandIndex::AFGHANISTAN, LandIndex::INDIA });
const Land Land::INDIA = Land(LandIndex::INDIA, { LandIndex::MIDDLE_EAST, LandIndex::AFGHANISTAN, LandIndex::CHINA, LandIndex::SIAM });
const Land Land::SIBERIA = Land(LandIndex::SIBERIA, { LandIndex::URAL, LandIndex::CHINA, LandIndex::MONGOLIA, LandIndex::IRKUTSK, LandIndex::YAKUTSK });
const Land Land::YAKUTSK = Land(LandIndex::YAKUTSK, { LandIndex::SIBERIA, LandIndex::IRKUTSK, LandIndex::KAMCHATKA });
const Land Land::KAMCHATKA = Land(LandIndex::KAMCHATKA, { LandIndex::YAKUTSK, LandIndex::IRKUTSK, LandIndex::MONGOLIA, LandIndex::JAPAN, LandIndex::ALASKA });
const Land Land::IRKUTSK = Land(LandIndex::IRKUTSK, { LandIndex::YAKUTSK, LandIndex::KAMCHATKA, LandIndex::MONGOLIA, LandIndex::SIBERIA });
const Land Land::JAPAN = Land(LandIndex::JAPAN, { LandIndex::KAMCHATKA, LandIndex::MONGOLIA });
const Land Land::MONGOLIA = Land(LandIndex::MONGOLIA, { LandIndex::SIBERIA, LandIndex::IRKUTSK, LandIndex::KAMCHATKA, LandIndex::JAPAN, LandIndex::CHINA });
const Land Land::CHINA = Land(LandIndex::CHINA, { LandIndex::MONGOLIA, LandIndex::SIBERIA, LandIndex::URAL, LandIndex::AFGHANISTAN, LandIndex::INDIA, LandIndex::SIAM });
const Land Land::SIAM = Land(LandIndex::SIAM, { LandIndex::INDIA, LandIndex::CHINA, LandIndex::INDONESIA });


const Land Land::INDONESIA = Land(LandIndex::INDONESIA, { LandIndex::SIAM, LandIndex::NEW_GUINEA, LandIndex::WESTERN_AUSTRALIA });
const Land Land::NEW_GUINEA = Land(LandIndex::NEW_GUINEA, { LandIndex::INDONESIA, LandIndex::EASTERN_AUSTRALIA, LandIndex::WESTERN_AUSTRALIA });
const Land Land::WESTERN_AUSTRALIA = Land(LandIndex::WESTERN_AUSTRALIA, { LandIndex::EASTERN_AUSTRALIA, LandIndex::NEW_GUINEA, LandIndex::INDONESIA });
const Land Land::EASTERN_AUSTRALIA = Land(LandIndex::EASTERN_AUSTRALIA, { LandIndex::WESTERN_AUSTRALIA, LandIndex::NEW_GUINEA });

const uint64_t Land::CARD_INFANTRY_BITMASK = Land::ALASKA.landIndexBitMask | Land::ARGENTINA.landIndexBitMask | Land::CONGO.landIndexBitMask | Land::CHINA.landIndexBitMask
| Land::EAST_AFRICA.landIndexBitMask | Land::EGYPT.landIndexBitMask | Land::ICELAND.landIndexBitMask | Land::KAMCHATKA.landIndexBitMask | Land::MIDDLE_EAST.landIndexBitMask 
| Land::MONGOLIA.landIndexBitMask | Land::NEW_GUINEA.landIndexBitMask | Land::PERU.landIndexBitMask | Land::SIAM.landIndexBitMask | Land::VENEZUELA.landIndexBitMask;

const uint64_t Land::CARD_HORSE_BITMASK = Land::AFGHANISTAN.landIndexBitMask | Land::ALBERTA.landIndexBitMask | Land::QUEBEC.landIndexBitMask | Land::GREENLAND.landIndexBitMask 
| Land::INDIA.landIndexBitMask | Land::IRKUTSK.landIndexBitMask | Land::MADAGASKAR.landIndexBitMask | Land::NORTH_AFRICA.landIndexBitMask | Land::ONTARIO.landIndexBitMask
| Land::UKRAINE.landIndexBitMask | Land::SCANDINAVIA.landIndexBitMask | Land::SIBERIA.landIndexBitMask | Land::URAL.landIndexBitMask | Land::YAKUTSK.landIndexBitMask;

const uint64_t Land::CARD_SIGE_BITMASK = Land::BRAZIL.landIndexBitMask | Land::CENTRAL_AMERICA.landIndexBitMask | Land::EASTERN_AUSTRALIA.landIndexBitMask | Land::EASTERN_UNITED_STATES.landIndexBitMask
| Land::GREAT_BRITAIN.landIndexBitMask | Land::INDONESIA.landIndexBitMask | Land::JAPAN.landIndexBitMask | Land::NORTHERN_EUROPE.landIndexBitMask | Land::NORTHWEST_TERRIOTRY.landIndexBitMask 
| Land::SOUTH_AFRICA.landIndexBitMask | Land::SOUTHERN_EUROPE.landIndexBitMask | Land::WESTERN_AUSTRALIA.landIndexBitMask | Land::WESTERN_EUROPE.landIndexBitMask 
| Land::WESTERN_UNITED_STATES.landIndexBitMask;

const LandIndex Land::SKIP_MOVE = LandIndex::Count;
const uint64_t Land::SKIP_MOVE_MASK = Utility::li2lm(LandIndex::Count);