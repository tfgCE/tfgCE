#include "font.h"

#include "..\game\game.h"
#include "..\jobSystem\jobSystem.h"
#include "..\library\library.h"
#include "..\library\usedLibraryStored.inl"
#include "..\text\localisedCharacters.h"

#include "..\..\core\concurrency\asynchronousJob.h"

//

// global references
DEFINE_STATIC_NAME_STR(grFontMesh, TXT("font mesh"));

// shader param (default for font mesh material)
DEFINE_STATIC_NAME(inTexture);

//

using namespace Framework;

//

REGISTER_FOR_FAST_CAST(Font);
LIBRARY_STORED_DEFINE_TYPE(Font, font);

Font::Font(Library * _library, LibraryName const & _name)
: base(_library, _name)
, font(new ::System::Font())
{
}

Font::~Font()
{
	defer_font_delete();
}

struct Font_DeleteFont
: public Concurrency::AsynchronousJobData
{
	::System::Font* font;
	Font_DeleteFont(::System::Font* _font)
	: font(_font)
	{}

	static void perform(Concurrency::JobExecutionContext const * _executionContext, void* _data)
	{
		Font_DeleteFont* data = (Font_DeleteFont*)_data;
		delete data->font;
	}
};

void Font::defer_font_delete()
{
	if (font)
	{
		Game::async_system_job(get_library()->get_game(), Font_DeleteFont::perform, new Font_DeleteFont(font));
	}
}

bool Font::load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc)
{
	bool result = base::load_from_xml(_node, _lc);

	textureDefinitions.clear();

	result &= font->load_from_xml(_node);

	for_every(node, _node->children_named(TXT("texture")))
	{
		TextureDefinition td;
		result &= td.texture.load_from_xml(node, TXT("use"), _lc);
		result &= td.textureArray.load_from_xml(node, TXT("use"), _lc);
		bool compressCharacters = node->get_bool_attribute_or_from_child_presence(TXT("compressCharacters"));
		for_every(n, node->all_children())
		{
			if (n->get_name() == TXT("rowAnsi") ||
				n->get_name() == TXT("ansi"))
			{
				if (!td.characters.is_empty() && !compressCharacters)
				{
					td.characters += TXT("\n");
				}
				String ansiCharacters;
				ansiCharacters = String::empty();
				for (tchar ch = compressCharacters? 33 : 32; ch < 128; ++ch)
				{
					ansiCharacters += ch;
				}
				td.characters += ansiCharacters;
			}
			if (n->get_name() == TXT("row"))
			{
				if (!td.characters.is_empty() && !compressCharacters)
				{
					td.characters += TXT("\n");
				}
				td.characters += n->get_text();
			}
			if (n->get_name() == TXT("rowLocalisedCharacters"))
			{
				if (!td.characters.is_empty() && !compressCharacters)
				{
					td.characters += TXT("\n");
				}
				Name id = n->get_name_attribute(TXT("setId"));
				if (id.is_valid())
				{
					td.characters += LocalisedCharacters::get_set(id);
					td.usesLocalisedCharacters = true;
				}
				td.characters += n->get_text();
			}
		}
		textureDefinitions.push_back(td);
	}
	
	fixedWidthCharacters = String::empty();
	for_every(node, _node->children_named(TXT("fixedWidthCharacters")))
	{
		fixedWidthCharacters += node->get_text();
	}
	colourCharacters = String::empty();
	for_every(node, _node->children_named(TXT("colourCharacters")))
	{
		colourCharacters += node->get_text();
	}

	for_every(td, textureDefinitions)
	{
		if (td->characters.is_empty() &&
			! td->usesLocalisedCharacters) /* it's ok for usesLocalisedCharacters to be empty, maybe we not loaded it yet */
		{
			error_loading_xml(_node, TXT("there is texture definition without <row/>s or <ansi/>"));
			result = false;
		}
	}

	return result;
}

struct UseTexturesAndPrepare
: public Concurrency::AsynchronousJobData
{
	Font* font;
	Library* library;
	UseTexturesAndPrepare(Font * _font, Library* _library)
	: font(_font)
	, library(_library)
	{}
};

