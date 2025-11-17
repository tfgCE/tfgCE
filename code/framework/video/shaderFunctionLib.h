#pragma once

#include "..\..\core\system\video\video3d.h"
#include "..\..\core\containers\array.h"

#include "..\library\libraryStored.h"

namespace Concurrency
{
	class JobExecutionContext;
};

namespace Framework
{
	/**
	 *	Works as additional source and set of "depends on shader function libs"
	 */
	class ShaderFunctionLib
	: public LibraryStored
	{
		FAST_CAST_DECLARE(ShaderFunctionLib);
		FAST_CAST_BASE(LibraryStored);
		FAST_CAST_END();
		LIBRARY_STORED_DECLARE_TYPE();

		typedef LibraryStored base;
	public:
		ShaderFunctionLib(Library * _library, LibraryName const & _name);

		::System::ShaderSetup const & get_shader_setup() const { return *shaderSetup.get(); }
		Array<LibraryName> const & get_depends_on_shader_libs() const { return dependsOnShaderFunctionLibs; }

	public: // LibraryStored
		override_ bool load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc);
		override_ bool prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext);

	private:
		Array<LibraryName> dependsOnShaderFunctionLibs;
		RefCountObjectPtr<::System::ShaderSetup> shaderSetup;

	};

};
