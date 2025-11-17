#pragma once

#include "..\meshGenElementShape.h"

namespace Framework
{
	namespace MeshGeneration
	{
		namespace AdvancedShapes
		{
			void add_shapes();

			/** params:
			 *		check data
			 */
			bool edges_and_surfaces(GenerationContext & _context, ElementInstance & _instance, ::Framework::MeshGeneration::ElementShapeData const * _data);
			ElementShapeData* create_edges_and_surfaces_data();

			/** params:
			 *		check data
			 */
			bool cross_section_cylindrical(GenerationContext & _context, ElementInstance & _instance, ::Framework::MeshGeneration::ElementShapeData const * _data);
			ElementShapeData* create_cross_section_cylindrical_data();

			/** params:
			 *		check data
			 */
			bool cross_sections(GenerationContext & _context, ElementInstance & _instance, ::Framework::MeshGeneration::ElementShapeData const * _data);
			ElementShapeData* create_cross_sections_data();

			/** params:
			 *		check data
			 */
			bool cross_section_along(GenerationContext & _context, ElementInstance & _instance, ::Framework::MeshGeneration::ElementShapeData const * _data);
			ElementShapeData* create_cross_section_along_data();

			/** params:
			 *		check data
			 */
			bool flat_display(GenerationContext & _context, ElementInstance & _instance, ::Framework::MeshGeneration::ElementShapeData const * _data);
			ElementShapeData* create_flat_display_data();

			/** params:
			 *		check data
			 */
			bool lightning(GenerationContext & _context, ElementInstance & _instance, ::Framework::MeshGeneration::ElementShapeData const * _data);
			ElementShapeData* create_lightning_data();

			/** params:
			 *		check data
			 */
			bool desert(GenerationContext & _context, ElementInstance & _instance, ::Framework::MeshGeneration::ElementShapeData const * _data);
			ElementShapeData* create_desert_data();

			/** params:
			 *		check data
			 */
			bool door_hole_complement(GenerationContext & _context, ElementInstance & _instance, ::Framework::MeshGeneration::ElementShapeData const * _data);
			ElementShapeData* create_door_hole_complement_data();

		};
	};
};
