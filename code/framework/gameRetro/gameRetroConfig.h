#pragma once

#include "..\game\gameConfig.h"

#include "gameRetroInput.h"

namespace Framework
{
	struct GameRetroConfig
	: public GameConfig
	{
		FAST_CAST_DECLARE(GameRetroConfig);
		FAST_CAST_BASE(GameConfig);
		FAST_CAST_END();

		typedef GameConfig base;
	public:
		static void initialise_static();

		GameRetroConfig();

		GameRetroInput::Config & access_input() { return inputConfig; }
		GameRetroInput::Config const& get_input() const { return inputConfig; }

	protected: // ::Framework::GameConfig
		override_ bool internal_load_from_xml(IO::XML::Node const * _node);
		override_ void internal_save_to_xml(IO::XML::Node * _container, bool _userSpecific) const;

	protected:
		GameRetroInput::Config inputConfig;
	};

};
