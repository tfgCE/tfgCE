#include "acp_lightning.h"

#include "..\..\appearanceControllerPoseContext.h"
#include "..\..\appearanceControllers.h"

#include "..\..\..\library\library.h"
#include "..\..\..\meshGen\meshGenGenerationContext.h"
#include "..\..\..\meshGen\meshGenParamImpl.inl"
#include "..\..\..\module\moduleAppearance.h"

#include "..\..\..\..\core\mesh\pose.h"

#include "..\..\..\..\core\debug\debugRenderer.h"
#include "..\..\..\..\core\performance\performanceUtils.h"

using namespace Framework;
using namespace AppearanceControllersLib;
using namespace Particles;

//

DEFINE_STATIC_NAME(lightning);

// default param names
DEFINE_STATIC_NAME(segmentName); // name0suffix
DEFINE_STATIC_NAME(segmentSuffixStart);
DEFINE_STATIC_NAME(segmentSuffixEnd);

// default values for params
DEFINE_STATIC_NAME(segment);
DEFINE_STATIC_NAME(start);
DEFINE_STATIC_NAME(end);

//

REGISTER_FOR_FAST_CAST(Lightning);

Lightning::Lightning(LightningData const * _data)
: base(_data)
, lightningData(_data)
{
	lengthVar = lightningData->lengthVariable.is_set()? lightningData->lengthVariable.get() : lengthVar;
	arcVar = lightningData->arcVariable.is_set() ? lightningData->arcVariable.get() : arcVar;
	radiusVar = lightningData->radiusVariable.is_set() ? lightningData->radiusVariable.get() : radiusVar;
	radiusRangeVar = lightningData->radiusVariable.is_set() ? lightningData->radiusVariable.get() : radiusRangeVar;
}

Lightning::~Lightning()
{
}

void Lightning::initialise(ModuleAppearance* _owner)
{
	base::initialise(_owner);

	auto* skeleton = get_owner()->get_skeleton()? get_owner()->get_skeleton()->get_skeleton() : nullptr;
	if (!skeleton)
	{
#ifdef AN_DEVELOPMENT_OR_PROFILER
		if (Framework::Library::may_ignore_missing_references())
		{
			return;
		}
#endif
		error(TXT("no skeleton"));
	}

	Name segName = NAME(segment);
	Name segSuffixStart = NAME(start);
	Name segSuffixEnd = NAME(end);

	if (lightningData->segmentName.is_value_set())
	{
		segName = lightningData->segmentName.get();
	}
	if (lightningData->segmentSuffixStart.is_value_set())
	{
		segSuffixStart = lightningData->segmentSuffixStart.get();
	}
	if (lightningData->segmentSuffixEnd.is_value_set())
	{
		segSuffixEnd = lightningData->segmentSuffixEnd.get();
	}

	for (int i = 0;; ++ i)
	{
		String startBoneString = String::printf(TXT("%S%i%S"), segName.to_char(), i, segSuffixStart.to_char());
		String endBoneString = String::printf(TXT("%S%i%S"), segName.to_char(), i, segSuffixEnd.to_char());
		Meshes::BoneID startBone;
		Meshes::BoneID endBone;
		
		startBone.look_up(skeleton, AllowToFail, &startBoneString);
		endBone.look_up(skeleton, AllowToFail, &endBoneString);

		if (startBone.is_valid() && endBone.is_valid())
		{
			Segment segment;
			segment.startBone = startBone;
			segment.endBone = endBone;
			{
				int parentIdx = skeleton->get_parent_of(segment.startBone.get_index());
				if (parentIdx != NONE)
				{
					segment.startBoneParent.set_name(skeleton->get_bones()[parentIdx].get_name());
					segment.startBoneParent.look_up(skeleton);
				}
				else
				{
					segment.startBoneParent.set_name(Name::invalid());
				}
			}
			{
				int parentIdx = skeleton->get_parent_of(segment.endBone.get_index());
				if (parentIdx != NONE)
				{
					segment.endBoneParent.set_name(skeleton->get_bones()[parentIdx].get_name());
					segment.endBoneParent.look_up(skeleton);
				}
				else
				{
					segment.endBoneParent.set_name(Name::invalid());
				}
			}
			segments.push_back(segment);
		}
		else
		{
			break;
		}
	}

	nodes.set_size(segments.get_size() + 1);
	if (!segments.is_empty())
	{
		float invSegCount = 1.0f / (float)(segments.get_size());
		for_every(node, nodes)
		{
			node->t = clamp(((float)for_everys_index(node)) * invSegCount, 0.0f, 1.0f);
		}
	}

	auto & varStorage = get_owner()->access_controllers_variable_storage();
	lengthVar.look_up<float>(varStorage);
	arcVar.look_up<float>(varStorage);
	radiusVar.look_up<float>(varStorage);
	radiusRangeVar.look_up<Range>(varStorage);

	if (lengthVar.is_valid() ||
		lightningData->length.is_value_set())
	{
		mode = Mode::Arc;
	}
	else if (radiusVar.is_valid() ||
			 radiusRangeVar.is_valid() ||
			 lightningData->radius.is_value_set())
	{
		mode = Mode::Sphere;
	}
	else
	{
		error(TXT("no valid mode for lightning"));
	}
}

