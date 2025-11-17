#pragma once

#include "..\moduleCustom.h"
#include "..\..\presence\relativeToPresencePlacement.h"
#include "..\..\sound\soundSource.h"

#include "..\..\..\core\functionParamsStruct.h"
#include "..\..\..\core\vr\iVR.h"
#include "..\..\..\core\types\hand.h"

namespace Framework
{
	namespace CustomModules
	{
		class TemporaryObjectSpawnerData;

		/*
		 *	This is used to fill values for vr appearance controller by objects that don't have vr controls, all faked player look alikes
		 */
		class VRPlacementsProvider
		: public ModuleCustom
		{
			FAST_CAST_DECLARE(VRPlacementsProvider);
			FAST_CAST_BASE(ModuleCustom);
			FAST_CAST_END();

			typedef Framework::ModuleCustom base;
		public:
			static Framework::RegisteredModule<Framework::ModuleCustom> & register_itself();

			VRPlacementsProvider(Framework::IModulesOwner* _owner);
			virtual ~VRPlacementsProvider();

			Optional<Transform> const & get_head() const { return head; }
			Optional<Transform> const & get_hand_left() const { return handLeft; }
			Optional<Transform> const & get_hand_right() const { return handRight; }

			void set_head(Optional<Transform> const& _at) { head = _at; }
			void set_hand_left(Optional<Transform> const& _at) { handLeft = _at; }
			void set_hand_right(Optional<Transform> const& _at) { handRight = _at; }

		public:	// Module
			override_ void reset();
			override_ void setup_with(::Framework::ModuleData const * _moduleData);

		protected:
			Optional<Transform> head;
			Optional<Transform> handLeft;
			Optional<Transform> handRight;
		};
	};
};
