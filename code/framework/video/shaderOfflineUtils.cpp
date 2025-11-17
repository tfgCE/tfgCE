#include "shaderOfflineUtils.h"

#include "..\..\core\debug\logInfoContext.h"
#include "..\..\core\math\math.h"

//

using namespace Framework;

//

void ShaderOfflineUtils::output_ssao_kernel(Optional<int> const& _samples, Optional<int> const& _seed)
{
	Array<Vector3> kernel;

	// create kernel
	{
		int samples = _samples.get(32);
		Random::Generator rg;
		if (_seed.is_set())
		{
			rg.set_seed(_seed.get(), 0);
		}

		for_count(int, i, samples)
		{
			Vector3 v;
			// random vector in a hemi-sphere at a random distance
			{
				v.x = rg.get_float(-1.0f, 1.0f);
				v.y = rg.get_float(-1.0f, 1.0f);
				v.z = rg.get_float( 0.0f, 1.0f);
				v.normalise();
				v *= rg.get_float(0.0f, 1.0f);
			}

			// start closer
			{
				float scale = clamp(1.1f * (float)i / (float)(samples - 1), 0.0f, 1.0f);
				scale = lerp(scale * scale, 0.1f, 1.0f);
				v *= scale;
			}

			kernel.push_back(v);
		}
	}

	// output code
	{
		LogInfoContext log;
		
		log.log(TXT("// auto-generated ssao kernel"));
		log.log(TXT("int kernelSizeInt = %i;"), kernel.get_size());
		log.log(TXT("float kernelSizeFloat = %i.0;"), kernel.get_size());
		log.log(TXT("vec3 kernel[%i] = vec3[]("), kernel.get_size());
		for_every(k, kernel)
		{
			log.log(TXT("\tvec3(%12.8f, %12.8f, %12.8f)%S"), k->x, k->y, k->z, for_everys_index(k) < kernel.get_size() - 1? TXT(",") : TXT(");"));
		}

		log.output_to_output();
	}
}
