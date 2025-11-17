#pragma once

#include "roomType.h"
#include "environmentParams.h"

namespace Framework
{
	class Material;

	struct EnvironmentInfo
	{
	public:
		EnvironmentInfo();

		void clear();

		void copy_from(EnvironmentInfo const & _source, Array<Name> const * _skipParams = nullptr);
		void fill_params_with(EnvironmentInfo const & _source, Array<Name> const * _skipParams = nullptr);
		void copy_params_from(EnvironmentInfo const & _source, Array<Name> const * _skipParams = nullptr);
		void set_params_from(EnvironmentInfo const & _source, Array<Name> const * _skipParams = nullptr);
		void lerp_params_with(float _amount, EnvironmentInfo const & _info, Array<Name> const * _skipParams = nullptr);

		void set_parent(EnvironmentInfo* _parent) { parent = _parent; }

	public: // accessors, they will try to get any value, if not set here, it will be from parent or finally from default
		bool should_allow_generation_as_room() const { return generateAsRoom; }
		bool is_location_fixed_against_camera() const { return locationFixedAgainstCamera.is_set() ? locationFixedAgainstCamera.get() : (parent ? parent->is_location_fixed_against_camera() : false); }
		bool does_propagate_placement_between_rooms() const { return propagatesPlacementBetweenRooms.is_set() ? propagatesPlacementBetweenRooms.get() : (parent ? parent->does_propagate_placement_between_rooms() : true); }
		Transform const & get_default_in_room_placement() const { return defaultInRoomPlacement.is_set() ? defaultInRoomPlacement.get() : (parent ? parent->get_default_in_room_placement() : Transform::identity); }

		EnvironmentParams const & get_params() const { return params; }

		// access
		EnvironmentParams & access_params() { return params; }

		Material* get_sky_box_material() const { return skyBoxMaterial.get(); }
		Material* get_clouds_material() const { return cloudsMaterial.get(); }
		Optional<float> const& get_sky_box_radius() const { return skyBoxRadius; }
		Optional<Vector3> const& get_sky_box_centre() const { return skyBoxCentre; }

	public: // loading, preparing
		bool load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc);
		bool prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext);

	private:
		EnvironmentInfo* parent;

		bool generateAsRoom; // always should be set

		Optional<bool> propagatesPlacementBetweenRooms;
		Optional<bool> locationFixedAgainstCamera;
		Optional<Transform> defaultInRoomPlacement;

		UsedLibraryStored<Material> skyBoxMaterial;
		UsedLibraryStored<Material> cloudsMaterial;
		Optional<float> skyBoxRadius;
		Optional<Vector3> skyBoxCentre;

		EnvironmentParams params;
		struct CopyParams
		{
			UsedLibraryStored<EnvironmentType> from;
			bool overrideExisting = false;
		};
		Array<CopyParams> copyParams; // please do not chain them together, sources should be only sources

	};
};
