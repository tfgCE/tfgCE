#include "artDir.h"

#include "game\gameDirector.h"
#include "game\gameSettings.h"

#include "pilgrimage\pilgrimageInstanceOpenWorld.h"

#include "..\core\mainConfig.h"
#include "..\core\mainSettings.h"
#include "..\core\mesh\mesh3dInstanceImpl.inl"
#include "..\core\mesh\mesh3dInstanceSetInlinedImpl.inl"
#include "..\core\types\colour.h"
#include "..\core\types\dirFourClockwise.h"
#include "..\core\system\video\video3d.h"
#include "..\core\system\video\shaderProgram.h"

#include "..\framework\game\game.h"
#include "..\framework\render\doorInRoomProxy.h"
#include "..\framework\render\environmentProxy.h"
#include "..\framework\render\presenceLinkProxy.h"
#include "..\framework\render\renderSystem.h"
#include "..\framework\render\roomProxy.h"
#include "..\framework\render\sceneBuildContext.h"
#include "..\framework\world\doorInRoom.h"
#include "..\framework\world\room.h"
#include "..\framework\video\texturePart.h"

#ifdef AN_DEVELOPMENT
#include "..\core\system\input.h"
#endif

#include "..\core\debug\debugRenderer.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

// art dir custom parameters set
DEFINE_STATIC_NAME(artDir);

// ??
DEFINE_STATIC_NAME(maxVoxelDistanceCoef);
DEFINE_STATIC_NAME(skyBox);
DEFINE_STATIC_NAME(voxelSize);
DEFINE_STATIC_NAME(voxelSizeInv);
DEFINE_STATIC_NAME(globalTint);
DEFINE_STATIC_NAME(globalDesaturate);
DEFINE_STATIC_NAME(globalDesaturateChangeInto);
DEFINE_STATIC_NAME(unifiedBackgroundColour);
DEFINE_STATIC_NAME(unifiedApplyFogColour);
DEFINE_STATIC_NAME(unifiedApplyFog);
DEFINE_STATIC_NAME(backgroundUp);
DEFINE_STATIC_NAME(backgroundAnchor);
DEFINE_STATIC_NAME(autoFogDistanceBackgroundAnchorXY);
DEFINE_STATIC_NAME(fogDistanceCoef);
DEFINE_STATIC_NAME(aboveFogDistanceCoef);
DEFINE_STATIC_NAME(useAboveFog_posNormUpSeg);
DEFINE_STATIC_NAME(useAboveFog_verticalSeg);
DEFINE_STATIC_NAME(applyFogSoFar);
DEFINE_STATIC_NAME(fogSoFarDistance);
DEFINE_STATIC_NAME(fogSoFarAmountRaw);
DEFINE_STATIC_NAME(fogSoFarColour);
DEFINE_STATIC_NAME(lightDir);
DEFINE_STATIC_NAME(backgroundSkyUseColourUpSeg);
DEFINE_STATIC_NAME(backgroundSkyColourUp);
DEFINE_STATIC_NAME(backgroundSkyColourOntoLight);
DEFINE_STATIC_NAME(backgroundSkyColourAwayLight);
DEFINE_STATIC_NAME(backgroundFogUseColourDownSeg);
DEFINE_STATIC_NAME(backgroundFogColourDown);
DEFINE_STATIC_NAME(backgroundFogColourOntoLight);
DEFINE_STATIC_NAME(backgroundFogColourAwayLight);
DEFINE_STATIC_NAME(backgroundUseFogSeg);
DEFINE_STATIC_NAME(uniformBackgroundFog);
DEFINE_STATIC_NAME(uniformBackgroundFogColour);
DEFINE_STATIC_NAME(useBackgroundVerticalForFogSeg);
DEFINE_STATIC_NAME(dontUseBackgroundUpForFogSeg);

// shader params
DEFINE_STATIC_NAME(inDirections);
DEFINE_STATIC_NAME(inDirectionsStart);
DEFINE_STATIC_NAME(inDirectionsRight);
DEFINE_STATIC_NAME(inDirectionsUp);
DEFINE_STATIC_NAME(inDirectionsColour);
DEFINE_STATIC_NAME(emissiveColour);
DEFINE_STATIC_NAME(emissiveBaseColour);
DEFINE_STATIC_NAME(emissivePower);

// shader option
DEFINE_STATIC_NAME(makeItFaster);

// tags
DEFINE_STATIC_NAME(lockIndicator);

// system tags
DEFINE_STATIC_NAME(android);
DEFINE_STATIC_NAME(androidHeadset);

// environment param
DEFINE_STATIC_NAME(ignoreSoFar);

//

using namespace TeaForGodEmperor;

//

Colour artDir_globalTint = Colour(0.0f, 0.0f, 0.0f, 0.0f);
float artDir_globalTintFade = 0.0f;
Colour artDir_globalDesaturate = Colour(0.0f, 0.0f, 0.0f, 0.0f);
Colour artDir_globalDesaturateChangeInto = Colour(0.0f, 0.0f, 0.0f, 0.0f); // keep as it is

struct LockIndicatorSetup
{
	bool setup = false;
	Colour locked = Colour::red;
	Colour open = Colour::green;
	Colour opening = Colour::green;
	Colour overridingLock = Colour::yellow;
} artDir_lockIndicatorSetup;

// build in uniforms
void ArtDir::set_global_tint(Optional<Colour> const& _colour)
{
	artDir_globalTint = _colour.get(Colour(0.0f, 0.0f, 0.0f, 0.0f));
}

void ArtDir::set_global_tint_fade(Optional<float> const& _fade)
{
	artDir_globalTintFade = _fade.get(0.0f);
}

void ArtDir::set_global_desaturate(Optional<Colour> const& _colour, Optional<Colour> const& _changeInto)
{
	artDir_globalDesaturate = _colour.get(Colour(0.0f, 0.0f, 0.0f, 0.0f));
	artDir_globalDesaturateChangeInto = _changeInto.get(Colour(0.0f, 0.0f, 0.0f, 0.0f));
}

//

#define GET_PARAM(Type, var, method) \
	Type var = zero<Type>(); \
	if (_shaderProgram) \
	{ \
		auto* param = _shaderProgram->get_current_values().get_shader_param(NAME(var)); \
		if (param && param->is_set()) \
		{ \
			var = param->method(); \
		} \
	} \
	else if (_environmentParams) \
	{ \
		auto* param = _environmentParams->get_param(NAME(var)); \
		if (param) \
		{ \
			var = param->method(); \
		} \
	}

Vector3 blend_rgb(float _alpha, Vector3 const & _a, Vector3 const& _b)
{
	return _a * (1.0f - _alpha) + _b * _alpha;
}

Vector4 blend_rgba(float _alpha, Vector4 const & _a, Vector4 const& _b)
{
	return _a * (1.0f - _alpha) + _b * _alpha;
}

float to_local_of_segment01(float _world, Vector2 const & _segment)
{
	return (_world - _segment.x) * _segment.y;
}

