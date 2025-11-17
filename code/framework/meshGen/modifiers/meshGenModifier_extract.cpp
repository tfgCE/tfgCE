#include "meshGenModifiers.h"

#include "..\meshGenGenerationContext.h"
#include "..\meshGenUtils.h"
#include "..\meshGenElementModifier.h"
#include "..\meshGenValueDef.h"

#include "..\..\world\pointOfInterest.h"

#include "..\..\..\core\mesh\boneRenameFunc.h"
#include "..\..\..\core\mesh\mesh3d.h"
#include "..\..\..\core\mesh\skeleton.h"
#include "..\..\..\core\mesh\socketDefinitionSet.h"

using namespace Framework;
using namespace MeshGeneration;
using namespace Modifiers;

/**
 *	Allows:
 *		modifying bone names (adding prefix/suffix, replacing bits)
 *		getting placement of bones and storing them
 */
class ExtractData
: public ElementModifierData
{
	FAST_CAST_DECLARE(ExtractData);
	FAST_CAST_BASE(ElementModifierData);
	FAST_CAST_END();

	typedef ElementModifierData base;
public:
	override_ bool load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc);

public: // we won't be exposed anywhere else, so we may be public here!
	::Meshes::BoneRenamer boneNames;
	ValueDef<Name> currentParent;

	struct Placements
	{
		struct GetPlacement
		{
			enum AsType
			{
				AsNone,
				AsVector3,
				AsTransform,
			};

			AsType getAs = AsNone;
			ValueDef<Name> thingName;
			Name intoParameterName; // stored in global parameter array

			GetPlacement() {}
			GetPlacement(AsType _getAs, ValueDef<Name> const & _thingName, Name _intoParameterName) : getAs(_getAs), thingName(_thingName), intoParameterName(_intoParameterName) {}

			static bool load_from_xml(IO::XML::Node const * _node, tchar const * _thingNameAttr, OUT_ GetPlacement & _into)
			{
				Placements::GetPlacement::AsType as = Placements::GetPlacement::AsNone;
				if (_node->get_name() == TXT("getVector3") ||
					_node->get_name() == TXT("getLocation"))
				{
					as = Placements::GetPlacement::AsVector3;
				}
				else if (_node->get_name() == TXT("getTransform") ||
						 _node->get_name() == TXT("getPlacement"))
				{
					as = Placements::GetPlacement::AsTransform;
				}
				if (as != Placements::GetPlacement::AsNone)
				{
					ValueDef<Name> thingName;
					thingName.load_from_xml(_node, _thingNameAttr);
					Name intoParameterName = _node->get_name_attribute(TXT("parameter"));
					if (thingName.is_set() && intoParameterName.is_valid())
					{
						_into = Placements::GetPlacement(as, thingName, intoParameterName);
						return true;
					}
					else
					{
						error_loading_xml(_node, TXT("rename node doesn't have both \"%S\" and \"parameter\""), _thingNameAttr);
						return false;
					}
				}
				error_loading_xml(_node, TXT("not provided as what it should be read"));
				return false;
			}
		};
		Array<GetPlacement> getPlacements;
	};

	Placements bonePlacements;
	Placements socketPlacements;
	Placements poiPlacements;
};

//

