#pragma once

#include "mesh3dInstance.h"
#include "mesh3dRenderingBuffer.h"

namespace Meshes
{
	class Mesh3DInstanceSet
	{
	public:
		~Mesh3DInstanceSet();

		void clear();
		Mesh3DInstance* add(IMesh3D const * _mesh, Transform const & _placement = Transform::identity);
		Mesh3DInstance* add(IMesh3D const * _mesh, Skeleton const * _skeleton, Transform const & _placement = Transform::identity);
		void remove(Mesh3DInstance* _instance);

		void render(::System::Video3D* _v3d, Mesh3DRenderingContext const & _renderingContext) const;

		void set_placement(int _index, Transform const & _placement);

		bool is_empty() const { return instances.is_empty(); }
		int get_instances_num() const { return instances.get_size(); }
		Array<Mesh3DInstance*> const & get_instances() const { return instances; }
		Array<Mesh3DInstance*> & access_instances() { return instances; }
		Mesh3DInstance* access_instance(int _index) { return instances.is_index_valid(_index) ? instances[_index] : nullptr; }
		Mesh3DInstance const* get_instance(int _index) const { return instances.is_index_valid(_index) ? instances[_index] : nullptr; }

	private:
		Array<Mesh3DInstance*> instances;

		Mesh3DInstanceSet const & operator = (Mesh3DInstanceSet const & _other); // do not implement_

		friend class Mesh3DInstanceSetInlined;
	};

};