float clamped_to_local_of_segment01(float _world, Vector2 const & _segment)
{
	return clamp((_world - _segment.x) * _segment.y, 0.0f, 1.0f);
}

float linear01_to_quadratic01(float _a)
{
	_a = (_a - 0.5f) * 2.0f;
	float signA = sign(_a);
	_a = 1.0f - abs(_a);
	_a = _a * _a;
	_a = signA * (1.0f - _a);
	_a = (_a + 1.0f) * 0.5f;
	return _a;
}

float calculate_apply_fog(::System::ShaderProgram* _shaderProgram, Framework::EnvironmentParams const* _environmentParams,
	// other variables already read and processing
	float prcPositionDistance, float prcPositionNormalisedAlongBackgroundUp, float prcPositionAlongBackgroundUpAnchorRel, float fogDistanceCoef, float aboveFogDistanceCoef,
	OUT_ float & raw_applyFog, OUT_ float & _useAboveFog, bool _forActualObject)
{
	GET_PARAM(Vector2, useAboveFog_posNormUpSeg, get_as_vector2);
	GET_PARAM(Vector2, useAboveFog_verticalSeg, get_as_vector2);
	GET_PARAM(float, applyFogSoFar, get_as_float);
	GET_PARAM(float, fogSoFarDistance, get_as_float);
	GET_PARAM(float, fogSoFarAmountRaw, get_as_float);

	if (!_forActualObject)
	{
		//applyFogSoFar = 1.0f;
	}

	float offsetDistance = max(0.0f, prcPositionDistance - fogSoFarDistance * applyFogSoFar);
	float applyLowFog = min(fogSoFarAmountRaw * applyFogSoFar + offsetDistance * fogDistanceCoef, 1.0f);
	float useAboveFog_posNorm = to_local_of_segment01(prcPositionNormalisedAlongBackgroundUp, useAboveFog_posNormUpSeg);
	float useAboveFog_vert = to_local_of_segment01(prcPositionAlongBackgroundUpAnchorRel, useAboveFog_verticalSeg);
	float useAboveFog = clamp(max(useAboveFog_posNorm, useAboveFog_vert), 0.0f, 1.0f);
	float applyAboveFog = min(fogSoFarAmountRaw * applyFogSoFar + offsetDistance * aboveFogDistanceCoef, 1.0f);
	float applyFog = lerp(useAboveFog, applyLowFog, applyAboveFog);

	// output
	_useAboveFog = useAboveFog;
	raw_applyFog = applyFog;
	applyFog = 1.0f - (1.0f - applyFog) * (1.0f - applyFog);
	return applyFog;
}

float calculate_force_use_background_for_fog(::System::ShaderProgram* _shaderProgram, Framework::EnvironmentParams const* _environmentParams,
	// other variables already read and processing
	float prcPositionAlongBackgroundUpAnchorRel)
{
	GET_PARAM(Vector2, useBackgroundVerticalForFogSeg, get_as_vector2);

	float useBk = clamped_to_local_of_segment01(prcPositionAlongBackgroundUpAnchorRel, useBackgroundVerticalForFogSeg);
	return useBk;
}

float calculate_force_use_background_for_fog_limit(float _applyFog)
{
	return 1.0f - _applyFog;
}

void calculate_background_colour(::System::ShaderProgram* _shaderProgram, Framework::EnvironmentParams const* _environmentParams,
	// input
	Vector3 const & _positionNormalised,
	// other variables already read and processing
	Vector3 const & prcLightDir, Vector3 const & backgroundUp,
	// output (global prc variables)
	OUT_ OPTIONAL_ Vector3 * _prcBackgroundColour,
	OUT_ OPTIONAL_ Vector3 * _prcBackgroundFogColour)
{
	GET_PARAM(Vector2, backgroundSkyUseColourUpSeg, get_as_vector2);
	GET_PARAM(Colour, backgroundSkyColourUp, get_as_colour);
	GET_PARAM(Colour, backgroundSkyColourAwayLight, get_as_colour);
	GET_PARAM(Colour, backgroundSkyColourOntoLight, get_as_colour);
	
	GET_PARAM(Vector2, backgroundFogUseColourDownSeg, get_as_vector2);
	GET_PARAM(Colour, backgroundFogColourDown, get_as_colour);
	GET_PARAM(Colour, backgroundFogColourOntoLight, get_as_colour);
	GET_PARAM(Colour, backgroundFogColourAwayLight, get_as_colour);

	GET_PARAM(Vector2, backgroundUseFogSeg, get_as_vector2);
	
	GET_PARAM(float, uniformBackgroundFog, get_as_float);
	GET_PARAM(Colour, uniformBackgroundFogColour, get_as_colour);
	
	if (uniformBackgroundFog >= 0.5f)
	{
		Vector3 fogColour = uniformBackgroundFogColour.to_vector3();
		assign_optional_out_param(_prcBackgroundColour, fogColour);
		assign_optional_out_param(_prcBackgroundFogColour, fogColour);
	}
	else
	{
		float pnWithLight = Vector3::dot(prcLightDir, _positionNormalised);
		float pnWithUp = Vector3::dot(backgroundUp, _positionNormalised);
		float pnWithLightNormalised = pnWithLight * 0.5f + 0.5f;
	
		/* calculate pure sky colour */
		float useSkyUp = clamped_to_local_of_segment01(pnWithUp, backgroundSkyUseColourUpSeg) * backgroundSkyColourUp.a;
		Vector3 actualSkyColour = blend_rgb(pnWithLightNormalised, backgroundSkyColourOntoLight.to_vector3(), backgroundSkyColourAwayLight.to_vector3());
		actualSkyColour = blend_rgb(useSkyUp, actualSkyColour, backgroundSkyColourUp.to_vector3());
		// not used by default actualSkyColour = adjust_background_sky_colour(actualSkyColour, pnWithLight);
	
		/* calculate pure fog colour */
		float useFogDown = clamped_to_local_of_segment01(pnWithUp, backgroundFogUseColourDownSeg) * backgroundFogColourDown.a;
		Vector3 actualFogColour = blend_rgb(pnWithLightNormalised, backgroundFogColourOntoLight.to_vector3(), backgroundFogColourAwayLight.to_vector3());
		actualFogColour = blend_rgb(useFogDown, actualFogColour, backgroundFogColourDown.to_vector3());

		/* how it should blend between sky and fog */
		float useFog = clamped_to_local_of_segment01(pnWithUp, backgroundUseFogSeg);
		useFog = linear01_to_quadratic01(useFog); /* make it softer */
		// not used by default useFog = adjust_background_use_fog(useFog);
	
		Vector3 fogColour = blend_rgb(useFog, actualSkyColour, actualFogColour);

		assign_optional_out_param(_prcBackgroundColour, fogColour);
		assign_optional_out_param(_prcBackgroundFogColour, actualFogColour);
	}
}

struct ShaderProgramValuesForLocation
{
	Vector3 backgroundColour = Vector3::zero;

