#include "agn_animationPlayback.h"

#include "..\animationGraphNodes.h"
#include "..\animationSet.h"

#include "..\..\module\moduleAppearance.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace Framework;
using namespace AnimationGraphNodesLib;

//

REGISTER_FOR_FAST_CAST(AnimationPlayback);

IAnimationGraphNode* AnimationPlayback::create_node()
{
	return new AnimationPlayback();
}

bool AnimationPlayback::load_from_xml(IO::XML::Node const* _node, LibraryLoadingContext& _lc)
{
	bool result = base::load_from_xml(_node, _lc);

	XML_LOAD_NAME_ATTR(_node, animation);
	XML_LOAD_BOOL_ATTR(_node, looped);
	XML_LOAD_FLOAT_ATTR(_node, playbackSpeed);
	XML_LOAD_NAME_ATTR(_node, playbackSpeedVarID);
	XML_LOAD(_node, playbackPt);
	XML_LOAD_NAME_ATTR(_node, playbackPtVarID);
	XML_LOAD_NAME_ATTR(_node, syncPlaybackPtID);
	XML_LOAD_NAME_ATTR(_node, provideSyncPlaybackPtID);
	XML_LOAD_FLOAT_ATTR(_node, syncPlaybackPtOffset);

	return result;
}

bool AnimationPlayback::prepare_runtime(AnimationGraphRuntime& _runtime, AnimationGraphNodeSet& _inSet)
{
	bool result = base::prepare_runtime(_runtime, _inSet);

	ANIMATION_GRAPH_RUNTIME_VARIABLES_START_DECLARING(AnimationPlayback);
	ANIMATION_GRAPH_RUNTIME_VARIABLE_DECLARE(AnimationLookUp, animation, AnimationLookUp());
	ANIMATION_GRAPH_RUNTIME_VARIABLE_DECLARE(float, atTime, 0.0f);
	ANIMATION_GRAPH_RUNTIME_VARIABLE_DECLARE(float, playbackSpeed, 1.0f);
	ANIMATION_GRAPH_RUNTIME_VARIABLE_DECLARE(SimpleVariableInfo, playbackSpeedVar, SimpleVariableInfo());
	ANIMATION_GRAPH_RUNTIME_VARIABLE_DECLARE(SimpleVariableInfo, playbackPtVar, SimpleVariableInfo());

	return result;
}

void AnimationPlayback::prepare_instance(AnimationGraphRuntime& _runtime) const
{
	base::prepare_instance(_runtime);

	ANIMATION_GRAPH_RUNTIME_VARIABLES_START_USING(AnimationPlayback);
	ANIMATION_GRAPH_RUNTIME_VARIABLE_USE(AnimationLookUp, animation);
	ANIMATION_GRAPH_RUNTIME_VARIABLE_NOT_USED(float, atTime);
	ANIMATION_GRAPH_RUNTIME_VARIABLE_NOT_USED(float, playbackSpeed);
	ANIMATION_GRAPH_RUNTIME_VARIABLE_USE(SimpleVariableInfo, playbackSpeedVar);
	ANIMATION_GRAPH_RUNTIME_VARIABLE_USE(SimpleVariableInfo, playbackPtVar);

	animation.look_up(_runtime.owner, this->animation);
	playbackSpeedVar.set_name(playbackSpeedVarID);
	playbackSpeedVar.look_up<float>(_runtime.imoOwner->access_variables());
	playbackPtVar.set_name(playbackPtVarID);
	playbackPtVar.look_up<float>(_runtime.imoOwner->access_variables());
}

