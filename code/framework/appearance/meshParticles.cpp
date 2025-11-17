#include "meshParticles.h"

#include "..\..\core\random\randomUtils.h"
#include "..\..\core\serialisation\importerHelper.h"
#include "..\..\core\system\core.h"
#include "..\..\core\system\video\indexFormatUtils.h"
#include "..\..\core\system\video\vertexFormatUtils.h"
#include "..\..\core\utils\utils_loadingForSystemShader.h"
#include "..\..\core\wheresMyPoint\simpleWMPOwner.h"
#include "..\..\core\wheresMyPoint\wmp_context.h"

#include "..\library\library.h"
#include "..\library\usedLibraryStored.inl"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace Framework;

//

bool MeshParticles::Element::load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc)
{
	bool result = true;

	result &= count.load_from_xml(_node, TXT("count"));
	result &= scale.load_from_xml(_node, TXT("scale"));
	boneParticleType = _node->get_name_attribute(TXT("boneParticleType"), boneParticleType);

	options.clear();

	{
		int count = 0;
		for_every(node, _node->children_named(TXT("mesh")))
		{
			++count;
		}
		options.set_size(count);

		int idx = 0;
		for_every(node, _node->children_named(TXT("mesh")))
		{
			Option & option = options[idx];
			if (option.load_from_xml(node, _lc))
			{
				// ok
			}
			else
			{
				error_loading_xml(node, TXT("could not load option"));
				result = false;
			}
			++idx;
		}
	}

	// to allow <add mesh="mesh" count="2" scale="1.0"/>
	if (options.is_empty())
	{
		options.grow_size(1);
		Option & option = options.get_first();
		if (option.load_from_xml(_node, _lc))
		{
			// ok
		}
		else
		{
			error_loading_xml(_node, TXT("could not load option"));
			result = false;
		}
	}

	for_every(forNode, _node->children_named(TXT("for")))
	{
		if (CoreUtils::Loading::should_load_for_system_or_shader_option(forNode))
		{
			result &= count.load_from_xml(forNode, TXT("count"));
		}
	}

	return result;
}

bool MeshParticles::Element::prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext)
{
	bool result = true;

	for_every(option, options)
	{
		result &= option->prepare_for_game(_library, _pfgContext);
	}

	return result;
}

//--

bool MeshParticles::Element::Option::load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc)
{
	bool result = true;

	probabilityCoef = _node->get_float_attribute(TXT("probCoef"), probabilityCoef);
	probabilityCoef = _node->get_float_attribute(TXT("probabilityCoef"), probabilityCoef);
	result &= particleMesh.load_from_xml(_node, TXT("mesh"), _lc);

	if (!particleMesh.is_name_valid())
	{
		error_loading_xml(_node, TXT("no particle mesh provided"));
		result = false;
	}

	result &= instanceCustomDataWMP.load_from_xml(_node->first_child_named(TXT("instanceCustomDataWMP")));

	return result;
}

bool MeshParticles::Element::Option::prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext)
{
	bool result = true;

	result &= particleMesh.prepare_for_game_find(_library, _pfgContext, LibraryPrepareLevel::Resolve);

	return result;
}

//--

REGISTER_FOR_FAST_CAST(MeshParticles);
LIBRARY_STORED_DEFINE_TYPE(MeshParticles, meshParticles);

MeshParticles::MeshParticles(Library * _library, LibraryName const & _name)
: base(_library, _name)
{
	meshUsage = Meshes::Usage::SkinnedToSingleBone;
}

MeshParticles::~MeshParticles()
{
}

bool MeshParticles::load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc)
{
	bool result = base::load_from_xml(_node, _lc);

	forceSingleMaterial = _node->get_bool_attribute_or_from_child_presence(TXT("forceSingleMaterial"), forceSingleMaterial);

	{
		int count = 0;
		for_every(pmNode, _node->children_named(TXT("particleMeshes")))
		{
			for_every(node, pmNode->children_named(TXT("add")))
			{
				++count;
			}
		}
		particleMeshes.clear();
		particleMeshes.set_size(count);

		int idx = 0;
		for_every(pmNode, _node->children_named(TXT("particleMeshes")))
		{
			Name boneParticleType = pmNode->get_name_attribute(TXT("boneParticleType"));
			for_every(node, pmNode->children_named(TXT("add")))
			{
				Element & element = particleMeshes[idx];
				element.boneParticleType = boneParticleType;
				if (element.load_from_xml(node, _lc))
				{
					// ok
				}
				else
				{
					result = false;
				}
				++ idx;
			}
		}
	}

	if (particleMeshes.is_empty())
	{
		error_loading_xml(_node, TXT("no particle meshes provided for mesh particle"));
		result = false;
	}

	if (skeleton.is_name_valid())
	{
		warn_loading_xml(_node, TXT("do not provide skeleton for mesh particle, clearing"));
		skeleton.clear();
	}

	if (useMeshGenerator.is_name_valid())
	{
		warn_loading_xml(_node, TXT("do not use mesh generator for mesh particle, clearing"));
		useMeshGenerator.clear();
	}

	return result;
}