	Vector3 applyFogColour = Vector3::zero;
	float applyFogAmount = 0.0f; // this is using 1 - sqr(1 - applyFogRaw)
	float applyFogAmountRaw = 0.0f;
	float fogDistance = 0.0f;
};

ShaderProgramValuesForLocation calculate_shader_program_values_for_location(::System::ShaderProgram* _shaderProgram, Framework::EnvironmentParams const * _environmentParams, Matrix44 const& _renderingBufferModelViewMatrixForRendering, Vector3 const& _location, bool _moveShaderParamsToMVS, bool _forActualObject)
{
	ShaderProgramValuesForLocation result;

	GET_PARAM(Vector3, backgroundUp, get_as_vector3);
	GET_PARAM(Vector3, backgroundAnchor, get_as_vector3);
	GET_PARAM(float, fogDistanceCoef, get_as_float);
	GET_PARAM(float, aboveFogDistanceCoef, get_as_float);
	GET_PARAM(Vector3, lightDir, get_as_vector3);
	GET_PARAM(float, applyFogSoFar, get_as_float);
	GET_PARAM(float, fogSoFarAmountRaw, get_as_float);
	GET_PARAM(Vector3, fogSoFarColour, get_as_vector3);

	if (!_forActualObject)
	{
		//applyFogSoFar = 1.0f;
	}

	// processing variables
	Vector3 backgroundUpMVS = _moveShaderParamsToMVS? _renderingBufferModelViewMatrixForRendering.vector_to_world(backgroundUp) : backgroundUp;
	Vector3 backgroundAnchorMVS = _moveShaderParamsToMVS? _renderingBufferModelViewMatrixForRendering.location_to_world(backgroundAnchor) : backgroundAnchor;
	Vector3 prcPosition = _renderingBufferModelViewMatrixForRendering.location_to_world(_location);
	Vector3 prcPositionNormalised = prcPosition.normal();
	float prcPositionDistance = prcPosition.length();
	float prcPositionAlongBackgroundUpAnchorRel = Vector3::dot(backgroundUpMVS, prcPosition - backgroundAnchorMVS);
	float prcPositionNormalisedAlongBackgroundUp = Vector3::dot(backgroundUpMVS, prcPositionNormalised);
	Vector3 prcLightDir = _moveShaderParamsToMVS? _renderingBufferModelViewMatrixForRendering.vector_to_world(lightDir) : lightDir;

	Vector3 prcBackgroundColour;
	Vector3 prcBackgroundFogColour;
	calculate_background_colour(_shaderProgram, _environmentParams, prcPositionNormalised,
		prcLightDir, backgroundUpMVS, &prcBackgroundColour, &prcBackgroundFogColour);
	result.backgroundColour = prcBackgroundColour;

	{
		// actual calculations, same as above for calculating fog!

		// output
		Vector3 outFogColour = Vector3::zero;
		float outFogAmount = 0.0f;
		float outFogAmountRaw = 0.0f;

		{
			/* standard fog */
			/* how relative to camera position faces light (we're looking into the light or away from it) */
			float applyFogRaw = 0.0f;
			float useAboveFog = 0.0f;
			float applyFog = calculate_apply_fog(_shaderProgram, _environmentParams, prcPositionDistance, prcPositionNormalisedAlongBackgroundUp, prcPositionAlongBackgroundUpAnchorRel, fogDistanceCoef, aboveFogDistanceCoef, OUT_ applyFogRaw, OUT_ useAboveFog, _forActualObject);
			float forceBk = calculate_force_use_background_for_fog(_shaderProgram, _environmentParams, prcPositionAlongBackgroundUpAnchorRel);
			forceBk = min(forceBk, 1.0f - useAboveFog);
			float forceBkLimit = calculate_force_use_background_for_fog_limit(applyFog);
			float forceBkUseColour = min(forceBk, forceBkLimit);
			applyFog = applyFog * (1.0f - forceBk) + forceBk;
			{
				Vector3 thisFogColour;
				thisFogColour = prcBackgroundColour;
				thisFogColour = forceBkUseColour * prcBackgroundFogColour + (1.0f - forceBkUseColour) * thisFogColour;
				float useFogSoFar = min(1.0f, min(applyFogRaw, fogSoFarAmountRaw)) / max(0.01f, min(1.0f, applyFogRaw)) * applyFogSoFar;
				thisFogColour = fogSoFarColour * useFogSoFar + (1.0f - useFogSoFar) * thisFogColour;
				outFogColour = thisFogColour;
				outFogAmount = applyFog;
				outFogAmountRaw = applyFogRaw;
			}
		}

		// store result
		result.fogDistance = prcPositionDistance;
		result.applyFogColour = outFogColour;
		result.applyFogAmount = outFogAmount;
		result.applyFogAmountRaw = outFogAmountRaw;
	}

	return result;
}

void setup_shader_program_bound_for_location(::System::ShaderProgram* _shaderProgram, Matrix44 const& _renderingBufferModelViewMatrixForRendering, Vector3 const & _location)
{
	ShaderProgramValuesForLocation values = calculate_shader_program_values_for_location(_shaderProgram, nullptr, _renderingBufferModelViewMatrixForRendering, _location, false, true);

	int unifiedBackgroundColourIdx = _shaderProgram->find_uniform_index(NAME(unifiedBackgroundColour));
	int unifiedApplyFogColourIdx = _shaderProgram->find_uniform_index(NAME(unifiedApplyFogColour));
	int unifiedApplyFogIdx = _shaderProgram->find_uniform_index(NAME(unifiedApplyFog));

	if (unifiedBackgroundColourIdx != NONE)
	{
		_shaderProgram->set_uniform(unifiedBackgroundColourIdx, values.backgroundColour);
	}

	if (unifiedApplyFogColourIdx != NONE &&
		unifiedApplyFogIdx != NONE)
	{
		// store result
		_shaderProgram->set_uniform(unifiedApplyFogColourIdx, values.applyFogColour);
		_shaderProgram->set_uniform(unifiedApplyFogIdx, values.applyFogAmount);
	}
}

