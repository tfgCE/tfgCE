#include "agn_randomAnimationPlayback.h"

#include "..\animationGraphNodes.h"
#include "..\animationSet.h"

#include "..\..\module\moduleAppearance.h"
#include "..\..\..\core\random\randomUtils.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace Framework;
using namespace AnimationGraphNodesLib;

//

REGISTER_FOR_FAST_CAST(RandomAnimationPlayback);

IAnimationGraphNode* RandomAnimationPlayback::create_node()
{
	return new RandomAnimationPlayback();
}

bool RandomAnimationPlayback::load_from_xml(IO::XML::Node const* _node, LibraryLoadingContext& _lc)
{
	bool result = base::load_from_xml(_node, _lc);

	// reset
	//
	animations.clear();
	looped = false;
	provideSyncPlaybackPtID = Name::invalid();

	// read
	//
	for_every(node, _node->children_named(TXT("random")))
	{
		animations.push_back();
		auto& a = animations.get_last();
		a.animation = node->get_name_attribute(TXT("animation"));
		a.probCoef = node->get_float_attribute(TXT("probCoef"), a.probCoef);
	}
	looped = _node->get_bool_attribute(TXT("looped"), looped);
	XML_LOAD_NAME_ATTR(_node, provideSyncPlaybackPtID);

	return result;
}

bool RandomAnimationPlayback::prepare_runtime(AnimationGraphRuntime& _runtime, AnimationGraphNodeSet& _inSet)
{
	bool result = base::prepare_runtime(_runtime, _inSet);

	ANIMATION_GRAPH_RUNTIME_VARIABLES_START_DECLARING(RandomAnimationPlayback);
	ANIMATION_GRAPH_RUNTIME_VARIABLE_DECLARE(AnimationLookUp, animation, AnimationLookUp());
	ANIMATION_GRAPH_RUNTIME_VARIABLE_DECLARE(float, atTime, 0.0f);
	ANIMATION_GRAPH_RUNTIME_VARIABLE_DECLARE(int, lastAnimIdx, -1);
	for_every(a, animations)
	{
		ANIMATION_GRAPH_RUNTIME_VARIABLE_DECLARE(AnimationLookUp, animationToSelect, AnimationLookUp());
	}

	return result;
}

void RandomAnimationPlayback::prepare_instance(AnimationGraphRuntime& _runtime) const
{
	base::prepare_instance(_runtime);

	ANIMATION_GRAPH_RUNTIME_VARIABLES_START_USING(RandomAnimationPlayback);
	ANIMATION_GRAPH_RUNTIME_VARIABLE_NOT_USED(AnimationLookUp, animation);
	ANIMATION_GRAPH_RUNTIME_VARIABLE_NOT_USED(float, atTime);
	ANIMATION_GRAPH_RUNTIME_VARIABLE_NOT_USED(int, lastAnimIdx);
	for_every(a, animations)
	{
		ANIMATION_GRAPH_RUNTIME_VARIABLE_USE(AnimationLookUp, animationToSelect);
		animationToSelect.look_up(_runtime.owner, a->animation);
	}
}

void RandomAnimationPlayback::on_activate(AnimationGraphRuntime& _runtime) const
{
	base::on_activate(_runtime);

	ANIMATION_GRAPH_RUNTIME_VARIABLES_START_USING(RandomAnimationPlayback);
	ANIMATION_GRAPH_RUNTIME_VARIABLE_USE(AnimationLookUp, animation);
	ANIMATION_GRAPH_RUNTIME_VARIABLE_USE(float, atTime);
	ANIMATION_GRAPH_RUNTIME_VARIABLE_USE(int, lastAnimIdx);

	animation.clear();
	atTime = 0.0f;
	lastAnimIdx = -1;
}

void RandomAnimationPlayback::on_deactivate(AnimationGraphRuntime& _runtime) const
{
	base::on_deactivate(_runtime);

	//ANIMATION_GRAPH_RUNTIME_VARIABLES_START_USING(RandomAnimationPlayback);
}

void RandomAnimationPlayback::execute(AnimationGraphRuntime& _runtime, AnimationGraphExecuteContext& _context, OUT_ AnimationGraphOutput& _output) const
{
	base::execute(_runtime, _context, _output);

	float timeLeft = _context.deltaTime;
	while (timeLeft > 0.0f)
	{
		// re read every time we enter the loop
		ANIMATION_GRAPH_RUNTIME_VARIABLES_START_USING(RandomAnimationPlayback);
		ANIMATION_GRAPH_RUNTIME_VARIABLE_USE(AnimationLookUp, animation);
		ANIMATION_GRAPH_RUNTIME_VARIABLE_USE(float, atTime);
		ANIMATION_GRAPH_RUNTIME_VARIABLE_USE(int, lastAnimIdx);

		if (!animation.is_valid())
		{
			Random::Generator rg;
			int idx = lastAnimIdx;
			while (idx == lastAnimIdx)
			{
				idx = RandomUtils::ChooseFromContainer<Array<Animation>, Animation>::choose(rg, animations, [](Animation const& _anim) { return _anim.probCoef; });
				if (animations.get_size() <= 2)
				{
					break;
				}
			}

			for_every(a, animations)
			{
				ANIMATION_GRAPH_RUNTIME_VARIABLE_USE(AnimationLookUp, animationToSelect);
				if (idx <= 0)
				{
					animation = animationToSelect;
					break;
				}
				--idx;
			}
			atTime = 0.0f;
		}

		if (auto* anim = animation.get())
		{
			float animLength = anim->get_length();

			float playbackSpeed = 1.0f;

			atTime += playbackSpeed * timeLeft;

			if (looped)
			{
				atTime = mod(atTime, animLength); // will keep us in this anim
				timeLeft = 0.0f;
			}
			else
			{
				if (atTime > animLength)
				{
					timeLeft = (atTime - animLength) / playbackSpeed;
				}
				else
				{
					timeLeft = 0.0f;
				}
			}

			anim->read(clamp(atTime, 0.0f, animLength), _output.access_root_motion(), _output.access_pose());
		
			if (provideSyncPlaybackPtID.is_valid())
			{
				Optional<float>* syncTo = nullptr;
				syncTo = &_runtime.access_sync_playback(provideSyncPlaybackPtID);
				if (syncTo)
				{
					// we should provide after full update
					float providePt = atTime / animLength/* - syncPlaybackPtOffset*/;
					providePt = looped ? mod(providePt, 1.0f) : clamp(providePt, 0.0f, 1.0f);
					*syncTo = providePt;
				}
			}
		}

		if (timeLeft)
		{
			animation.clear();
		}
	}
}
