#pragma once

namespace Framework
{
	namespace MeshGeneration
	{
		class ElementShapeData;
		struct ElementInstance;
		struct GenerationContext;
	};
};

namespace TeaForGodEmperor
{
	namespace MeshGeneration
	{
		namespace Shapes
		{
			void add_shapes();

			bool platforms_linear(::Framework::MeshGeneration::GenerationContext& _context, ::Framework::MeshGeneration::ElementInstance& _instance, ::Framework::MeshGeneration::ElementShapeData const* _data);
			::Framework::MeshGeneration::ElementShapeData* create_platforms_linear_data();
		};
	};
};