void ArtDir::setup_rendering()
{
	//
	// --- functions (built-in) ---
	//
	/*
	 * segments
	 * 		vec2 segment01 is
	 *		x start
	 *		y length inverted (1 / length)
	 *	with this we can change value from world z to local n:
	 *		if z is at start, we have 0
	 *		if z is at start+length, we have 1
	 *		
	 *		n = (z - x) * y
	 */
	bool androidHeadset = false; // bits are hardcoded right now
#ifdef AN_QUEST
	androidHeadset = true;
#endif
#ifdef AN_VIVE
	androidHeadset = true;
#endif
#ifdef AN_PICO
	androidHeadset = true;
#endif
	if (::MainConfig::global().get_forced_system() == TXT("quest") ||
		::MainConfig::global().get_forced_system() == TXT("vive") ||
		::MainConfig::global().get_forced_system() == TXT("pico") ||
		::System::Core::get_system_tags().get_tag(NAME(android)) ||
		::System::Core::get_system_tags().get_tag(NAME(androidHeadset)))
	{
		androidHeadset = true;
	}
	{
		String func;
		func += TXT("float to_local_of_segment01(float _world, vec2 _segment)\n");
		func += TXT("{\n");
		func += TXT("  return (_world - _segment.x) * _segment.y;\n");
		func += TXT("}\n");
		::System::ShaderSourceCustomisation::get()->functions.push_back(func);
	}
	{
		String func;
		func += TXT("float clamped_to_local_of_segment01(float _world, vec2 _segment)\n");
		func += TXT("{\n");
		func += TXT("  return clamp((_world - _segment.x) * _segment.y, 0.0, 1.0);\n");
		func += TXT("}\n");
		::System::ShaderSourceCustomisation::get()->functions.push_back(func);
	}
	// ---
	{
		String func;
		func += TXT("float calc_noise(HIGHP vec3 _pos, HIGHP float _time)\n");
		func += TXT("{\n");
		func += TXT("  HIGHP vec2 p = vec2(_pos.x * 54.234 + _pos.z * 23.252, _pos.y * 94.345 + _pos.z * 12.579);\n");
		func += TXT("  \n");
		func += TXT("  p += vec2(0.07 * fract(_time));\n");
		func += TXT("  p = fract(p * vec2(5.3987, 5.4421));\n");
		func += TXT("  p += dot(p.yx, p.xy + vec2(21.5351, 14.3137));\n");
		func += TXT("  \n");
		func += TXT("  HIGHP float pxy = p.x * p.y;\n");
		func += TXT("  HIGHP float tri = (fract(pxy * 95.4307) + fract(pxy * 75.04961)) * 0.5;\n");
		func += TXT("  return tri;\n");
		func += TXT("}\n");
		::System::ShaderSourceCustomisation::get()->functions.push_back(func);
	}
	// ---
	{
		String func;
		func += TXT("float linear01_to_quadratic01(float _a)\n");
		func += TXT("{\n");
		/*
		// original
		func += TXT("  _a = (_a - 0.5) * 2.0;\n");
		func += TXT("  float signA = sign(_a);\n");
		func += TXT("  _a = 1.0 - abs(_a);\n");
		func += TXT("  _a = _a * _a;\n");
		func += TXT("  _a = signA * (1.0 - _a);\n");
		func += TXT("  _a = (_a + 1.0) * 0.5;\n");
		*/
		func += TXT("  _a = fma(_a, 2.0, -1.0);\n"); /* (_a - 0.5) * 2.0 = _a * 2.0 - 1.0 */
		func += TXT("  float signA = sign(_a);\n");
		func += TXT("  _a = 1.0 - abs(_a);\n");
		func += TXT("  _a = _a * _a;\n");
		func += TXT("  _a = fma(-_a, signA, signA);\n"); /* signA * (1.0 - _a) = signA - signA * _a = -_a * signA + signA*/
		func += TXT("  _a = fma(_a, 0.5, 0.5);\n"); /* (_a + 1.0) * 0.5 = _a * 0.5 + 0.5 */
		func += TXT("  return _a;\n");
		func += TXT("}\n");
		::System::ShaderSourceCustomisation::get()->functions.push_back(func);
	}
	// ---

	{
		String func;
		func += TXT("vec3 blend_rgb(float _alpha, vec3 _a, vec3 _b)\n");
		func += TXT("{\n");
		func += TXT("  return mix(_a, _b, _alpha);\n");
		func += TXT("}\n");
		::System::ShaderSourceCustomisation::get()->functions.push_back(func);
	}
	{
		String func;
		func += TXT("vec4 blend_rgba(float _alpha, vec4 _a, vec4 _b)\n");
		func += TXT("{\n");
		func += TXT("  return mix(_a, _b, _alpha);\n");
		func += TXT("}\n");
		::System::ShaderSourceCustomisation::get()->functions.push_back(func);
	}
	// ---
	{
		::System::ShaderSourceCustomisation::get()->data.push_back(String(TXT("float prcRandomValue;")));
	}
	// ---
	{
		String func;
		func += TXT("void prepare_random_value_random(HIGHP vec3 _position, HIGHP float _time)\n");
		func += TXT("{\n");
		func += TXT("  \n");
		func += TXT("  HIGHP vec2 p = vec2(_position.x * 54.234 + _position.z * 23.252, _position.y * 94.345 + _position.z * 12.579); \n");
		func += TXT("  \n");
		func += TXT("  p += vec2(0.07 * fract(_time)); \n");
		func += TXT("  p = fract(p * vec2(5.3987, 5.4421)); \n");
		func += TXT("  p += dot(p.yx, p.xy + vec2(21.5351, 14.3137)); \n");
		func += TXT("  \n");
		func += TXT("  HIGHP float pxy = p.x * p.y; \n");
		func += TXT("  prcRandomValue = (fract(pxy * 95.4307) + fract(pxy * 75.04961)) * 0.5; \n");
		func += TXT("}\n");
		::System::ShaderSourceCustomisation::get()->functions.push_back(func);
	}
	// ---
	{
		String func;
		func += TXT("vec3 apply_anti_banding_to(LOWP vec3 _colour)\n");
		func += TXT("{\n");
		func += TXT("  float tri = prcRandomValue; \n");
		if (androidHeadset)
		{
			//func += TXT("  _colour.rgb += (-0.5 + tri * 2.0) * 0.01; // stronger\n");
			// (-0.5 + tri * 2.0) * 0.01
			// -0.5 * 0.01 + tri * 2.0 * 0.01
			// tri * 0.02 - 0.005
			func += TXT("  _colour.rgb += fma(tri, 0.02, -0.005); // stronger\n");
		}
		else
		{
			//func += TXT("  _colour.rgb += 0.00390625 * (-0.5 + tri * 2.0); \n");
			func += TXT("  _colour.rgb += 0.00390625 * fma(tri, 2.0, -0.5); \n");
		}
		func += TXT("  return _colour; \n");
		func += TXT("}\n");
		::System::ShaderSourceCustomisation::get()->functions.push_back(func);
	}
	// ---
	{
		String func;
		func += TXT("vec3 apply_flat_anti_banding_to(LOWP vec3 _colour)\n");
		func += TXT("{\n");
		func += TXT("  float tri = 0.5; \n");
		if (androidHeadset)
		{
			//func += TXT("  _colour.rgb += (-0.5 + tri * 2.0) * 0.01; // stronger\n");
			// (-0.5 + tri * 2.0) * 0.01
			// -0.5 * 0.01 + tri * 2.0 * 0.01
			// tri * 0.02 - 0.005
			func += TXT("  _colour.rgb += fma(tri, 0.02, -0.005); // stronger\n");
		}
		else
		{
			//func += TXT("  _colour.rgb += 0.00390625 * (-0.5 + tri * 2.0); \n");
			func += TXT("  _colour.rgb += 0.00390625 * fma(tri, 2.0, -0.5); \n");
		}
		func += TXT("  return _colour; \n");
		func += TXT("}\n");
		::System::ShaderSourceCustomisation::get()->functions.push_back(func);
	}
	//
	// --- macros ---
	//
	//#define nl macro += TXT(" \\\n");
	#define nl ;
	{
		String macro;
		macro += TXT("#define CALCULATE_BACKGROUND_COLOUR(out_backgroundColour, out_backgroundFogColour, _positionNormalised)"); nl;
		macro += TXT("{"); nl;
		macro += TXT("  if (uniformBackgroundFog >= 0.5)"); nl;
		macro += TXT("  {"); nl;
		macro += TXT("    LOWP vec3 fogColour = uniformBackgroundFogColour.rgb;"); nl;
		macro += TXT("    out_backgroundColour = fogColour;"); nl;
		macro += TXT("    out_backgroundFogColour = fogColour;"); nl;
		macro += TXT("  }"); nl;
		macro += TXT("  else"); nl;
		macro += TXT("  {"); nl;
		macro += TXT("    HIGHP float pnWithLight = dot(prcLightDir, _positionNormalised);"); nl;
		macro += TXT("    float pnWithUp = dot(backgroundUp, _positionNormalised);"); nl;
		macro += TXT("    float pnWithLightNormalised = fma(pnWithLight, 0.5, 0.5);"); nl;
		macro += TXT("    "); nl;
		macro += TXT("    /* calculate pure sky colour */"); nl;
		macro += TXT("    float useSkyUp = clamped_to_local_of_segment01(pnWithUp, backgroundSkyUseColourUpSeg) * backgroundSkyColourUp.a;"); nl;
		macro += TXT("    LOWP vec3 actualSkyColour = blend_rgb(pnWithLightNormalised, backgroundSkyColourOntoLight.rgb, backgroundSkyColourAwayLight.rgb);"); nl;
		macro += TXT("    actualSkyColour = blend_rgb(useSkyUp, actualSkyColour, backgroundSkyColourUp.rgb);"); nl;
		macro += TXT("    actualSkyColour = adjust_background_sky_colour(actualSkyColour, pnWithLight);"); nl;
		macro += TXT("    "); nl;
		macro += TXT("    /* calculate pure fog colour */"); nl;
		macro += TXT("    LOWP float useFogDown = clamped_to_local_of_segment01(pnWithUp, backgroundFogUseColourDownSeg) * backgroundFogColourDown.a;"); nl;
		macro += TXT("    LOWP vec3 actualFogColour = blend_rgb(pnWithLightNormalised, backgroundFogColourOntoLight.rgb, backgroundFogColourAwayLight.rgb);"); nl;
		macro += TXT("    actualFogColour = blend_rgb(useFogDown, actualFogColour, backgroundFogColourDown.rgb);"); nl;
		macro += TXT("    "); nl;
		macro += TXT("    /* how it should blend between sky and fog */"); nl;
		macro += TXT("    float useFog = clamped_to_local_of_segment01(pnWithUp, backgroundUseFogSeg);"); nl;
		macro += TXT("    useFog = linear01_to_quadratic01(useFog); /* make it softer */"); nl;
		macro += TXT("    useFog = adjust_background_use_fog(useFog);"); nl;
		macro += TXT("    "); nl;
		macro += TXT("    out_backgroundColour = blend_rgb(useFog, actualSkyColour, actualFogColour);"); nl;
		macro += TXT("    out_backgroundFogColour = actualFogColour;"); nl;
		macro += TXT("  }"); nl;
		macro += TXT("}");

		::System::ShaderSourceCustomisation::get()->macros.push_back(macro);
	}
	{
		String macro;
		macro += TXT("#define CALCULATE_FOG(out_fogColour, out_fogAmount)"); nl;
		macro += TXT("{"); nl;
		macro += TXT("  out_fogColour = vec3(0.0, 0.0, 0.0);"); nl;
		macro += TXT("  out_fogAmount = 0.0;"); nl;
		macro += TXT("  LOWP float accumulatedFog = 0.0;"); nl;
		macro += TXT("  {"); nl;
		macro += TXT("	  /* standard fog */"); nl;
		if (::MainSettings::global().get_shader_options().get_tag_as_int(NAME(makeItFaster)))
		{
			macro += TXT("      calculate_background_colour();"); nl;
		}
		macro += TXT("	  /* how relative to camera position faces light (we're looking into the light or away from it) */"); nl;
		macro += TXT("	  LOWP vec3 applyFogPack = calculate_apply_fog_pack();"); nl;
		macro += TXT("	  LOWP float applyFog = applyFogPack.x;"); nl;
		macro += TXT("	  LOWP float applyFogRaw = applyFogPack.y;"); nl;
		macro += TXT("	  LOWP float useAboveFog = applyFogPack.z;"); nl;
		macro += TXT("	  LOWP float forceBk = calculate_force_use_background_for_fog();"); nl; // required for underground stuff
		macro += TXT("	  forceBk = min(forceBk, 1.0 - useAboveFog);"); nl;
		macro += TXT("	  LOWP float forceBkLimit = calculate_force_use_background_for_fog_limit(applyFog);"); nl; // required for underground stuff
		macro += TXT("	  LOWP float forceBkUseColour = min(forceBk, forceBkLimit);"); nl;
		macro += TXT("	  applyFog = mix(applyFog, 1.0, forceBk); "); nl; /* applyFog * (1.0 - forceBk) + forceBk */
		macro += TXT("	  {"); nl;
		macro += TXT("	    LOWP vec3 thisFogColour = prcBackgroundColour;"); nl;
		macro += TXT("	    thisFogColour = mix(thisFogColour, prcBackgroundFogColour, forceBkUseColour);"); nl;
		macro += TXT("	    LOWP float useFogSoFar = min(1.0, min(applyFogRaw, fogSoFarAmountRaw)) / max(0.01, min(1.0, applyFogRaw)) * applyFogSoFar;"); nl;
		macro += TXT("	    thisFogColour = mix(thisFogColour, fogSoFarColour, useFogSoFar);"); nl;
		macro += TXT("	    out_fogColour = thisFogColour;"); nl;
		macro += TXT("	    out_fogAmount = applyFog;"); nl;
		macro += TXT("	  }"); nl;
		macro += TXT("  }"); nl;
		macro += TXT("} ");

		::System::ShaderSourceCustomisation::get()->macros.push_back(macro);
	}
	{
		String macro;
		macro += TXT("#define CALCULATE_FOG_SIMPLE(out_fogColour, out_fogAmount)"); nl;
		macro += TXT("{"); nl;
		macro += TXT("  out_fogColour = vec3(0.0, 0.0, 0.0);"); nl;
		macro += TXT("  out_fogAmount = 0.0;"); nl;
		macro += TXT("  LOWP float accumulatedFog = 0.0;"); nl;
		macro += TXT("  {"); nl;
		macro += TXT("	  /* standard fog */"); nl;
		if (::MainSettings::global().get_shader_options().get_tag_as_int(NAME(makeItFaster)))
		{
			macro += TXT("      calculate_background_colour();"); nl;
		}
		macro += TXT("	  /* how relative to camera position faces light (we're looking into the light or away from it) */"); nl;
		macro += TXT("	  LOWP vec3 applyFogPack = calculate_apply_fog_simple_pack();"); nl;
		macro += TXT("	  LOWP float applyFog = applyFogPack.x;"); nl;
		macro += TXT("	  {"); nl;
		macro += TXT("	    LOWP vec3 thisFogColour = prcBackgroundColour;"); nl;
		macro += TXT("	    out_fogColour = thisFogColour;"); nl;
		macro += TXT("	    out_fogAmount = applyFog;"); nl;
		macro += TXT("	  }"); nl;
		macro += TXT("  }"); nl;
		macro += TXT("} ");

		::System::ShaderSourceCustomisation::get()->macros.push_back(macro);
	}

	::Framework::Render::System::get()->setup_shader_program_on_bound_for_presence_link =
		[](::System::ShaderProgram* _shaderProgram, Matrix44 const& _renderingBufferModelViewMatrixForRendering, Matrix44 const& _placement, ::Framework::Render::PresenceLinkProxy const* _presenceLink)
	{
		setup_shader_program_bound_for_location(_shaderProgram, _renderingBufferModelViewMatrixForRendering, _placement.location_to_world(_presenceLink->get_placement_in_room().get_translation()));
	};

	::Framework::Render::System::get()->setup_shader_program_on_bound_for_door_in_room =
		[](::System::ShaderProgram* _shaderProgram, Matrix44 const& _renderingBufferModelViewMatrixForRendering, ::Framework::Render::DoorInRoomProxy const* _doorInRoom)
	{
		setup_shader_program_bound_for_location(_shaderProgram, _renderingBufferModelViewMatrixForRendering, _doorInRoom->get_placement_in_room().get_translation());
	};
}