bool Font::prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext)
{
	bool result = base::prepare_for_game(_library, _pfgContext);;

	for_every(td, textureDefinitions)
	{
		td->texture.prepare_for_game_find_may_fail(_library, _pfgContext, LibraryPrepareLevel::Resolve);
		td->textureArray.prepare_for_game_find_may_fail(_library, _pfgContext, LibraryPrepareLevel::Resolve);
	}

	IF_PREPARE_LEVEL(_pfgContext, LibraryPrepareLevel::LoadFont)
	{
		Array<System::Font::TextureDefinition> fontTextureDefinitions;
		for_every(td, textureDefinitions)
		{
			System::Font::TextureDefinition ftd;
			if (!td->texture.get() &&
				!td->textureArray.get())
			{
				error(TXT("could not find texture or texture array named \"%S\""), td->texture.get_name().to_string().to_char());
			}
			ftd.characters = td->characters;
			ftd.texture = td->texture.get() ? td->texture.get()->get_texture() : nullptr;
			fontTextureDefinitions.push_back(ftd);
		}
		font->use_textures_and_prepare(fontTextureDefinitions, fixedWidthCharacters, colourCharacters,
			[this](int _textureDefinitionIdx, int _subTextureIdx)
			{
				auto& td = textureDefinitions[_textureDefinitionIdx];
				if (_subTextureIdx == 0)
				{
					if (auto* t = td.texture.get())
					{
						return t->get_texture();
					}
				}
				if (auto* ta = td.textureArray.get())
				{
					if (auto* t = ta->get_texture(_subTextureIdx))
					{
						return t;
					}
				}
				return (::System::Texture*)nullptr;
			});
	}

	return result;
}

void Font::draw_text_at(::System::Video3D* _v3d, String const & _text, Colour const & _colour, Vector2 const & _leftBottom, Vector2 const & _scale, Vector2 const & _anchorRelPlacement, Optional<bool> const& _alignToPixels, ::System::FontDrawingContext const & _fontDrawingContext) const
{
	font->draw_text_at(_v3d, _text, _colour, _leftBottom, _scale, _anchorRelPlacement, _alignToPixels, _fontDrawingContext);
}

void Font::draw_text_at(::System::Video3D* _v3d, tchar const * _text, Colour const & _colour, Vector2 const & _leftBottom, Vector2 const & _scale, Vector2 const & _anchorRelPlacement, Optional<bool> const& _alignToPixels, ::System::FontDrawingContext const& _fontDrawingContext) const
{
	font->draw_text_at(_v3d, _text, _colour, _leftBottom, _scale, _anchorRelPlacement, _alignToPixels, _fontDrawingContext);
}

void Font::draw_text_at_with_border(::System::Video3D* _v3d, String const & _text, Colour const & _colour, Colour const & _borderColour, float _border, Vector2 const & _leftBottom, Vector2 const & _scale, Vector2 const & _anchorRelPlacement, Optional<bool> const& _alignToPixels,::System::FontDrawingContext const& _fontDrawingContext) const
{
	font->draw_text_at_with_border(_v3d, _text, _colour, _borderColour, _border, _leftBottom, _scale, _anchorRelPlacement, _alignToPixels, _fontDrawingContext);
}

float Font::get_height() const
{
	return font->get_height();
}

Vector2 Font::calculate_char_size() const
{
	return font->calculate_char_size();
}

void Font::construct_mesh(Mesh* _mesh, String const& _text, Optional<bool> const& _for3d, float const& _letterHeight, Vector2 const& _anchorRelPlacement) const
{
	auto* material = Library::get_current()->get_global_references().get<Material>(NAME(grFontMesh));
	if (material)
	{
		construct_mesh(_mesh, material, NAME(inTexture), _text, _for3d, _letterHeight, _anchorRelPlacement);
	}
	else
	{
		error(TXT("no material available to construct font, provide \"font mesh\" material global reference"));
	}
}

void Font::construct_mesh(Mesh* _mesh, Material* _usingMaterial, Name const& _textureIdParam, String const& _text, Optional<bool> const& _for3d, float const& _letterHeight, Vector2 const& _anchorRelPlacement) const
{
	Array<::System::TextureID> usingTextures;
	_mesh->make_sure_mesh_exists();
	int parts = _mesh->get_mesh()->get_parts_num();
	font->construct_mesh(_mesh->get_mesh(), usingTextures, _text, _for3d, _letterHeight, _anchorRelPlacement);

	int matIdx = parts;
	for_every(ut, usingTextures)
	{
		MaterialSetup ms;
		ms.material = _usingMaterial;
		{
			ShaderParam sp;
			sp.name = _textureIdParam;
			sp.param = ::System::ShaderParam(*ut);
			ms.params.params.push_back(sp);
		}
		_mesh->set_material_setup(matIdx, ms);

		++matIdx;
	}
}

void Font::debug_draw() const
{
	font->debug_draw();
}

bool Font::preview_render() const
{
	return font->preview_render();
}

