#pragma once

#include "..\iGameEnvironment.h"

#include "..\..\types\optional.h"

#include <functional>

namespace GameEnvironment
{
	class Pico
	: public IGameEnvironment
	{
	public:
		static void init();

	public:
		implement_ Optional<bool> is_ok() const;

		implement_ void set_achievement(Name const& _achievementName);

	protected:
		Pico();
		virtual ~Pico();
	};
}