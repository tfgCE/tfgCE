#pragma once

#include "..\iGameEnvironment.h"

namespace GameEnvironment
{
	class Steamworks
	: public IGameEnvironment
	{
	public:
		static void init(tchar const* _appId);

	public:
		implement_ Optional<bool> is_ok() const { return isOk; }

		implement_ void set_achievement(Name const& _achievementName);

		implement_ tchar const* get_language();

	protected:
		Steamworks(tchar const* _appId);
		virtual ~Steamworks();

	private:
		bool isOk = true;
	};
}