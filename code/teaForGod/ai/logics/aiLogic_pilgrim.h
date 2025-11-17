#pragma once

#include "..\..\..\core\other\simpleVariableStorage.h"

#include "..\..\..\framework\ai\aiLogicData.h"
#include "..\..\..\framework\ai\aiLogicWithLatentTask.h"

namespace Framework
{
	interface_class IModulesOwner;
};

namespace TeaForGodEmperor
{
	class ModuleBox;

	namespace AI
	{
		namespace Logics
		{
			class PilgrimData;

			/**
			 */
			class Pilgrim
			: public ::Framework::AI::LogicWithLatentTask
			{
				FAST_CAST_DECLARE(Pilgrim);
				FAST_CAST_BASE(::Framework::AI::LogicWithLatentTask);
				FAST_CAST_END();

				typedef ::Framework::AI::LogicWithLatentTask base;
			public:
				static ::Framework::AI::Logic* create_ai_logic(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData) { return new Pilgrim(_mind, _logicData); }

			public:
				Pilgrim(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData);
				virtual ~Pilgrim();

				Vector3 const & get_direction_os() const { return directionOS; }
				float get_section_dist() const { return sectionDist; }
				bool is_section_active() const { return sectionActive; }
				float get_total_dist() const { return totalDist; }
				float consume_dist() { float ret = distToConsume; distToConsume = 0.0f; return ret; }

			public: // Logic
				implement_ void advance(float _deltaTime);

			private:
				static LATENT_FUNCTION(execute_logic);
				static LATENT_FUNCTION(guidance);
				static LATENT_FUNCTION(update_dist);

			private:
				PilgrimData const * pilgrimData = nullptr;

				Vector3 directionOS = Vector3::zero;

				int sectionIdx = NONE;
				bool sectionActive = false;
				float sectionDist = 0.0f;
				float totalDist = 0.0f;
				float distToConsume = 0.0f;
			};


			class PilgrimData
			: public ::Framework::AI::LogicData
			{
				FAST_CAST_DECLARE(PilgrimData);
				FAST_CAST_BASE(::Framework::AI::LogicData);
				FAST_CAST_END();

				typedef ::Framework::AI::LogicData base;
			public:
				static ::Framework::AI::LogicData* create_ai_logic_data() { return new PilgrimData(); }

			public: // LogicData
				virtual bool load_from_xml(IO::XML::Node const * _node, ::Framework::LibraryLoadingContext & _lc);

			private:

				friend class Pilgrim;
			};
		};
	};
};