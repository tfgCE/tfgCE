#pragma once

#include "..\..\..\..\framework\ai\aiMessageHandler.h"
#include "..\..\..\..\framework\appearance\socketID.h"

#include "..\..\..\..\core\functionParamsStruct.h"
#include "..\..\..\..\core\memory\safeObject.h"
#include "..\..\..\..\core\mesh\boneID.h"
#include "..\..\..\..\core\other\simpleVariableStorage.h"

namespace Framework
{
	class Display;
	class Library;
	interface_class IModulesOwner;
	struct LibraryLoadingContext;
	struct LibraryPrepareContext;
	namespace AI
	{
		class MindInstance;
	};
};

namespace TeaForGodEmperor
{
	class ModuleBox;

	namespace AI
	{
		namespace Logics
		{
			struct Confussion
			{
			public:
				Confussion();
				~Confussion();

			public:
				void setup(Framework::AI::MindInstance* _mind);

				void advance(float _deltaTime);

			public:
				bool is_confused() const { return confused || confussionTimeLeft > 0.0f; }
					
				void be_not_confused(tchar const* _reason);

			private:
				SafePtr< Framework::IModulesOwner> owner;

				::Framework::AI::MessageHandler messageHandler;

				bool confused = false;
				float confussionTimeLeft = 0.0f;

				SimpleVariableInfo confusedVar;
				bool emissive_confused = false;

				void update_emissive_and_var();
			};
		};
	};
};