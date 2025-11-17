#include "model.h"
#include "..\io\xml.h"

#include "checkSegmentResult.h"
#include "checkCollisionContext.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace Collision;

//

Model::Model()
{
}

Model::~Model()
{
	clear();
}

Model* Model::create_copy() const
{
	Model* model = new Model();

	*model = *this;
	model->clear_cache();

	return model;
}

void Model::clear()
{
	boxes.clear();
	hulls.clear();
	spheres.clear();
	capsules.clear();
	meshes.clear();
	skinnedMeshes.clear();

	collisionInfoProvider.clear();
	material = nullptr;
}

void Model::fuse(Model const * const * _models, int _count)
{
	while (_count)
	{
		boxes.add_from((*_models)->boxes);
		hulls.add_from((*_models)->hulls);
		spheres.add_from((*_models)->spheres);
		capsules.add_from((*_models)->capsules);
		meshes.add_from((*_models)->meshes);
		skinnedMeshes.add_from((*_models)->skinnedMeshes);
		++_models;
		--_count;
	}
	// keep main material and collision info provider
	clear_cache();
}

void Model::fuse(Array<Model const *> const & _models)
{
	fuse(_models.get_data(), _models.get_size());
}

void Model::fuse(Array<Model *> const & _models)
{
	fuse(_models.get_data(), _models.get_size());
}

#define ADD_TRANSFORMED(_array) \
	_array.make_space_for_additional(_model->_array.get_size()); \
	for_every(e, _model->_array) \
	{ \
		auto ce = *e; \
		ce.apply_transform(transform); \
		_array.push_back(ce); \
	}

void Model::fuse(Model const* _model, Optional<Transform> const& _placement)
{
	if (!_model)
	{
		return;
	}
	if (_placement.is_set())
	{
		Matrix44 transform = _placement.get().to_matrix();
		ADD_TRANSFORMED(boxes);
		ADD_TRANSFORMED(hulls);
		ADD_TRANSFORMED(spheres);
		ADD_TRANSFORMED(capsules);
		ADD_TRANSFORMED(meshes);
		ADD_TRANSFORMED(skinnedMeshes);
	}
	else
	{
		boxes.add_from(_model->boxes);
		hulls.add_from(_model->hulls);
		spheres.add_from(_model->spheres);
		capsules.add_from(_model->capsules);
		meshes.add_from(_model->meshes);
		skinnedMeshes.add_from(_model->skinnedMeshes);
	}
	clear_cache();
}

void Model::fuse(Array<FuseModel> const& _models)
{
	for_every(fuseModel, _models)
	{
		fuse(fuseModel->model, fuseModel->placement);
	}
}

void Model::reverse_triangles()
{
	for_every(mesh, meshes)
	{
		mesh->reverse_triangles();
	}
	for_every(skinnedMesh, skinnedMeshes)
	{
		skinnedMesh->reverse_triangles();
	}
}

void Model::apply_transform(Matrix44 const & _transform)
{
	for_every(box, boxes)
	{
		box->apply_transform(_transform);
	}
	for_every(hull, hulls)
	{
		hull->apply_transform(_transform);
	}
	for_every(sphere, spheres)
	{
		sphere->apply_transform(_transform);
	}
	for_every(capsule, capsules)
	{
		capsule->apply_transform(_transform);
	}
	for_every(mesh, meshes)
	{
		mesh->apply_transform(_transform);
	}
	for_every(skinnedMesh, skinnedMeshes)
	{
		skinnedMesh->apply_transform(_transform);
	}
}

void Model::skin_to(Name const & _boneName, bool _onlyIfNotSkinned)
{
	for_every(box, boxes)
	{
		if (!box->get_bone().is_valid() || !_onlyIfNotSkinned)
		{
			box->set_bone(_boneName);
		}
	}
	for_every(hull, hulls)
	{
		if (!hull->get_bone().is_valid() || !_onlyIfNotSkinned)
		{
			hull->set_bone(_boneName);
		}
	}
	for_every(sphere, spheres)
	{
		if (!sphere->get_bone().is_valid() || !_onlyIfNotSkinned)
		{
			sphere->set_bone(_boneName);
		}
	}
	for_every(capsule, capsules)
	{
		if (!capsule->get_bone().is_valid() || !_onlyIfNotSkinned)
		{
			capsule->set_bone(_boneName);
		}
	}
	for_every(mesh, meshes)
	{
		if (!mesh->get_bone().is_valid() || !_onlyIfNotSkinned)
		{
			mesh->set_bone(_boneName);
		}
	}
	for_every(skinnedMesh, skinnedMeshes)
	{
		if (!skinnedMesh->get_bone().is_valid() || !_onlyIfNotSkinned)
		{
			skinnedMesh->set_bone(_boneName);
		}
	}
}

