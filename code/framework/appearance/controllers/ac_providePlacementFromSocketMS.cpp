#include "ac_providePlacementFromSocketMS.h"

#include "ikUtils.h"

#include "..\appearanceControllerPoseContext.h"
#include "..\appearanceControllers.h"

#include "..\..\meshGen\meshGenGenerationContext.h"
#include "..\..\meshGen\meshGenParamImpl.inl"
#include "..\..\module\moduleAppearance.h"

#include "..\..\..\core\mesh\pose.h"
#include "..\..\..\core\wheresMyPoint\iWMPTool.h"

#include "..\..\..\core\performance\performanceUtils.h"
#include "..\..\..\core\system\input.h"

//

using namespace Framework;
using namespace AppearanceControllersLib;

//

DEFINE_STATIC_NAME(providePlacementFromSocketMS);

//

int ProvidePlacementFromSocketMS::State::ChooseSocket::compare(void const * _a, void const * _b)
{
	ChooseSocket const * a = (ChooseSocket const *)(_a);
	ChooseSocket const * b = (ChooseSocket const *)(_b);
	if (a->value < b->value)
	{
		return A_BEFORE_B;
	}
	if (a->value > b->value)
	{
		return B_BEFORE_A;
	}
	return A_AS_B;
}

//

REGISTER_FOR_FAST_CAST(ProvidePlacementFromSocketMS);

ProvidePlacementFromSocketMS::ProvidePlacementFromSocketMS(ProvidePlacementFromSocketMSData const * _data)
: base(_data)
, providePlacementFromSocketMSData(_data)
{
	resultPlacementMSVar = providePlacementFromSocketMSData->resultPlacementMSVar.get();

	idleSocket.set_name(providePlacementFromSocketMSData->idleSocket.get(Name::invalid()));
	baseBone = providePlacementFromSocketMSData->baseBone.get(Name::invalid());
	
	offset = providePlacementFromSocketMSData->offset.get(Transform::identity);

	states.make_space_for(providePlacementFromSocketMSData->states.get_size());
	for_every(s, providePlacementFromSocketMSData->states)
	{
		State state;
		state.priority = s->priority;
		state.stateVar = s->stateVar.get();
		if (s->socket.is_set())
		{
			state.socket.set_name(s->socket.get());
		}
		else if (s->chooseSocketVar.is_set())
		{
			state.chooseSocketVar = s->chooseSocketVar.get();
			for_every(c, s->chooseSockets)
			{
				State::ChooseSocket chooseSocket;
				chooseSocket.socket.set_name(c->socket.get());
				chooseSocket.value = c->value.get();
				state.chooseSockets.push_back(chooseSocket);
			}
			sort(state.chooseSockets);
		}
		else
		{
			error(TXT("no socket or choose sockets"));
		}
		states.push_back(state);
	}
}

ProvidePlacementFromSocketMS::~ProvidePlacementFromSocketMS()
{
}

void ProvidePlacementFromSocketMS::initialise(ModuleAppearance* _owner)
{
	base::initialise(_owner);

	auto & varStorage = get_owner()->access_controllers_variable_storage();
	resultPlacementMSVar.look_up<Transform>(varStorage);

	idleSocket.look_up(get_owner()->get_mesh());
	if (Meshes::Skeleton const * skeleton = get_owner()->get_skeleton()->get_skeleton())
	{
		if (baseBone.is_name_valid())
		{
			baseBone.look_up(skeleton);
		}
	}
	for_every(s, states)
	{
		s->socket.look_up(get_owner()->get_mesh());
		s->stateVar.look_up<float>(varStorage);
		s->chooseSocketVar.look_up<float>(varStorage);
		for_every(c, s->chooseSockets)
		{
			c->socket.look_up(get_owner()->get_mesh());
			c->toNextInv = 0.0f;
		}
		for (int i = 0; i < s->chooseSockets.get_size() - 1; ++i)
		{
			auto & c = s->chooseSockets[i];
			auto & n = s->chooseSockets[i + 1];
			c.toNextInv = n.value != c.value ? 1.0f / (n.value - c.value) : 0.0f;
		}
	}
	sort(states);
}

void ProvidePlacementFromSocketMS::advance_and_adjust_preliminary_pose(REF_ AppearanceControllerPoseContext & _context)
{
	base::advance_and_adjust_preliminary_pose(_context);
}

