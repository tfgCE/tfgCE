#pragma once

#include "..\globalDefinitions.h"
#include "..\fastCast.h"
#include "mesh3dRenderingContext.h"
#include "..\system\video\videoEnums.h"

namespace System
{
	interface_class IMaterialInstanceProvider;
	class Video3D;
};

namespace Meshes
{
	interface_class IMesh3DRenderBonesProvider;

	struct Mesh3DRenderingOrder
	{
		int priority = 0; // used to tell which mesh to render first, high order, the higher the order, the later it is rendered
		typedef
#ifdef AN_64
			long long
#else
			int
#endif
			pointerAsNumber;
		pointerAsNumber meshPtr = 0; // used to group same meshes, low order. this is pointer to the mesh cast to int
	};

	interface_class IMesh3D
	{
		FAST_CAST_DECLARE(IMesh3D);
		FAST_CAST_END();
	public:
		IMesh3D();
		virtual ~IMesh3D();

		void set_rendering_order_priority(int _priority) { renderingOrder.priority = _priority; }
		Mesh3DRenderingOrder const & get_rendering_order() const { return renderingOrder; }
		virtual void render(::System::Video3D * _v3d, ::System::IMaterialInstanceProvider const * _materialProvider = nullptr, Mesh3DRenderingContext const & _renderingContext = Mesh3DRenderingContext::none()) const = 0;
		virtual ::System::MaterialShaderUsage::Type get_material_usage() const = 0;
		
		virtual int calculate_triangles() const = 0;

		virtual void log(LogInfoContext& _log) const = 0;

	private:
		Mesh3DRenderingOrder renderingOrder;
	};

};