void ArtDir::add_custom_build_in_uniforms()
{
	Array<Name> provideUniforms;
	// voxelSize should remain as it is
	provideUniforms.push_back(NAME(voxelSizeInv));
	provideUniforms.push_back(NAME(maxVoxelDistanceCoef));
	provideUniforms.push_back(NAME(globalTint));
	provideUniforms.push_back(NAME(globalDesaturate));
	provideUniforms.push_back(NAME(globalDesaturateChangeInto));

	::System::Video3D::get()->add_custom_set_build_in_uniforms(
		NAME(artDir), provideUniforms,
		[](::System::ShaderProgram* _shader)
		{
			int globalTintUniformID = _shader->find_uniform_index(NAME(globalTint));
			if (globalTintUniformID != NONE)
			{
				// if does not exist, won't be set
				Colour globalTint = artDir_globalTint;
				// doesn't work/look good -- globalTint = Colour::lerp(artDir_globalTintFade * 1.0f, globalTint, Colour::black);
				_shader->set_uniform(globalTintUniformID, globalTint.to_vector4());
			}
			int globalDesaturateUniformID = _shader->find_uniform_index(NAME(globalDesaturate));
			if (globalDesaturateUniformID != NONE)
			{
				// if does not exist, won't be set
				_shader->set_uniform(globalDesaturateUniformID, artDir_globalDesaturate.to_vector4());
			}
			int globalDesaturateChangeIntoUniformID = _shader->find_uniform_index(NAME(globalDesaturateChangeInto));
			if (globalDesaturateChangeIntoUniformID != NONE)
			{
				// if does not exist, won't be set
				_shader->set_uniform(globalDesaturateChangeIntoUniformID, artDir_globalDesaturateChangeInto.to_vector4());
			}
			int voxelSizeUniformID = _shader->find_uniform_index(NAME(voxelSize));
			if (voxelSizeUniformID != NONE)
			{
				if (auto const * vsParam = _shader->get_current_values().get_shader_param(voxelSizeUniformID))
				{
					float voxelSize = 0.1f;
					if (vsParam->is_set())
					{
						voxelSize = vsParam->get_as_float();
					}
					else
					{
						GLfloat glVoxelSize = 0.0f;
						int shaderParamId = _shader->get_uniform_shader_param_id(voxelSizeUniformID); // get uniform location
						if (shaderParamId != NONE)
						{
							DIRECT_GL_CALL_ glGetUniformfv(_shader->get_shader_program_id(), shaderParamId, &glVoxelSize); AN_GL_CHECK_FOR_ERROR
						}
						voxelSize = glVoxelSize;
						// cache it!
						_shader->set_uniform(voxelSizeUniformID, voxelSize);
					}
#ifdef AN_N_RENDERING
					if (::System::Input::get()->has_key_been_pressed(System::Key::N))
					{
						LogInfoContext log;
						log.log(TXT("built-in voxel size %.3f"), voxelSize);
						log.output_to_output();
					}
#endif
					int voxelSizeInvUniformID = _shader->find_uniform_index(NAME(voxelSizeInv));
					if (voxelSizeInvUniformID != NONE)
					{
						_shader->set_uniform(NAME(voxelSizeInv), voxelSize != 0.0f ? 1.0f / voxelSize : 1.0f);
					}

					auto v3d = ::System::Video3D::get();
					VectorInt2 viewportSize = v3d->get_viewport_size();

					// we need to find distance at which voxels become pixels
					// td = (tan_deg(fov * 0.5f)) * 2.0f;
					// correctedDistance = distance * td;
					// voxelSizeAtDistance = voxelSize / correctedDistance;
					// voxelSizeAtDistance = 1.0 / vsY;
					// then we have:
					// distance = correctedDistance / td;
					// correctedDistance = voxelSize / voxelSizeAtDisance;
					// then:
					// distance = (voxelSize * vsY) / td);

					float down = tan_deg(v3d->get_fov() * 0.5f) * 2.0f;
					float maxVoxelDistance = down != 0.0f ? voxelSize * (float)viewportSize.y / down : 1.0f;

					float maxVoxelDistanceCoef = 1.0f / maxVoxelDistance;
					int maxVoxelDistanceCoefShaderParamIdx = _shader->find_uniform_index(NAME(maxVoxelDistanceCoef));
					if (maxVoxelDistanceCoefShaderParamIdx != NONE)
					{
						_shader->set_uniform(maxVoxelDistanceCoefShaderParamIdx, maxVoxelDistanceCoef);
					}
				}
			}
		}
	);
}

