#pragma once

#include "..\..\core\types\optional.h"

class SimpleVariableStorage;

namespace Framework
{
	namespace MeshGeneration
	{
		struct GenerationContext;

		struct UVDef
		{
			void clear();

			float get_u(GenerationContext const & _context) const;
			void set_u(float _u) { u = _u; }

			bool load_from_xml(IO::XML::Node const * _node);
			bool load_from_xml(IO::XML::Node const * _node, tchar const * _uAttr, tchar const * _uParamAttr);
			bool load_from_xml_with_clearing(IO::XML::Node const * _node); // if loads anything, first clears

			bool is_u_valid() const { return u.is_set() || uParam.is_set(); }

			bool operator==(UVDef const& _other) const
			{
				if (uParam.is_set())
				{
					return uParam == _other.uParam;
				}
				else
				{
					return u == _other.u;
				}
			}

		private:
			Optional<float> u;
			Optional<Name> uParam;
		};
	};
};
