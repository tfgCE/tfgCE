#pragma once

#include "..\..\..\framework\module\moduleCustom.h"
#include "..\..\..\framework\module\moduleData.h"

#include "..\..\..\core\physicalSensations\physicalSensations.h"
#include "..\..\..\core\types\colour.h"

namespace TeaForGodEmperor
{
	namespace CustomModules
	{
		/**
		 *	Tells that object can be grabbed (and dragged)
		 */
		class Grabable
		: public Framework::ModuleCustom
		{
			FAST_CAST_DECLARE(Grabable);
			FAST_CAST_BASE(Framework::ModuleCustom);
			FAST_CAST_END();

			typedef Framework::ModuleCustom base;
		public:
			static Framework::RegisteredModule<Framework::ModuleCustom> & register_itself();

			Grabable(Framework::IModulesOwner* _owner);
			virtual ~Grabable();

			// change and return last one
			bool set_can_be_grabbed(bool _canBeGrabbed);
			bool can_be_grabbed() const { return canBeGrabbed && !forceRelease; }
			
			void set_grabbed(Framework::IModulesOwner* _by, bool _grabbed);
			bool is_grabbed() const { return !grabbedBy.is_empty(); }
			ArrayStatic<SafePtr<::Framework::IModulesOwner>, 8> const& get_grabbed_by() const { return grabbedBy; }

			void set_force_release(bool _forceRelease = true) { forceRelease = _forceRelease; }
			bool should_be_forced_released() const { return forceRelease; }

			Name const & get_grabable_axis_socket_name() const { return grabableAxisSocket; }
			Name const & get_grabable_dial_socket_name() const { return grabableDialSocket; }

			void process_control(Transform const & _controlWS);

		public:	// Module
			override_ void setup_with(::Framework::ModuleData const * _moduleData);

		private:
			enum ReferenceFrame
			{
				Attachment
			};

			Concurrency::SpinLock canBeGrabbedLock = Concurrency::SpinLock(TXT("TeaForGodEmperor.CustomModules.Grabable.canBeGrabbedLock")); // to disallow changes
			bool canBeGrabbed = true;
			bool forceRelease = false;

			Concurrency::SpinLock grabbedByLock = Concurrency::SpinLock(TXT("TeaForGodEmperor.CustomModules.Grabable.grabbedByLock")); // to disallow changes
			ArrayStatic<SafePtr<::Framework::IModulesOwner>, 8> grabbedBy;

			ReferenceFrame referenceFrame = ReferenceFrame::Attachment; // only type right now!

			// these are relative to reference frame
			Optional<Transform> startingPlacementRS;
			Optional<Transform> startingControlRS;

			// params
			Name grabableAxisSocket; // forward dir is along the axis
			Name grabableDialSocket; // if is a dial (Z axis is always up, we rotate around it), top of the surface

			struct PhysicalSensationPullPush
			{
				::Framework::IModulesOwner* pullerPusher = nullptr; // just reference info
				Optional<PhysicalSensations::Sensation::ID> physSens;
				float timeSinceMoving = 0.0f;

				void update(ArrayStatic<SafePtr<::Framework::IModulesOwner>, 8> const & grabbedBy);
			} physicalSensationPullPush;
		};
	};
};