void Model::reskin(Meshes::BoneRenameFunc _rename)
{
	for_every(box, boxes)
	{
		if (box->get_bone().is_valid())
		{
			box->set_bone(_rename(box->get_bone(), box->get_centre()));
		}
	}
	for_every(hull, hulls)
	{
		if (hull->get_bone().is_valid())
		{
			hull->set_bone(_rename(hull->get_bone(), hull->get_centre()));
		}
	}
	for_every(sphere, spheres)
	{
		if (sphere->get_bone().is_valid())
		{
			sphere->set_bone(_rename(sphere->get_bone(), sphere->get_centre()));
		}
	}
	for_every(capsule, capsules)
	{
		if (capsule->get_bone().is_valid())
		{
			capsule->set_bone(_rename(capsule->get_bone(), capsule->get_centre()));
		}
	}
	for_every(mesh, meshes)
	{
		if (mesh->get_bone().is_valid())
		{
			mesh->set_bone(_rename(mesh->get_bone(), mesh->get_centre()));
		}
	}
	for_every(skinnedMesh, skinnedMeshes)
	{
		if (skinnedMesh->get_bone().is_valid())
		{
			skinnedMesh->set_bone(_rename(skinnedMesh->get_bone(), skinnedMesh->get_centre()));
		}
	}	
}

template<typename Shape>
bool Model::try_loading_from_xml(IO::XML::Node const * _node, LoadingContext const & _context, Array<Shape>& _shapes, tchar const * _nodeName)
{
	bool result = true;
	int childrenCount = 0;
	for_every(child, _node->children_named(_nodeName))
	{
		++childrenCount;
	}
	if (! childrenCount)
	{
		// nothing to read
		return result;
	}
	// make enough space so we won't reallocate memory - to keep it in same place
	_shapes.make_space_for_additional(childrenCount);
	// load!
	for_every(child, _node->children_named(_nodeName))
	{
		_shapes.push_back(Shape());
		if (_shapes.get_last().load_from_xml(child, _context))
		{
			// all good
			continue;
		}
		else
		{
			_shapes.remove_fast_at(_shapes.get_size() - 1);
			result = false;
		}
	}
	return result;
}

bool Model::load_from_xml(IO::XML::Node const * _node, LoadingContext const & _context)
{
	bool result = true;
	result &= try_loading_from_xml(_node, _context, boxes, TXT("box"));
	result &= try_loading_from_xml(_node, _context, hulls, TXT("convexhull"));
	result &= try_loading_from_xml(_node, _context, hulls, TXT("hull"));
	result &= try_loading_from_xml(_node, _context, spheres, TXT("sphere"));
	result &= try_loading_from_xml(_node, _context, capsules, TXT("capsule"));
	result &= try_loading_from_xml(_node, _context, meshes, TXT("mesh"));
	result &= try_loading_from_xml(_node, _context, skinnedMeshes, TXT("skinnedMesh"));
	result &= collisionInfoProvider.load_from_xml(_node);
	result &= PhysicalMaterial::load_material_from_xml(material, _node, _context);
	return result;
}

void Model::debug_draw(bool _frontBorder, bool _frontFill, Colour const & _colour, float _alphaFill, Meshes::PoseMatBonesRef const & _poseMatBonesRef) const
{
	for_every(box, boxes)
	{
		box->debug_draw(_frontBorder, _frontFill, _colour, _alphaFill, _poseMatBonesRef);
	}
	for_every(hull, hulls)
	{
		hull->debug_draw(_frontBorder, _frontFill, _colour, _alphaFill, _poseMatBonesRef);
	}
	for_every(sphere, spheres)
	{
		sphere->debug_draw(_frontBorder, _frontFill, _colour, _alphaFill, _poseMatBonesRef);
	}
	for_every(capsule, capsules)
	{
		capsule->debug_draw(_frontBorder, _frontFill, _colour, _alphaFill, _poseMatBonesRef);
	}
	for_every(mesh, meshes)
	{
		mesh->debug_draw(_frontBorder, _frontFill, _colour, _alphaFill, _poseMatBonesRef);
	}
	for_every(skinnedMesh, skinnedMeshes)
	{
		skinnedMesh->debug_draw(_frontBorder, _frontFill, _colour, _alphaFill, _poseMatBonesRef);
	}	
}

