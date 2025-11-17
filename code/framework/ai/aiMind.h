#pragma once

#include "aiEnums.h"

#include "..\library\libraryStored.h"
#include "..\nav\navFlags.h"

#include "..\..\core\random\randomNumber.h"

namespace Framework
{
	namespace AI
	{
		class LogicData;
		class SocialData;

		class Mind
		: public LibraryStored
		{
			FAST_CAST_DECLARE(Mind);
			FAST_CAST_BASE(LibraryStored);
			FAST_CAST_END();
			LIBRARY_STORED_DECLARE_TYPE();

			typedef LibraryStored base;
		public:
			Mind(Library * _library, LibraryName const & _name);
			virtual ~Mind();

			Name const & get_logic_name() const { return logicName; }
			LogicData const * get_logic_data() const { return logicData; }

			SocialData const * get_social_data() const { return socialData; }

			Nav::Flags const & get_nav_flags() const { return navFlags; }
			bool does_use_locomotion() const { return useLocomotion; }
			
			bool does_allow_wall_crawling() const { return allowWallCrawling; }
			bool does_assume_wall_crawling() const { return assumeWallCrawling; }
			Random::Number<float> const& get_wall_crawling_yaw_offset() const { return wallCrawlingYawOffset; }
			
			LocomotionState::Type get_locomotion_when_mind_disabled() const { return locomotionWhenMindDisabled; }

		public: // LibraryStored
			override_ bool load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc);
			override_ bool prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext);

		private:
			Name logicName;
			Nav::Flags navFlags = Nav::Flags::none(); // at least one is required - this is to determine the whole path
			bool useLocomotion = true;
			bool allowWallCrawling = false;
			bool assumeWallCrawling = false; // if set to true, assumes all rooms are crawable
			Random::Number<float> wallCrawlingYawOffset; // when walking on walls, ai may want to wander off the path a bit
			bool processAIMessages = true;

			LocomotionState::Type locomotionWhenMindDisabled = LocomotionState::Stop;
			LogicData* logicData = nullptr;
			SocialData* socialData = nullptr;
		};
	};
};

