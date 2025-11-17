#include "gse_customUniforms.h"

#include "..\..\game\game.h"
#include "..\..\game\gameDirector.h"

#include "..\..\..\core\io\xml.h"
#include "..\..\..\core\system\video\video3d.h"
#include "..\..\..\core\system\video\shaderProgram.h"
#include "..\..\..\framework\gameScript\gameScript.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace TeaForGodEmperor;
using namespace GameScript;
using namespace Elements;

//

struct CustomUniformsStatic
{
	struct Uniform
	{
		Name uniform;
		float value = 0.0f;
		float target = 0.0f;
		float blendTime = 0.0f;
	};
	Concurrency::SpinLock uniformsLock; // only for adding and removing
	ArrayStatic<Uniform, 16> uniforms;
} g_customUniformsStatic;

//

bool CustomUniforms::load_from_xml(IO::XML::Node const* _node, Framework::LibraryLoadingContext & _lc)
{
	bool result = true;

	clear = _node->get_bool_attribute_or_from_child_presence(TXT("clear"), clear);

	provide.clear();
	for_every(n, _node->children_named(TXT("provide")))
	{
		Name p = n->get_name_attribute(TXT("uniform"));
		if (p.is_valid())
		{
			provide.push_back(p);
		}
	}

	id = _node->get_name_attribute_or_from_child(TXT("id"), id);
	error_loading_xml_on_assert(provide.is_empty() || id.is_valid(), result, _node, TXT("no id provided but wants to provide uniforms"));

	untilStoryFact = _node->get_name_attribute(TXT("untilStoryFact"), untilStoryFact);

	blendUniforms.clear();
	for_every(n, _node->children_named(TXT("blend")))
	{
		BlendUniform bu;
		bu.uniform = n->get_name_attribute(TXT("uniform"), bu.uniform);
		bu.target = n->get_float_attribute(TXT("target"), bu.target);
		bu.blendTime = n->get_float_attribute(TXT("blendTime"), bu.blendTime);
		blendUniforms.push_back(bu);
	}

	return result;
}

Framework::GameScript::ScriptExecutionResult::Type CustomUniforms::execute(Framework::GameScript::ScriptExecution& _execution, Framework::GameScript::ScriptExecution::Flags _flags) const
{
	bool endNow = false;

	if (is_flag_set(_flags, Framework::GameScript::ScriptExecution::Flag::Entered) && clear)
	{
		Concurrency::ScopedSpinLock lock(g_customUniformsStatic.uniformsLock);
		g_customUniformsStatic.uniforms.clear();
	}

	if (untilStoryFact.is_valid())
	{
		if (auto* gd = TeaForGodEmperor::GameDirector::get_active())
		{
			int actualValue;
			{
				Concurrency::ScopedMRSWLockRead lock(gd->access_story_execution().access_facts_lock());
				actualValue = gd->access_story_execution().get_facts().get_tag_as_int(untilStoryFact);
			}
			if (actualValue)
			{
				endNow = true;
			}
		}
	}
	else if (provide.is_empty())
	{
		endNow = true;
	}

	if (!provide.is_empty() && id.is_valid())
	{
		if (is_flag_set(_flags, Framework::GameScript::ScriptExecution::Flag::Entered) && !endNow)
		{
			Game::get()->add_sync_world_job(TXT("add custom built in uniforms"), [this]()
				{
					::System::Video3D::get()->add_custom_set_build_in_uniforms(
						id, provide,
						[](::System::ShaderProgram* _shader)
						{
							int c = g_customUniformsStatic.uniforms.get_size();
							for_count(int, i, c)
							{
								auto& p = g_customUniformsStatic.uniforms[i];
								int uniformID = _shader->find_uniform_index(p.uniform);
								if (uniformID != NONE)
								{
									_shader->set_uniform(uniformID, p.value);
								}
							}
						});
				});

			// add immediately as we might be processing it soon
			{
				Concurrency::ScopedSpinLock lock(g_customUniformsStatic.uniformsLock);
				for_every(p, provide)
				{
					bool exists = false;
					for_every(cus, g_customUniformsStatic.uniforms)
					{
						if (cus->uniform == *p)
						{
							exists = true;
							break;
						}
					}
					if (!exists)
					{
						CustomUniformsStatic::Uniform u;
						u.uniform = *p;
						g_customUniformsStatic.uniforms.push_back(u);
					}
				}
			}
		}
	}

	if (!blendUniforms.is_empty() && 
		(is_flag_set(_flags, Framework::GameScript::ScriptExecution::Flag::Entered) || provide.is_empty()))
	{
		{
			Concurrency::ScopedSpinLock lock(g_customUniformsStatic.uniformsLock);
			for_every(bu, blendUniforms)
			{
				for_every(cu, g_customUniformsStatic.uniforms)
				{
					if (bu->uniform == cu->uniform)
					{
						cu->blendTime = bu->blendTime;
						cu->target = bu->target;
						if (cu->blendTime == 0.0f)
						{
							cu->value = cu->target;
						}
					}
				}
			}
		}
	}

	if (!provide.is_empty() && id.is_valid())
	{
		if (auto* game = Game::get_as<Game>())
		{
			float deltaTime = game->get_delta_time();
			Concurrency::ScopedSpinLock lock(g_customUniformsStatic.uniformsLock);
			for_every(p, provide)
			{
				for_every(cu, g_customUniformsStatic.uniforms)
				{
					if (cu->uniform == *p)
					{
						cu->value = blend_to_using_time(cu->value, cu->target, cu->blendTime, deltaTime);
					}
				}
			}
		}
		if (endNow)
		{
			clean_up_custom_uniforms();
		}
	}

	if (!provide.is_empty() && ! endNow)
	{
		// blend those provided
	}

	return endNow? Framework::GameScript::ScriptExecutionResult::Continue : Framework::GameScript::ScriptExecutionResult::Repeat;
}

void CustomUniforms::interrupted(Framework::GameScript::ScriptExecution& _execution) const
{
	base::interrupted(_execution);

	clean_up_custom_uniforms();
}

void CustomUniforms::clean_up_custom_uniforms() const
{
	if (!provide.is_empty())
	{
		Game::get()->add_sync_world_job(TXT("remove custom built in uniforms"), [this]()
			{
				::System::Video3D::get()->remove_custom_set_build_in_uniforms(id);

				{
					Concurrency::ScopedSpinLock lock(g_customUniformsStatic.uniformsLock);
					for (int i = 0; i < g_customUniformsStatic.uniforms.get_size(); ++i)
					{
						auto& u = g_customUniformsStatic.uniforms[i];
						if (provide.does_contain(u.uniform))
						{
							g_customUniformsStatic.uniforms.remove_at(i);
							--i;
						}
					}
				}
			});

	}
}
