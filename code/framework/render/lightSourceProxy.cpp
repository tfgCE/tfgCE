#include "lightSourceProxy.h"

#include "..\appearance\lightSource.h"

#include "..\module\custom\mc_lightSources.h"

#include "..\..\core\mainSettings.h"
#include "..\..\core\containers\arrayStack.h"
#include "..\..\core\system\video\shaderProgramBindingContext.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace Framework;
using namespace Framework::Render;

//

// shader params
DEFINE_STATIC_NAME(lightsCount);
DEFINE_STATIC_NAME(lights);
DEFINE_STATIC_NAME(coneLightsCount);
DEFINE_STATIC_NAME(coneLights);

// main settings
DEFINE_STATIC_NAME(pointLights);

//

int light_matrix_compare(void const* _a, void const* _b)
{
	Matrix44 const* a = plain_cast<Matrix44>(_a);
	Matrix44 const* b = plain_cast<Matrix44>(_b);
	if (a->m03 < b->m03) return A_BEFORE_B;
	if (a->m03 > b->m03) return B_BEFORE_A;
	return A_AS_B;
}

//

void LightSourceProxy::copy_from(LightSource const & _light)
{
	location = _light.location;
	stick = _light.stick;
	coneDir = _light.coneDir;
	colour = _light.colour;
	distance = _light.distance;
	power = _light.power;
	coneInnerAngle = _light.coneInnerAngle;
	coneOuterAngle = _light.coneOuterAngle;
	priority = _light.priority;
}

void LightSourceProxy::turn_into_matrix(OUT_ Matrix44 & _matrix44, Matrix44 const& _referenceModelViewMatrix) const
{
	if (!coneDir.is_zero())
	{
		auto& lightAsMatrix = _matrix44;
		// check shader for info about what is where in matrix
		Vector3 useLocation = _referenceModelViewMatrix.location_to_world(location);
		Vector3 useConeDir = _referenceModelViewMatrix.vector_to_world(coneDir);
		lightAsMatrix.m00 = useLocation.x;
		lightAsMatrix.m01 = useLocation.y;
		lightAsMatrix.m02 = useLocation.z;
		lightAsMatrix.m10 = useConeDir.x;
		lightAsMatrix.m11 = useConeDir.y;
		lightAsMatrix.m12 = useConeDir.z;
		lightAsMatrix.m20 = colour.r;
		lightAsMatrix.m21 = colour.g;
		lightAsMatrix.m22 = colour.b;
		lightAsMatrix.m30 = distance != 0.0f ? power : 0.0f;
		lightAsMatrix.m31 = distance != 0.0f ? 1.0f / sqr(distance) : 0.0f;
		float dotInner = cos_deg(coneInnerAngle);
		float dotOuter = cos_deg(coneOuterAngle);
		lightAsMatrix.m32 = dotInner;
		lightAsMatrix.m33 = (0.0f - 1.0f) / max(0.0001f, (dotOuter - dotInner));
	}
	else
	{
		auto& lightAsMatrix = _matrix44;
		// check shader for info about what is where in matrix
		Vector3 useLocation = _referenceModelViewMatrix.location_to_world(location);
		lightAsMatrix.m00 = useLocation.x;
		lightAsMatrix.m01 = useLocation.y;
		lightAsMatrix.m02 = useLocation.z;
		lightAsMatrix.m10 = colour.r;
		lightAsMatrix.m11 = colour.g;
		lightAsMatrix.m12 = colour.b;
		lightAsMatrix.m20 = distance != 0.0f ? power : 0.0f;
		lightAsMatrix.m21 = distance != 0.0f ? 1.0f / sqr(distance) : 0.0f;
		Vector3 useStick = _referenceModelViewMatrix.vector_to_world(stick);
		Vector3 useStickNormalised = useStick.normal();
		lightAsMatrix.m30 = useStickNormalised.x;
		lightAsMatrix.m31 = useStickNormalised.y;
		lightAsMatrix.m32 = useStickNormalised.z;
		lightAsMatrix.m33 = useStick.length() * 0.5f;
	}

	_matrix44.m03 = distanceToOwner * 0.1f - (float)priority - power;
}

//

LightSourceSetForPresenceLinkProxy::LightSourceSetForPresenceLinkProxy()
{
	SET_EXTRA_DEBUG_INFO(lights, TXT("LightSourceSetForPresenceLinkProxy.lights"));
}

