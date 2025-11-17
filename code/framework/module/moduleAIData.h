#pragma once

#include "moduleData.h"
#include "..\..\core\math\math.h"
#include "..\..\core\containers\array.h"
#include "..\library\usedLibraryStored.h"
#include "..\ai\aiMind.h"

namespace Framework
{
	namespace AI
	{
		class Mind;
	};

	class ModuleAIData
	: public ModuleData
	{
		FAST_CAST_DECLARE(ModuleAIData);
		FAST_CAST_BASE(ModuleData);
		FAST_CAST_END();
		typedef ModuleData base;
	public:
		ModuleAIData(LibraryStored* _inLibraryStored);

		void use_mind(AI::Mind* _mind) { mind.set(_mind); }
		void use_mind_controlled_by_player(AI::Mind* _mind) { mindControlledByPlayer.set(_mind); }

	public: // ModuleData
		override_ bool read_parameter_from(IO::XML::Node const * _node, LibraryLoadingContext & _lc);
		override_ bool prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext);

	protected:	friend class ModuleAI;
		UsedLibraryStored<AI::Mind> mind;
		UsedLibraryStored<AI::Mind> mindControlledByPlayer;
	};
};