void ArtDir::customise_game(Framework::Game* _game)
{
	_game->access_customisation().render.setup_environment_proxy = environment_proxy__setup;
	_game->access_customisation().render.setup_door_in_room_proxy = door_in_room_proxy__setup;
}

void ArtDir::environment_proxy__setup(Framework::Render::SceneBuildContext const& _context, Framework::Render::EnvironmentProxy* _environmentProxy, Framework::Render::EnvironmentProxy* _previousEnvironmentProxy, Framework::Room const * _forRoom)
{
	//environment_proxy__setup_sky_box_material_from_light(_context, _environmentProxy, _previousEnvironmentProxy, _forRoom);
	environment_proxy__setup_fog_so_far(_context, _environmentProxy, _previousEnvironmentProxy, _forRoom);
	environment_proxy__setup_background_anchor(_context, _environmentProxy, _previousEnvironmentProxy, _forRoom);
	environment_proxy__setup_pilgrimage_rotation(_context, _environmentProxy, _previousEnvironmentProxy, _forRoom);
}

void ArtDir::environment_proxy__setup_sky_box_material_from_light(Framework::Render::SceneBuildContext const& _context, Framework::Render::EnvironmentProxy* _environmentProxy, Framework::Render::EnvironmentProxy* _previousEnvironmentProxy, Framework::Room const* _forRoom)
{
	for_every(instance, _environmentProxy->access_apearance().access_instances())
	{
		for_count(int, i, instance->get_material_instance_count())
		{
			if (auto * material = instance->access_material_instance(i))
			{
				an_assert(material->get_material());
				if (material->get_material()->get_tags().get_tag(NAME(skyBox)))
				{
					// light dir is set through binding context
					// colours?
				}
			}
		}
	}
}