void ProvidePlacementFromSocketMS::calculate_final_pose(REF_ AppearanceControllerPoseContext & _context)
{
	base::calculate_final_pose(_context);

#ifdef MEASURE_PERFORMANCE_APPEARANCE_CONTROLLERS_DETAILED
	MEASURE_PERFORMANCE(providePlacementFromSocketMS_finalPose);
#endif

	auto const * mesh = get_owner()->get_mesh();
	if (!mesh)
	{
		return;
	}

	Meshes::Pose * poseLS = _context.access_pose_LS();

	Transform placementMS = idleSocket.is_valid()? mesh->calculate_socket_using_ls(idleSocket.get_index(), poseLS) : resultPlacementMSVar.get<Transform>();
	Transform baseBoneMS = placementMS;
	if (baseBone.is_valid())
	{
		baseBoneMS = poseLS->get_bone_ms_from_ls(baseBone.get_index());
	}

	for_every(state, states)
	{
		float stateVal = state->stateVar.get<float>();
		if (stateVal > 0.0f)
		{
			Transform socketMS;
			if (state->socket.is_valid())
			{
				socketMS = mesh->calculate_socket_using_ls(state->socket.get_index(), poseLS);
			}
			else if (!state->chooseSockets.is_empty() && state->chooseSocketVar.is_valid())
			{
				float value = state->chooseSocketVar.get<float>();
				int idx = 0;
				float pt = 0.0f;
				auto * c = state->chooseSockets.begin();
				auto * n = c + 1;
				if (value <= state->chooseSockets.get_first().value ||
					state->chooseSockets.get_size() == 1)
				{
					idx = 0;
					pt = 0.0f;
				}
				else if (value >= state->chooseSockets.get_last().value)
				{
					idx = state->chooseSockets.get_size() - 1;
					pt = 0.0f;
				}
				else
				{
					for (int i = 0; i < state->chooseSockets.get_size() - 1; ++i, ++c, ++n)
					{
						if (value >= c->value && value < n->value)
						{
							idx = i;
							pt = (value - c->value) * c->toNextInv;
						}
					}
				}
				if (pt == 0.0f)
				{
					socketMS = mesh->calculate_socket_using_ls(state->chooseSockets[idx].socket.get_index(), poseLS);
				}
				else
				{
					Transform socket0MS = mesh->calculate_socket_using_ls(state->chooseSockets[idx].socket.get_index(), poseLS);
					Transform socket1MS = mesh->calculate_socket_using_ls(state->chooseSockets[idx + 1].socket.get_index(), poseLS);
					socketMS = Transform::lerp(pt, socket0MS, socket1MS);
				}
			}
			else
			{
				continue;
			}
			if (baseBone.is_valid())
			{
				float dist0 = (placementMS.get_translation() - baseBoneMS.get_translation()).length();
				float dist1 = (socketMS.get_translation() - baseBoneMS.get_translation()).length();
				struct MixUtil
				{
					float dist0;
					float dist1;
					float maxDist;
					float stateVal;
					MixUtil(float d0, float d1, float sv)
						: dist0(d0)
						, dist1(d1)
						, stateVal(sv)
					{
						maxDist = max(dist0, dist1);
					}
					float get(float _threshold)
					{
						return stateVal < _threshold ? lerp(stateVal / _threshold, dist0, maxDist) : lerp((1.0f - stateVal) / (1.0f - _threshold), dist1, maxDist);
					}
				} mixUtil(dist0, dist1, stateVal);
				float mixed = lerp(0.8f, mixUtil.get(0.2f), mixUtil.get(0.8f));
				float distLerped = lerp(stateVal, dist0, dist1);
				distLerped = lerp(0.9f, distLerped, mixed);
				placementMS = Transform::lerp(stateVal, placementMS, socketMS);
				placementMS.set_translation(baseBoneMS.get_translation() + (placementMS.get_translation() - baseBoneMS.get_translation()).normal() * distLerped);
			}
			else
			{
				placementMS = Transform::lerp(stateVal, placementMS, socketMS);
			}
		}
	}

	placementMS = placementMS.to_world(offset);

	an_assert(placementMS.is_ok());

	if (get_active() == 1.0f)
	{
		resultPlacementMSVar.access<Transform>() = placementMS;
	}
	else
	{
		resultPlacementMSVar.access<Transform>() = Transform::lerp(get_active(), resultPlacementMSVar.get<Transform>(), placementMS);
	}
}

void ProvidePlacementFromSocketMS::get_info_for_auto_processing_order(OUT_ ArrayStack<int> & dependsOnBones, OUT_ ArrayStack<int> & dependsOnParentBones, OUT_ ArrayStack<int> & usesBones, OUT_ ArrayStack<int> & providesBones,
	OUT_ ArrayStack<Name> & dependsOnVariables, OUT_ ArrayStack<Name>& providesVariables, OUT_ ArrayStack<Name>& dependsOnControllers, OUT_ ArrayStack<Name>& providesControllers) const
{
	base::get_info_for_auto_processing_order(OUT_ dependsOnBones, OUT_ dependsOnParentBones, OUT_ usesBones, OUT_ providesBones,
		OUT_ dependsOnVariables, OUT_ providesVariables, OUT_ dependsOnControllers, OUT_ providesControllers);
}

//

REGISTER_FOR_FAST_CAST(ProvidePlacementFromSocketMSData);

AppearanceControllerData* ProvidePlacementFromSocketMSData::create_data()
{
	return new ProvidePlacementFromSocketMSData();
}

void ProvidePlacementFromSocketMSData::register_itself()
{
	AppearanceControllers::register_appearance_controller(NAME(providePlacementFromSocketMS), create_data);
}

