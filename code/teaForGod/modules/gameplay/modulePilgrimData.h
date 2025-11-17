#pragma once

#include "..\..\..\core\types\hand.h"
#include "..\..\..\core\vr\iVR.h"
#include "..\..\..\framework\module\moduleData.h"
#include "..\..\..\framework\library\usedLibraryStored.h"

namespace Framework
{
	class ActorType;
};

namespace TeaForGodEmperor
{
	class ModulePilgrim;

	class ModulePilgrimData
	: public Framework::ModuleData
	{
		FAST_CAST_DECLARE(ModulePilgrimData);
		FAST_CAST_BASE(Framework::ModuleData);
		FAST_CAST_END();
		typedef Framework::ModuleData base;
	public: // types are exposed

		struct Pocket
		{
			Name name;
			int pocketLevel = 0;
			Name socket;
			Optional<Hand::Type> side;
			Array<Name> objectHoldSockets;
			int materialParamIndex = 0; // index in an array
		};

		struct Forearm
		{
			Name forearmStartSocket;
			Name forearmEndSocket;
			Name handOverSocket; // socket in hand that is decided to check whether hand is over forearm or not (in hand actor!)
		};
		struct AttachableActor
		{
			::Framework::UsedLibraryStored<Framework::ActorType> actorType;
			Name attachToSocket;
		};
		struct SocketSet
		{
			bool allowAnyOnSpecificHand = true;
			Name any; // if left or right is not defined, this one is chosen
			Name left;
			Name right;

			Name const & get() const { return any; }
			Name const & get(Hand::Type _hand, Optional<bool> const & _overrideAllowance = NP) const { return _hand == Hand::Left ? get_left(_overrideAllowance) : get_right(_overrideAllowance); }
			Name const & get_left(Optional<bool> const & _overrideAllowance = NP) const { return left.is_valid()? left : (_overrideAllowance.get(allowAnyOnSpecificHand) ? any : Name::invalid()); }
			Name const & get_right(Optional<bool> const & _overrideAllowance = NP) const { return right.is_valid() ? right : (_overrideAllowance.get(allowAnyOnSpecificHand)? any : Name::invalid()); }

			SocketSet() {}
			SocketSet(bool _allowAnyOnSpecificHand) : allowAnyOnSpecificHand(_allowAnyOnSpecificHand) {}
		};
		struct GrabPoint
		{
			// axis should be aligned with grabbed object axis (it doesn't have to point in same dir, just have to be aligned/parallel, although for picking up items we choose two directional)
			Axis::Type axis = Axis::Z;
			float axisSign = 1.0f; // relevant only for dials
			// for dials it is turning axis

			// dir points where grabable axis of grabbed object is (this points from hand towards grabbed object)
			Axis::Type dirAxis = Axis::Z;
			// in grabbed object Forward axis always tells the direction of grabbed handle/alings with handle
			float dirSign = 1.0f;
			// for dials this is forward

			GrabPoint();
			GrabPoint(Axis::Type _axis, float _axisSign, Axis::Type _dirAxis, float _dirSign) : axis(_axis), axisSign(_axisSign), dirAxis(_dirAxis), dirSign(_dirSign) {}
		};

	public:
		ModulePilgrimData(Framework::LibraryStored* _inLibraryStored);
		virtual ~ModulePilgrimData();

		AttachableActor const & get_hand(Hand::Type _hand) const { return _hand == Hand::Left? leftHand : rightHand; }
		//
		AttachableActor const& get_hand_forearm_display(Hand::Type _hand, OUT_ bool& _attachToHand) const { _attachToHand = _hand == Hand::Left ? leftHandForearmDisplayAttachToHand : rightHandForearmDisplayAttachToHand;  return _hand == Hand::Left ? leftHandForearmDisplay : rightHandForearmDisplay; }
		//
		SocketSet const & get_hand_hold_point_sockets() const { return handHoldPointSockets; }
		//
		SocketSet const & get_hand_grab_point_sockets() const { return handGrabPointSockets; }
		GrabPoint const & get_hand_grab_point(Hand::Type _hand) const { return _hand == Hand::Left ? leftHandGrabPoint : rightHandGrabPoint; }
		//
		SocketSet const & get_hand_grab_point_dial_sockets() const { return handGrabPointDialSockets; }
		GrabPoint const & get_hand_grab_point_dial(Hand::Type _hand) const { return _hand == Hand::Left ? leftHandGrabPointDial : rightHandGrabPointDial; }
		//
		SocketSet const & get_port_hold_point_sockets() const { return portHoldPointSockets; }
		//
		AttachableActor const & get_rest_point(Hand::Type _hand) const { return _hand == Hand::Left ? leftRestPoint : rightRestPoint; }
		SocketSet const & get_rest_hold_point_sockets() const { return restHoldPointSockets; }
		
		Name const& get_physical_violence_dir_socket() const { return physicalViolenceDirSocket; }
		VR::Pulse const& get_physical_violence_pulse() const { return physicalViolencePulse; }
		VR::Pulse const& get_strong_physical_violence_pulse() const { return strongPhysicalViolencePulse; }

		SimpleVariableStorage const& get_equipment_parameters() const { return equipmentParameters; }

	public: // ModuleData
		override_ bool read_parameter_from(IO::XML::Attribute const* _attr, Framework::LibraryLoadingContext& _lc);
		override_ bool read_parameter_from(IO::XML::Node const * _node, Framework::LibraryLoadingContext & _lc);
		override_ bool prepare_for_game(Framework::Library* _library, Framework::LibraryPrepareContext& _pfgContext);

	private:
		Framework::UsedLibraryStored<Framework::ActorType> pilgrimKeeperActorType;

		AttachableActor leftHand;
		AttachableActor rightHand;

		AttachableActor leftHandForearmDisplay;
		AttachableActor rightHandForearmDisplay;
		bool leftHandForearmDisplayAttachToHand = false;
		bool rightHandForearmDisplayAttachToHand = false;

		Forearm leftForearm;
		Forearm rightForearm;

		// always try to use hold point specific to the hand
		// hold points are point in equipment (and hand) that should be matched, where do we attach one relatively to another
		// this is used in both equipment and hand
		SocketSet handHoldPointSockets;

		// this is used only by hand, proper placement is provided by grabable object
		SocketSet handGrabPointSockets = SocketSet(false);
		GrabPoint leftHandGrabPoint = GrabPoint(Axis::Z, 1.0f, Axis::Z, 1.0f);
		GrabPoint rightHandGrabPoint = GrabPoint(Axis::Z, 1.0f, Axis::Z, -1.0f);
		SocketSet handGrabPointDialSockets = SocketSet(false);
		GrabPoint leftHandGrabPointDial = GrabPoint(Axis::X, 1.0f, Axis::Y, 1.0f);
		GrabPoint rightHandGrabPointDial = GrabPoint(Axis::X, 1.0f, Axis::Y, 1.0f);

		// as above but for port
		SocketSet portHoldPointSockets;

		// as above but for rest
		SocketSet restHoldPointSockets;

		// rest points for main equipment
		AttachableActor leftRestPoint;
		AttachableActor rightRestPoint;

		// equipment params
		SimpleVariableStorage equipmentParameters;

		// energy inhale
		Name energyInhaleSocket;

		// head (for detection of in game menu?)
		Name headMenuSocket;
		Name headMenuHandTriggerSocket; // in both hands has the same name

		// physical violence
		Name physicalViolenceDirSocket; // in both hands has the same name
		VR::Pulse physicalViolencePulse;
		VR::Pulse strongPhysicalViolencePulse;

		// pockets
		Array<Pocket> pockets;
		Optional<int> pocketsMaterialIndex;
		Name pocketsMaterialParam; // array (vec4)

		friend class ModulePilgrim;
	};
};


