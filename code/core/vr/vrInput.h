#pragma once

#ifdef AN_SDL
#include "SDL.h"
#endif

#include "..\math\math.h"
#include "..\tags\tag.h"
#include "..\types\hand.h"
#include "..\types\names.h"

namespace VR
{
	namespace Input
	{
		typedef uint DeviceID;

		struct Device
		{
			Name name;
			Name forPromptName;
			bool force = false;
			DeviceID id = 0; // should be filled when loading
			Array<Name> forcedForVRSystems;
			Array<String> modelNumbers;
			Array<String> trackingSystemNames;
			Array<String> controllerNames;
			Tags inputTags;

			struct Axis
			{
				enum Component
				{
					X,
					Y
				};
				int axis = NONE;
				Component component = X;

				Axis() {}
				Axis(int _axis) : axis(_axis) {}

				bool load_from_xml(IO::XML::Node const * _node, tchar const * axisAttr, tchar const * componentAttr);
				bool save_to_xml(IO::XML::Node * _node, tchar const * axisAttr, tchar const * componentAttr);
			};
			Axis triggerAxis = 1;
			Axis gripAxis;

			Axis thumbAxis;
			Axis pointerAxis;
			Axis middleAxis;
			Axis ringAxis;
			Axis pinkyAxis;

			bool openVRUseLegacyInput = false; // this is to use older versions of input
			bool fingersFromSkeleton = false;

			bool load_from_xml(IO::XML::Node const * _node);
			bool save_to_xml(IO::XML::Node * _node);
		};

		struct DeviceGroupName
		{
			Name name;
			Name forPromptName;
			DeviceID id = 0; // should be filled when loading
			Array<Name> devices;

			bool load_from_xml(IO::XML::Node const* _node);
			bool save_to_xml(IO::XML::Node* _node);
		};

		class Devices
		{
		public:
			static const uint all = 0xffffffff;
			static void initialise_static();
			static void close_static();

			static bool load_from_xml(IO::XML::Node const * _node);
			static bool save_to_xml(IO::XML::Node * _node);

			static Array<Device> const & get_all();
			static Device const* get(DeviceID _id);
			static DeviceID find(String const & _modelNumber, String const & _trackingSystemName, String const& _controllerName);
			static DeviceID parse(REF_ String & _inputString, Optional<bool> const & _onlyDevices = NP); // alters input string to remove unnecessary

			static String to_string(DeviceID _id, Optional<Hand::Type> _hand);
			static Name const& get_for_prompt_name(DeviceID _id);

		private:
			static Devices* s_devices;

			Array<Device> devices;
			Array<DeviceGroupName> groupNames;
		};

		namespace Stick
		{
			enum Type
			{
				None = -1,
				//
				DPad,
				Joystick,
				Trackpad, // trackpad, if not touched 0,0
				TrackpadTouch, // touching trackpad (last touch is stored)
				TrackpadDir, // directional track pad (like dpad)
				//
				Max,
				//
			};

			tchar const* to_char(Type _type);

			struct WithSource
			{
				Type type = None;
				DeviceID source = Devices::all;
				Optional<Hand::Type> hand;

				WithSource() {}
				WithSource(Type _type, DeviceID _source = Devices::all, Optional<Hand::Type> _hand = NP) : type(_type), source(_source), hand(_hand) {}

				static WithSource parse(String const & _value, Type _default);
				static String to_string(WithSource _stick);

				bool operator==(WithSource const & _other) const { return type == _other.type && source == _other.source; }
			};
		};

		namespace StickDir
		{
			enum Type
			{
				None,
				Left,
				Right,
				Down,
				Up
			};

			Type parse(String const & _value, Type _default);
			String to_string(Type _stick);
		};

		namespace Mouse
		{
			struct WithSource
			{
				DeviceID source = Devices::all;
				Optional<Hand::Type> hand;

				WithSource() {}
				WithSource(DeviceID _source = Devices::all, Optional<Hand::Type> _hand = NP) : source(_source), hand(_hand) {}

				static WithSource parse(String const & _value);
				static String to_string(WithSource _mouse);

				bool operator==(WithSource const & _other) const { return source == _other.source; }
			};
		};

		namespace Button
		{
			enum Type
			{
				None = -1,
				//
				InUse,
				UsesHandTracking,
				//
				Primary,
				Secondary,
				SystemMenu, // oculus
				SystemGestureProcessing, // oculus + hand tracking
				Grip,
				PointerPinch,
				MiddlePinch,
				RingPinch,
				PinkyPinch,
				Trigger,
				Trackpad,
				TrackpadTouch,
				//
				Pose, // is making a pose (detected by the controller)
				UpsideDown, // hand is upside down
				InsideToHead, // hand's inside is towards head
				//
				HeadSideTouch, // touching side of the head
				//
				// folded (non hand-tracking)
				Thumb,
				Pointer,
				Middle,
				Ring,
				Pinky,
				// folded
				ThumbFolded,
				PointerFolded,
				MiddleFolded,
				RingFolded,
				PinkyFolded,
				// straight
				ThumbStraight,
				PointerStraight,
				MiddleStraight,
				RingStraight,
				PinkyStraight,
				//
				DPadUp,
				DPadDown,
				DPadLeft,
				DPadRight,
				//
				JoystickUp,
				JoystickDown,
				JoystickLeft,
				JoystickRight,
				JoystickPress,
				// not implemented?
				TrackpadUp,
				TrackpadDown,
				TrackpadLeft,
				TrackpadRight,
				//
				// when track pad is touched and pressed
				TrackpadDirCentre,
				TrackpadDirUp,
				TrackpadDirDown,
				TrackpadDirLeft,
				TrackpadDirRight,
				//
				Max,
				//
				// aliases
				Action = Primary,
				Menu = Secondary,
				A = Action,
				B = Menu,
				X = Action,
				Y = Menu,
			};

			Type parse(String const& _value);
			tchar const * to_char(Type _type);

			struct WithSource
			{
				Type type = None;
				DeviceID source = Devices::all;
				Optional<Hand::Type> hand;
				bool released = false;

				WithSource() {}
				WithSource(Type _type, DeviceID _source = Devices::all, Optional<Hand::Type> _hand = NP, bool _released = false) : type(_type), source(_source), hand(_hand), released(_released) {}

				static WithSource parse(String const & _value, Type _default);
				static String to_string(WithSource _button);
				String released_to_string() const { return released ? String(TXT("released")) : String::empty(); }

				static WithSource from_stick(Stick::WithSource _stick, StickDir::Type _dir);

				bool operator==(WithSource const & _other) const { return type == _other.type && source == _other.source && hand == _other.hand; }
			};
		};
	};
};
