#pragma once

#include "..\..\core\containers\array.h"
#include "..\..\core\math\math.h"
#include "..\..\core\types\optional.h"
#include "..\..\core\types\string.h"

namespace Framework
{
	namespace MeshGeneration
	{
		struct GenerationInfo
		{
		public:
			GenerationInfo();
			~GenerationInfo();

			void clear();

			void add_info(Vector3 const& _from, Vector3 const& _to, String const& _info, Optional<Colour> const & _colour = NP );
			int get_number_of_elements() const { return elements.get_size(); }

			void debug_draw();

		private:
			struct Element
			{
				Vector3 from;
				Vector3 to;
				String info;
				Optional<Colour> colour;
			};
			Array<Element> elements;
		};
	};
};
