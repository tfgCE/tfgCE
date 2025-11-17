#include "meshDepot.h"

#include "..\debug\previewGame.h"
#include "..\game\game.h"
#include "..\library\library.h"
#include "..\library\usedLibraryStored.inl"
#include "..\meshGen\meshGenerator.h"
#include "..\modulesOwner\modulesOwner.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace Framework;

//

REGISTER_FOR_FAST_CAST(Framework::MeshDepot);
LIBRARY_STORED_DEFINE_TYPE(MeshDepot, meshDepot);

MeshDepot::MeshDepot(Library* _library, LibraryName const& _name)
: LibraryStored(_library, _name)
{
}

MeshDepot::~MeshDepot()
{
}

Mesh* MeshDepot::get_mesh_for(IModulesOwner* _imo, SimpleVariableStorage const* _additionalVariables)
{
	an_assert(!does_require_further_loading());

	GeneratedMeshDeterminants meshDeterminants;
	meshDeterminants.gather_for(_imo, _additionalVariables, generatedMeshesDeterminants);
	return get_mesh(Library::get_current(), meshDeterminants, _imo->get_individual_random_generator());
}

void MeshDepot::prepare_to_unload()
{
	base::prepare_to_unload();
	sets.clear();
	generatedMeshesDeterminants.clear();
}

#ifdef AN_DEVELOPMENT
void MeshDepot::ready_for_reload()
{
	base::ready_for_reload();
	sets.clear();
	generatedMeshesDeterminants.clear();
}
#endif

bool MeshDepot::load_from_xml(IO::XML::Node const* _node, LibraryLoadingContext& _lc)
{
	bool result = LibraryStored::load_from_xml(_node, _lc);

	result &= generatedMeshesDeterminants.load_from_xml_child_node(_node, TXT("determinants"));

	sets.clear();
	{
		GeneratedSet defSet;
		defSet.generatedMeshesDeterminants = generatedMeshesDeterminants;
		sets.push_back(defSet);
	}
	for_every(n, _node->children_named(TXT("prepareForDeterminants"))) // will auto generate all
	{
		GeneratedSet set;
		set.generatedMeshesDeterminants.load_from_xml(n);
		sets.push_back(set);
	}
	result &= generatedMeshesDeterminants.load_from_xml_child_node(_node, TXT("determinants"));

	result &= meshGenerator.load_from_xml(_node, TXT("meshGenerator"), _lc);

	varietyAmount = _node->get_int_attribute_or_from_child(TXT("varietyAmount"), varietyAmount);
	generateDefaultSet = _node->get_bool_attribute_or_from_child_presence(TXT("generateDefaultSet"), generateDefaultSet);

	error_loading_xml_on_assert(meshGenerator.is_name_valid(), result, _node, TXT("no \"meshGenerator\" provided"));

	return result;
}

bool MeshDepot::prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext)
{
	bool result = LibraryStored::prepare_for_game(_library, _pfgContext);

	result &= meshGenerator.prepare_for_game_find(_library, _pfgContext, LibraryPrepareLevel::Resolve);

	IF_PREPARE_LEVEL(_pfgContext, LibraryPrepareLevel::GenerateMeshes)
	{
		Random::Generator rg;
		for (int setIdx = 0; setIdx < sets.get_size(); ++setIdx)
		{
#ifndef AN_DEVELOPMENT
			if (generateDefaultSet)
			{
				for_count(int, i, varietyAmount)
				{
					// if we exceed the count, we're still okay
					get_mesh(_library, sets[setIdx].generatedMeshesDeterminants, rg);
					rg.advance_seed(3280, 9572);
				}
			}
			else
#endif
			{
				auto& detsToAdd = sets[setIdx].generatedMeshesDeterminants;
				bool added = false;
				for_every(set, sets)
				{
					if (set->generatedMeshesDeterminants == detsToAdd)
					{
						added = true;
						break;
					}
				}

				if (!added)
				{
					sets.push_back();
					auto& gs = sets.get_last();
					gs.generatedMeshesDeterminants = detsToAdd;
				}
			}
		}
	}

	return result;
}

Mesh* MeshDepot::get_mesh(Library* _library, GeneratedMeshDeterminants const& _meshDeterminants, Optional<Random::Generator> const& _rg)
{
	ASSERT_ASYNC_OR_LOADING_OR_PREVIEW;

	Random::Generator rg = _rg.get(Random::Generator());

	Concurrency::ScopedSpinLock lock(setsLock);

	for_every(set, sets)
	{
		if (set->generatedMeshesDeterminants == _meshDeterminants &&
			set->meshes.get_size() >= varietyAmount)
		{
			return set->meshes[rg.get_int(set->meshes.get_size())].get();
		}
	}

	// only use parameters provided by the mesh depot itself or through determinants, nothing else!
	// otherwise we will get some individual stuff from objects that we can't deal with later
	SimpleVariableStorage parameters;
	parameters.set_from(get_custom_parameters());
	_meshDeterminants.apply_to(parameters);

	MeshGeneratorRequest meshGeneratorRequest;
	meshGeneratorRequest.using_parameters(parameters);
	meshGeneratorRequest.using_random_regenerator(rg);

	auto mesh = meshGenerator->generate_temporary_library_mesh(_library, meshGeneratorRequest);

	if (mesh.get())
	{
		bool added = false;
		for_every(set, sets)
		{
			if (set->generatedMeshesDeterminants == _meshDeterminants)
			{
				set->meshes.push_back(mesh);
				added = true;
				break;
			}
		}

		if (!added)
		{
			sets.push_back();
			auto& gs = sets.get_last();
			gs.generatedMeshesDeterminants = _meshDeterminants;
			gs.meshes.push_back(mesh);
		}

		return mesh.get();
	}

	return nullptr;
}