void ArtDir::environment_proxy__setup_fog_so_far(Framework::Render::SceneBuildContext const& _context, Framework::Render::EnvironmentProxy* _environmentProxy, Framework::Render::EnvironmentProxy* _previousEnvironmentProxy, Framework::Room const* _forRoom)
{
	ShaderProgramValuesForLocation values;
	if (_context.get_depth() > 0 && _previousEnvironmentProxy)
	{
		bool updateSoFar = true;
		if (auto* ignoreSoFar = _environmentProxy->access_params().get_param(NAME(ignoreSoFar)))
		{
			if (ignoreSoFar->get_as_float() > 0.5f)
			{
				updateSoFar = false;
			}
		}
		if (updateSoFar)
		{
			if (_previousEnvironmentProxy->get_environment() == _environmentProxy->get_environment() &&
				_previousEnvironmentProxy->get_for_room() && _environmentProxy->get_for_room() &&
				_previousEnvironmentProxy->get_for_room()->get_environment_overlays() == _environmentProxy->get_for_room()->get_environment_overlays())
			{
				// if both environments are the same and have the same environment overlays, just copy values.
				// this way, just in any case, we get better consistency
				values.fogDistance = _previousEnvironmentProxy->access_params().access_param(NAME(fogSoFarDistance), Framework::EnvironmentParamType::floatNumber)->as<float>();
				values.applyFogAmountRaw = _previousEnvironmentProxy->access_params().access_param(NAME(fogSoFarAmountRaw), Framework::EnvironmentParamType::floatNumber)->as<float>();
				values.applyFogColour = _previousEnvironmentProxy->access_params().access_param(NAME(fogSoFarColour), Framework::EnvironmentParamType::colour3)->as<Vector3>();
			}
			else
			{
				Vector3 location = Vector3::zero;
				{
					auto& doorLeave = _environmentProxy->get_for_room_proxy()->get_door_we_entered_trough_info();
					if (doorLeave.hole)
					{
						Vector3 cameraLocWS = _context.get_view_matrix().inverted().get_translation();
						Vector3 pointOnDoorPlaneWS = doorLeave.plane.get_dropped(cameraLocWS);
						Vector3 pointOnDoorPlaneDS = doorLeave.outboundMatrix.location_to_local(pointOnDoorPlaneWS);
						Vector3 closestPointOnDoorDS = doorLeave.hole->move_point_inside(doorLeave.side, doorLeave.holeOutboundMeshScale, pointOnDoorPlaneDS, 0.0f);
						Vector3 closestPointOnDoorWS = doorLeave.outboundMatrix.location_to_world(closestPointOnDoorDS);
						location = closestPointOnDoorWS;
					}
				}

				// we use previous as we want to calculate what was before, up to the door
				values = calculate_shader_program_values_for_location(nullptr, &_previousEnvironmentProxy->access_params(),
					_context.get_view_matrix(), location, true, false);
			}
		}
	}	
	_environmentProxy->access_params().access_param(NAME(fogSoFarDistance), Framework::EnvironmentParamType::floatNumber)->as<float>() = values.fogDistance;
	_environmentProxy->access_params().access_param(NAME(fogSoFarAmountRaw), Framework::EnvironmentParamType::floatNumber)->as<float>() = values.applyFogAmountRaw;
	_environmentProxy->access_params().access_param(NAME(fogSoFarColour), Framework::EnvironmentParamType::colour3)->as<Vector3>() = values.applyFogColour;
}

void ArtDir::environment_proxy__setup_background_anchor(Framework::Render::SceneBuildContext const& _context, Framework::Render::EnvironmentProxy* _environmentProxy, Framework::Render::EnvironmentProxy* _previousEnvironmentProxy, Framework::Room const* _forRoom)
{
	if (auto* ep = _environmentProxy->access_params().access_param(NAME(backgroundAnchor)))
	{
		ep->as<Vector3>() = _environmentProxy->get_matrix_from_room_proxy().get_translation();
	}
	if (false) // disabled for now
	if (auto* autoFogXY = _environmentProxy->access_params().access_param(NAME(autoFogDistanceBackgroundAnchorXY)))
	{
		float autoFogDistanceBackgroundAnchorXY = autoFogXY->as<float>();
		if (autoFogDistanceBackgroundAnchorXY > 0.0f)
		{
			if (auto* fogDistanceCoef = _environmentProxy->access_params().access_param(NAME(fogDistanceCoef)))
			{
				Vector3 camLoc = _context.get_camera().get_placement().get_translation();
				Vector3 relCamLoc = _environmentProxy->get_matrix_from_room_proxy().location_to_local(camLoc);
				float fogDistance = autoFogDistanceBackgroundAnchorXY;
				if (relCamLoc.z > 0.0f)
				{
					fogDistance = sqrt(sqr(fogDistance) + sqr(2.0f * relCamLoc.z)); // good enough when not going too far away
				}
				fogDistanceCoef->as<float>() = 1.0f / fogDistance;
			}
		}
	}
}

void ArtDir::environment_proxy__setup_pilgrimage_rotation(Framework::Render::SceneBuildContext const& _context, Framework::Render::EnvironmentProxy* _environmentProxy, Framework::Render::EnvironmentProxy* _previousEnvironmentProxy, Framework::Room const* _forRoom)
{
	if (auto* pi = PilgrimageInstance::get())
	{
		_environmentProxy->access_matrix_from_room_proxy() = _environmentProxy->access_matrix_from_room_proxy().to_world(rotation_xy_matrix(pi->get_environment_rotation_yaw()));
	}
}

void ArtDir::door_in_room_proxy__setup(Framework::Render::SceneBuildContext const& _context, Framework::Render::DoorInRoomProxy* _doorInRoomProxy, Framework::DoorInRoom const* _doorInRoom)
{
	door_in_room_proxy__setup_open_world_directions(_context, _doorInRoomProxy, _doorInRoom);
	door_in_room_proxy__setup_lock_indicators(_context, _doorInRoomProxy, _doorInRoom);
}

