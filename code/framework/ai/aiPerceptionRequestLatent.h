#pragma once

#include "..\..\core\latent\latent.h"
#include "aiPerceptionRequest.h"

namespace Framework
{
	namespace AI
	{
		class Perception;
		struct PerceptionRequestLatent;

		struct PerceptionRequestLatent
		: public PerceptionRequest
		{
			FAST_CAST_DECLARE(PerceptionRequestLatent);
			FAST_CAST_BASE(PerceptionRequest);
			FAST_CAST_END();

			typedef PerceptionRequest base;
		public:
			PerceptionRequestLatent(LATENT_FUNCTION_VARIABLE(_executeFunction));
			virtual ~PerceptionRequestLatent();

			::Latent::Frame & access_latent_frame() { return latentFrame; }

		protected: friend class Perception;
			override_ bool process(); // return true if processed

		private:
			::Latent::Frame latentFrame;
			LATENT_FUNCTION_VARIABLE(executeFunction);
			bool latentEnded = false;

			void stop_latent();
		};
	};
};