void LightSourceSetForPresenceLinkProxy::on_release()
{
	lights.clear();
}

void LightSourceSetForPresenceLinkProxy::setup_shader_program(::System::ShaderProgram* _shaderProgram, Matrix44 const & _referenceModelViewMatrix, Optional<int> const& _maxLights, Optional<int> const& _maxConeLights) const
{
	int maxLights = _maxLights.get(::MainSettings::global().get_shader_options().get_tag_as_int(NAME(pointLights)));
	int maxConeLights = _maxConeLights.get(::MainSettings::global().get_shader_options().get_tag_as_int(NAME(coneLights)));

	// prepare matrices first

	ArrayStatic<Matrix44, 64> preparedLights;
	ArrayStatic<Matrix44, 64> preparedConeLights;

	for_every(light, lights)
	{
		if (!light->coneDir.is_zero())
		{
			if (maxConeLights)
			{
				preparedConeLights.grow_size(1);
				auto& lightAsMatrix = preparedConeLights.get_last();
				light->turn_into_matrix(lightAsMatrix, _referenceModelViewMatrix);
			}
		}
		else
		{
			if (maxLights)
			{
				preparedLights.grow_size(1);
				auto& lightAsMatrix = preparedLights.get_last();
				light->turn_into_matrix(lightAsMatrix, _referenceModelViewMatrix);
			}
		}
	}

	if (maxLights)
	{
		sort(preparedLights, light_matrix_compare);
		int lights = min(maxLights, preparedLights.get_size());
		_shaderProgram->set_uniform(NAME(lightsCount), lights);
		_shaderProgram->set_uniform(NAME(lights), preparedLights.get_data(), lights);
	}

	if (maxConeLights)
	{
		sort(preparedConeLights, light_matrix_compare);
		int coneLights = min(maxConeLights, preparedConeLights.get_size());
		_shaderProgram->set_uniform(NAME(coneLightsCount), coneLights);
		_shaderProgram->set_uniform(NAME(coneLights), preparedConeLights.get_data(), coneLights);
	}
}

void LightSourceSetForPresenceLinkProxy::add(LightSource const & _light, float _distanceToOwner)
{
	if (_light.flickeringLight && !LightSource::allowFlickeringLights)
	{
		return;
	}
	todo_important(TXT("do not use it yet? I haven't even decided should it be room space, objects space or what"));
	if (lights.get_size() < lights.get_max_size())
	{
		lights.set_size(lights.get_size() + 1);
		auto & light = lights.get_last();
		light.copy_from(_light);
		light.distanceToOwner = _distanceToOwner;
	}
	else
	{
		todo_important(TXT("select!"));
	}
}

//

LightSourceSetForRoomProxy::LightSourceSetForRoomProxy()
{
	SET_EXTRA_DEBUG_INFO(lights, TXT("LightSourceSetForRoomProxy.lights"));
	SET_EXTRA_DEBUG_INFO(preparedLights, TXT("LightSourceSetForRoomProxy.preparedLights"));
	SET_EXTRA_DEBUG_INFO(preparedConeLights, TXT("LightSourceSetForRoomProxy.preparedConeLights"));
}

void LightSourceSetForRoomProxy::on_release()
{
	lights.clear();

	prepared = false;
	preparedLights.clear();
	preparedConeLights.clear();
}

void LightSourceSetForRoomProxy::prepare(Optional<int> const& _maxLights, Optional<int> const& _maxConeLights)
{
	int maxLights = _maxLights.get(::MainSettings::global().get_shader_options().get_tag_as_int(NAME(pointLights)));
	int maxConeLights = _maxConeLights.get(::MainSettings::global().get_shader_options().get_tag_as_int(NAME(coneLights)));

	// prepare matrices first

	preparedLights.clear();
	preparedConeLights.clear();
	for_every(light, lights)
	{
		if (!light->coneDir.is_zero())
		{
			if (maxConeLights)
			{
				preparedConeLights.grow_size(1);
				light->turn_into_matrix(preparedConeLights.get_last(), Matrix44::identity);
			}
		}
		else
		{
			if (maxLights)
			{
				preparedLights.grow_size(1);
				light->turn_into_matrix(preparedLights.get_last(), Matrix44::identity);
			}
		}
	}

	if (maxLights)
	{
		sort(preparedLights, light_matrix_compare);
		int lights = min(maxLights, preparedLights.get_size());
		preparedLights.set_size(lights);
	}

	if (maxConeLights)
	{
		sort(preparedConeLights, light_matrix_compare);
		int coneLights = min(maxConeLights, preparedConeLights.get_size());
		preparedConeLights.set_size(coneLights);
	}

	prepared = true;
}

