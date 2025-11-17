#include "iWMPTool.h"

#include "..\concurrency\scopedMRSWLock.h"
#include "..\io\xml.h"

//

#include "wmp_access.h"
#include "wmp_betweenPoints.h"
#include "wmp_block.h"
#include "wmp_bool.h"
#include "wmp_break.h"
#include "wmp_centre.h"
#include "wmp_chooseByIntValue.h"
#include "wmp_chooseOne.h"
#include "wmp_convertStore.h"
#include "wmp_copyStore.h"
#include "wmp_customTool.h"
#include "wmp_defaultValue.h"
#include "wmp_dropOnto.h"
#include "wmp_expandBy.h"
#include "wmp_flattenToCentre.h"
#include "wmp_float.h"
#include "wmp_get.h"
#include "wmp_getPt.h"
#include "wmp_if.h"
#include "wmp_include.h"
#include "wmp_int.h"
#include "wmp_intersect.h"
#include "wmp_isEmpty.h"
#include "wmp_isNameValid.h"
#include "wmp_length.h"
#include "wmp_lerp.h"
#include "wmp_local.h"
#include "wmp_logicOps.h"
#include "wmp_loopIterator.h"
#include "wmp_mainSettings.h"
#include "wmp_mathComparisons.h"
#include "wmp_mathOps.h"
#include "wmp_mathOpsSimple.h"
#include "wmp_matrix.h"
#include "wmp_mayRestore.h"
#include "wmp_mirror.h"
#include "wmp_name.h"
#include "wmp_normalise.h"
#include "wmp_output.h"
#include "wmp_plane.h"
#include "wmp_point.h"
#include "wmp_pointDirection.h"
#include "wmp_quat.h"
#include "wmp_randSeed.h"
#include "wmp_range.h"
#include "wmp_range2.h"
#include "wmp_range3.h"
#include "wmp_remap.h"
#include "wmp_renameStore.h"
#include "wmp_restore.h"
#include "wmp_rotator3.h"
#include "wmp_sphere.h"
#include "wmp_store.h"
#include "wmp_system.h"
#include "wmp_todo.h"
#include "wmp_transform.h"
#include "wmp_transformOps.h"
#include "wmp_uniqueInt.h"
#include "wmp_vector2.h"
#include "wmp_vector3.h"
#include "wmp_vector4.h"
#include "wmp_vectorInt2.h"
#include "wmp_vectorOps.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace WheresMyPoint;

ITool * ITool::create_from_xml(IO::XML::Node const * _node)
{
	if (!_node)
	{
		return nullptr;
	}
	if (ITool* tool = RegisteredTools::base::create(_node->get_name()))
	{
		if (tool->load_from_xml(_node))
		{
			return tool;
		}
		else
		{
			error_loading_xml(_node, TXT("couldn't load tool \"%S\""), _node->get_name().to_char());
			delete tool;
			return nullptr;
		}
	}
	{
		ITool* createdTool = nullptr;
		bool handled = false;
		RegisteredPrefixedTools::for_every_key([&createdTool, &handled, _node](::String const& _toolPrefix)
			{
				if (_node->get_name().has_prefix(_toolPrefix))
				{
					if ((createdTool = RegisteredPrefixedTools::create(_toolPrefix)))
					{
						handled = true;
						if (createdTool->load_prefixed_from_xml(_node, _toolPrefix))
						{
							// ok
						}
						else
						{
							createdTool = nullptr;
							error_loading_xml(_node, TXT("couldn't load tool \"%S\""), _node->get_name().to_char());
						}
					}

					return handled;
				}
				else
				{
					return false;
				}

			});
		if (handled)
		{
			return createdTool;
		}
	}
	// allow loading custom
	if (ITool* tool = RegisteredTools::create(_node->get_name()))
	{
		if (tool->load_from_xml(_node))
		{
			return tool;
		}
		else
		{
			error_loading_xml(_node, TXT("couldn't load tool \"%S\""), _node->get_name().to_char());
			delete tool;
			return nullptr;
		}
	}
	error_loading_xml(_node, TXT("\"where's my point\" tool \"%S\" not recognised"), _node->get_name().to_char());
	return nullptr;
}

bool ITool::load_from_xml(IO::XML::Node const * _node)
{
	bool result = true;

	sourceLocation = _node? _node->get_location_info() : ::String(TXT("no source?"));

	return result;
}

