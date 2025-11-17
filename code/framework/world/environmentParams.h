#pragma once

#include "..\..\core\types\name.h"

#include "..\library\usedLibraryStored.h"

struct Matrix44;

namespace System
{
	class ShaderProgramBindingContext;
};

namespace IO
{
	namespace XML
	{
		class Node;
	};
};

namespace Framework
{
	struct LibraryLoadingContext;
	class Library;
	struct LibraryPrepareContext;
	class LibraryTools;
	class Texture;

	struct EnvironmentParams;

	namespace EnvironmentParamType
	{
		enum Type
		{
			notSet,
			floatNumber,
			floatNumberInverted,
			cycle, // float from valueF[1] to valueF[2] (default: 0 to 1) cycled (cycling is important for lerping!)
			vector2,
			colour, // rgba
			colour3, // rgb
			segment01,
			direction,
			location,
			texture,
			matrix33
		};
	};

	struct EnvironmentParam
	{
	public:
		template<typename Class>
		Class& as() { return *plain_cast<Class>(valueF); }

		template<typename Class>
		Class const & as() const { return *plain_cast<Class>(valueF); }

		Name const & get_name() const { return name; }

		void lerp_with(float _amount, EnvironmentParam const & _param);
		void add(float _amount, EnvironmentParam const & _param);

	public: // make it easier to access
		float const& get_as_float() const { return as<float>(); }
		Vector2 const& get_as_vector2() const { return as<Vector2>(); }
		Vector3 const& get_as_vector3() const { return as<Vector3>(); }
		Colour const& get_as_colour() const { return as<Colour>(); }

	private: friend struct EnvironmentParams;
			 friend class LibraryTools;
		Name name;
		EnvironmentParamType::Type type;
		float valueF[16]; // enought to hold matrix44
		UsedLibraryStored<Texture> texture;

		static EnvironmentParam not_set() { return EnvironmentParam(EnvironmentParamType::notSet); }

		EnvironmentParam();
		EnvironmentParam(EnvironmentParamType::Type _type);

		void setup_shader_binding_context(::System::ShaderProgramBindingContext * _bindingContext, Matrix44 const & _modelViewMatrixReadiedForRendering) const;

		bool load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc);
		bool prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext);

		void log(LogInfoContext & _log) const;
	};

	struct EnvironmentParams
	{
	public:
		void clear();
		void fill_with(EnvironmentParams const & _source, Array<Name> const * _skipParams = nullptr); // source would be parent most of the time, if already set, is skipped
		void set_from(EnvironmentParams const & _source, Array<Name> const * _skipParams = nullptr);
		void set_missing_from(EnvironmentParams const & _source, Array<Name> const * _skipParams = nullptr);
		void copy_from(EnvironmentParams const & _source, Array<Name> const * _skipParams = nullptr);
		void lerp_with(float _amount, EnvironmentParams const & _source, Array<Name> const * _skipParams = nullptr);

		EnvironmentParam const * get_param(Name const & _name) const;
		EnvironmentParam* access_param(Name const & _name, Optional<EnvironmentParamType::Type> const & _addOfTypeIfNotThere = NP);

		Array<EnvironmentParam> & access_params() { return params; }
		Array<EnvironmentParam> const & get_params() const { return params; }

		void setup_shader_binding_context(::System::ShaderProgramBindingContext * _bindingContext, Matrix44 const & _modelViewMatrixReadiedForRendering) const;

		bool load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc);
		bool prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext);

#ifdef AN_TWEAKS
		void tweak(EnvironmentParam const & _param);
#endif

		void log(LogInfoContext & _log) const;

	private:
		Array<EnvironmentParam> params;
	};
};
