#include "environmentInfo.h"

#include "..\library\library.h"
#include "..\library\usedLibraryStored.inl"

using namespace Framework;

EnvironmentInfo::EnvironmentInfo()
: parent(nullptr)
, generateAsRoom(false)
{
}

void EnvironmentInfo::clear()
{
	parent = nullptr;

	generateAsRoom = false;

	propagatesPlacementBetweenRooms.clear();
	locationFixedAgainstCamera.clear();
	defaultInRoomPlacement.clear();

	skyBoxMaterial.clear();
	cloudsMaterial.clear();
	skyBoxRadius.clear();
	skyBoxCentre.clear();

	params.clear();

	copyParams.clear();
}

void EnvironmentInfo::copy_from(EnvironmentInfo const & _source, Array<Name> const * _skipParams)
{
	generateAsRoom = _source.generateAsRoom;

	propagatesPlacementBetweenRooms.set(_source.propagatesPlacementBetweenRooms);
	locationFixedAgainstCamera.set(_source.locationFixedAgainstCamera);
	defaultInRoomPlacement.set(_source.defaultInRoomPlacement);

	copy_params_from(_source, _skipParams);

	skyBoxMaterial = _source.skyBoxMaterial;
	cloudsMaterial = _source.cloudsMaterial;
	skyBoxRadius = _source.skyBoxRadius;
	skyBoxCentre = _source.skyBoxCentre;
}

void EnvironmentInfo::fill_params_with(EnvironmentInfo const & _source, Array<Name> const * _skipParams)
{
	params.fill_with(_source.params, _skipParams);
}

void EnvironmentInfo::copy_params_from(EnvironmentInfo const & _source, Array<Name> const * _skipParams)
{
	params.copy_from(_source.params, _skipParams);
}

void EnvironmentInfo::set_params_from(EnvironmentInfo const & _source, Array<Name> const * _skipParams)
{
	params.set_from(_source.params, _skipParams);
}

void EnvironmentInfo::lerp_params_with(float _amount, EnvironmentInfo const & _info, Array<Name> const * _skipParams)
{
	params.lerp_with(_amount, _info.params, _skipParams);
}

bool EnvironmentInfo::load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc)
{
	bool result = true;

	{
		bool readGenerationInfo = false;
		for_every(node, _node->all_children())
		{
			if (node->get_name() == TXT("empty"))
			{
				generateAsRoom = false;
				readGenerationInfo = true;
			}
			if (node->get_name() == TXT("noGeneration"))
			{
				generateAsRoom = false;
				readGenerationInfo = true;
			}
			if (node->get_name() == TXT("asRoomGenerator"))
			{
				generateAsRoom = true;
				readGenerationInfo = true;
			}

			if (node->get_name() == TXT("locationFixedAgainstCamera"))
			{
				locationFixedAgainstCamera.set(true);
			}
			if (node->get_name() == TXT("locationNotFixedAgainstCamera"))
			{
				locationFixedAgainstCamera.set(false);
			}

			if (node->get_name() == TXT("propagatesPlacementBetweenRooms"))
			{
				propagatesPlacementBetweenRooms.set(true);
			}
			if (node->get_name() == TXT("doesntPropagatePlacementBetweenRooms"))
			{
				propagatesPlacementBetweenRooms.set(false);
			}
		}
		if (!readGenerationInfo)
		{
			error_loading_xml(_node, TXT("not defined how environment should be generated"));
			result = false;
		}
	}

	for_every(node, _node->children_named(TXT("defaultInRoomPlacement")))
	{
		if (!defaultInRoomPlacement.is_set())
		{
			defaultInRoomPlacement.set(Transform::identity);
		}
		result &= defaultInRoomPlacement.access().load_from_xml(node);
	}

	for_every(node, _node->children_named(TXT("light")))
	{
		warn_loading_xml(node, TXT("use \"params\", as built-in \"light\" is deprecated"));
	}

	for_every(node, _node->children_named(TXT("distanceMod")))
	{
		warn_loading_xml(node, TXT("use \"params\", as built-in \"distanceMod\" is deprecated"));
	}

	for_every(node, _node->children_named(TXT("fog")))
	{
		warn_loading_xml(node, TXT("use \"params\", as built-in \"fog\" is deprecated"));
	}

	for_every(node, _node->children_named(TXT("params")))
	{
		result &= params.load_from_xml(node, _lc);
	}

	for_every(node, _node->children_named(TXT("copyParams")))
	{
		CopyParams cp;
		cp.from.load_from_xml(node, TXT("from"), _lc);
		cp.overrideExisting = node->get_bool_attribute_or_from_child_presence(TXT("overrideExisting"), cp.overrideExisting);
		if (cp.from.is_name_valid())
		{
			copyParams.push_back(cp);
		}
		else
		{
			error_loading_xml(node, TXT("no valid \"from\""));
			result = false;
		}
	}

	result &= skyBoxMaterial.load_from_xml(_node, TXT("skyBoxMaterial"), _lc);
#ifndef AN_ANDROID
	// no clouds for android (quest etc) for time being
	result &= cloudsMaterial.load_from_xml(_node, TXT("cloudsMaterial"), _lc);
#endif
	result &= skyBoxRadius.load_from_xml(_node, TXT("skyBoxRadius"));
	result &= skyBoxCentre.load_from_xml(_node, TXT("skyBoxCentre"));

	return result;
}

bool EnvironmentInfo::prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext)
{
	bool result = true;

	result &= params.prepare_for_game(_library, _pfgContext);

	result &= skyBoxMaterial.find(_library);
	result &= cloudsMaterial.find(_library);

	IF_PREPARE_LEVEL(_pfgContext, LibraryPrepareLevel::CreateFromTemplates)
	{
		for_every(cp, copyParams)
		{
			cp->from.prepare_for_game_find(_library, _pfgContext, LibraryPrepareLevel::CreateFromTemplates);
			if (auto* ls = cp->from.get())
			{
				if (cp->overrideExisting)
				{
					params.set_from(ls->get_info().params);
				}
				else
				{
					params.set_missing_from(ls->get_info().params);
				}
			}
			else
			{
				error(TXT("couldn't find environment type \"%S\""), cp->from.get_name().to_string().to_char());
				result = false;
			}
		}
	}

	return result;
}
