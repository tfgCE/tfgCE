#include "displayType.h"

#include "..\postProcess\postProcess.h"
#include "..\video\font.h"
#include "..\video\texture.h"
#include "..\library\library.h"
#include "..\library\usedLibraryStored.inl"

using namespace Framework;

REGISTER_FOR_FAST_CAST(DisplayType);
LIBRARY_STORED_DEFINE_TYPE(DisplayType, displayType);

DisplayType::DisplayType(Library * _library, LibraryName const & _name)
: base(_library, _name)
{
}

DisplayType::~DisplayType()
{
}

bool DisplayType::load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc)
{
	bool result = base::load_from_xml(_node, _lc);

	result &= shortDescription.load_from_xml(_node->first_child_named(TXT("shortDesc")), _lc, TXT("short desc"));
	result &= shortDescription.load_from_xml(_node->first_child_named(TXT("shortDescription")), _lc, TXT("short desc"));

	result &= setup.load_from_xml(_node->first_child_named(TXT("display")), _lc);
	result &= hardwareMeshSetup.load_from_xml(_node->first_child_named(TXT("hardwareMesh")), _lc);

	for_every(containerNode, _node->children_named(TXT("sounds")))
	{
		backgroundSoundSample.load_from_xml_child_node(containerNode, TXT("background"), TXT("sample"), _lc);
		for_every(node, containerNode->children_named(TXT("sound")))
		{
			Name id = node->get_name_attribute(TXT("id"));
			if (id.is_valid())
			{
				result &= soundSamples[id].load_from_xml(node, TXT("sample"), _lc);
			}
			else
			{
				error_loading_xml(node, TXT("no id provided for soundSample in display type"));
				result = false;
			}
		}
	}

	return result;
}

bool DisplayType::prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext)
{
	bool result = base::prepare_for_game(_library, _pfgContext);

	result &= setup.prepare_for_game(_library, _pfgContext);
	result &= hardwareMeshSetup.prepare_for_game(_library, _pfgContext);

	if (backgroundSoundSample.is_name_valid())
	{
		result &= backgroundSoundSample.prepare_for_game_find(_library, _pfgContext, LibraryPrepareLevel::Resolve);
	}

	for_every(soundSample, soundSamples)
	{
		if (!soundSample->prepare_for_game_find(_library, _pfgContext, LibraryPrepareLevel::Resolve))
		{
			error(TXT("could not find soundSample"));
		}
	}

	return result;
}

void DisplayType::debug_draw() const
{
	LibraryName previewMeshName(String(TXT("previewMeshes")), get_name().to_string());

	auto* library = Library::get_current();

	if (auto* mesh = library->get_meshes().find(previewMeshName))
	{
		mesh->debug_draw();
	}
	else
	{
		DisplayHardwareMeshSetup setup = get_hardware_mesh_setup();
		setup.with_size_2d(Vector2(4.0f,3.0f) / 3.0f);
		auto* newMesh = DisplayHardwareMeshGenerator::create_3d_mesh(setup, library, previewMeshName);
		newMesh->set_missing_materials(library);
	}
}

void DisplayType::log(LogInfoContext & _context) const
{
	base::log(_context);
}
