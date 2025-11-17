#include "roomGeneratorInfoSet.h"

#include "regionGenerator.h"
#include "roomGeneratorInfo.h"

#include "..\game\game.h"

#include "..\..\core\io\xml.h"
#include "..\..\core\random\random.h"
#include "..\..\core\random\randomUtils.h"

//

using namespace Framework;

//

bool RoomGeneratorInfoOption::load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc, tchar const * _defaultGeneratorType)
{
	bool result = true;

	String generatorType = _node->get_string_attribute(TXT("type"));
	if (generatorType.is_empty())
	{
		if (_defaultGeneratorType)
		{
			generatorType = _defaultGeneratorType;
			warn_loading_xml(_node, TXT("please provide type, assuming \"%S\""), _defaultGeneratorType);
		}
		else
		{
			error_loading_xml(_node, TXT("no room generator type provided"));
			result = false;
		}
	}
	roomGeneratorInfo = RoomGeneratorInfo::create(generatorType);
	if (roomGeneratorInfo.get())
	{
		result &= roomGeneratorInfo->load_from_xml(_node, _lc);
		if (!result)
		{
			error_loading_xml(_node, TXT("problem loading room generator info \"%S\""), generatorType.to_char());
		}
	}
	else
	{
		error_loading_xml(_node, TXT("could not recognise room generator type \"%S\""), generatorType.to_char());
		result = false;
	}

	probabilityCoef = _node->get_float_attribute_or_from_child(TXT("probCoef"), probabilityCoef);
	probabilityCoef = _node->get_float_attribute_or_from_child(TXT("probabilityCoef"), probabilityCoef);
	probabilityCoef = _node->get_float_attribute_or_from_child(TXT("probability"), probabilityCoef);

	gameTagsCondition.load_from_xml_attribute_or_child_node(_node, TXT("gameTagsCondition"));

	return result;

}

bool RoomGeneratorInfoOption::is_ok_to_be_used() const
{
	if (!gameTagsCondition.is_empty())
	{
		if (auto* g = Game::get())
		{
			if (!gameTagsCondition.check(g->get_game_tags()))
			{
				return false;
			}
		}
	}
	return true;
}

bool RoomGeneratorInfoOption::prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext)
{
	bool result = true;

	result &= roomGeneratorInfo->prepare_for_game(_library, _pfgContext);

	return result;
}

RoomGeneratorInfoOption RoomGeneratorInfoOption::create_copy() const
{
	RoomGeneratorInfoOption result;
	result.probabilityCoef = probabilityCoef;
	result.roomGeneratorInfo = roomGeneratorInfo->create_copy();
	return result;
}

bool RoomGeneratorInfoOption::apply_renamer(LibraryStoredRenamer const & _renamer, Library * _library)
{
	bool result = true;
	result &= roomGeneratorInfo->apply_renamer(_renamer, _library);
	return result;
}

//

bool RoomGeneratorInfoSet::load_from_xml(IO::XML::Node const * _containerNode, tchar const * _subContainerName, tchar const * _singleNodeName, LibraryLoadingContext & _lc, tchar const * _defaultGeneratorType)
{
	bool result = true;

	for_every(node, _containerNode->children_named(_subContainerName))
	{
		result &= load_from_xml(node, _subContainerName, _singleNodeName, _lc, _defaultGeneratorType);
	}

	for_every(node, _containerNode->children_named(_singleNodeName))
	{
		RoomGeneratorInfoOption option;
		if (option.load_from_xml(node, _lc, _defaultGeneratorType))
		{
			infos.push_back(option);
		}
		else
		{
			result = false;
		}
	}

	return result;
}

bool RoomGeneratorInfoSet::prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext)
{
	bool result = true;

	for_every(info, infos)
	{
		result &= info->prepare_for_game(_library, _pfgContext);
	}

	return result;
}

RoomGeneratorInfo const * RoomGeneratorInfoSet::get(Random::Generator & _rg) const
{
	if (infos.is_empty())
	{
		return nullptr;
	}

	bool anyAvailable = false;
	for_every(info, infos)
	{
		if (info->is_ok_to_be_used())
		{
			anyAvailable = true;
			break;
		}
	}

	if (anyAvailable)
	{
		int idx = RandomUtils::ChooseFromContainer<Array<RoomGeneratorInfoOption>, RoomGeneratorInfoOption>::choose(_rg, infos,
			[](RoomGeneratorInfoOption const & _option)
		{
			return _option.is_ok_to_be_used()? _option.get_probability_coef() : 0.0f;
		}
		);

		return infos[idx].get_room_generator_info();
	}
	else
	{
		return nullptr;
	}
}

RoomGeneratorInfo const * RoomGeneratorInfoSet::get(int _idx) const
{
	if (! infos.is_index_valid(_idx))
	{
		return nullptr;
	}

	return infos[_idx].is_ok_to_be_used()? infos[_idx].get_room_generator_info() : nullptr;
}

RoomGeneratorInfoSet RoomGeneratorInfoSet::create_copy() const
{
	RoomGeneratorInfoSet result;
	result.infos.make_space_for(infos.get_size());
	for_every(info, infos)
	{
		result.infos.push_back(info->create_copy());
	}
	return result;
}

bool RoomGeneratorInfoSet::apply_renamer(LibraryStoredRenamer const & _renamer, Library * _library)
{
	bool result = true;
	for_every(info, infos)
	{
		result &= info->apply_renamer(_renamer, _library);
	}
	return result;
}

void RoomGeneratorInfoSet::clear()
{
	infos.clear();
}

RefCountObjectPtr<PieceCombiner::Piece<RegionGenerator>> RoomGeneratorInfoSet::generate_piece_for_region_generator(REF_ PieceCombiner::Generator<Framework::RegionGenerator>& _generator, Region* _region, REF_ Random::Generator & _randomGenerator) const
{
	RefCountObjectPtr<PieceCombiner::Piece<RegionGenerator>> ptr;
	if (infos.get_size() > 0)
	{
		for_count(int, rgIdx, infos.get_size())
		{
			if (infos[rgIdx].is_ok_to_be_used())
			{
				if (auto* rgi = infos[rgIdx].get_room_generator_info())
				{
					if (ptr.is_set())
					{
						warn(TXT("this should be used only if there is one info in room generator info"));
						break;
					}
					ptr = rgi->generate_piece_for_region_generator(_generator, _region, REF_ _randomGenerator);
				}
			}
		}
	}
	return ptr;
}