void Model::log(LogInfoContext & _context) const
{
	if (!boxes.is_empty())
	{
		_context.log(TXT("boxes:"));
		LOG_INDENT(_context);
		for_every(box, boxes)
		{
			box->log(_context);
		}
	}
	if (!hulls.is_empty())
	{
		_context.log(TXT("hulls:"));
		LOG_INDENT(_context);
		for_every(hull, hulls)
		{
			hull->log(_context);
		}
	}
	if (!spheres.is_empty())
	{
		_context.log(TXT("spheres:"));
		LOG_INDENT(_context);
		for_every(sphere, spheres)
		{
			sphere->log(_context);
		}
	}
	if (!capsules.is_empty())
	{
		_context.log(TXT("capsules:"));
		LOG_INDENT(_context);
		for_every(capsule, capsules)
		{
			capsule->log(_context);
		}
	}
	if (!meshes.is_empty())
	{
		_context.log(TXT("meshes:"));
		LOG_INDENT(_context);
		for_every(mesh, meshes)
		{
			mesh->log(_context);
		}
	}
	if (!skinnedMeshes.is_empty())
	{
		_context.log(TXT("skinnedMeshes:"));
		LOG_INDENT(_context);
		for_every(skinnedMesh, skinnedMeshes)
		{
			skinnedMesh->log(_context);
		}
	}	
}

bool Model::check_segment(REF_ Segment & _segment, REF_ Collision::CheckSegmentResult & _result, Collision::CheckCollisionContext & _context, float _increaseSize) const
{
	bool ret = false;
	Collision::CheckSegmentResult tempResult = _result;
	for_every(box, boxes)
	{
		ret |= box->check_segment(REF_ _segment, REF_ tempResult, _context, _increaseSize);
	}
	for_every(hull, hulls)
	{
		ret |= hull->check_segment(REF_ _segment, REF_ tempResult, _context, _increaseSize);
	}
	for_every(sphere, spheres)
	{
		ret |= sphere->check_segment(REF_ _segment, REF_ tempResult, _context, _increaseSize);
	}
	for_every(capsule, capsules)
	{
		ret |= capsule->check_segment(REF_ _segment, REF_ tempResult, _context, _increaseSize);
	}
	for_every(mesh, meshes)
	{
		ret |= mesh->check_segment(REF_ _segment, REF_ tempResult, _context, _increaseSize);
	}
	for_every(skinnedMesh, skinnedMeshes)
	{
		ret |= skinnedMesh->check_segment(REF_ _segment, REF_ tempResult, _context, _increaseSize);
	}	
	if (ret)
	{
		if (!tempResult.material &&
			material.get() &&
			!Flags::check(_context.get_collision_flags(), material->get_collision_flags()))
		{
			ret = false;
		}
	}
	if (ret)
	{
		_result = tempResult;
#ifdef AN_DEVELOPMENT
		_result.model = this;
#endif
		if (!_result.material)
		{
			_result.material = material.get();
		}
		if (_context.is_collision_info_needed())
		{
			// apply gravity to fill any missing ones
			collisionInfoProvider.apply_to(REF_ _result);
			if (material.get())
			{
				material->apply_to(REF_ _result);
			}
		}
	}
	return ret;
}

void Model::refresh_materials()
{
	for_every(box, boxes)
	{
		box->refresh_material();
	}
	for_every(hull, hulls)
	{
		hull->refresh_material();
	}
	for_every(sphere, spheres)
	{
		sphere->refresh_material();
	}
	for_every(capsule, capsules)
	{
		capsule->refresh_material();
	}
	for_every(mesh, meshes)
	{
		mesh->refresh_material();
	}
	for_every(skinnedMesh, skinnedMeshes)
	{
		skinnedMesh->refresh_material();
	}	
}

void Model::cache_for(Meshes::Skeleton const * _skeleton)
{
	for_every(box, boxes)
	{
		box->cache_for(_skeleton);
	}
	for_every(hull, hulls)
	{
		hull->cache_for(_skeleton);
	}
	for_every(sphere, spheres)
	{
		sphere->cache_for(_skeleton);
	}
	for_every(capsule, capsules)
	{
		capsule->cache_for(_skeleton);
	}
	for_every(mesh, meshes)
	{
		mesh->cache_for(_skeleton);
	}
	for_every(skinnedMesh, skinnedMeshes)
	{
		skinnedMesh->cache_for(_skeleton);
	}	
}

