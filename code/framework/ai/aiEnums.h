#pragma once

struct String;

namespace Framework
{
	class ModuleController;

	namespace AI
	{
		class Locomotion;

		namespace LocomotionState
		{
			enum Type
			{
				Keep,
				KeepControllerOnly, // locomotion is cleared, but controller is still set
				NoControl,
				Stop,
			};

			Type parse(String const & _value, Type _default);

			void apply(Type _ls, Locomotion & _locomotion, ModuleController* _controller);
		};
	};
};