bool ITool::load_prefixed_from_xml(IO::XML::Node const * _node, ::String const & _prefix)
{
	bool result = true;

	sourceLocation = _node? _node->get_location_info() : ::String(TXT("no source?"));

	return result;
}

//

void RegisteredTools::development_output_all(LogInfoContext& _log)
{
	_log.log(TXT("WheresMyPoint built-in instructions (prefixed)"));
	{
		LOG_INDENT(_log);
		RegisteredPrefixedTools::development_output_all(_log);
	}
	_log.log(TXT(""));
	_log.log(TXT("WheresMyPoint built-in instructions"));
	{
		LOG_INDENT(_log);
		base::development_output_all(_log);
	}
	_log.log(TXT(""));
	_log.log(TXT("custom tools"));
	{
		LOG_INDENT(_log);

		struct AlphabeticalOrder
		{
			RegisteredCustomTool* r;

			AlphabeticalOrder() {}
			explicit AlphabeticalOrder(RegisteredCustomTool* _r) : r(_r) {}

			static int compare(void const* _a, void const* _b)
			{
				AlphabeticalOrder a = *plain_cast<AlphabeticalOrder>(_a);
				AlphabeticalOrder b = *plain_cast<AlphabeticalOrder>(_b);
				return ::String::diff_icase(a.r->name.to_char(), b.r->name.to_char());
			}
		};
		Array<AlphabeticalOrder> alphabeticalOrder;

		{
			Concurrency::ScopedMRSWLockRead lock(registeredCustomToolsLock);
			for_every_ptr(reg, *registeredCustomTools)
			{
				alphabeticalOrder.push_back(AlphabeticalOrder(reg));
			}
		}

		sort(alphabeticalOrder);

		_log.log(TXT("in alphabetical order"));
		LOG_INDENT(_log);
		for_every(ao, alphabeticalOrder)
		{
			_log.log(TXT("%S"), ao->r->name.to_char());
			if (! ao->r->inputParameters.is_empty())
			{	// input
				LOG_INDENT(_log);
				_log.log(TXT("> input"));
				LOG_INDENT(_log);
				for_every(p, ao->r->inputParameters)
				{
					_log.log(TXT("%S %S"), RegisteredType::get_name_of(p->type), p->name.to_char());
				}
			}
			if (!ao->r->outputParameters.is_empty())
			{	// output
				LOG_INDENT(_log);
				_log.log(TXT("> output"));
				LOG_INDENT(_log);
				for_every(p, ao->r->outputParameters)
				{
					_log.log(TXT("%S %S"), RegisteredType::get_name_of(p->type), p->name.to_char());
				}
			}
		}
	}
	_log.log(TXT(""));
}

