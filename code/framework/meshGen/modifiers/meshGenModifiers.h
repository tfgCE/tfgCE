#pragma once

#include "..\meshGenElementModifier.h"

namespace Framework
{
	namespace MeshGeneration
	{
		class ElementModifierData;

		namespace Modifiers
		{
			void add_modifiers();
			
			// extract bone placements and other
			bool extract(GenerationContext & _context, ElementInstance & _instigatorInstance, Element const * _subject, ::Framework::MeshGeneration::ElementModifierData const * _data);
			ElementModifierData* create_extract_data();

			// vertex alterations
			bool skew(GenerationContext & _context, ElementInstance & _instigatorInstance, Element const * _subject, ::Framework::MeshGeneration::ElementModifierData const * _data);
			ElementModifierData* create_skew_data();

			bool scale(GenerationContext & _context, ElementInstance & _instigatorInstance, Element const * _subject, ::Framework::MeshGeneration::ElementModifierData const * _data);
			ElementModifierData* create_scale_data();

			bool scale_per_vertex(GenerationContext & _context, ElementInstance & _instigatorInstance, Element const * _subject, ::Framework::MeshGeneration::ElementModifierData const * _data);
			ElementModifierData* create_scale_per_vertex_data();

			bool simplify_to_box(GenerationContext& _context, ElementInstance& _instigatorInstance, Element const* _subject, ::Framework::MeshGeneration::ElementModifierData const* _data);
			ElementModifierData* create_simplify_to_box_data();
			
			bool invert_normals(GenerationContext& _context, ElementInstance& _instigatorInstance, Element const* _subject, ::Framework::MeshGeneration::ElementModifierData const* _data);
			ElementModifierData* create_invert_normals_data();

			bool match_placement(GenerationContext& _context, ElementInstance& _instigatorInstance, Element const* _subject, ::Framework::MeshGeneration::ElementModifierData const* _data);
			ElementModifierData* create_match_placement_data();

			bool drop_mesh(GenerationContext& _context, ElementInstance& _instigatorInstance, Element const* _subject, ::Framework::MeshGeneration::ElementModifierData const* _data);
			ElementModifierData* create_drop_mesh_data();

			bool drop_space_blocker(GenerationContext& _context, ElementInstance& _instigatorInstance, Element const* _subject, ::Framework::MeshGeneration::ElementModifierData const* _data);
			ElementModifierData* create_drop_space_blocker_data();

			bool cut(GenerationContext& _context, ElementInstance& _instigatorInstance, Element const* _subject, ::Framework::MeshGeneration::ElementModifierData const* _data);
			ElementModifierData* create_cut_data();

			// splits only movement collision! and does so within collision meshes!
			bool split_movement_collision(GenerationContext& _context, ElementInstance& _instigatorInstance, Element const* _subject, ::Framework::MeshGeneration::ElementModifierData const* _data);
			ElementModifierData* create_split_movement_collision_data();

			// braeks movement collisions into convex meshes (not hulls!)! and does so within collision meshes!
			bool convexify_movement_collision(GenerationContext& _context, ElementInstance& _instigatorInstance, Element const* _subject, ::Framework::MeshGeneration::ElementModifierData const* _data);
			ElementModifierData* create_convexify_movement_collision_data();

			// checks if new space blockers are good against old ones, if not, removes stuff
			bool validate_space_blocker(GenerationContext& _context, ElementInstance& _instigatorInstance, Element const* _subject, ::Framework::MeshGeneration::ElementModifierData const* _data);
			ElementModifierData* create_validate_space_blocker_data();

			// checks if there is any mesh data
			bool any_mesh_data(GenerationContext& _context, ElementInstance& _instigatorInstance, Element const* _subject, ::Framework::MeshGeneration::ElementModifierData const* _data);
			ElementModifierData* create_any_mesh_data_data();

			// mirrors

			// just mirrors against a plane
			bool mirror(GenerationContext & _context, ElementInstance & _instigatorInstance, Element const * _subject, ::Framework::MeshGeneration::ElementModifierData const * _data);
			ElementModifierData* create_mirror_data();

			// creates mirror copy - both parts are clipped against a plane
			bool make_mirror(GenerationContext & _context, ElementInstance & _instigatorInstance, Element const * _subject, ::Framework::MeshGeneration::ElementModifierData const * _data);

			// sort of mirror, as cuts and pastes stuff together, modifies mesh only!
			bool kaleidoscope(GenerationContext & _context, ElementInstance & _instigatorInstance, Element const * _subject, ::Framework::MeshGeneration::ElementModifierData const * _data);
			ElementModifierData* create_kaleidoscope_data();

			// skinning

			bool skin_along(GenerationContext & _context, ElementInstance & _instigatorInstance, Element const * _subject, ::Framework::MeshGeneration::ElementModifierData const * _data);
			ElementModifierData* create_skin_along_data();

			bool skin_along_chain(GenerationContext & _context, ElementInstance & _instigatorInstance, Element const * _subject, ::Framework::MeshGeneration::ElementModifierData const * _data);
			ElementModifierData* create_skin_along_chain_data();

			bool skin_range(GenerationContext & _context, ElementInstance & _instigatorInstance, Element const * _subject, ::Framework::MeshGeneration::ElementModifierData const * _data);
			ElementModifierData* create_skin_range_data();

			// texturing

			bool set_v_along(GenerationContext & _context, ElementInstance & _instigatorInstance, Element const * _subject, ::Framework::MeshGeneration::ElementModifierData const * _data);
			ElementModifierData* create_set_v_along_data();

			// custom data

			bool set_custom_data(GenerationContext & _context, ElementInstance & _instigatorInstance, Element const * _subject, ::Framework::MeshGeneration::ElementModifierData const * _data);
			ElementModifierData* create_set_custom_data_data();
		};
	};
};