void Lightning::activate()
{
	base::activate();

	segmentsSet = false;

	if (mode == Mode::Arc)
	{
		lengthFromData = 0.0f;
		if (lightningData->length.is_value_set())
		{
			lengthFromData = rg.get(lightningData->length.get());
		}
	}
	if (mode == Mode::Sphere)
	{
		radiusGrowth = 0.0f;
		if (lightningData->radiusGrowthSpeed.is_value_set() && ! lightningData->radiusGrowthSpeed.get().is_empty())
		{
			radiusGrowthSpeed = rg.get(lightningData->radiusGrowthSpeed.get());
		}
		else
		{
			radiusGrowthSpeed = 0.0f;
		}
	}
}

void Lightning::advance_and_adjust_preliminary_pose(REF_ AppearanceControllerPoseContext & _context)
{
	base::advance_and_adjust_preliminary_pose(_context);
}

void Lightning::calculate_final_pose(REF_ AppearanceControllerPoseContext & _context)
{
	base::calculate_final_pose(_context);

	if (! get_active())
	{
		return;
	}

#ifdef MEASURE_PERFORMANCE_APPEARANCE_CONTROLLERS_DETAILED
	MEASURE_PERFORMANCE(sway_finalPose);
#endif

	auto const * mesh = get_owner()->get_mesh();

	if (!mesh)
	{
		return;
	}

	// update base values
	bool reset = false;
	// time advance
	if (has_just_activated() || !segmentsSet)
	{
		reset = true;
	}

	if (! lightningData->resetInterval.is_empty())
	{
		timeToReset -= _context.get_delta_time();
		if (timeToReset < 0.0f)
		{
			reset = true;
			timeToReset = rg.get(lightningData->resetInterval);
		}
	}

	switch (mode)
	{
	case Mode::Arc:
	{
		float currLength = lengthVar.is_valid()? lengthVar.get<float>() : lengthFromData;
		if (currLength == 0.0f)
		{
			currLength = 1.0f;
		}

		float currArc = arcVar.is_valid() ? arcVar.get<float>() : 0.0f;

		if (reset)
		{
			length = currLength;
			arc = currArc;
			nodes.get_first().location = Vector3::zero;
			nodes.get_first().velocity = Vector3::zero;
			nodes.get_last().location = Vector3::yAxis * currLength;
			nodes.get_last().velocity = Vector3::zero;
			if (nodes.get_size() > 2)
			{
				Node* a = &nodes[0];
				Node* b = &nodes[nodes.get_size() - 1];

				float segmentLength = length / (float)segments.get_size();
				float offDist = segmentLength * 0.35f;
				bool updateA = true;
				while (true)
				{
					Node* u = updateA ? a + 1 : b - 1;
					if (u == a || u == b)
					{
						break;
					}

					u->location = (a->location + b->location) * 0.5f;
					u->location.x += rg.get_float(-offDist, offDist);
					u->location.z += rg.get_float(-offDist, offDist);
					u->location.y = (float)(u - &nodes.get_first()) * segmentLength;

					if (updateA)
					{
						++a;
					}
					else
					{
						--b;
					}
					updateA = !updateA;
				}
			}
			segmentsSet = true;
		}
		if (lightningData->updateLengthAndArc)
		{
			if (length != currLength)
			{
				nodes.get_first().location = Vector3::zero;
				nodes.get_first().velocity = Vector3::zero;
				nodes.get_last().location = Vector3::yAxis * currLength;
				nodes.get_last().velocity = Vector3::zero;

				float scaleY = length / currLength;
				for_every(node, nodes)
				{
					node->location.y *= scaleY;
				}
				length = currLength;
			}
			arc = currArc;
		}
		{
			BezierCurve<Vector3> arcCurve;
			arcCurve.p0 = nodes.get_first().location;
			arcCurve.p3 = nodes.get_last().location;
			Vector3 p03 = arcCurve.p3 - arcCurve.p0;
			float arcDirLength = p03.length() * (0.33f + arc * 0.5f);
			Vector3 p01 = Rotator3(arc * 90.0f, 0.0f, 0.0f).get_forward() * arcDirLength;
			arcCurve.p1 = arcCurve.p0 + p01;
			arcCurve.p2 = arcCurve.p3 + p01 * Vector3(1.0f, -1.0f, 1.0f);
			for_every(node, nodes)
			{
				Vector3 right = Vector3::xAxis;
				Vector3 forward = arcCurve.calculate_tangent_at(node->t).normal();
				Vector3 up = Vector3::cross(right, forward).normal();
				Vector3 baseLoc = p03 * node->t;
				Vector3 relLoc = node->location - baseLoc;
				node->finalLoc = look_matrix(arcCurve.calculate_at(node->t), forward, up).location_to_world(relLoc);
			}
		}
	} break;
	case Mode::Sphere:
	{
		radiusGrowth += radiusGrowthSpeed * _context.get_delta_time();
		if (reset)
		{
			Range radius = radiusRangeVar.is_valid()? radiusRangeVar.get<Range>() : lightningData->radius.get();
			if (radius.is_empty() || radius.is_zero())
			{
				radius = Range(radiusVar.get<float>());
			}
			Range segmentLength = radius * magic_number 0.6f; // pre growth
			radius += radiusGrowth;
			Vector3 loc = Vector3(rg.get_float(-1.0f, 1.0f), rg.get_float(-1.0f, 1.0f), rg.get_float(-1.0f, 1.0f)).normal() * rg.get(radius);
			Vector3 dir = Vector3::zero;
			nodes.get_first().location = loc;
			nodes.get_first().velocity = Vector3::zero;
			for_range(int, i, 1, nodes.get_size() - 1)
			{
				Node & n = nodes[i];
				float const useStraightDir = 0.3f;
				int triesLeft = 10;
				while (triesLeft)
				{
					if (dir.is_zero())
					{
						n.location = loc + Vector3(rg.get_float(-1.0f, 1.0f), rg.get_float(-1.0f, 1.0f), rg.get_float(-1.0f, 1.0f)).normal() * rg.get(segmentLength);
					}
					else
					{
						n.location = loc + (dir * useStraightDir + (1.0f - useStraightDir) * dir * Vector3(rg.get_float(-1.0f, 1.0f), rg.get_float(-1.0f, 1.0f), rg.get_float(-1.0f, 1.0f)).normal()).normal() * rg.get(segmentLength);
					}
					n.location = n.location.normal() * rg.get(radius);
					float dot = Vector3::dot(n.location.normal(), loc.normal());
					if (dot < 0.999f)
					{
						break;
					}
					--triesLeft;
				}
				dir = n.location - loc;
				loc = n.location;
			}
			segmentsSet = true;
		}
		{
			for_every(node, nodes)
			{
				node->finalLoc = node->location;
			}
		}
	} break;
	}


	Meshes::Pose * poseLS = _context.access_pose_LS();

	for_every(segment, segments)
	{
		Transform sStartBoneMS = poseLS->get_bone_ms_from_ls(segment->startBone.get_index());
		Transform sEndBoneMS = poseLS->get_bone_ms_from_ls(segment->endBone.get_index());
		Node const & nStart = nodes[for_everys_index(segment)];
		Node const & nEnd = nodes[for_everys_index(segment) + 1];
		Vector3 upDir;
		switch (mode)
		{
		case Mode::Arc: upDir = Vector3::zAxis; break;
		case Mode::Sphere: upDir = nStart.location.normal(); upDir = upDir.is_zero()? Vector3::zAxis : upDir; break;
		}
		sStartBoneMS = look_at_matrix(nStart.finalLoc, nEnd.finalLoc, upDir).to_transform();
		sEndBoneMS = look_at_matrix(nEnd.finalLoc, nStart.finalLoc, upDir).to_transform();

		Transform sStartBoneLS = segment->startBoneParent.is_valid()? poseLS->get_bone_ms_from_ls(segment->startBoneParent.get_index()).to_local(sStartBoneMS) : sStartBoneMS;
		Transform sEndBoneLS = segment->endBoneParent.is_valid() ? poseLS->get_bone_ms_from_ls(segment->endBoneParent.get_index()).to_local(sEndBoneMS) : sEndBoneMS;
		poseLS->set_bone(segment->startBone.get_index(), sStartBoneLS);
		poseLS->set_bone(segment->endBone.get_index(), sEndBoneLS);
	}
}

