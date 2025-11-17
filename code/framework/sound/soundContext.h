#pragma once

namespace System
{
	class Sound;
};

namespace Framework
{
	namespace Sound
	{
		/**
		 *	
		 */
		class Context
		{
		public:
			Context(::System::Sound* _sound);
			~Context();

			inline ::System::Sound* get_sound() const { return sound; }

		private:
			::System::Sound* sound;

			Context(Context const & _other); // inaccessible
			Context & operator=(Context const & _other); // inaccessible
		};

	};
};