bool Modifiers::extract(GenerationContext & _context, ElementInstance & _instigatorInstance, Element const * _subject, ::Framework::MeshGeneration::ElementModifierData const * _data)
{
	bool result = true;

	if (ExtractData const * data = fast_cast<ExtractData>(_data))
	{
		{	// apply bone names
			_context.use_bone_renamer(data->boneNames);
		}

		{	// set parameters from placements
			for_every(getPlacement, data->bonePlacements.getPlacements)
			{
				int32 boneIdx = _context.find_bone_index(getPlacement->thingName.get(_context));
				if (boneIdx != NONE)
				{
					if (_context.has_skeleton())
					{
						Transform bonePlacement = _context.get_bone_placement_MS(boneIdx);
						if (getPlacement->getAs == ExtractData::Placements::GetPlacement::AsVector3)
						{
							// both on stack and in instance
							_context.set_parameter<Vector3>(getPlacement->intoParameterName, bonePlacement.get_translation());
						}
						else if (getPlacement->getAs == ExtractData::Placements::GetPlacement::AsTransform)
						{
							// both on stack and in instance
							_context.set_parameter<Transform>(getPlacement->intoParameterName, bonePlacement);
						}
					}
					else
					{
						error_generating_mesh(_instigatorInstance, TXT("no skeleton to get placement from"));
					}
				}
				else
				{
					error_generating_mesh(_instigatorInstance, TXT("could not find bone \"%S\""), getPlacement->thingName.get(_context).to_char());
				}
			}
		}
		{	// set parameters from placements
			for_every(getPlacement, data->socketPlacements.getPlacements)
			{
				if (auto* socket = _context.access_sockets().find_socket(getPlacement->thingName.get(_context)))
				{
					Transform placementMS = Transform::identity;
					if (socket->get_placement_MS().is_set())
					{
						placementMS = socket->get_placement_MS().get();
					}
					else
					{
						if (socket->get_bone_name().is_valid())
						{
							int32 boneIdx = _context.find_bone_index(socket->get_bone_name());
							if (boneIdx != NONE)
							{
								if (_context.has_skeleton())
								{
									placementMS = _context.get_bone_placement_MS(boneIdx);
								}
							}
						}
						if (socket->get_relative_placement().is_set())
						{
							placementMS = placementMS.to_world(socket->get_relative_placement().get());
						}
					}
					if (getPlacement->getAs == ExtractData::Placements::GetPlacement::AsVector3)
					{
						// both on stack and in instance
						_context.set_parameter<Vector3>(getPlacement->intoParameterName, placementMS.get_translation());
					}
					else if (getPlacement->getAs == ExtractData::Placements::GetPlacement::AsTransform)
					{
						// both on stack and in instance
						_context.set_parameter<Transform>(getPlacement->intoParameterName, placementMS);
					}
				}
				else
				{
					error_generating_mesh(_instigatorInstance, TXT("could not find bone \"%S\""), getPlacement->thingName.get(_context).to_char());
				}
			}
		}
		{	// set parameters from placements
			for_every(getPlacement, data->poiPlacements.getPlacements)
			{
				Name poiName = getPlacement->thingName.get(_context);
				bool poiFound = false;
				for_every_ref(poi, _context.access_pois())
				{
					if (poi->name == poiName)
					{
						poiFound = true;
						Transform placementMS = poi->offset;
						if (getPlacement->getAs == ExtractData::Placements::GetPlacement::AsVector3)
						{
							// both on stack and in instance
							_context.set_parameter<Vector3>(getPlacement->intoParameterName, placementMS.get_translation());
						}
						else if (getPlacement->getAs == ExtractData::Placements::GetPlacement::AsTransform)
						{
							// both on stack and in instance
							_context.set_parameter<Transform>(getPlacement->intoParameterName, placementMS);
						}
					}
				}
				if (!poiFound)
				{
					error_generating_mesh(_instigatorInstance, TXT("could not find bone \"%S\""), getPlacement->thingName.get(_context).to_char());
				}
			}
		}
	}

	_instigatorInstance.update_wmp_directly();

	if (_subject)
	{
		result &= _subject && _context.process(_subject, &_instigatorInstance);
	}

	if (ExtractData const * data = fast_cast<ExtractData>(_data))
	{
		if (data->currentParent.is_set())
		{
			_context.set_current_parent_bone(data->currentParent.get(_context));
		}
	}

	return result;
}

::Framework::MeshGeneration::ElementModifierData* Modifiers::create_extract_data()
{
	return new ExtractData();
}

//

REGISTER_FOR_FAST_CAST(ExtractData);

bool ExtractData::load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc)
{
	bool result = true;

	currentParent.load_from_xml(_node, TXT("currentParent"));

	for_every(node, _node->children_named(TXT("boneNames")))
	{
		result &= boneNames.load_from_xml(_node);
	}

	for_every(node, _node->children_named(TXT("bonePlacements")))
	{
		for_every(getNode, node->all_children())
		{
			Placements::GetPlacement bone;
			if (Placements::GetPlacement::load_from_xml(getNode, TXT("bone"), OUT_ bone))
			{
				if (bone.getAs != Placements::GetPlacement::AsNone)
				{
					bonePlacements.getPlacements.push_back(bone);
				}
			}
			else
			{
				result = false;
			}
		}
	}

	for_every(node, _node->children_named(TXT("socketPlacements")))
	{
		for_every(getNode, node->all_children())
		{
			Placements::GetPlacement socket;
			if (Placements::GetPlacement::load_from_xml(getNode, TXT("socket"), OUT_ socket))
			{
				if (socket.getAs != Placements::GetPlacement::AsNone)
				{
					socketPlacements.getPlacements.push_back(socket);
				}
			}
			else
			{
				result = false;
			}
		}
	}

	for_every(node, _node->children_named(TXT("poiPlacements")))
	{
		for_every(getNode, node->all_children())
		{
			Placements::GetPlacement poi;
			if (Placements::GetPlacement::load_from_xml(getNode, TXT("poi"), OUT_ poi))
			{
				if (poi.getAs != Placements::GetPlacement::AsNone)
				{
					poiPlacements.getPlacements.push_back(poi);
				}
			}
			else
			{
				result = false;
			}
		}
	}

	return result;
}