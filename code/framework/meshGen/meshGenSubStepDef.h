#pragma once

#include "meshGenValueDef.h"

class SimpleVariableStorage;

namespace Framework
{
	namespace MeshGeneration
	{
		struct GenerationContext;

		struct SubStepDef
		{
			bool is_set() const { return count.is_set() || divider.is_set(); }
			bool should_sub_divide_linear(GenerationContext const& _context) const { return subDivideLinear.is_set() && subDivideLinear.get(_context); }

			int calculate_sub_step_count_for(float _length, GenerationContext const & _context, Optional<float> const & _details = NP /* 1.0f */, Optional<bool> const & _ignoreAutoModifiers = NP) const; // it should be length, details will increase number of sub steps

			bool load_from_xml(IO::XML::Node const * _node);
			bool load_from_xml_child_node(IO::XML::Node const * _node, tchar const * const _subStepChildName = TXT("subStep"));

			void load_from_variable_storage(SimpleVariableStorage const & _storage);

			void load_from_context(GenerationContext const & _context);

		private:
			ValueDef<bool> subDivideLinear; // to ignore if linear
			ValueDef<bool> evenNumber; // will always make sure it is an even number (if odd, will add 1)
			ValueDef<bool> ignoreLODDetails; // will always make sure it is an even number (if odd, will add 1)
			ValueDef<int> count; // forced value
			ValueDef<int> minCount;
			ValueDef<int> maxCount;
			ValueDef<float> divider; // if count is not set, this is used to divide <length> to get count, this is requested length of single segment
			ValueDef<float> maxDivider; // if not zero, then applied (used after details)
			ValueDef<float> dividerLessDetails; // if not "more details" is set in game
			ValueDef<float> dividerCoefLessDetails; // if not "more details" is set in game
		};
	};
};
