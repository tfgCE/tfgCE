#pragma once

#include "..\meshGenElementShape.h"

namespace Framework
{
	namespace MeshGeneration
	{
		namespace BasicShapes
		{
			void add_shapes();
			ElementShapeData* create_data();

			/** params:
			 */
			bool bounding_box(GenerationContext& _context, ElementInstance& _instance, ::Framework::MeshGeneration::ElementShapeData const* _data);

			/** params:
			 *		radius/size			float		radius
			 *		halfDir				vec3		half dir of a,b separation (half distance between points!)
			 *		subStepCount		int			number of edges (when dividing 90'), if 0 or negative, will be auto calculated using global values
			 */
			bool capsule(GenerationContext & _context, ElementInstance & _instance, ::Framework::MeshGeneration::ElementShapeData const * _data);
			ElementShapeData* create_capsule_data();

			/** params:
			 *		size				float		size of cube
			 *		size				Vector3		size of each edge (yes, handles cuboids too)
			 */
			bool cube(GenerationContext & _context, ElementInstance & _instance, ::Framework::MeshGeneration::ElementShapeData const * _data);

			/** params:
			 *		radius/size			float		radius
			 *		subStepCount		int			number of edges (when dividing 90'), if 0 or negative, will be auto calculated using global values
			 */
			bool sphere(GenerationContext & _context, ElementInstance & _instance, ::Framework::MeshGeneration::ElementShapeData const * _data);
			ElementShapeData* create_sphere_data();

			/** params:
			 *		radius				float		radius
			 *		height/length		float		height/length
			 *		subStepCount		int			number of edges (when dividing 90'), if 0 or negative, will be auto calculated using global values
			 */
			bool cylinder(GenerationContext & _context, ElementInstance & _instance, ::Framework::MeshGeneration::ElementShapeData const * _data);

			/** params:
			 *		vertexCount			int			vertex count
			 *		withinBox			Vector3		has to fit within box
			 *		withinSphere		float		has to fit within sphere
			 */
			bool convex_hull(GenerationContext & _context, ElementInstance & _instance, ::Framework::MeshGeneration::ElementShapeData const * _data);
			ElementShapeData* create_convex_hull_data();
		};
	};
};
