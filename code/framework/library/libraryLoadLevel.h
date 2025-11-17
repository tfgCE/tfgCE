#pragma once


namespace Framework
{
	namespace LibraryLoadLevel
	{
		enum Predefined
		{
			Init			= 0,	// when engine starts
			Close			= 0,	// close whole game

			System			= 10,	// system loaded

			Main			= 20,	// main stuff, always loaded, menus, fonts etc
			AlwaysLoaded	= 20,

			PostLoad		= 50,	// all objects created after loading game if other library load level is not set

			Game			= 100,	// game related stuff, characters

			Level			= 200,	// for one level only

			WithoutLibrary	= 1000,
		};
		typedef int Type;
	};
};