struct MeshPartWorking
{
	Meshes::Primitive::Type primitiveType;
	MaterialSetup materialSetup;
	Array<int8> vertexData;
	Array<int8> elementData;
	::System::VertexFormat vf;
	::System::VertexFormat vfRef; // to compare as vf has skinning set to single bone
	::System::IndexFormat ef;
	int vertexDataSize;
	int elementDataSize = 0;

	// returns false if material setup differs
	bool add(Mesh* _mesh, int _partIdx, int _boneIdx, float _scale, WheresMyPoint::SimpleWMPOwner const & _instanceData, bool _forceSingleMaterial)
	{
		auto const& variables = _instanceData.get_variables();
		_mesh->generated_on_demand_if_required(); // to catch inappropriate use
		if (auto * srcMesh = fast_cast<Meshes::Mesh3D>(_mesh->get_mesh()))
		{
			if (auto* part = srcMesh->get_part(_partIdx))
			{
				if (vertexData.is_empty())
				{
					// new one!
					if (part->get_number_of_elements() != 0)
					{
						// copy elements
						ef = part->get_index_format();
						ef.calculate_stride();
						elementDataSize = ef.get_stride();
					}

					vf = part->get_vertex_format();
					vfRef = vf;
					//vf.with_skinning_data(System::DataFormatValueType::Uint8, 1);
					vf.with_skinning_to_single_bone_data(System::DataFormatValueType::Uint8);
					vf.calculate_stride_and_offsets();
					vertexDataSize = vf.get_stride();

					primitiveType = part->get_primitive_type();
					materialSetup = _mesh->get_material_setups()[_partIdx];
				}

				if (primitiveType != part->get_primitive_type() ||
					(!_forceSingleMaterial && materialSetup != _mesh->get_material_setups()[_partIdx]) ||
					part->get_vertex_format() != vfRef || // material setup might be different but we still want to have same vertex format
					(part->get_number_of_elements() != 0 && elementDataSize == 0) ||
					(part->get_number_of_elements() == 0 && elementDataSize != 0))
				{
					return false;
				}

				int vdSoFar = vertexData.get_size();
				int edSoFar = elementData.get_size();
				int newVDSize = part->get_number_of_vertices() * vertexDataSize;
				int newEDSize = part->get_number_of_elements() * elementDataSize;
				vertexData.grow_size(newVDSize);
				elementData.grow_size(newEDSize);

				::System::VertexFormatUtils::convert_data(part->get_vertex_format(), part->get_vertex_data().get_data(), part->get_vertex_data().get_data_size(), vf, &vertexData.get_data()[vdSoFar]);

				::System::VertexFormatUtils::fill_custom_data(vf, &vertexData.get_data()[vdSoFar], newVDSize, variables);

				{	// setup bone indices
					int locationOffset = vf.get_location_offset();
					int skinningOffset = vf.get_skinning_index_offset();
					int8* v = &vertexData.get_data()[vdSoFar];
					for (int j = 0; j < part->get_number_of_vertices(); ++j, v += vf.get_stride())
					{
						uint8* skinningToBone = (uint8*)(v + skinningOffset);
						*skinningToBone = (uint8)_boneIdx;
						Vector3* location = (Vector3*)(v + locationOffset);
						*location = *location * _scale;
					}
				}

				if (elementDataSize)
				{
					int8* e = &elementData.get_data()[edSoFar];
					memory_copy(e, part->get_element_data().get_data(), part->get_element_data().get_data_size());
					// alter vertices point by indices
					int offsetVertex = vdSoFar / vertexDataSize;
					for (int j = 0; j < part->get_number_of_elements(); ++j, e += ef.get_stride())
					{
						int v = System::IndexFormatUtils::get_value(ef, e);
						System::IndexFormatUtils::set_value(ef, e, v + offsetVertex);
					}
				}

				return true;
			}
		}
		an_assert(false, TXT("no part provided?"));
		return true;
	}
};