void RegisteredTools::initialise_static()
{
	base::initialise_static();
	RegisteredPrefixedTools::initialise_static(); 
	{
		Concurrency::ScopedMRSWLockWrite lock(registeredCustomToolsLock);
		an_assert(registeredCustomTools == nullptr);
		registeredCustomTools = new RegisteredCustomToolsArray();
	}

	set_log_development_info([](LogInfoContext& _log, ::String const& _name)
	{
		if (auto* tool = create(_name))
		{
			if (auto* text = tool->development_description())
			{
				if (tstrlen(text) > 0)
				{
					_log.log(TXT("%S"), text);
					_log.log(TXT("")); // and an empty line
				}
			}
			delete tool;
		}
	});

	REGISTER_WHERES_MY_POINT_TOOL_PREFIXED(TXT("'@"), []() { return new RestoreTemp(); });
	REGISTER_WHERES_MY_POINT_TOOL_PREFIXED(TXT("'$"), []() { return new StoreTemp(); });
	REGISTER_WHERES_MY_POINT_TOOL_PREFIXED(TXT("@"), []() { return new Restore(); });
	REGISTER_WHERES_MY_POINT_TOOL_PREFIXED(TXT("$"), []() { return new Store(); });
	REGISTER_WHERES_MY_POINT_TOOL_PREFIXED(TXT("^"), []() { return new Get(); });
	REGISTER_WHERES_MY_POINT_TOOL_PREFIXED(TXT("*"), []() { return new Access(); });

	REGISTER_WHERES_MY_POINT_TOOL(TXT("abs"), [](){ return new Abs(); });
	REGISTER_WHERES_MY_POINT_TOOL(TXT("access"), [](){ return new Access(); });
	REGISTER_WHERES_MY_POINT_TOOL(TXT("add"), [](){ return new Add(); });
	REGISTER_WHERES_MY_POINT_TOOL(TXT("along"), [](){ return new Along(); });
	REGISTER_WHERES_MY_POINT_TOOL(TXT("and"), [](){ return new And(); });
	REGISTER_WHERES_MY_POINT_TOOL(TXT("betweenPoints"), [](){ return new BetweenPoints(); });
	REGISTER_WHERES_MY_POINT_TOOL(TXT("block"), [](){ return new Block(); });
	REGISTER_WHERES_MY_POINT_TOOL(TXT("bool"), [](){ return new Bool(); });
	REGISTER_WHERES_MY_POINT_TOOL(TXT("boolRandom"), []() { return new BoolRandom(); });
	REGISTER_WHERES_MY_POINT_TOOL(TXT("break"), [](){ return new Break(); });
	REGISTER_WHERES_MY_POINT_TOOL(TXT("callCustomTool"), []() { return new CallCustomTool(); });
	REGISTER_WHERES_MY_POINT_TOOL(TXT("callWheresMyPointFunction"), []() { return new CallCustomTool(); });
	REGISTER_WHERES_MY_POINT_TOOL(TXT("ceil"), []() { return new Ceil(); });
	REGISTER_WHERES_MY_POINT_TOOL(TXT("centre"), []() { return new Centre(); });
	REGISTER_WHERES_MY_POINT_TOOL(TXT("chooseByIntValue"), []() { return new ChooseByIntValue(); });
	REGISTER_WHERES_MY_POINT_TOOL(TXT("chooseOne"), []() { return new ChooseOne(); });
	REGISTER_WHERES_MY_POINT_TOOL(TXT("convertStore"), [](){ return new ConvertStore(); });
	REGISTER_WHERES_MY_POINT_TOOL(TXT("copyStore"), [](){ return new CopyStore(); });
	REGISTER_WHERES_MY_POINT_TOOL(TXT("cos"), [](){ return new Cos(); });
	REGISTER_WHERES_MY_POINT_TOOL(TXT("cross"), [](){ return new Cross(); });
	REGISTER_WHERES_MY_POINT_TOOL(TXT("crossProduct"), [](){ return new Cross(); });
	REGISTER_WHERES_MY_POINT_TOOL(TXT("decrease"), []() { return new Decrease(); });
	REGISTER_WHERES_MY_POINT_TOOL(TXT("defaultValue"), []() { return new DefaultValue(); });
	REGISTER_WHERES_MY_POINT_TOOL(TXT("div"), [](){ return new Div(); });
	REGISTER_WHERES_MY_POINT_TOOL(TXT("dot"), [](){ return new Dot(); });
	REGISTER_WHERES_MY_POINT_TOOL(TXT("dotProduct"), [](){ return new Dot(); });
	REGISTER_WHERES_MY_POINT_TOOL(TXT("double"), [](){ return new Double(); });
	REGISTER_WHERES_MY_POINT_TOOL(TXT("dropOnto"), [](){ return new DropOnto(); });
	REGISTER_WHERES_MY_POINT_TOOL(TXT("dropUsing"), []() { return new DropUsing(); });
	REGISTER_WHERES_MY_POINT_TOOL(TXT("equalTo"), []() { return new EqualTo(); });
	REGISTER_WHERES_MY_POINT_TOOL(TXT("error"), []() { return new Error(); });
	REGISTER_WHERES_MY_POINT_TOOL(TXT("expandBy"), [](){ return new ExpandBy(); });
	REGISTER_WHERES_MY_POINT_TOOL(TXT("false"), [](){ return new False(); });
	REGISTER_WHERES_MY_POINT_TOOL(TXT("flattenToCentre"), [](){ return new FlattenToCentre(); });
	REGISTER_WHERES_MY_POINT_TOOL(TXT("float"), [](){ return new Float(); });
	REGISTER_WHERES_MY_POINT_TOOL(TXT("floatRandom"), [](){ return new FloatRandom(); });
	REGISTER_WHERES_MY_POINT_TOOL(TXT("floor"), [](){ return new Floor(); });
	REGISTER_WHERES_MY_POINT_TOOL(TXT("get"), [](){ return new Get(); });
	REGISTER_WHERES_MY_POINT_TOOL(TXT("getPt"), [](){ return new GetPt(); });
	REGISTER_WHERES_MY_POINT_TOOL(TXT("greaterOrEqual"), [](){ return new GreaterOrEqual(); });
	REGISTER_WHERES_MY_POINT_TOOL(TXT("greaterThan"), [](){ return new GreaterThan(); });
	REGISTER_WHERES_MY_POINT_TOOL(TXT("half"), [](){ return new Half(); });
	REGISTER_WHERES_MY_POINT_TOOL(TXT("if"), [](){ return new If(); });
	REGISTER_WHERES_MY_POINT_TOOL(TXT("ifFalse"), []() { return new IfFalse(); });
	REGISTER_WHERES_MY_POINT_TOOL(TXT("ifIsEmpty"), []() { return new IfIsEmpty(); });
	REGISTER_WHERES_MY_POINT_TOOL(TXT("ifIsNotEmpty"), []() { return new IfIsNotEmpty(); });
	REGISTER_WHERES_MY_POINT_TOOL(TXT("ifTrue"), []() { return new IfTrue(); });
	REGISTER_WHERES_MY_POINT_TOOL(TXT("include"), [](){ return new Include(); });
	REGISTER_WHERES_MY_POINT_TOOL(TXT("increase"), []() { return new Increase(); });
	REGISTER_WHERES_MY_POINT_TOOL(TXT("int"), [](){ return new Int(); });
	REGISTER_WHERES_MY_POINT_TOOL(TXT("intersect"), [](){ return new Intersect(); });
	REGISTER_WHERES_MY_POINT_TOOL(TXT("intRandom"), [](){ return new IntRandom(); });
	REGISTER_WHERES_MY_POINT_TOOL(TXT("invert"), []() { return new Invert(); });
	REGISTER_WHERES_MY_POINT_TOOL(TXT("isEmpty"), []() { return new IsEmpty(); });
	REGISTER_WHERES_MY_POINT_TOOL(TXT("isNameValid"), [](){ return new IsNameValid(); });
	REGISTER_WHERES_MY_POINT_TOOL(TXT("isZero"), []() { return new IsZero(); });
	REGISTER_WHERES_MY_POINT_TOOL(TXT("isZeroVector"), [](){ return new IsZeroVector(); });
	REGISTER_WHERES_MY_POINT_TOOL(TXT("length"), [](){ return new Length(); });
	REGISTER_WHERES_MY_POINT_TOOL(TXT("length2d"), [](){ return new Length2D(); });
	REGISTER_WHERES_MY_POINT_TOOL(TXT("lerp"), [](){ return new Lerp(); });
	REGISTER_WHERES_MY_POINT_TOOL(TXT("lessOrEqual"), [](){ return new LessOrEqual(); });
	REGISTER_WHERES_MY_POINT_TOOL(TXT("lessThan"), [](){ return new LessThan(); });
	REGISTER_WHERES_MY_POINT_TOOL(TXT("local"), [](){ return new Local(); });
	REGISTER_WHERES_MY_POINT_TOOL(TXT("locationToWorldOf"), [](){ return new LocationToWorldOf(); });
	REGISTER_WHERES_MY_POINT_TOOL(TXT("locationToLocalOf"), [](){ return new LocationToLocalOf(); });
	REGISTER_WHERES_MY_POINT_TOOL(TXT("log"), [](){ return new Log(); });
	REGISTER_WHERES_MY_POINT_TOOL(TXT("loopIterator"), [](){ return new LoopIterator(); });
	REGISTER_WHERES_MY_POINT_TOOL(TXT("mainSettings"), [](){ return new MainSettings(); });
	REGISTER_WHERES_MY_POINT_TOOL(TXT("max"), [](){ return new Max(); });
	REGISTER_WHERES_MY_POINT_TOOL(TXT("mayRestore"), [](){ return new MayRestore(); });
	REGISTER_WHERES_MY_POINT_TOOL(TXT("mayRestoreGlobal"), [](){ return new MayRestoreGlobal(); });
	REGISTER_WHERES_MY_POINT_TOOL(TXT("min"), [](){ return new Min(); });
	REGISTER_WHERES_MY_POINT_TOOL(TXT("mirror"), [](){ return new Mirror(); });
	REGISTER_WHERES_MY_POINT_TOOL(TXT("mod"), [](){ return new Mod(); });
	REGISTER_WHERES_MY_POINT_TOOL(TXT("mul"), [](){ return new Mul(); });
	REGISTER_WHERES_MY_POINT_TOOL(TXT("name"), [](){ return new WheresMyPoint::Name(); });
	REGISTER_WHERES_MY_POINT_TOOL(TXT("negate"), [](){ return new Negate(); });
	REGISTER_WHERES_MY_POINT_TOOL(TXT("normal"), []() { return new Normalise(); }); // synonym
	REGISTER_WHERES_MY_POINT_TOOL(TXT("normalise"), [](){ return new Normalise(); });
	REGISTER_WHERES_MY_POINT_TOOL(TXT("not"), []() { return new Not(); }); // synonym
	REGISTER_WHERES_MY_POINT_TOOL(TXT("or"), [](){ return new Or(); });
	REGISTER_WHERES_MY_POINT_TOOL(TXT("output"), [](){ return new Output(); });
	REGISTER_WHERES_MY_POINT_TOOL(TXT("plane"), [](){ return new Plane(); });
	REGISTER_WHERES_MY_POINT_TOOL(TXT("point"), [](){ return new Point(); });
	REGISTER_WHERES_MY_POINT_TOOL(TXT("pointDirection"), [](){ return new PointDirection(); });
	REGISTER_WHERES_MY_POINT_TOOL(TXT("quat"), []() { return new Quat(); });
	REGISTER_WHERES_MY_POINT_TOOL(TXT("quatToLocalOf"), []() { return new QuatToLocalOf(); });
	REGISTER_WHERES_MY_POINT_TOOL(TXT("quatToWorldOf"), []() { return new QuatToWorldOf(); });
	REGISTER_WHERES_MY_POINT_TOOL(TXT("randSeed"), [](){ return new RandSeed(); });
	REGISTER_WHERES_MY_POINT_TOOL(TXT("range"), [](){ return new Range(); });
	REGISTER_WHERES_MY_POINT_TOOL(TXT("range2"), [](){ return new Range2(); });
	REGISTER_WHERES_MY_POINT_TOOL(TXT("range3"), [](){ return new Range3(); });
	REGISTER_WHERES_MY_POINT_TOOL(TXT("rawMatrix44"), [](){ return new RawMatrix44(); });
	REGISTER_WHERES_MY_POINT_TOOL(TXT("remap"), [](){ return new Remap(); });
	REGISTER_WHERES_MY_POINT_TOOL(TXT("renameStore"), []() { return new RenameStore(); });
	REGISTER_WHERES_MY_POINT_TOOL(TXT("restore"), [](){ return new Restore(); });
	REGISTER_WHERES_MY_POINT_TOOL(TXT("restoreGlobal"), [](){ return new RestoreGlobal(); });
	REGISTER_WHERES_MY_POINT_TOOL(TXT("restoreTemp"), [](){ return new RestoreTemp(); });
	REGISTER_WHERES_MY_POINT_TOOL(TXT("rotate90Right"), []() { return new Rotate90Right(); });
	REGISTER_WHERES_MY_POINT_TOOL(TXT("rotator3"), []() { return new Rotator3(); });
	REGISTER_WHERES_MY_POINT_TOOL(TXT("round"), []() { return new Round(); });
	REGISTER_WHERES_MY_POINT_TOOL(TXT("sign"), [](){ return new Sign(); });
	REGISTER_WHERES_MY_POINT_TOOL(TXT("sin"), []() { return new Sin(); });
	REGISTER_WHERES_MY_POINT_TOOL(TXT("sphere"), []() { return new Sphere(); });
	REGISTER_WHERES_MY_POINT_TOOL(TXT("sqr"), []() { return new Sqr(); });
	REGISTER_WHERES_MY_POINT_TOOL(TXT("sqrt"), []() { return new Sqrt(); });
	REGISTER_WHERES_MY_POINT_TOOL(TXT("store"), [](){ return new Store(); });
	REGISTER_WHERES_MY_POINT_TOOL(TXT("storeGlobal"), [](){ return new StoreGlobal(); });
	REGISTER_WHERES_MY_POINT_TOOL(TXT("storeSwap"), [](){ return new StoreSwap(); });
	REGISTER_WHERES_MY_POINT_TOOL(TXT("storeTemp"), [](){ return new StoreTemp(); });
	REGISTER_WHERES_MY_POINT_TOOL(TXT("sub"), [](){ return new Sub(); });
	REGISTER_WHERES_MY_POINT_TOOL(TXT("system"), [](){ return new System(); });
	REGISTER_WHERES_MY_POINT_TOOL(TXT("tan"), [](){ return new Tan(); });
	REGISTER_WHERES_MY_POINT_TOOL(TXT("todo"), [](){ return new ToDo(); });
	REGISTER_WHERES_MY_POINT_TOOL(TXT("toFloat"), [](){ return new ToFloat(); });
	REGISTER_WHERES_MY_POINT_TOOL(TXT("toInt"), [](){ return new ToInt(); });
	REGISTER_WHERES_MY_POINT_TOOL(TXT("toLocalOf"), [](){ return new ToLocalOf(); });
	REGISTER_WHERES_MY_POINT_TOOL(TXT("toQuat"), []() { return new ToQuat(); });
	REGISTER_WHERES_MY_POINT_TOOL(TXT("toTransform"), [](){ return new ToTransform(); });
	REGISTER_WHERES_MY_POINT_TOOL(TXT("toWorldOf"), [](){ return new ToWorldOf(); });
	REGISTER_WHERES_MY_POINT_TOOL(TXT("toVector2"), []() { return new ToVector2(); });
	REGISTER_WHERES_MY_POINT_TOOL(TXT("toVector3"), []() { return new ToVector3(); });
	REGISTER_WHERES_MY_POINT_TOOL(TXT("toVector4"), []() { return new ToVector4(); });
	REGISTER_WHERES_MY_POINT_TOOL(TXT("toVectorInt2"), []() { return new ToVectorInt2(); });
	REGISTER_WHERES_MY_POINT_TOOL(TXT("transform"), [](){ return new Transform(); });
	REGISTER_WHERES_MY_POINT_TOOL(TXT("true"), [](){ return new True(); });
	REGISTER_WHERES_MY_POINT_TOOL(TXT("uniqueInt"), [](){ return new UniqueInt(); });
	REGISTER_WHERES_MY_POINT_TOOL(TXT("vector2"), [](){ return new Vector2(); });
	REGISTER_WHERES_MY_POINT_TOOL(TXT("vector3"), [](){ return new Vector3(); });
	REGISTER_WHERES_MY_POINT_TOOL(TXT("vector4"), [](){ return new Vector4(); });
	REGISTER_WHERES_MY_POINT_TOOL(TXT("vectorInt2"), []() { return new VectorInt2(); });
	REGISTER_WHERES_MY_POINT_TOOL(TXT("vectorToWorldOf"), [](){ return new VectorToWorldOf(); });
	REGISTER_WHERES_MY_POINT_TOOL(TXT("vectorToLocalOf"), [](){ return new VectorToLocalOf(); });
	REGISTER_WHERES_MY_POINT_TOOL(TXT("warn"), []() { return new Warning(); });
	REGISTER_WHERES_MY_POINT_TOOL(TXT("warning"), []() { return new Warning(); });
	REGISTER_WHERES_MY_POINT_TOOL(TXT("xor"), []() { return new Xor(); });
}