void LightSourceSetForRoomProxy::setup_shader_binding_context_program_with_prepared(::System::ShaderProgramBindingContext& _shaderProgramBindingContext, System::VideoMatrixStack const& _matrixStack) const
{
	an_assert(prepared);

	ArrayStatic<Matrix44, 64> toSendLights = preparedLights; SET_EXTRA_DEBUG_INFO(toSendLights, TXT("LightSourceSetForRoomProxy::setup_shader_binding_context_program_with_prepared.toSendLights"));
	ArrayStatic<Matrix44, 64> toSendConeLights = preparedConeLights; SET_EXTRA_DEBUG_INFO(toSendConeLights, TXT("LightSourceSetForRoomProxy::setup_shader_binding_context_program_with_prepared.toSendConeLights"));

	Matrix44 modelViewMatrix = _matrixStack.get_current();
	_matrixStack.ready_for_rendering(REF_ modelViewMatrix);

	for_every(lightMatrix, toSendLights)
	{
		Vector3 location(lightMatrix->m00, lightMatrix->m01, lightMatrix->m02);
		location = modelViewMatrix.location_to_world(location);
		lightMatrix->m00 = location.x;
		lightMatrix->m01 = location.y;
		lightMatrix->m02 = location.z;

		Vector3 stick(lightMatrix->m30, lightMatrix->m31, lightMatrix->m32);
		stick = modelViewMatrix.vector_to_world(stick);
		lightMatrix->m30 = stick.x;
		lightMatrix->m31 = stick.y;
		lightMatrix->m32 = stick.z;
	}

	for_every(lightMatrix, toSendConeLights)
	{
		Vector3 location(lightMatrix->m00, lightMatrix->m01, lightMatrix->m02);
		location = modelViewMatrix.location_to_world(location);
		lightMatrix->m00 = location.x;
		lightMatrix->m01 = location.y;
		lightMatrix->m02 = location.z;

		Vector3 coneDir(lightMatrix->m10, lightMatrix->m11, lightMatrix->m12);
		coneDir = modelViewMatrix.vector_to_world(coneDir);
		lightMatrix->m10 = coneDir.x;
		lightMatrix->m11 = coneDir.y;
		lightMatrix->m12 = coneDir.z;
	}

	_shaderProgramBindingContext.access_shader_param(NAME(lightsCount)).param.set_uniform(toSendLights.get_size());
	_shaderProgramBindingContext.access_shader_param(NAME(lights)).param.set_uniform(toSendLights.get_data(), toSendLights.get_size());

	_shaderProgramBindingContext.access_shader_param(NAME(coneLightsCount)).param.set_uniform(toSendConeLights.get_size());
	_shaderProgramBindingContext.access_shader_param(NAME(coneLights)).param.set_uniform(toSendConeLights.get_data(), toSendConeLights.get_size());
}

void LightSourceSetForRoomProxy::add(LightSource const & _light, Transform const& _placement)
{
	if (_light.flickeringLight && !LightSource::allowFlickeringLights)
	{
		return;
	}
	if (_light.power <= 0.0f)
	{
		return;
	}
	if (lights.get_size() < lights.get_max_size())
	{
		lights.set_size(lights.get_size() + 1);
		auto & light = lights.get_last();
		light.copy_from(_light);
		light.location = _placement.location_to_world(light.location);
		if (!light.coneDir.is_zero())
		{
			light.coneDir = _placement.vector_to_world(light.coneDir);
		}
		if (!light.stick.is_zero())
		{
			light.stick = _placement.vector_to_world(light.stick);
		}
	}
	else
	{
		todo_important(TXT("select!"));
	}
}

void LightSourceSetForRoomProxy::add(CustomModules::LightSources const * _lightSources, Transform const& _placement)
{
	_lightSources->for_every_light_source([this, _placement](LightSource const& _ls)
		{
			add(_ls, _placement);
		});
}
