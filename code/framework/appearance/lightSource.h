#pragma once

#include "..\..\core\math\math.h"
#include "..\..\core\types\colour.h"

namespace Framework
{
	struct LibraryLoadingContext;

	namespace Render
	{
		struct LightSourceProxy;
	};

	struct LightSource
	{
	public:
		static bool allowFlickeringLights; // easier to lookup this way

	public:
		Vector3 location = Vector3::zero; // object space
		Vector3 stick = Vector3::zero; // whole vector - will be placed with centre at location
		Vector3 coneDir = Vector3::zero; // if cone light, can't be stick and cone
		Colour colour = Colour::full;
		float distance = 1.0f;
		float power = 1.0f;
		float coneInnerAngle = 0.0f;
		float coneOuterAngle = 0.0f;
		int priority = 0;
		bool flickeringLight = false;

		bool load_from_xml(IO::XML::Node const* _node, LibraryLoadingContext& _lc);

		friend struct Render::LightSourceProxy;
	};

};