void Lightning::get_info_for_auto_processing_order(OUT_ ArrayStack<int> & dependsOnBones, OUT_ ArrayStack<int> & dependsOnParentBones, OUT_ ArrayStack<int> & usesBones, OUT_ ArrayStack<int> & providesBones,
	OUT_ ArrayStack<Name> & dependsOnVariables, OUT_ ArrayStack<Name>& providesVariables, OUT_ ArrayStack<Name>& dependsOnControllers, OUT_ ArrayStack<Name>& providesControllers) const
{
	base::get_info_for_auto_processing_order(OUT_ dependsOnBones, OUT_ dependsOnParentBones, OUT_ usesBones, OUT_ providesBones,
		OUT_ dependsOnVariables, OUT_ providesVariables, OUT_ dependsOnControllers, OUT_ providesControllers);

	for_every(segment, segments)
	{
		dependsOnParentBones.push_back(segment->startBone.get_index());
		providesBones.push_back(segment->startBone.get_index());
		dependsOnParentBones.push_back(segment->endBone.get_index());
		providesBones.push_back(segment->endBone.get_index());
	}
}

//

REGISTER_FOR_FAST_CAST(LightningData);

AppearanceControllerData* LightningData::create_data()
{
	return new LightningData();
}

void LightningData::register_itself()
{
	AppearanceControllers::register_appearance_controller(NAME(lightning), create_data);
}

