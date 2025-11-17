#include "meshGenGenerationInfo.h"

#include "..\..\core\debug\debugRenderer.h"

using namespace Framework;
using namespace MeshGeneration;

//

GenerationInfo::GenerationInfo()
{
}

GenerationInfo::~GenerationInfo()
{
}

void GenerationInfo::clear()
{
	elements.clear();
}

void GenerationInfo::add_info(Vector3 const& _from, Vector3 const& _to, String const& _info, Optional<Colour> const& _colour)
{
	Element e;
	e.from = _from;
	e.to = _to;
	e.info = _info;
	e.colour = _colour;
	elements.push_back(e);
}

void GenerationInfo::debug_draw()
{
#ifdef AN_DEBUG_RENDERER
	for_every(e, elements)
	{
		Colour colour = e->colour.get(Colour::white);
		debug_draw_arrow_double_side(true, colour, e->from, e->to);
		debug_draw_text(true, colour, (e->from + e->to) * 0.5f, Vector2(0.0f, 0.5f), true, 2.0f, true, e->info.to_char());
	}
#endif
}