bool MeshParticles::prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext)
{
	bool result = base::prepare_for_game(_library, _pfgContext);

	for_every(element, particleMeshes)
	{
		result &= element->prepare_for_game(_library, _pfgContext);
	}

	IF_PREPARE_LEVEL(_pfgContext, LibraryPrepareLevel::GenerateParticleMeshes)
	{
		// create mesh
		make_sure_mesh_exists();
		mesh->remove_all_parts();

		particleCount = 0;

		Random::Generator rg;

		Array<MeshPartWorking*> mpws;
		int particleCountSoFar = 0;
		WheresMyPoint::SimpleWMPOwner instanceData;
		for_every(element, particleMeshes)
		{
			int count = rg.get(element->count);
			for_count(int, i, count)
			{
				int idx = RandomUtils::ChooseFromContainer<Array<Element::Option>, Element::Option>::choose(rg, element->options, [](Element::Option const & _option){ return _option.probabilityCoef; });
				auto const& option = element->options[idx];
				Mesh* mesh = option.particleMesh.get();
				an_assert(mesh || Framework::is_preview_game());
				if (mesh)
				{
					if (!option.instanceCustomDataWMP.is_empty())
					{
						// get values for instance
						WheresMyPoint::Context context(&instanceData);
						context.access_random_generator() = rg.spawn();
						option.instanceCustomDataWMP.update(context);
					}
					mesh->generated_on_demand_if_required(); // to catch inappropriate use
					if (auto * srcMesh = fast_cast<Meshes::Mesh3D>(mesh->get_mesh()))
					{
						float scale = rg.get(element->scale);
						for_count(int, partIdx, srcMesh->get_parts_num())
						{
							bool added = false;
							for_every_ptr(mpw, mpws)
							{
								if (mpw->add(mesh, partIdx, particleCount, scale, instanceData, forceSingleMaterial))
								{
									added = true;
									break;
								}
							}
							if (!added)
							{
								MeshPartWorking* mpw = new MeshPartWorking();
								added = mpw->add(mesh, partIdx, particleCount, scale, instanceData, forceSingleMaterial);
								an_assert(added);
								mpws.push_back(mpw);
							}
							an_assert(added);
						}
						++particleCount;
					}
				}
			}
			while (particleCountSoFar < particleCount)
			{
				boneParticleTypes.push_back(element->boneParticleType);
				++particleCountSoFar;
			}
		}

		for_every_ptr(mpw, mpws)
		{
			RefCountObjectPtr<Meshes::Mesh3DPart> newPart(new Meshes::Mesh3DPart(false, false));

			if (mpw->elementDataSize)
			{
				newPart->load_data(mpw->vertexData.get_data(), mpw->elementData.get_data(), mpw->primitiveType, mpw->vertexData.get_data_size() / mpw->vertexDataSize, mpw->elementData.get_data_size() / mpw->elementDataSize, mpw->vf, mpw->ef);
			}
			else
			{
				newPart->load_data(mpw->vertexData.get_data(), mpw->primitiveType, mpw->vertexData.get_data_size() / mpw->vertexDataSize, mpw->vf);
			}

			mesh->add_part(newPart.get());

			// do not forget about materials!
			set_material_setup(for_everys_index(mpw), mpw->materialSetup);

			delete mpw;
		}

		set_missing_materials(_library);

		// and skeleton now, as we now have proper particle count

		// create skeleton with as many bones as particleCount
		skeleton = Library::get_current()->get_skeletons().create_temporary(get_name());
		an_assert(!skeleton->has_skeleton());
		skeleton->replace_skeleton(new Meshes::Skeleton());
		if (auto* skel = skeleton->get_skeleton())
		{
			skel->access_bones().set_size(particleCount);
			for_every(bone, skel->access_bones())
			{
				bone->set_name(Name(String::printf(TXT("%i"), for_everys_index(bone))));
				bone->set_placement_MS(Transform::identity);
			}
			skel->prepare_to_use();
		}
		else
		{
			error(TXT("no skeleton for temporary skeleton for \"%S\""), get_name().to_string().to_char());
			result = false;
		}

	}

	return result;
}

bool MeshParticles::load_mesh_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc)
{
	// do not load mesh - we will be creating one
	return true;
}

void MeshParticles::get_particle_indices(Name const & _boneParticleType, OUT_ Array<int> & _particleIndices) const
{
	_particleIndices.clear();
	_particleIndices.make_space_for(boneParticleTypes.get_size());
	for_every(boneParticleType, boneParticleTypes)
	{
		if (*boneParticleType == _boneParticleType)
		{
			_particleIndices.push_back(for_everys_index(boneParticleType));
		}
	}
	_particleIndices.prune();
	if (_particleIndices.is_empty())
	{
		error(TXT("could not find particles of bone particle type \"%S\""), _boneParticleType.to_char());
	}
}
