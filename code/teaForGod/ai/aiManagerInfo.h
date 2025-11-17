#pragma once

#include "..\..\core\tags\tagCondition.h"
#include "..\..\framework\meshGen\meshGenParam.h"
#include "..\..\framework\library\libraryName.h"

#include "aiManagerData.h"

namespace Framework
{
	class Region;
	class Room;
};

namespace TeaForGodEmperor
{
	namespace AI
	{
		class ManagerData;

		class ManagerInfo
		{
		public:
			bool load_from_xml(IO::XML::Node const * _node, Framework::LibraryLoadingContext & _lc);
			bool prepare_for_game(::Framework::Library* _library, ::Framework::LibraryPrepareContext& _pfgContext);

			bool add_to_room(Framework::Room* _room, SimpleVariableStorage const * _additionalOptionalParameters = nullptr) const;
			bool add_to_region(Framework::Region* _region, SimpleVariableStorage const * _additionalOptionalParameters = nullptr) const;

			template <typename Class>
			Class const * get_data(Name const & _name) const;

			void log(LogInfoContext& _log) const;

		private:
			struct RunGameScript
			{
				TagCondition tagCondition;
				Framework::MeshGenParam<Framework::LibraryName> gameScript;
				Framework::MeshGenParam<float> chance;
			};
			Array<RunGameScript> runGameScripts;

			struct AIMind
			{
				TagCondition tagCondition;
				Framework::MeshGenParam<Framework::LibraryName> aiMind;
				Framework::MeshGenParam<float> chance;
				SimpleVariableStorage parameters; // will be overridden by anything else out there
				Tags tagged; // will tag new object
			};
			Array<AIMind> aiMinds;
			Array<RefCountObjectPtr<ManagerData>> datas;

			bool add_to(Framework::Room* _room, Framework::Region* _region, SimpleVariableStorage const * _additionalOptionalParameters = nullptr) const;
		};

		#include "aiManagerInfo.inl"
	};
};