Array<RegisteredCustomTool*>* RegisteredTools::registeredCustomTools = nullptr;
Concurrency::MRSWLock RegisteredTools::registeredCustomToolsLock = Concurrency::MRSWLock(TXT("RegisteredTools::registeredCustomTools"));

void RegisteredTools::close_static()
{
	Concurrency::ScopedMRSWLockWrite lock(registeredCustomToolsLock);

	an_assert(registeredCustomTools != nullptr);
	for_every(registeredCustomTool, *registeredCustomTools)
	{
		delete_and_clear(*registeredCustomTool);
	}
	delete_and_clear(registeredCustomTools);
	RegisteredPrefixedTools::close_static();
	base::close_static();
}

ITool* RegisteredTools::create(::String const & _name)
{
	if (ITool* tool = base::create(_name))
	{
		return tool;
	}
	// assume custom tool
	return new CustomTool();
}

RegisteredCustomTool const * RegisteredTools::find_custom(::Name const & _name)
{
	Concurrency::ScopedMRSWLockRead lock(registeredCustomToolsLock);

	an_assert(registeredCustomTools != nullptr);
	for_every_ptr(registeredCustomTool, *registeredCustomTools)
	{
		if (registeredCustomTool->name == _name)
		{
			return registeredCustomTool;
		}
	}
	error(TXT("could not find registered custom tool \"%S\""), _name.to_char());
	return nullptr;
}

