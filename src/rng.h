#pragma once

#include <random>

class Rng
{
	std::random_device RD;
	std::default_random_engine RNG_ENGINE;

	std::uniform_int_distribution<int> RNG_INT;
	std::uniform_int_distribution<int> RNG_DICE;
	std::uniform_real_distribution<float> RNG_FLOAT;

	Rng()
	{
		RNG_ENGINE = std::default_random_engine(RD());

		RNG_INT = std::uniform_int_distribution<int>(0, RAND_MAX);
		RNG_DICE = std::uniform_int_distribution<int>(1, 6);
		RNG_FLOAT = std::uniform_real_distribution<float>(0.0, 1.0);
	}
public:
	int rInt()
	{
		return RNG_INT(RNG_ENGINE);
	}

	int rDice()
	{
		return RNG_DICE(RNG_ENGINE);
	}

	float rFloat()
	{
		return RNG_FLOAT(RNG_ENGINE);
	}

	static Rng& getInstance()
	{
		static Rng INSTANCE;
		return INSTANCE;
	}

	std::default_random_engine& getEngine()
	{
		return RNG_ENGINE;
	}
};

static Rng& RNG = Rng::getInstance();