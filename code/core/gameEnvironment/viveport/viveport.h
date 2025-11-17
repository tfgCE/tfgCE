#pragma once

#include "..\iGameEnvironment.h"

#include "..\..\types\optional.h"

#include <functional>

namespace GameEnvironment
{
	struct ViveportImpl;

	class Viveport
	: public IGameEnvironment
	{
	public:
		static void init(tchar const* _viveportId, tchar const* _viveportKey);

	public:
		implement_ Optional<bool> is_ok() const;

		implement_ void set_achievement(Name const& _achievementName);

	protected:
		Viveport(tchar const* _viveportId, tchar const* _viveportKey);
		virtual ~Viveport();

	private:
		ViveportImpl* impl;
	};
}