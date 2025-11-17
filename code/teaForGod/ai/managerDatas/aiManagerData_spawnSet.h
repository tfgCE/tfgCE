#pragma once

#include "..\aiManagerData.h"

#include "..\..\..\core\random\randomNumber.h"
#include "..\..\..\core\tags\tagCondition.h"

#include "..\..\..\framework\library\libraryName.h"
#include "..\..\..\framework\library\usedLibraryStored.h"

namespace Framework
{
	class ObjectType;
	class Region;
	class Room;
};

namespace TeaForGodEmperor
{
	namespace AI
	{
		namespace ManagerDatasLib
		{
			struct SpawnSetChosenExtraInfo
			{
				int spawnBatch = 1;
				Vector3 spawnBatchOffset = Vector3::zero;
			};
			class SpawnSet
			: public ManagerData
			{
				FAST_CAST_DECLARE(SpawnSet);
				FAST_CAST_BASE(ManagerData);
				FAST_CAST_END();

				typedef ManagerData base;
			public:
				static void register_itself();

				static Framework::ObjectType* choose(Name const & _spawnSet, Framework::Region* _region, Framework::Room* _room, Random::Generator & _rg,
													 OUT_ SpawnSetChosenExtraInfo* _extraInfo = nullptr,
													 std::function<int(Framework::ObjectType* _ofType, int _any)> _how_many_already_spawned = nullptr); // if room provided, region may be skipped
				static bool has_anything(Name const& _spawnSet, Framework::Region* _region, Framework::Room* _room);

			public: // ManagerData
				override_ bool load_from_xml(IO::XML::Node const * _node, ::Framework::LibraryLoadingContext & _lc);
				override_ bool prepare_for_game(::Framework::Library* _library, ::Framework::LibraryPrepareContext& _pfgContext);

				override_ void log(LogInfoContext& _log) const;

			protected:
				struct Element
				{
					float probCoef = 1.0f;
					Optional<int> limit;
					Optional<int> limitAny; // how many may there be in a spawn manager etc (if multiple types are mixed, if limitAny is set to 1 (can also be provided as limit at the top level of xml node, check loading)
					Random::Number<int> spawnBatch; // will spawn all of them (in the same loc)
					Vector3 spawnBatchOffset = Vector3(0.0f, 0.0f, 0.2f); // by default we assume small ones are spawned in batches and their spawn on top of each other
					Framework::LibraryName name;
					TagCondition tagged;
					struct ObjectType
					{
						Framework::UsedLibraryStored<Framework::ObjectType> objectType;
						float probCoef = 1.0f;

						ObjectType() {}
						ObjectType(Framework::ObjectType* _ot);
					};
					Array<ObjectType> objectTypes;
				};
				Array<Element> elements;

				bool allowGoingUp = false; // if set, will allow getting elements from region above, by default: false = won't go up if a spawn set is found

				static const int MAX_CHOOSE_FROM_ELEMENTS = 256;
				static Framework::ObjectType* choose_from(ArrayStatic<Element, MAX_CHOOSE_FROM_ELEMENTS> & _from, Random::Generator& _rg, OUT_ SpawnSetChosenExtraInfo* _extraInfo, std::function<int(Framework::ObjectType* _ofType, int _any)> _how_many_already_spawned = nullptr);
			};
		};
	};
};
