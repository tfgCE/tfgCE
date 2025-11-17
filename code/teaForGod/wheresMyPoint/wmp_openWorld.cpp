#include "wmp_openWorld.h"

#include "..\pilgrimage\pilgrimageInstanceOpenWorld.h"

#include "..\..\framework\framework.h"
#include "..\..\framework\meshGen\meshGenGenerationContext.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace TeaForGodEmperor;

//

// variables
DEFINE_STATIC_NAME(openWorldCellContext);

//

bool IsOpenWorld::update(REF_ WheresMyPoint::Value & _value, WheresMyPoint::Context & _context) const
{
	bool result = true;

	_value.set<bool>(PilgrimageInstanceOpenWorld::get() != nullptr);

	return result;
}

//

bool OpenWorld::load_from_xml(IO::XML::Node const* _node)
{
	bool result = base::load_from_xml(_node);

	getValue = _node->get_name_attribute(TXT("get"), getValue);
	outputInfo = _node->get_name_attribute(TXT("output"), outputInfo);

	return result;
}

bool OpenWorld::update(REF_ WheresMyPoint::Value & _value, WheresMyPoint::Context & _context) const
{
	bool result = true;

	auto* piow = PilgrimageInstanceOpenWorld::get();

	if (getValue == TXT("cellSize"))
	{
		_value.set<float>(piow ? piow->get_cell_size() :
			(Framework::is_preview_game()? 60.0f : 0.0f));
	}
	if (getValue == TXT("cellSizeInner"))
	{
		_value.set<float>(piow ? piow->get_cell_size_inner() :
			(Framework::is_preview_game() ? 30.0f : 0.0f));
	}
	if (getValue == TXT("dirExitsCount"))
	{
		_value.set<int>(piow ? piow->get_dir_exits_count_for_cell_context() :
			(Framework::is_preview_game() ? 4 : 0));
	}
	if (getValue == TXT("pilgrimageDevicesCount"))
	{
		_value.set<int>(piow ? piow->get_pilgrimage_devices_count_for_cell_context() :
			(0));
	}
	if (getValue == TXT("cellContext"))
	{
		if (piow && piow->is_cell_context_set())
		{
			_value.set<VectorInt2>(piow->get_cell_context());
		}
		else
		{
			if (!_context.get_owner()->restore_value_for_wheres_my_point(NAME(openWorldCellContext), _value))
			{
				error_processing_wmp_tool(this, TXT("couldn't read open world cell context"));
			}
		}
	}

	if (outputInfo == TXT("cellContext"))
	{
		if (piow)
		{
			output(TXT("cell context: %S"), piow->get_cell_context().to_string().to_char());
		}
		else
		{
			output(TXT("no open world"));
		}
	}

	return result;
}

//

bool UseOpenWorldCellRandomSeed::update(REF_ WheresMyPoint::Value& _value, WheresMyPoint::Context& _context) const
{
	bool result = true;

	if (auto* piow = PilgrimageInstanceOpenWorld::get())
	{
		Random::Generator const useRG = piow->get_random_seed_for_cell_context();
		_context.access_random_generator() = useRG;
		if (auto* ei = fast_cast<Framework::MeshGeneration::ElementInstance>(_context.get_owner()))
		{
			ei->access_context().use_random_generator(useRG);
		}
		if (auto* gc = fast_cast<Framework::MeshGeneration::GenerationContext>(_context.get_owner()))
		{
			gc->use_random_generator(useRG);
		}
	}

	return result;
}

//

bool IsSameDependentOnForOpenWorldCell::load_from_xml(IO::XML::Node const* _node)
{
	bool result = base::load_from_xml(_node);

	localDir = DirFourClockwise::parse(_node->get_string_attribute(TXT("localDir")));

	return result;
}

bool IsSameDependentOnForOpenWorldCell::update(REF_ WheresMyPoint::Value& _value, WheresMyPoint::Context& _context) const
{
	bool result = true;

	if (auto* piow = PilgrimageInstanceOpenWorld::get())
	{
		_value.set<bool>(piow->has_same_depends_on_in_local_dir_for_cell_context(localDir));
	}
	else
	{
		_value.set<bool>(false);
	}

	return result;
}
