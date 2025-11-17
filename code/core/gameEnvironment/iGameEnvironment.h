#pragma once

#include "..\globalDefinitions.h"

#include "..\types\optional.h"

namespace GameEnvironment
{
	interface_class IGameEnvironment
	{
	public:
		static IGameEnvironment* get() { return s_gameEnvironment; }

		static void close_static();

	public:
		virtual Optional<bool> is_ok() const = 0; // NP if still unknown

		virtual void set_achievement(Name const& _achievementName) = 0;
		
		virtual tchar const* get_language() { return nullptr; } // leave it to system

	protected:
		IGameEnvironment();
		virtual ~IGameEnvironment();

	private:
		static IGameEnvironment* s_gameEnvironment;
	};
};