void Model::clear_cache()
{
	for_every(box, boxes)
	{
		box->clear_cache();
	}
	for_every(hull, hulls)
	{
		hull->clear_cache();
	}
	for_every(sphere, spheres)
	{
		sphere->clear_cache();
	}
	for_every(capsule, capsules)
	{
		capsule->clear_cache();
	}
	for_every(mesh, meshes)
	{
		mesh->clear_cache();
	}
	for_every(skinnedMesh, skinnedMeshes)
	{
		skinnedMesh->clear_cache();
	}
}

Range3 Model::calculate_bounding_box(Transform const & _usingPlacement, Meshes::Pose const * _poseMS, bool _quick) const
{
	Range3 boundingBox = Range3::empty;
	for_every(box, boxes)
	{
		boundingBox.include(box->calculate_bounding_box(_usingPlacement, _poseMS, _quick));
	}
	for_every(hull, hulls)
	{
		boundingBox.include(hull->calculate_bounding_box(_usingPlacement, _poseMS, _quick));
	}
	for_every(sphere, spheres)
	{
		boundingBox.include(sphere->calculate_bounding_box(_usingPlacement, _poseMS, _quick));
	}
	for_every(capsule, capsules)
	{
		boundingBox.include(capsule->calculate_bounding_box(_usingPlacement, _poseMS, _quick));
	}
	for_every(mesh, meshes)
	{
		boundingBox.include(mesh->calculate_bounding_box(_usingPlacement, _poseMS, _quick));
	}
	for_every(skinnedMesh, skinnedMeshes)
	{
		boundingBox.include(skinnedMesh->calculate_bounding_box(_usingPlacement, _poseMS, _quick));
	}	
	return boundingBox;
}

void Model::break_meshes_into_convex_hulls()
{
	Array<Mesh> newMeshes;
	for_every(mesh, meshes)
	{
		mesh->break_into_convex_hulls(hulls, newMeshes);
	}
	meshes.clear();
	meshes.add_from(newMeshes);
}

void Model::break_meshes_into_convex_meshes()
{
	Array<Mesh> newMeshes;
	newMeshes.make_space_for(1000); // it could get really big
	for (int i = 0; i < meshes.get_size(); ++i)
	{
		if (meshes[i].break_into_convex_meshes(newMeshes))
		{
			meshes.remove_fast_at(i);
			--i;
		}
	}
	meshes.add_from(newMeshes);
}

void Model::remove_at_or_behind(Plane const & _plane, float threshold)
{
	for (int i = 0; i < boxes.get_size(); ++i)
	{
		if (_plane.get_in_front(boxes[i].get_centre()) <= threshold)
		{
			boxes.remove_at(i);
			--i;
		}
	}
	for (int i = 0; i < hulls.get_size(); ++i)
	{
		if (_plane.get_in_front(hulls[i].get_centre()) <= threshold)
		{
			hulls.remove_at(i);
			--i;
		}
	}
	for (int i = 0; i < spheres.get_size(); ++i)
	{
		if (_plane.get_in_front(spheres[i].get_centre()) <= threshold)
		{
			spheres.remove_at(i);
			--i;
		}
	}
	for (int i = 0; i < capsules.get_size(); ++i)
	{
		if (_plane.get_in_front(capsules[i].get_centre()) <= threshold)
		{
			capsules.remove_at(i);
			--i;
		}
	}
	for (int i = 0; i < meshes.get_size(); ++i)
	{
		if (_plane.get_in_front(meshes[i].get_centre()) <= threshold)
		{
			meshes.remove_at(i);
			--i;
		}
	}
	for (int i = 0; i < skinnedMeshes.get_size(); ++i)
	{
		if (_plane.get_in_front(skinnedMeshes[i].get_centre()) <= threshold)
		{
			skinnedMeshes.remove_at(i);
			--i;
		}
	}
}

