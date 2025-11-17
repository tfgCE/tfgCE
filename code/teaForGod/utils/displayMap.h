#pragma once

#include "..\..\core\math\math.h"
#include "..\..\core\memory\safeObject.h"

#include "..\..\framework\library\libraryName.h"

namespace Framework
{
	class Display;
	class Room;
	class TexturePart;
	interface_class IAIObject;
	interface_class IModulesOwner;
};

namespace TeaForGodEmperor
{
	class DisplayMap
	{
	public:
		DisplayMap();

		void set_room(Framework::Room* _room) { room = _room; }
		void add(Framework::IAIObject* _object, Framework::TexturePart* _tp, Optional<Colour> const & _colour);

		void update();
		void draw(Framework::Display* _display, RangeInt2 const & _displayAt);

	private:
		RangeInt2 displayAt;
		Range2 mapRoom = Range2::empty;

		Framework::Room* room = nullptr;

		struct Object
		{
			SafePtr<Framework::IAIObject> object;
			Framework::TexturePart* tp = nullptr;
			Optional<Colour> colour;
		};

		Array<Object> objects;

		struct ObjectFromUpdate
		{
			SafePtr<Framework::IModulesOwner> object;
			Framework::LibraryName libraryName;
			Optional<Colour> colour;
		};

		Array<ObjectFromUpdate> objectsFromUpdate;
	};

};