bool RegisteredTools::register_custom_from_xml(IO::XML::Node const * _node)
{
	Concurrency::ScopedMRSWLockWrite lock(registeredCustomToolsLock);

	::Name toolName = _node->get_name_attribute(TXT("name"));
	an_assert(registeredCustomTools != nullptr);
	for_every_ptr(registeredCustomTool, *registeredCustomTools)
	{
		if (registeredCustomTool->name == toolName)
		{
			registeredCustomTool->reset();
			return registeredCustomTool->load_from_xml(_node);
		}
	}

	RegisteredCustomTool* tool = new RegisteredCustomTool();
	if (tool->load_from_xml(_node))
	{
		registeredCustomTools->push_back(tool);
		return true;
	}

	error_loading_xml(_node, TXT("problem loading custom tool"));
	return false;
}

//

ToolSet::~ToolSet()
{
	clear();
}

void ToolSet::clear()
{
	for_every(tool, tools)
	{
		delete_and_clear(*tool);
	}
	tools.clear();
}

bool ToolSet::load_from_xml(IO::XML::Node const * _node)
{
	bool result = true;

	if (_node)
	{
		for_every(child, _node->all_children())
		{
			if (ITool* tool = ITool::create_from_xml(child))
			{
				if (limit > 0 && tools.get_size() >= limit)
				{
					delete_and_clear(tool);
					error_loading_xml(child, TXT("exceeded limit of %i for tool set"), limit);
					break;
				}
				else
				{
					tools.push_back(tool);
				}
			}
		}
	}

	return result;
}