void ArtDir::door_in_room_proxy__setup_open_world_directions(Framework::Render::SceneBuildContext const& _context, Framework::Render::DoorInRoomProxy* _doorInRoomProxy, Framework::DoorInRoom const* _doorInRoom)
{
	door_in_room_mesh__setup_open_world_directions(&_doorInRoomProxy->access_appearance(), nullptr, _doorInRoom, _doorInRoomProxy->did_came_through());
}

#define SET_SHADER_PARAM(_name, _value) \
	if (_instanceSetInlined) _instanceSetInlined->set_shader_param(_name, _value); \
	if (_instance) _instance->set_shader_param(_name, _value);

void ArtDir::door_in_room_mesh__setup_open_world_directions(Meshes::Mesh3DInstanceSetInlined * _instanceSetInlined, Meshes::Mesh3DInstance* _instance, Framework::DoorInRoom const* _doorInRoom, Optional<bool> const& _didCameThrough)
{
	// look for open world directions and set the right texture depending also on core mod time
	ARRAY_STACK(Framework::TexturePartUse, dtps, 32);

	bool worldSeparator = false;
	if (!_didCameThrough.get(false) &&
		!GameSettings::get().difficulty.autoMapOnly &&
		(!GameDirector::get_active() ||
		 !GameDirector::get_active()->are_door_directions_blocked()))
	{
		worldSeparator = PilgrimageInstanceOpenWorld::fill_direction_texture_parts_for(_doorInRoom, dtps);
	}

	Framework::TexturePartUse dtp;

	Colour colour = Colour::white;
	float strength = 0.5f;

	float size = hardcoded (7.0f / 8.0f) * 0.5f;
	Vector2 size2(size, size);
	if (!dtps.is_empty())
	{
		float pulse = 0.4f + (0.2f * _doorInRoom->get_individual());
		float timer = ::System::Core::get_timer_mod(pulse);
		if (timer < pulse * 0.8f || worldSeparator)
		{
			float inTimer = ::System::Core::get_timer_mod(pulse * (float)dtps.get_size());
			int idx = clamp(TypeConversions::Normal::f_i_cells(inTimer / pulse), 0, dtps.get_size() - 1);
			dtp = dtps[idx];
		}
		if (worldSeparator)
		{
			colour = Colour::lerp(0.5f, Colour::white, Colour::yellow);
			strength *= 0.85f;
		}
	}

	colour.a *= strength;

	Vector2 startCorner = Vector2::zero;
	Vector2 upSide = Vector2::zero;
	Vector2 rightSide = Vector2::zero;

	if (dtp.texturePart && dtp.texturePart->get_texture_id() != ::System::texture_id_none())
	{
		SET_SHADER_PARAM(NAME(inDirections), dtp.texturePart->get_texture_id());
		dtp.get_uv_info(OUT_ startCorner, OUT_ upSide, OUT_ rightSide);
	}
	else
	{
		SET_SHADER_PARAM(NAME(inDirections), ::System::texture_id_none());
		colour = Colour::none;
	}
	SET_SHADER_PARAM(NAME(inDirectionsStart), startCorner);
	SET_SHADER_PARAM(NAME(inDirectionsUp), upSide);
	SET_SHADER_PARAM(NAME(inDirectionsRight), rightSide);
	SET_SHADER_PARAM(NAME(inDirectionsColour), colour.to_vector4());
}

void ArtDir::door_in_room_proxy__setup_lock_indicators(Framework::Render::SceneBuildContext const& _context, Framework::Render::DoorInRoomProxy* _doorInRoomProxy, Framework::DoorInRoom const* _doorInRoom)
{
	if (auto* d = _doorInRoom->get_door())
	{
		if (auto* dt = d->get_door_type())
		{
			if (dt->get_tags().get_tag(NAME(lockIndicator)))
			{
				Optional<Colour> liColour;

				if (auto* gd = GameDirector::get())
				{
					if (!gd->are_lock_indicators_disabled())
					{
						if (!artDir_lockIndicatorSetup.setup)
						{
							artDir_lockIndicatorSetup.setup = true;
							artDir_lockIndicatorSetup.locked.parse_from_string(String(TXT("lockIndicator_locked")));
							artDir_lockIndicatorSetup.open.parse_from_string(String(TXT("lockIndicator_open")));
							artDir_lockIndicatorSetup.opening.parse_from_string(String(TXT("lockIndicator_opening")));
							artDir_lockIndicatorSetup.overridingLock.parse_from_string(String(TXT("lockIndicator_overriding")));
						}

						if (!_doorInRoom->get_door_on_other_side())
						{
							liColour = artDir_lockIndicatorSetup.locked;
						}
						else if (is_flag_set(d->get_game_related_system_flags(), DoorFlags::AppearLockedIfNotOpen) && d->get_abs_open_factor() < 0.001f)
						{
							liColour = artDir_lockIndicatorSetup.locked;
						}
						else if (d->get_operation() == Framework::DoorOperation::StayOpen ||
							d->get_operation() == Framework::DoorOperation::Auto)
						{
							liColour = artDir_lockIndicatorSetup.open;
						}
						else if (d->get_operation() == Framework::DoorOperation::StayClosedTemporarily)
						{
							liColour = (::System::Core::get_timer_mod(1.0f) < 0.25f) ? artDir_lockIndicatorSetup.locked : artDir_lockIndicatorSetup.open;
						}
						else if (d->get_operation() == Framework::DoorOperation::StayClosed)
						{
							liColour = artDir_lockIndicatorSetup.locked;
							auto ds = gd->get_door_state(d);
							if (ds.opening)
							{
								if (ds.timeToOpenPeriod > 0.0f)
								{
									float t = mod(ds.timeToOpen, ds.timeToOpenPeriod) / ds.timeToOpenPeriod;
									liColour = t > 0.75f? artDir_lockIndicatorSetup.open : artDir_lockIndicatorSetup.locked;
								}
								else
								{
									liColour = Colour::lerp(0.5f, artDir_lockIndicatorSetup.open, artDir_lockIndicatorSetup.locked);
								}
							}
							if (ds.overridingLock && ds.overridingLockHighlight)
							{
								liColour = artDir_lockIndicatorSetup.overridingLock;
							}
						}
						else if (d->get_operation() == Framework::DoorOperation::AutoEagerToClose)
						{
							liColour = (::System::Core::get_timer_mod(0.4f) < 0.05f)? artDir_lockIndicatorSetup.locked : artDir_lockIndicatorSetup.open;
						}
					}
				}

				Colour useColour = liColour.get(Colour::none);
				Vector4 useColourV4 = useColour.with_alpha(1.0f).to_vector4();
				_doorInRoomProxy->access_appearance().set_shader_param(NAME(emissiveColour), useColourV4);
				_doorInRoomProxy->access_appearance().set_shader_param(NAME(emissiveBaseColour), useColourV4);
				_doorInRoomProxy->access_appearance().set_shader_param(NAME(emissivePower), useColour.a * (1.5f + 0.3f * sin_deg(360.0f * ::System::Core::get_timer_mod(3.0f) / 3.0f)));
			}
		}
	}
}