LightningData::LightningData()
{
	segmentName.set_param(NAME(segmentName));
	segmentSuffixStart.set_param(NAME(segmentSuffixStart));
	segmentSuffixEnd.set_param(NAME(segmentSuffixEnd));
}

bool LightningData::load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc)
{
	bool result = base::load_from_xml(_node, _lc);

	result &= segmentName.load_from_xml(_node, TXT("segmentName"));
	result &= segmentSuffixStart.load_from_xml(_node, TXT("segmentSuffixStart"));
	result &= segmentSuffixEnd.load_from_xml(_node, TXT("segmentSuffixEnd"));

	result &= length.load_from_xml(_node, TXT("length"));
	result &= lengthVariable.load_from_xml(_node, TXT("lengthVarID"));
	result &= arcVariable.load_from_xml(_node, TXT("arcVarID"));

	result &= radius.load_from_xml(_node, TXT("radius"));
	result &= radiusVariable.load_from_xml(_node, TXT("radiusVarID"));	
	result &= radiusGrowthSpeed.load_from_xml(_node, TXT("radiusGrowthSpeed"));

	result &= resetInterval.load_from_xml(_node, TXT("resetInterval"));
	updateLengthAndArc = _node->get_bool_attribute_or_from_child_presence(TXT("updateLengthAndArc"), updateLengthAndArc);

	return result;
}

bool LightningData::prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext)
{
	bool result = base::prepare_for_game(_library, _pfgContext);

	return result;
}

AppearanceControllerData* LightningData::create_copy() const
{
	LightningData* copy = new LightningData();
	*copy = *this;
	return copy;
}

AppearanceController* LightningData::create_controller()
{
	return new Lightning(this);
}

void LightningData::apply_mesh_gen_params(MeshGeneration::GenerationContext const & _context)
{
	base::apply_mesh_gen_params(_context);

	segmentName.fill_value_with(_context, AllowToFail);
	segmentSuffixStart.fill_value_with(_context, AllowToFail);
	segmentSuffixEnd.fill_value_with(_context, AllowToFail);

	length.fill_value_with(_context);
	lengthVariable.fill_value_with(_context);
	arcVariable.fill_value_with(_context);

	// Sphere
	radius.fill_value_with(_context);
	radiusVariable.fill_value_with(_context);
	radiusGrowthSpeed.fill_value_with(_context);
}

void LightningData::apply_transform(Matrix44 const & _transform)
{
	base::apply_transform(_transform);
}

void LightningData::reskin(Meshes::BoneRenameFunc _rename)
{
	base::reskin(_rename);

	segmentName.if_value_set([this, _rename](){ segmentName = _rename(segmentName.get(), NP); });
}
