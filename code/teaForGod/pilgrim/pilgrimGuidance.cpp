#include "pilgrimGuidance.h"

#include "..\ai\logics\aiLogic_pilgrim.h"

#include "..\..\framework\ai\aiMindInstance.h"
#include "..\..\framework\module\moduleAI.h"
#include "..\..\framework\module\moduleAppearance.h"
#include "..\..\framework\module\modulePresence.h"
#include "..\..\framework\modulesOwner\modulesOwner.h"

#define DEBUG_DRAW_PILGRIM_GUIDANCE

#ifdef DEBUG_DRAW_PILGRIM_GUIDANCE
#include "..\..\core\debug\debugRenderer.h"

DEFINE_STATIC_NAME(pilgrim_guidance);
#endif

//

using namespace TeaForGodEmperor;

//

bool PilgrimGuidance::update(Framework::IModulesOwner const * _pilgrim, Framework::IModulesOwner const * _owner, float _deltaTime, Optional<bool> const& _forceUpdate, Name const & _guidanceSocketInOwner, Name const & _povSocketInPilgrim, float _usePOVLine)
{
	an_assert(_usePOVLine == 0.0f || _usePOVLine == 1.0f, TXT("do not mix at the moment"));

	bool updateMayBeRequired = false;
	validGuidance = false;
	if (auto* pilgrimAI = _pilgrim->get_ai())
	{
		if (auto* pilgrimLogic = fast_cast<AI::Logics::Pilgrim>(pilgrimAI->get_mind()->get_logic()))
		{
			Vector3 directionDirOS = pilgrimLogic->get_direction_os();

			if (directionDirOS.is_zero())
			{
				updateMayBeRequired = !guidanceDirOS.is_zero();
				guidanceDirOS = Vector3::zero;
			}
			else
			{
				Vector3 processedDirectionDirOS = directionDirOS;

#ifdef DEBUG_DRAW_PILGRIM_GUIDANCE
				Transform debugRelativeOwnerPlacement = Transform(Vector3(0.0f, 0.5f, 1.5f), Quat::identity);
#endif

				if (abs(processedDirectionDirOS.z) > 0.7f)
				{
					// keep it, just make it fully vertical
					processedDirectionDirOS.x = 0.0f;
					processedDirectionDirOS.y = 0.0f;
					processedDirectionDirOS.z = sign(processedDirectionDirOS.z);
				}
				else if (auto * pathToAttachedTo = _owner->get_presence()->get_path_to_attached_to())
				{
					if (pathToAttachedTo->is_active())
					{
						validGuidance = true;
						Transform pilgrimPlacement = _pilgrim->get_presence()->get_placement();
						// get owner in pilgrim's placement
						Transform ownerPlacement = pathToAttachedTo->from_owner_to_target(_owner->get_presence()->get_placement());
						if (_guidanceSocketInOwner.is_valid() &&
							_owner->get_appearance())
						{
							ownerPlacement = ownerPlacement.to_world(_owner->get_appearance()->calculate_socket_os(_guidanceSocketInOwner));
						}
						// relative to pilgrim
						Transform relativeOwnerPlacement = pilgrimPlacement.to_local(ownerPlacement);
#ifdef DEBUG_DRAW_PILGRIM_GUIDANCE
						debugRelativeOwnerPlacement = Transform(relativeOwnerPlacement.get_translation(), Quat::identity);
#endif
						if (_usePOVLine < 1.0f)
						{
							if (guidanceType == Guidance::HotCold)
							{
								// we will by trying to pull it with our owner and compare if we're pulling along dir or not
								// give result only if actively pulling
								Transform pilgrimPlacementVR = _pilgrim->get_presence()->get_vr_anchor().to_local(pilgrimPlacement);
								Vector3 newGuidanceOwnerLocVR = pilgrimPlacementVR.location_to_world(relativeOwnerPlacement.get_translation());

								guidanceOwnerLocVR = blend_to_using_time(guidanceOwnerLocVR, newGuidanceOwnerLocVR, 0.25f, _deltaTime);
								Vector3 g2n = newGuidanceOwnerLocVR - guidanceOwnerLocVR;
								float g2nLength = g2n.length();
								Vector3 g2nDir = g2n.normal();
								guidanceOwnerLocVR = newGuidanceOwnerLocVR - g2nDir * min(0.05f, g2nLength);

#ifdef DEBUG_DRAW_PILGRIM_GUIDANCE
								debug_context(_owner->get_presence()->get_in_room());
								debug_filter(pilgrim_guidance);
								debug_push_transform(_pilgrim->get_presence()->get_vr_anchor());
								debug_draw_arrow(true, Colour::green, guidanceOwnerLocVR, newGuidanceOwnerLocVR);
								debug_draw_arrow(true, Colour::white, newGuidanceOwnerLocVR, newGuidanceOwnerLocVR + pilgrimPlacementVR.vector_to_world(directionDirOS) * 0.05f);
								debug_draw_text(true, Colour::red, newGuidanceOwnerLocVR, Vector2::zero, true, 0.3f, NP, TXT("%.3f"), g2nLength);
#endif
								if (g2nLength > 0.01f)
								{
									Vector3 directionDirVR = pilgrimPlacementVR.vector_to_world(directionDirOS);
									directionDirVR.z = 0.0f;
									directionDirVR.normalise(); // make it flat XY too!
									float dot = Vector3::dot(g2nDir, directionDirVR);
									float dotThreshold = 0.7f;
									processedDirectionDirOS = abs(dot) > dotThreshold ? Vector3::yAxis * sign(dot) : Vector3::zero;
								}
								else
								{
									processedDirectionDirOS = Vector3::zero;
								}
#ifdef DEBUG_DRAW_PILGRIM_GUIDANCE
								debug_draw_arrow(true, Colour::yellow, newGuidanceOwnerLocVR, newGuidanceOwnerLocVR + pilgrimPlacementVR.vector_to_world(processedDirectionDirOS) * 0.05f);
								debug_pop_transform();
								debug_no_filter();
								debug_no_context();
#endif
							}
							else
							{
								processedDirectionDirOS = relativeOwnerPlacement.vector_to_local(directionDirOS);
#ifdef DEBUG_DRAW_PILGRIM_GUIDANCE
								Vector3 orgDirectionDirOS = processedDirectionDirOS;
#endif
								processedDirectionDirOS.z = 0.0f;
								processedDirectionDirOS.normalise(); // make it flat XY too!
#ifdef DEBUG_DRAW_PILGRIM_GUIDANCE
								debug_context(_pilgrim->get_presence()->get_in_room());
								debug_filter(pilgrim_guidance);
								debug_push_transform(_pilgrim->get_presence()->get_placement());
								debug_push_transform(relativeOwnerPlacement);
								debug_draw_arrow(true, Colour::green, Vector3::zero, Vector3(0.0f, 0.2f, 0.0f));
								debug_draw_line(true, Colour::green, Vector3::zero, Vector3(0.0f, -0.05f, 0.0f));
								debug_draw_line(true, Colour::green, Vector3::zero, Vector3(0.05f, 0.0f, 0.0f));
								debug_draw_line(true, Colour::green, Vector3::zero, Vector3(-0.05f, 0.0f, 0.0f));
								debug_draw_arrow(true, Colour::white, Vector3::zero, processedDirectionDirOS * 0.3f);
								debug_draw_arrow(true, Colour::grey, Vector3::zero, orgDirectionDirOS * 0.15f);
								debug_pop_transform();
								debug_pop_transform();
								debug_no_context();
								// just object & guidance
								debug_context(_owner->get_presence()->get_in_room());
								debug_push_transform(ownerPlacement);
								debug_draw_arrow(true, Colour::red, Vector3::zero, Vector3(0.0f, 0.2f, 0.0f));
								debug_draw_arrow(true, Colour::blue, Vector3::zero, Vector3(0.0f, 0.0f, 0.2f));
								debug_pop_transform();
								debug_no_filter();
								debug_no_context();
#endif
							}
						}

						if (_usePOVLine > 0.0f)
						{
							// pov line:
							// using pov socket (head) we create transform that starts at pov socket and points at guidance socket
							// then we get direction is in relation to this
							Transform povPilgrimPlacement = _pilgrim->get_presence()->get_centre_of_presence_transform_WS();
							if (_povSocketInPilgrim.is_valid())
							{
								povPilgrimPlacement = pilgrimPlacement.to_world(_pilgrim->get_appearance()->calculate_socket_os(_povSocketInPilgrim));
							}
							Transform relativePOVPilgrimPlacement = pilgrimPlacement.to_local(povPilgrimPlacement);

							Transform povLinePlacement = look_at_matrix(relativePOVPilgrimPlacement.get_translation(), relativeOwnerPlacement.get_translation(), relativePOVPilgrimPlacement.get_axis(Axis::Z)).to_transform();

							Vector3 povDirectionDirOS = povLinePlacement.vector_to_local(directionDirOS);
							povDirectionDirOS.z = 0.0f;
							povDirectionDirOS.normalise(); // make it flat XY too!
							processedDirectionDirOS = processedDirectionDirOS * (1.0f - _usePOVLine) + povDirectionDirOS * _usePOVLine;
							processedDirectionDirOS.normalise();
						}
					}
				}

				if (_forceUpdate.get(false) || guidanceDirOS.is_zero() || Vector3::dot(guidanceDirOS, processedDirectionDirOS) < 0.995f)
				{
					guidanceDirOS = processedDirectionDirOS;
					updateMayBeRequired = true;
				}

#ifdef DEBUG_DRAW_PILGRIM_GUIDANCE
				debug_context(_pilgrim->get_presence()->get_in_room());
				debug_filter(pilgrim_guidance);
				debug_push_transform(_pilgrim->get_presence()->get_placement());
				debug_push_transform(debugRelativeOwnerPlacement);
				debug_draw_arrow(true, Colour::yellow, Vector3::zero, guidanceDirOS * 0.5f);
				debug_pop_transform();
				debug_pop_transform();
				debug_no_filter();
				debug_no_context();
#endif
			}
		}
	}
	return updateMayBeRequired;
}
