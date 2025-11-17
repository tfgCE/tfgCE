#pragma once

#include "..\..\core\containers\map.h"
#include "..\..\core\types\name.h"
#include "..\..\core\other\factoryOfNamed.h"

namespace Framework
{
	class AppearanceControllerData;

	/**
	 *	Function that can register appearance controllers by name and allows creation of data.
	 *	Data object itself creates actual appearance controllers.
	 */
	class AppearanceControllers
	: private FactoryOfNamed<AppearanceControllerData, Name>
	{
		typedef FactoryOfNamed<AppearanceControllerData, Name> base;
	public:
		static void initialise_static();
		static void close_static();

		static void register_appearance_controller(Name const & _name, Create _create) { base::add(_name, _create); }
		static AppearanceControllerData* create_data(Name const & _name) { return base::create(_name); }
	};
};
