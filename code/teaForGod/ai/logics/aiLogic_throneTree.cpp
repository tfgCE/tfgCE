#include "aiLogic_throneTree.h"

#include "..\..\..\framework\ai\aiLatentTask.h"
#include "..\..\..\framework\ai\aiLogicWithLatentTask.h"
#include "..\..\..\framework\ai\aiMindInstance.h"
#include "..\..\..\framework\module\moduleAppearance.h"
#include "..\..\..\framework\modulesOwner\modulesOwnerImpl.inl"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace TeaForGodEmperor;
using namespace AI;
using namespace Logics;

//

// variables
DEFINE_STATIC_NAME(throneTreeSeparation);
DEFINE_STATIC_NAME(throneTreeSpeed);
DEFINE_STATIC_NAME(throneTreeBackwardChance);
DEFINE_STATIC_NAME(throneTreeLengthBack);
DEFINE_STATIC_NAME(throneTreeLengthFront);
DEFINE_STATIC_NAME(throneTreeMaxOffset);
DEFINE_STATIC_NAME(throneTreeMinOffset);
DEFINE_STATIC_NAME(throneTreeBlendIn);
DEFINE_STATIC_NAME(throneTreeBlendOut);

// shader params
DEFINE_STATIC_NAME(throneRoomContentNum);
DEFINE_STATIC_NAME(throneRoomContentColour);
DEFINE_STATIC_NAME(throneRoomContentValues);

//

REGISTER_FOR_FAST_CAST(ThroneTree);

ThroneTree::ThroneTree(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData)
: base(_mind, _logicData, execute_logic)
{
}

ThroneTree::~ThroneTree()
{
}

void ThroneTree::learn_from(SimpleVariableStorage& _parameters)
{
	base::learn_from(_parameters);
	
	separation = _parameters.get_value<Range>(NAME(throneTreeSeparation), separation);
	speed = _parameters.get_value<Range>(NAME(throneTreeSpeed), speed);
	backwardChance = _parameters.get_value<float>(NAME(throneTreeBackwardChance), backwardChance);
	lengthBack = _parameters.get_value<Range>(NAME(throneTreeLengthBack), lengthBack);
	lengthFront = _parameters.get_value<Range>(NAME(throneTreeLengthFront), lengthFront);
	maxOffset = _parameters.get_value<Range>(NAME(throneTreeMaxOffset), maxOffset);
	minOffset = _parameters.get_value<Range>(NAME(throneTreeMinOffset), minOffset);
	blendIn = _parameters.get_value<float>(NAME(throneTreeBlendIn), blendIn);
	blendOut = _parameters.get_value<float>(NAME(throneTreeBlendOut), blendOut);

	currentSeparation = separation.min;
}

void ThroneTree::advance(float _deltaTime)
{
	if (waves.get_size() < MAX_WAVE_COUNT - 2)
	{
		bool add = waves.is_empty() || waves.get_last().offset > currentSeparation;

		if (add)
		{
			currentSeparation = rg.get_float(separation);
			Wave w;
			w.speed = rg.get_float(speed);
			w.lengthBack = rg.get_float(lengthBack);
			w.lengthFront = rg.get_float(lengthFront);
			if (rg.get_chance(backwardChance))
			{
				w.speed = -w.speed;
				w.offset = rg.get_float(maxOffset);
				swap(w.lengthBack, w.lengthFront);
			}
			w.maxOffset = rg.get_float(maxOffset);
			w.minOffset = rg.get_float(minOffset);
			waves.push_back(w);
		}
	}

	if (waves.get_size() >= MAX_WAVE_COUNT - 3)
	{
		waves[0].powerTarget = 0.0f;
	}

	if (!waves.is_empty() &&
		waves[0].power <= 0.01f &&
		waves[0].powerTarget == 0.0f)
	{
		waves.pop_front();
	}

	for_every(w, waves)
	{
		w->offset += w->speed * _deltaTime;

		if (w->speed > 0.0f)
		{
			if (w->offset > w->maxOffset)
			{
				w->powerTarget = 0.0f;
			}
			w->power = blend_to_using_time(w->power, w->powerTarget, w->powerTarget < w->power ? blendOut : blendIn, _deltaTime);
		}
		else
		{
			if (w->offset < w->minOffset)
			{
				w->powerTarget = 0.0f;
			}

			w->power = blend_to_using_time(w->power, w->powerTarget, w->powerTarget < w->power ? blendIn : blendOut , _deltaTime);
		}
	}

	if (auto* imo = get_mind()->get_owner_as_modules_owner())
	{
		Vector4 colours[MAX_WAVE_COUNT];
		Vector4 values[MAX_WAVE_COUNT];

		memory_zero(colours, sizeof(Vector4) * MAX_WAVE_COUNT);
		memory_zero(values, sizeof(Vector4) * MAX_WAVE_COUNT);

		for_every(w, waves)
		{
			int idx = for_everys_index(w);
			if (idx < MAX_WAVE_COUNT)
			{
				colours[idx] = w->colour.to_vector4();
				auto& v = values[idx];
				v.x = w->offset;
				v.y = w->lengthBack != 0.0f? 1.0f / w->lengthBack : 0.0f;
				v.z = w->lengthFront != 0.0f? 1.0f / w->lengthFront : 0.0f;
				v.w = w->power;
			}
		}

		if (auto* appearance = imo->get_appearance())
		{
			auto& mi = appearance->access_mesh_instance();
			for_count(int, i, mi.get_material_instance_count())
			{
				if (auto* mat = appearance->access_mesh_instance().access_material_instance(i))
				{
					if (auto* spi = mat->get_shader_program_instance())
					{
						{
							int idx = spi->get_shader_program()->find_uniform_index(NAME(throneRoomContentNum));
							if (idx != NONE)
							{
								spi->set_uniform(idx, waves.get_size());
							}
						}
						{
							int idx = spi->get_shader_program()->find_uniform_index(NAME(throneRoomContentColour));
							if (idx != NONE)
							{
								spi->set_uniform(idx, colours, MAX_WAVE_COUNT);
							}
						}
						{
							int idx = spi->get_shader_program()->find_uniform_index(NAME(throneRoomContentValues));
							if (idx != NONE)
							{
								spi->set_uniform(idx, values, MAX_WAVE_COUNT);
							}
						}
					}
				}
			}
		}
	}


	base::advance(_deltaTime);
}

LATENT_FUNCTION(ThroneTree::execute_logic)
{
	LATENT_SCOPE();

	LATENT_PARAM_NOT_USED(::Framework::AI::MindInstance*, mind); // auto
	LATENT_PARAM_NOT_USED(::Framework::AI::Logic*, logic); // auto
	LATENT_END_PARAMS();

	LATENT_BEGIN_CODE();

	while (true)
	{
		LATENT_YIELD();
	}

	LATENT_ON_BREAK();
	//

	LATENT_END_CODE();
	LATENT_RETURN();
}