bool ToolSet::update(REF_ Value & _value, Context & _context) const
{
	bool result = true;
	for_every_ptr(tool, tools)
	{
		if (!tool->update(_value, _context))
		{
			error_processing_wmp_tool(tool, TXT("invalid update"));
			result = false;
		}
		//an_assert(result);
	}
	return result;
}

bool ToolSet::update(Context & _context) const
{
	Value value;
	return update(REF_ value, _context);
}

//

bool RegisteredCustomTool::Parameter::load_from_xml(IO::XML::Node const * _node)
{
	if (auto * attr = _node->get_attribute(TXT("name")))
	{
		name = attr->get_as_name();
		if (!name.is_valid())
		{
			error_loading_xml(_node, TXT("parameter name invalid"));
			return false;
		}
	}
	else
	{
		error_loading_xml(_node, TXT("no parameter name"));
		return false;
	}
	if (auto * attr = _node->get_attribute(TXT("type")))
	{
		type = RegisteredType::get_type_id_by_name(attr->get_as_string().to_char());
		if (type == RegisteredType::none())
		{
			error_loading_xml(_node, TXT("could not recognise parameter type \"%S\" for parameter \"%S\""), attr->get_as_string().to_char(), name.to_char());
			return false;
		}
	}
	else
	{
		error_loading_xml(_node, TXT("no type for parameter \"%S\""), name.to_char());
		return false;
	}
	return true;
}

//

bool RegisteredCustomTool::load_from_xml(IO::XML::Node const * _node)
{
	bool result = true;

	name = _node->get_name_attribute(TXT("name"));
	if (!name.is_valid())
	{
		error_loading_xml(_node, TXT("no name provided for custom tool for where's my point"));
		return false;
	}

	if (_node)
	{
		for_every(child, _node->all_children())
		{
			if (child->get_name() == TXT("inputParameter"))
			{
				Parameter parameter;
				if (parameter.load_from_xml(child))
				{
					inputParameters.push_back(parameter);
				}
				else
				{
					result = false;
				}
			}
			else if (child->get_name() == TXT("outputParameter"))
			{
				Parameter parameter;
				if (parameter.load_from_xml(child))
				{
					outputParameters.push_back(parameter);
				}
				else
				{
					result = false;
				}
			}
			else if (ITool* tool = ITool::create_from_xml(child))
			{
				tools.tools.push_back(tool);
			}
			else
			{
				error_loading_xml(child, TXT("problem loading tool \"%S\""), child->get_name().to_char());
				result = false;
			}
		}
	}

	return result;
}
