#pragma once

#include "..\fastCast.h"
#include "..\memory\refCountObject.h"
#include "..\cachedPointer.h"
#include "collisionInfoProvider.h"
#include "collisionFlags.h"

namespace IO
{
	namespace XML
	{
		class Node;
		class Attribute;
	};
};

namespace Collision
{
	struct LoadingContext;
	struct CheckSegmentResult;

	class PhysicalMaterial
	: public virtual RefCountObject
	{
		FAST_CAST_DECLARE(PhysicalMaterial);
		FAST_CAST_END();
	public:
		PhysicalMaterial();

		virtual bool load_from_xml(IO::XML::Node const * _node, LoadingContext const & _loadingContext);
		virtual bool load_from_xml(IO::XML::Attribute const * _attr, LoadingContext const & _loadingContext);

		static bool load_material_from_xml(RefCountObjectPtr<PhysicalMaterial>& material, IO::XML::Node const * _node, LoadingContext const & _loadingContext);

		void apply_to(REF_ CheckSegmentResult & _result) const; // doesn't clear if wasn't set

		inline Flags const & get_collision_flags() const { return collisionFlags; }

		virtual void log_usage_info(LogInfoContext & _context) const;

	public:
		template <typename Class>
		void be() { pointerToSelf.setup<Class>(this); }

		template <typename Class>
		Class * get_as() { return pointerToSelf.get_as<Class>(); }

		template <typename Class>
		Class const * get_as() const { return pointerToSelf.get_as<Class>(); }

	protected:
		virtual ~PhysicalMaterial();

	private:
		CachedPointer pointerToSelf; // to different class

		CollisionInfoProvider collisionInfoProvider;

		Flags collisionFlags; // actual flags
	};
};
