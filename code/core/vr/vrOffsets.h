#pragma once

#include "..\types\hand.h"

struct Transform;

namespace IO
{
	namespace XML
	{
		class Node;
	};
};

namespace VR
{
	/*
	 * hides different offsets per different vr system
	 */
	class Offsets
	{
	public:
#ifdef BUILD_NON_RELEASE
		static Transform& dev_access_hand(Hand::Type _hand);
#endif

		static void apply_custom_offsets();

		static Transform get_left_hand();
		static Transform get_right_hand();
		static Transform get_hand(Hand::Type _hand);

		// for hand tracking
		static Transform get_left_hand_tracking();
		static Transform get_right_hand_tracking();
		static Transform get_hand_tracking(Hand::Type _hand);

		static Transform get_left_finger_tracking();
		static Transform get_right_finger_tracking();
		static Transform get_finger_tracking(Hand::Type _hand);

		static bool load_from_xml(IO::XML::Node const * _node);
		static bool save_to_xml(IO::XML::Node * _node);
	};
};
