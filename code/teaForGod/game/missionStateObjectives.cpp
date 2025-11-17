#include "missionStateObjectives.h"

#include "..\pilgrim\pilgrimOverlayInfo.h"

#include "..\library\library.h"

#include "..\..\framework\library\library.h"
#include "..\..\framework\library\usedLibraryStored.inl"
#include "..\..\framework\object\object.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace TeaForGodEmperor;

//

// custom parameters
DEFINE_STATIC_NAME(missionItemIcon);

//

bool MissionStateObjectives::save_to_xml(IO::XML::Node* _node) const
{
	bool result = true;

	for_every(itemType, bringItemsTypes)
	{
		auto* node = _node->add_node(TXT("bringItem"));
		if (!itemType->save_to_xml(node, TXT("type")))
		{
			result = false;
		}
	}

	return result;
}

bool MissionStateObjectives::load_from_xml(IO::XML::Node const* _node)
{
	bool result = true;

	// reset
	//

	bringItemsTypes.clear();

	// load
	//

	for_every(node, _node->children_named(TXT("bringItem")))
	{
		bringItemsTypes.push_back();
		if (!bringItemsTypes.get_last().load_from_xml(node, TXT("type")))
		{
			bringItemsTypes.pop_back();
			error_loading_xml(node, TXT("couldn't load item to bring"));
			result = false;
		}
	}

	return result;
}

void MissionStateObjectives::reset()
{
	Concurrency::ScopedMRSWLockWrite l(lock, true);

	bringItemsTypes.clear();
}

void MissionStateObjectives::copy_from(MissionStateObjectives const& _other)
{
	Concurrency::ScopedMRSWLockWrite l(lock, true);
	Concurrency::ScopedMRSWLockRead lOther(_other.lock, true);

	bringItemsTypes = _other.bringItemsTypes;
}

void MissionStateObjectives::prepare_for_gameplay()
{
	auto* library = Library::get_current();
	for_every(it, bringItemsTypes)
	{
		it->find_if_required(library);
	}
}

void MissionStateObjectives::prepare_to_render(PilgrimOverlayInfoRenderableData& _rd, Vector3 const& _xAxis, Vector3 const& _yAxis, Vector2 const& _startingOffset, float _scaleLineModel) const
{
	Concurrency::ScopedMRSWLockRead l(lock, true);

	Array<LineModel*> lineModels;

	for_every_ref(it, bringItemsTypes)
	{
		if (auto* ln = it->get_custom_parameters().get_existing<Framework::LibraryName>(NAME(missionItemIcon)))
		{
			if (auto* lm = Library::get_current_as<Library>()->get_line_models().find(*ln))
			{
				lineModels.push_back(lm);
			}
		}
	}

	//

	{
		_rd.mark_new_data_required();
		_rd.clear();

		{
			Transform at = Transform(_xAxis * _startingOffset.x * _scaleLineModel + _yAxis * _startingOffset.y * _scaleLineModel, Quat::identity);
			Vector3 nextOffset = -_yAxis * _scaleLineModel * 1.2f;
			for_every_ptr(lm, lineModels)
			{
				for_every(l, lm->get_lines())
				{
					Colour c = l->colour.get(Colour::white);
					Vector3 a = _xAxis * l->a.x + _yAxis * l->a.y;
					Vector3 b = _xAxis * l->b.x + _yAxis * l->b.y;
					a = at.location_to_world(a * _scaleLineModel);
					b = at.location_to_world(b * _scaleLineModel);

					_rd.add_line(a, b, c);
				}

				at.set_translation(at.get_translation() + nextOffset);
			}
		}

		_rd.mark_new_data_done();
		_rd.mark_data_available_to_render();
	}

}

bool MissionStateObjectives::compare(MissionStateObjectives const& _a, MissionStateObjectives const& _b)
{
	Concurrency::ScopedMRSWLockRead la(_a.lock, true);
	Concurrency::ScopedMRSWLockRead lb(_b.lock, true);

	return _a.bringItemsTypes == _b.bringItemsTypes;
}

bool MissionStateObjectives::is_item_required(Framework::IModulesOwner* _imo) const
{
	if (auto* obj = _imo->get_as_object())
	{
		if (auto* ot = obj->get_object_type())
		{
			Concurrency::ScopedMRSWLockRead lockRead(lock);
			for_every_ref(type, bringItemsTypes)
			{
				if (ot == type)
				{
					return true;
				}
			}
		}
	}
	return false;
}