void AnimationPlayback::on_activate(AnimationGraphRuntime& _runtime) const
{
	base::on_activate(_runtime);

	ANIMATION_GRAPH_RUNTIME_VARIABLES_START_USING(AnimationPlayback);
	ANIMATION_GRAPH_RUNTIME_VARIABLE_NOT_USED(AnimationLookUp, animation);
	ANIMATION_GRAPH_RUNTIME_VARIABLE_USE(float, atTime);
	ANIMATION_GRAPH_RUNTIME_VARIABLE_USE(float, playbackSpeed);
	ANIMATION_GRAPH_RUNTIME_VARIABLE_USE(SimpleVariableInfo, playbackSpeedVar);
	ANIMATION_GRAPH_RUNTIME_VARIABLE_NOT_USED(SimpleVariableInfo, playbackPtVar);

	atTime = 0.0f;
	playbackSpeed = this->playbackSpeed;
	if (playbackSpeedVar.is_valid())
	{
		playbackSpeed = playbackSpeedVar.get<float>();
	}
}

void AnimationPlayback::on_reactivate(AnimationGraphRuntime& _runtime) const
{
	base::on_reactivate(_runtime);

	//ANIMATION_GRAPH_RUNTIME_VARIABLES_START_USING(AnimationPlayback);
}

void AnimationPlayback::on_deactivate(AnimationGraphRuntime& _runtime) const
{
	base::on_deactivate(_runtime);

	//ANIMATION_GRAPH_RUNTIME_VARIABLES_START_USING(AnimationPlayback);
}

void AnimationPlayback::execute(AnimationGraphRuntime& _runtime, AnimationGraphExecuteContext& _context, OUT_ AnimationGraphOutput& _output) const
{
	base::execute(_runtime, _context, _output);

	ANIMATION_GRAPH_RUNTIME_VARIABLES_START_USING(AnimationPlayback);
	ANIMATION_GRAPH_RUNTIME_VARIABLE_USE(AnimationLookUp, animation);
	ANIMATION_GRAPH_RUNTIME_VARIABLE_USE(float, atTime);
	ANIMATION_GRAPH_RUNTIME_VARIABLE_USE(float, playbackSpeed);
	ANIMATION_GRAPH_RUNTIME_VARIABLE_USE(SimpleVariableInfo, playbackSpeedVar);
	ANIMATION_GRAPH_RUNTIME_VARIABLE_USE(SimpleVariableInfo, playbackPtVar);

	if (auto* anim = animation.get())
	{
		float animLength = anim->get_length();

		bool fullUpdate = true;
		Optional<float> * syncTo = nullptr;
		if (syncPlaybackPtID.is_valid())
		{
			syncTo = &_runtime.access_sync_playback(syncPlaybackPtID);
			if (syncTo->is_set())
			{
				// don't do full update only if we have it right away
				fullUpdate = false;
			}
			else
			{
				// we have to use the last frame, we should do full update
				if (auto* pt = _runtime.get_prev_sync_playback(syncPlaybackPtID))
				{
					*syncTo = *pt;
				}
			}
			if (syncTo->is_set())
			{
				// read provided by something else
				float toPt = syncTo->get() + syncPlaybackPtOffset;
				toPt = looped ? mod(toPt, 1.0f) : clamp(toPt, 0.0f, 1.0f);
				atTime = toPt * animLength;
			}
		}
		if (fullUpdate)
		{
			if (playbackPt.is_set())
			{
				atTime = anim->get_length() * playbackPt.get();
			}
			else if (playbackPtVar.is_valid())
			{
				atTime = anim->get_length() * playbackPtVar.get<float>();
			}
			else
			{
				if (playbackSpeedVar.is_valid())
				{
					playbackSpeed = playbackSpeedVar.get<float>();
				}
				atTime += playbackSpeed * _context.deltaTime;
			}
			{
				if (looped)
				{
					atTime = mod(atTime, animLength);
				}
				else
				{
					atTime = clamp(atTime, 0.0f, animLength);
				}
			}
		}
		if (provideSyncPlaybackPtID.is_valid())
		{
			syncTo = &_runtime.access_sync_playback(provideSyncPlaybackPtID);
		}
		if (syncTo &&
			fullUpdate)
		{
			// we should provide after full update
			float providePt = atTime / animLength - syncPlaybackPtOffset;
			providePt = looped ? mod(providePt, 1.0f) : clamp(providePt, 0.0f, 1.0f);
			*syncTo = providePt;
		}
		anim->read(atTime, _output.access_root_motion(), _output.access_pose());
	}
}
