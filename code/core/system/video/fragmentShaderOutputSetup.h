#pragma once

#include "outputSetup.h"

namespace System
{
	class RenderTarget;

	struct FragmentShaderOutputSetup
	: public OutputSetup<>
	{
	public:
		typedef OutputSetup<> base;

	public:
		FragmentShaderOutputSetup()
		: sizeMultiplier(Vector2::one)
		{
		}

		bool load_from_xml(IO::XML::Node const * _node, OutputTextureDefinition const & _defaultOutputTextureDefinition);

		RenderTarget* create_render_target(SIZE_ VectorInt2 _size) const;

		bool does_allow_default_output() const { return allowsDefaultOutput; }
		void allow_default_output(bool _allowsDefaultOutput = true) { allowsDefaultOutput = _allowsDefaultOutput; }
		void have_at_least_one_default_output(OutputTextureDefinition const & _default);

		Vector2 get_size_multiplier() const { return sizeMultiplier; }

	protected:
		Vector2 sizeMultiplier;
		bool allowsDefaultOutput = false; // will create "colour" default output, using have_at_least_one_default_output
	};

};
