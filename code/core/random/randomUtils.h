#pragma once

#include <functional>

#include "random.h"

#include "..\utils.h"
#include "..\containers\arrayStack.h"

namespace RandomUtils
{
	template <typename ContainerClass, typename ContainedObject>
	struct ChooseFromContainer
	{
		typedef std::function<float(ContainedObject const & _object)> GetChance;
		typedef std::function<bool(ContainedObject const & _object)> IsValid;

		/*
			int idx = RandomUtils::ChooseFromContainer<Array<Struct>, Struct>::choose(*randomGenerator, array, [](Struct const & _ci) { return _ci.probCoef; });
			int idx = RandomUtils::ChooseFromContainer<Array<Struct>, Struct>::choose(Random::next_int(), array, [](Struct const & _ci) { return _ci.probCoef; });
		*/
		static int choose(Random::Generator & _usingGenerator, ContainerClass const & _container, GetChance _get_chance, IsValid _is_valid = nullptr, bool _noneIfAllInvalid = false);
		static int choose(int _usingRandomNumber, ContainerClass const & _container, GetChance _get_chance, IsValid _is_valid = nullptr, bool _noneIfAllInvalid = false);
	};

	#include "randomUtils.inl"
};