ProvidePlacementFromSocketMSData::ProvidePlacementFromSocketMSData()
{
}

bool ProvidePlacementFromSocketMSData::load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc)
{
	bool result = base::load_from_xml(_node, _lc);

	result &= resultPlacementMSVar.load_from_xml(_node, TXT("resultPlacementMSVarID"));
	result &= resultPlacementMSVar.load_from_xml(_node, TXT("outPlacementMSVarID"));

	result &= idleSocket.load_from_xml(_node, TXT("idleSocket"));
	result &= idleSocket.load_from_xml(_node, TXT("socket"));
	result &= baseBone.load_from_xml(_node, TXT("baseBone"));
	result &= offset.load_from_xml(_node, TXT("offset"));

	for_every(node, _node->children_named(TXT("state")))
	{
		State state;
		if (state.load_from_xml(node))
		{
			states.push_back(state);
		}
		else
		{
			result = false;
		}
	}

	return result;
}

bool ProvidePlacementFromSocketMSData::prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext)
{
	bool result = base::prepare_for_game(_library, _pfgContext);

	return result;
}

AppearanceControllerData* ProvidePlacementFromSocketMSData::create_copy() const
{
	ProvidePlacementFromSocketMSData* copy = new ProvidePlacementFromSocketMSData();
	*copy = *this;
	return copy;
}

AppearanceController* ProvidePlacementFromSocketMSData::create_controller()
{
	return new ProvidePlacementFromSocketMS(this);
}

void ProvidePlacementFromSocketMSData::reskin(Meshes::BoneRenameFunc _rename)
{
	base::reskin(_rename);

	resultPlacementMSVar.if_value_set([this, _rename]() { resultPlacementMSVar = _rename(resultPlacementMSVar.get(), NP); });
	idleSocket.if_value_set([this, _rename]() { idleSocket = _rename(idleSocket.get(), NP); });
	baseBone.if_value_set([this, _rename]() { baseBone = _rename(baseBone.get(), NP); });
	for_every(s, states)
	{
		s->stateVar.if_value_set([s, _rename]() { s->stateVar = _rename(s->stateVar.get(), NP); });
		s->socket.if_value_set([s, _rename]() { s->socket = _rename(s->socket.get(), NP); });
		s->chooseSocketVar.if_value_set([s, _rename]() { s->chooseSocketVar = _rename(s->chooseSocketVar.get(), NP); });
		for_every(c, s->chooseSockets)
		{
			c->socket.if_value_set([c, _rename]() { c->socket = _rename(c->socket.get(), NP); });
		}
	}
}

void ProvidePlacementFromSocketMSData::apply_mesh_gen_params(MeshGeneration::GenerationContext const & _context)
{
	base::apply_mesh_gen_params(_context);

	resultPlacementMSVar.fill_value_with(_context);
	idleSocket.fill_value_with(_context);
	baseBone.fill_value_with(_context);
	offset.fill_value_with(_context);
	for_every(s, states)
	{
		s->stateVar.fill_value_with(_context);
		s->socket.fill_value_with(_context);
		s->chooseSocketVar.fill_value_with(_context);
		for_every(c, s->chooseSockets)
		{
			c->socket.fill_value_with(_context);
			c->value.fill_value_with(_context);
		}
	}
}

void ProvidePlacementFromSocketMSData::apply_transform(Matrix44 const & _transform)
{
	base::apply_transform(_transform);

	// this should get mirrors
	Vector3 resZAxis = Vector3::cross(_transform.get_x_axis(), _transform.get_y_axis());
	if (Vector3::dot(resZAxis, _transform.get_z_axis()) < 0.0f)
	{
		offset.if_value_set([this, _transform]() { offset.access().set_translation(_transform.location_to_world(offset.get().get_translation()));
												   offset.access().set_orientation(look_matrix33(_transform.vector_to_world(offset.get().get_axis(Axis::Y)),
																								 _transform.vector_to_world(offset.get().get_axis(Axis::Z))).to_quat()); });
	}
}

//

bool ProvidePlacementFromSocketMSData::State::load_from_xml(IO::XML::Node const * _node)
{
	bool result = true;

	priority = _node->get_int_attribute_or_from_child(TXT("priority"), priority);
	result &= stateVar.load_from_xml(_node, TXT("stateVarID"));
	result &= socket.load_from_xml(_node, TXT("socket"));
	result &= chooseSocketVar.load_from_xml(_node, TXT("chooseSocketVarID"));

	for_every(node, _node->children_named(TXT("choose")))
	{
		ChooseSocket chooseSocket;
		if (chooseSocket.load_from_xml(node))
		{
			chooseSockets.push_back(chooseSocket);
		}
		else
		{
			result = false;
		}
	}

	return result;
}

//

bool ProvidePlacementFromSocketMSData::State::ChooseSocket::load_from_xml(IO::XML::Node const * _node)
{
	bool result = true;

	result &= socket.load_from_xml(_node, TXT("socket"));
	result &= value.load_from_xml(_node, TXT("value"));

	return result;
}
