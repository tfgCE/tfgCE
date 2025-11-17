#pragma once

#include <functional>

#include "..\fastCast.h"
#include "..\types\name.h"
#include "..\memory\refCountObjectPtr.h"

namespace Collision
{
	class PhysicalMaterial;

	typedef std::function< void(RefCountObjectPtr<PhysicalMaterial>& _physicalMaterial) > CreatePhysicalMaterial;

	struct LoadingContext
	{
		FAST_CAST_DECLARE(LoadingContext);
		FAST_CAST_END();
	public:
		LoadingContext();
		virtual ~LoadingContext();

		void load_material_from_attribute(Name const & _attributeName) { loadMaterialFromAttribute = _attributeName; }
		void load_material_from_child(Name const & _childName) { loadMaterialFromChild = _childName; }
		bool should_load_material_from_attribute(OUT_ Name & _attributeName) const { _attributeName = loadMaterialFromAttribute; return loadMaterialFromAttribute.is_valid(); }
		bool should_load_material_from_child(OUT_ Name & _childName) const { _childName = loadMaterialFromChild; return loadMaterialFromChild.is_valid(); }

		void set_create_physical_material(CreatePhysicalMaterial _cpm);
		inline void create_physical_material(RefCountObjectPtr<PhysicalMaterial>& _physicalMaterialPointer) const { an_assert(create_physical_material_func != nullptr); create_physical_material_func(_physicalMaterialPointer); }

	private:
		CreatePhysicalMaterial create_physical_material_func;
		Name loadMaterialFromAttribute;
		Name loadMaterialFromChild;
	};
	
};
