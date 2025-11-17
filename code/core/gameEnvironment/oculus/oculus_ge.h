#pragma once

#include "..\iGameEnvironment.h"

#include "..\..\types\optional.h"

#include <functional>

namespace GameEnvironment
{
	class Oculus
	: public IGameEnvironment
	{
	public:
		static void init();

	public:
		implement_ Optional<bool> is_ok() const;

		implement_ void set_achievement(Name const& _achievementName);

	protected:
		Oculus();
		virtual ~Oculus();
	};
}