#pragma once

#include "..\overlayInfoElement.h"
#include "..\..\..\framework\library\usedLibraryStored.h"

namespace Meshes
{
	class Mesh3DInstance;
}

namespace TeaForGodEmperor
{
	namespace OverlayInfo
	{
		namespace Elements
		{
			struct Mesh
			: public Element
			{
				FAST_CAST_DECLARE(Mesh);
				FAST_CAST_BASE(Element);
				FAST_CAST_END();

				typedef Element base;
			public:
				Mesh() {}
				Mesh(Meshes::Mesh3DInstance const & _meshInstance): meshInstance(&_meshInstance) {}

				Mesh* with_scale(float _scale) { scale = _scale; return this; }
				Mesh* with_use_additional_scale(Optional<float> const& _scale) { useAdditionalScale = _scale; return this; }

				Mesh* in_world() { placementType = InWorld; return this; }
				Mesh* face_camera() { placementType = FaceCamera; return this; }
				
				Mesh* with_active_shader_param(Name const & _n) { activeShaderParamName = _n; return this; }
				Mesh* with_max_dist_blend(float _maxDistBlend) { maxDistBlend = _maxDistBlend; return this; }
				
				Mesh* with_front_axis(Optional<Vector3> const & _axis) { frontAxis = _axis; return this; }

			public:
				implement_ void request_render_layers(OUT_ ArrayStatic<int, 8>& _renderLayers) const;
				implement_ void render(System const* _system, int _renderLayer) const;

			protected:
				Meshes::Mesh3DInstance const* meshInstance;

				float scale = 1.0f;
				Optional<float> useAdditionalScale;
				Optional<float> maxDistBlend; // if not set, not blended
				
				Optional<Vector3> frontAxis;

				Name activeShaderParamName;

				enum PlacementType
				{
					InWorld,
					FaceCamera
				};
				PlacementType placementType = InWorld;
			};
		}
	};
};