void Model::split_meshes_at(Plane const& _plane, Optional<Range3> const& _inRange, float _threshold)
{
	{
		meshes.make_space_for(meshes.get_size() * 2);
		int orgSize = meshes.get_size();
		for (int i = 0; i < orgSize; ++i)
		{
			auto& m = meshes[i];
			if (!_inRange.is_set() ||
				_inRange.get().overlaps(m.get_range()))
			{
				meshes.grow_size(1);
				auto& nm = meshes.get_last();
				nm = m;
				nm.clear();
				m.split(_plane, nm, _threshold);
				if (nm.is_empty())
				{
					meshes.pop_back();
				}
			}
		}
		for (int i = 0; i < meshes.get_size(); ++i)
		{
			if (meshes[i].is_empty())
			{
				meshes.remove_fast_at(i);
				--i;
			}
		}
	}
	{
		skinnedMeshes.make_space_for(skinnedMeshes.get_size() * 2);
		int orgSize = skinnedMeshes.get_size();
		for (int i = 0; i < orgSize; ++i)
		{
			auto& m = skinnedMeshes[i];
			if (!_inRange.is_set() ||
				_inRange.get().overlaps(m.get_range()))
			{
				skinnedMeshes.grow_size(1);
				auto& nm = skinnedMeshes.get_last();
				nm = m;
				nm.clear();
				m.split(_plane, nm, _threshold);
				if (nm.is_empty())
				{
					skinnedMeshes.pop_back();
				}
			}
		}
		for (int i = 0; i < skinnedMeshes.get_size(); ++i)
		{
			if (skinnedMeshes[i].is_empty())
			{
				skinnedMeshes.remove_fast_at(i);
				--i;
			}
		}
	}
}

void Model::convexify_static_meshes()
{
	{
		meshes.make_space_for(meshes.get_size() * 2);
		int orgSize = meshes.get_size();
		for (int i = 0; i < orgSize; ++i)
		{
			auto& m = meshes[i];
			{
				Array<Mesh> newMeshes;
				newMeshes.make_space_for(50);
				m.convexify([&m, &newMeshes](int _meshIdx)
					{
						while (_meshIdx >= newMeshes.get_size())
						{
							newMeshes.grow_size(1);
							auto& nm = newMeshes.get_last();
							nm = m;
							nm.clear();
						}
						return &newMeshes[_meshIdx];
					});
				m.clear(); // we wille be using new meshes now
				meshes.add_from(newMeshes);
			}
		}
		for (int i = 0; i < meshes.get_size(); ++i)
		{
			if (meshes[i].is_empty())
			{
				meshes.remove_fast_at(i);
				--i;
			}
		}
	}
}

void Model::for_every_primitive(std::function<void(ICollidableShape*)> _do)
{
	for_every(box, boxes)
	{
		_do(box);
	}
	for_every(hull, hulls)
	{
		_do(hull);
	}
	for_every(sphere, spheres)
	{
		_do(sphere);
	}
	for_every(capsule, capsules)
	{
		_do(capsule);
	}
	for_every(mesh, meshes)
	{
		_do(mesh);
	}
	for_every(skinnedMesh, skinnedMeshes)
	{
		_do(skinnedMesh);
	}
}

bool Model::get_closest_shape(Collision::CheckCollisionContext& _context, Vector3 const& _loc, REF_ Optional<float>& _closestDist, REF_ ICollidableShape const*& _closestShape) const
{
	bool updated = false;
	for_every(box, boxes)
	{
		float dist = max(0.0f, box->calculate_outside_distance_local(box->location_to_local(box->from_external_to_model(_loc, _context))));
		if (!_closestDist.is_set() || dist < _closestDist.get())
		{
			_closestDist = dist;
			_closestShape = box;
			updated = true;
		}
	}
	for_every(hull, hulls)
	{
		todo_important(TXT("not implemented for hulls"));
	}
	for_every(sphere, spheres)
	{
		float dist = sphere->calculate_outside_distance(sphere->from_external_to_model(_loc, _context));
		if (!_closestDist.is_set() || dist < _closestDist.get())
		{
			_closestDist = dist;
			_closestShape = sphere;
			updated = true;
		}
	}
	for_every(capsule, capsules)
	{
		float dist = capsule->calculate_outside_distance(capsule->from_external_to_model(_loc, _context));
		if (!_closestDist.is_set() || dist < _closestDist.get())
		{
			_closestDist = dist;
			_closestShape = capsule;
			updated = true;
		}
	}
	for_every(mesh, meshes)
	{
		todo_future(TXT("we're using simplified approach, as using full mesh would be much heavier"));
		float dist = mesh->calculate_outside_box_distance(mesh->from_external_to_model(_loc, _context));
		if (!_closestDist.is_set() || dist < _closestDist.get())
		{
			_closestDist = dist;
			_closestShape = mesh;
			updated = true;
		}
	}
	for_every(skinnedMesh, skinnedMeshes)
	{
		todo_important(TXT("not implemented for skinned meshes"));
	}
	return updated;
}
