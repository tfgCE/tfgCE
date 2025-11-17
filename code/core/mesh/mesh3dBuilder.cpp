#include "mesh3dBuilder.h"

#include "mesh3d.h"

#include "..\other\parserUtils.h"
#include "..\system\video\vertexFormatUtils.h"
#include "..\types\vectorPacked.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

#define USE_OFFSET(_pointer, _offset) (void*)(((int8*)_pointer) + _offset)

#define RESERVE_BUFFER_BYTES 131072

//

using namespace ::System;
using namespace Meshes;

//

REGISTER_FOR_FAST_CAST(Mesh3DBuilder);

Mesh3DBuilder::Mesh3DBuilder()
: usage(Usage::Static)
, primitiveType(Primitive::Triangle)
, vertexData(RESERVE_BUFFER_BYTES)
, numberOfVertices(0)
{
	clear();
	vertexData.make_space_for( RESERVE_BUFFER_BYTES );
}

Mesh3DBuilder::~Mesh3DBuilder()
{
	clear();
}

void Mesh3DBuilder::reset_input()
{
	inputLocation = Vector3::zero;
	inputNormal = Vector3::zero;
	inputColour = Colour::white;
	inputUV = Vector2::zero;
}

void Mesh3DBuilder::make_space_for(uint32 _for)
{
	an_assert(vertexFormat.is_ok_to_be_used(), TXT("stride and offsets not calculated"));

	vertexData.make_space_for(_for * vertexFormat.get_stride());
}

void Mesh3DBuilder::set_data_size(uint32 _for)
{
	an_assert(vertexFormat.is_ok_to_be_used(), TXT("stride and offsets not calculated"));

	memory_leak_suspect;
	vertexData.set_size(_for * vertexFormat.get_stride());
	forget_memory_leak_suspect;
}

void Mesh3DBuilder::setup(Primitive::Type _primitiveType, VertexFormat const & _vertexFormat, uint32 _reserveSpaceForPrimitivesCount)
{
	clear();
	reset_input();

	numberOfVertices = 0;
	primitiveType = _primitiveType;
	vertexFormat = _vertexFormat;
	vertexFormat.used_for_dynamic_data();
	vertexFormat.calculate_stride_and_offsets();

	make_space_for(Primitive::primitive_count_to_vertex_count(primitiveType, _reserveSpaceForPrimitivesCount));
}

#define TOKEN_VERTEX_LOCATION TXT("v")
#define TOKEN_VERTEX_NORMAL TXT("vn")
#define TOKEN_VERTEX_TEXTURE_COORDS TXT("vt")
#define TOKEN_FACE TXT("f")
#define TOKEN_FACE_SEPARATOR TXT("/")
#define TOKEN_MATERIAL TXT("usemtl")

int read_int_token(IO::File* _file, REF_ Token& token, bool _justRead = false)
{
	int value = 0;
	if (!_justRead)
	{
		_file->read_token(token);
	}
	if (token == TXT("-"))
	{
		_file->read_token(token);
		value = -ParserUtils::parse_int(token.to_char());
	}
	else
	{
		value = ParserUtils::parse_int(token.to_char()) - 1;
	}
	return value;
}

bool Mesh3DBuilder::load_wavefront_obj(IO::File * _file, Meshes::Mesh3DImportOptions const & _options)
{
	if (! _file || ! _file->is_open())
	{
		return false;
	}
	Array<Vector3> locations;
	Array<Vector3> normals;
	Array<Vector2> textureCoords;

	// setup reserving - to get less reallocations

	bool locationsMoved = false;
	locations.make_space_for(RESERVE_BUFFER_BYTES);
	locations.set_reserve_buffer_bytes(RESERVE_BUFFER_BYTES);
	normals.make_space_for(RESERVE_BUFFER_BYTES);
	normals.set_reserve_buffer_bytes(RESERVE_BUFFER_BYTES);
	textureCoords.make_space_for(RESERVE_BUFFER_BYTES);
	textureCoords.set_reserve_buffer_bytes(RESERVE_BUFFER_BYTES);
	vertexData.set_reserve_buffer_bytes(RESERVE_BUFFER_BYTES);
	vertexData.make_space_for(RESERVE_BUFFER_BYTES);

	int numberOfVerticesInMaterial = 0;

	{	// add first part
		Part* part = new Part();
		part->address = 0;
		part->numberOfVertices = 0;
		parts.push_back(part);
	}

	// read
	_file->go_to(0);
	while (true)
	{
		Token token;
		if (! _file->read_token(token))
		{
			break;
		}
		if (token == TOKEN_VERTEX_LOCATION)
		{
			Vector3 vec;
			_file->read_float(vec.x);
			_file->read_float(vec.z);
			_file->read_float(vec.y);
			locations.push_back(vec);
		}
		else if (token == TOKEN_VERTEX_NORMAL)
		{
			Vector3 vec;
			_file->read_float(vec.x);
			_file->read_float(vec.z);
			_file->read_float(vec.y);
			if (_options.importFromPlacement.is_set())
			{
				vec = _options.importFromPlacement.get().vector_to_local(vec);
			}
			if (_options.placeAfterImport.is_set())
			{
				vec = _options.placeAfterImport.get().vector_to_world(vec);
			}
			vec.normalise();
			normals.push_back(vec);
		}
		else if (token == TOKEN_VERTEX_TEXTURE_COORDS)
		{
			Vector2 vec;
			_file->read_float(vec.x);
			_file->read_float(vec.y);
			if (_options.setU.is_set())
			{
				vec.x = _options.setU.get();
			}
			textureCoords.push_back(vec);
		}
		else if (token == TOKEN_MATERIAL)
		{
			if (! parts.is_empty())
			{
				Part* part = parts.get_last();
				part->numberOfVertices = numberOfVerticesInMaterial;
			}
			if (numberOfVerticesInMaterial != 0)
			{
				parts.push_back(new Part());
			}
			Part* part = parts.get_last();
			part->address = vertexData.get_size();
			part->numberOfVertices = 0;
			numberOfVerticesInMaterial = 0;
		}
		else if (token == TOKEN_FACE)
		{
			if (!locationsMoved)
			{
				Vector3 importOffset = Vector3::zero;
				Vector3 importScaleFit = Vector3::one;
				if (_options.importFromAutoCentre.is_set() ||
					_options.importFromAutoPt.is_set() ||
					_options.importScaleToFit.is_set())
				{
					Range3 range = Range3::empty;
					for_every(l, locations)
					{
						range.include(*l);
					}
					if (!range.is_empty())
					{
						if (_options.importFromAutoCentre.is_set())
						{
							importOffset = -range.centre() * _options.importFromAutoCentre.get();
						}
						if (_options.importFromAutoPt.is_set())
						{
							importOffset = -range.get_at(_options.importFromAutoPt.get());
						}
						if (_options.importScaleToFit.is_set())
						{
							if (_options.importScaleToFit.get().x != 0.0f) importScaleFit.x = _options.importScaleToFit.get().x / max(0.001f, range.x.length());
							if (_options.importScaleToFit.get().y != 0.0f) importScaleFit.y = _options.importScaleToFit.get().y / max(0.001f, range.y.length());
							if (_options.importScaleToFit.get().z != 0.0f) importScaleFit.z = _options.importScaleToFit.get().z / max(0.001f, range.z.length());
						}
					}
				}
				for_every(l, locations)
				{
					*l += importOffset;
					*l *= importScaleFit;
					if (_options.importFromPlacement.is_set())
					{
						*l = _options.importFromPlacement.get().location_to_local(*l);
					}
					*l *= _options.importScale;
					if (_options.placeAfterImport.is_set())
					{
						*l = _options.placeAfterImport.get().location_to_world(*l);
					}
				}
				locationsMoved = true;
			}
			add_triangle();
			int tokenValue0;
			int tokenValue1;
			int tokenValue2;
			bool hasSeparator0 = false;
			bool hasSeparator1 = false;
			bool justReadLocTokenOneTime = false;
			int locIndices[4];
			int uvIndices[4] = { NONE, NONE, NONE, NONE };
			int norIndices[4] = { NONE, NONE, NONE, NONE };
			for (int i = 0; i < 4; ++i)
			{
				add_point();

				tokenValue0 = read_int_token(_file, REF_ token, justReadLocTokenOneTime);
				justReadLocTokenOneTime = false;

				// check if we're loading separator after token or just skip it
				if (i == 0)
				{
					_file->read_token(token);
					hasSeparator0 = token == TOKEN_FACE_SEPARATOR;
					justReadLocTokenOneTime = !hasSeparator0;
				}
				else
				{
					if (hasSeparator0)
					{
						_file->read_token(token);
					}
				}

				if (hasSeparator0)
				{
					tokenValue1 = read_int_token(_file, REF_ token);

					// check if we're loading separator after token or just skip it
					if (i == 0)
					{
						_file->read_token(token);
						hasSeparator1 = token == TOKEN_FACE_SEPARATOR;
						justReadLocTokenOneTime = !hasSeparator1;
					}
					else
					{
						if (hasSeparator1)
						{
							_file->read_token(token);
						}
					}

					if (hasSeparator1)
					{
						tokenValue2 = read_int_token(_file, REF_ token);
					}
				}

				// location (always)
				{
					int locIdx = tokenValue0;
					if (locIdx < 0)
					{
						// starts from the end
						locIdx = locations.get_size() + locIdx;
					}
					if (! locations.is_index_valid(locIdx))
					{
						error(TXT("invalid location %i"), locIdx);
						return false;
					}
					locIndices[i] = locIdx;
					location(locations[locIdx]);
				}

				// check what we've got
				Optional<int> texCoordIdx;
				Optional<int> norIdx;
				if (hasSeparator0 && hasSeparator1)
				{
					// both
					texCoordIdx = tokenValue1;
					norIdx = tokenValue2;
				}
				if (hasSeparator0 && !hasSeparator1)
				{
					// just one extra
					// if we don't have normals loaded but we have texture coords, load texture coords
					if (normals.is_empty() && !textureCoords.is_empty())
					{
						texCoordIdx = tokenValue1;
					}
					else
					{
						norIdx = tokenValue1;
					}
				}

				// texture
				if (texCoordIdx.is_set())
				{
					int uvIdx = texCoordIdx.get();
					if (uvIdx < 0)
					{
						// starts from the end
						uvIdx = textureCoords.get_size() + uvIdx;
					}
					if (!textureCoords.is_index_valid(uvIdx))
					{
						error(TXT("invalid texture coords %i"), uvIdx);
						return false;
					}
					uvIndices[i] = uvIdx;
					uv(textureCoords[uvIdx]);
				}

				// normal
				if (norIdx.is_set())
				{
					if (norIdx.get() < 0)
					{
						// starts from the end
						norIdx = normals.get_size() + norIdx.get();
					}
					if (!normals.is_index_valid(norIdx.get()))
					{
						error(TXT("invalid normal %i"), norIdx.get());
						return false;
					}
					norIndices[i] = norIdx.get();
					normal(normals[norIdx.get()]);
				}

				done_point();
				++ numberOfVerticesInMaterial;

				if (i > 2)
				{
					// fan support
					locIndices[2] = locIndices[3];
					uvIndices[2] = uvIndices[3];
					norIndices[2] = norIndices[3];
					i = 2;
				}
				if (i == 2)
				{
					// check if it might be a quad (two triangles)
					if (_file->has_last_read_char() &&
						(_file->get_last_read_char() == ' ' ||
						 _file->get_last_read_char() == '\t'))
					{
						done_triangle();
						add_triangle();
						add_point();
						location(locations[locIndices[0]]);
						if (uvIndices[0] != NONE) { uv(textureCoords[uvIndices[0]]); }
						if (norIndices[0] != NONE) { normal(normals[norIndices[0]]); }
						done_point();
						++numberOfVerticesInMaterial;
						add_point();
						location(locations[locIndices[2]]);
						if (uvIndices[2] != NONE) { uv(textureCoords[uvIndices[2]]); }
						if (norIndices[2] != NONE) { normal(normals[norIndices[2]]); }
						done_point();
						++numberOfVerticesInMaterial;
					}
					else
					{
						break;
					}
				}
			}
			done_triangle();
		}
		if (! _file->read_next_line())
		{
			break;
		}
	}
	if (Part* part = parts.get_last())
	{
		part->numberOfVertices = numberOfVerticesInMaterial;
	}
	return true;
}

#undef TOKEN_VERTEX_LOCATION
#undef TOKEN_VERTEX_NORMAL
#undef TOKEN_VERTEX_TEXTURE_COORDS
#undef TOKEN_FACE
#undef TOKEN_FACE_SEPARATOR

bool Mesh3DBuilder::load_from_file(String const & _fileName, Meshes::Mesh3DImportOptions const & _options)
{
	if (_fileName.get_right(4) == TXT(".obj"))
	{
		auto vf = vertex_format().with_normal();
		if (!_options.skipUV)
		{
			vf.with_texture_uv();
			if (_options.floatUV)
			{
				vf.with_texture_uv_type(System::DataFormatValueType::Float);
			}
		}
		if (_options.setU.is_set())
		{
			vf.with_texture_uv();
		}
		if (! _options.skipColour)
		{
			vf.with_colour_rgb();
		}
		setup(Primitive::Triangle, vf);
		colour(Colour(1.0f, 1.0f, 1.0f));
		uv(Vector2::zero);
		IO::File file;
		if (! file.open(_fileName))
		{
			return false;
		}
		file.set_type(IO::InterfaceType::Text);
		bool result = load_wavefront_obj(&file, _options);
		build();
		return result;
	}
	// TODO error - not recognized format
	return false;
}

void Mesh3DBuilder::add_point()
{
	if (openPoint)
	{
		done_point();
	}
	++ numberOfVertices;
	open_point();
}

void Mesh3DBuilder::open_point()
{
	if (! openPoint)
	{
		openPoint = true;
		set_data_size(numberOfVertices);

		load_location(inputLocation);
		load_normal(inputNormal);
		load_colour(inputColour);
		load_uv(inputUV);
		if (skinToSingle.is_set())
		{
			load_skinning_single(skinToSingle.get());
		}
		else
		{
			load_no_skinning();
		}
	}
}

void* Mesh3DBuilder::get_current_vertex_data()
{
	return &vertexData[(numberOfVertices - 1) * vertexFormat.get_stride()];
}

void Mesh3DBuilder::load_location(Vector2 const & _location)
{
	if (vertexFormat.get_location() == VertexFormatLocation::XY)
	{
		memory_copy(USE_OFFSET(get_current_vertex_data(), vertexFormat.get_location_offset()), &_location.x, 2 * sizeof(float));
	}
	if (vertexFormat.get_location() == VertexFormatLocation::XYZ)
	{
		error(TXT("not supported!"));
	}
}

void Mesh3DBuilder::load_location(Vector3 const & _location)
{
	if (vertexFormat.get_location() == VertexFormatLocation::XY)
	{
		memory_copy(USE_OFFSET(get_current_vertex_data(), vertexFormat.get_location_offset()), &_location.x, 2 * sizeof(float));
	}
	if (vertexFormat.get_location() == VertexFormatLocation::XYZ)
	{
		memory_copy(USE_OFFSET(get_current_vertex_data(), vertexFormat.get_location_offset()), &_location.x, 3 * sizeof(float));
	}
}

void Mesh3DBuilder::load_normal(Vector3 const & _normal)
{
	if (vertexFormat.get_normal() == VertexFormatNormal::Yes)
	{
		if (vertexFormat.is_normal_packed())
		{
			((VectorPacked*)(USE_OFFSET(get_current_vertex_data(), vertexFormat.get_normal_offset())))->set_as_vertex_normal(_normal);
		}
		else
		{
			memory_copy(USE_OFFSET(get_current_vertex_data(), vertexFormat.get_normal_offset()), &_normal.x, 3 * sizeof(float));
		}
	}
}

void Mesh3DBuilder::load_colour(Colour const & _colour)
{
	if (vertexFormat.get_colour() == VertexFormatColour::RGB)
	{
		memory_copy(USE_OFFSET(get_current_vertex_data(), vertexFormat.get_colour_offset()), &_colour.r, 3 * sizeof(float));
	}
	if (vertexFormat.get_colour() == VertexFormatColour::RGBA)
	{
		memory_copy(USE_OFFSET(get_current_vertex_data(), vertexFormat.get_colour_offset()), &_colour.r, 4 * sizeof(float));
	}
}

void Mesh3DBuilder::load_uv(Vector2 const & _uv)
{
	if (vertexFormat.get_texture_uv() == VertexFormatTextureUV::U)
	{
		::System::VertexFormatUtils::store((int8*)USE_OFFSET(get_current_vertex_data(), vertexFormat.get_texture_uv_offset()), vertexFormat.get_texture_uv_type(), _uv.x);
	}
	if (vertexFormat.get_texture_uv() == VertexFormatTextureUV::UV)
	{
		::System::VertexFormatUtils::store((int8*)USE_OFFSET(get_current_vertex_data(), vertexFormat.get_texture_uv_offset()), vertexFormat.get_texture_uv_type(), _uv);
	}
	if (vertexFormat.get_texture_uv() == VertexFormatTextureUV::UVW)
	{
		::System::VertexFormatUtils::store((int8*)USE_OFFSET(get_current_vertex_data(), vertexFormat.get_texture_uv_offset()), vertexFormat.get_texture_uv_type(), _uv.to_vector3());
	}
}

void Mesh3DBuilder::load_skinning_single(int _boneIdx)
{
	if (!vertexFormat.has_skinning_data())
	{
		return;
	}
	int skinningOffset = vertexFormat.get_skinning_element_count() > 1 ? vertexFormat.get_skinning_indices_offset() : vertexFormat.get_skinning_index_offset();
	int skinningWeightOffset = vertexFormat.get_skinning_element_count() > 1 ? vertexFormat.get_skinning_weights_offset() : NONE;
	int bonesRequired = vertexFormat.get_skinning_element_count();
	if (bonesRequired > 4)
	{
		error(TXT("no more than 4 bones supported!"));
		todo_important(TXT("add support for more bones"));
		bonesRequired = 4;
	}
	if (vertexFormat.get_skinning_index_type() == ::System::DataFormatValueType::Uint8)
	{
		uint8* out = (uint8*)((int8*)get_current_vertex_data() + skinningOffset);
		for (int idx = 0; idx < bonesRequired; ++idx, ++out)
		{
			*out = idx == 0? _boneIdx : NONE;
		}
	}
	else if (vertexFormat.get_skinning_index_type() == ::System::DataFormatValueType::Uint16)
	{
		uint16* out = (uint16*)((int8*)get_current_vertex_data() + skinningOffset);
		for (int idx = 0; idx < bonesRequired; ++idx, ++out)
		{
			*out = idx == 0 ? _boneIdx : NONE;
		}
	}
	else if (vertexFormat.get_skinning_index_type() == ::System::DataFormatValueType::Uint32)
	{
		uint32* out = (uint32*)((int8*)get_current_vertex_data() + skinningOffset);
		for (int idx = 0; idx < bonesRequired; ++idx, ++out)
		{
			*out = idx == 0 ? _boneIdx : NONE;
		}
	}
	else
	{
		todo_important(TXT("implement_ other skinning index type"));
	}
	if (skinningWeightOffset != NONE)
	{
		float* out = (float*)((int8*)get_current_vertex_data() + skinningWeightOffset);
		for (int idx = 0; idx < bonesRequired; ++idx, ++out)
		{
			*out = idx == 0 ? 1.0f : 0.0f;
		}
	}
}

void Mesh3DBuilder::load_no_skinning()
{
	if (!vertexFormat.has_skinning_data())
	{
		return;
	}
	int skinningOffset = vertexFormat.get_skinning_element_count() > 1 ? vertexFormat.get_skinning_indices_offset() : vertexFormat.get_skinning_index_offset();
	int bonesRequired = vertexFormat.get_skinning_element_count();
	if (bonesRequired > 4)
	{
		error(TXT("no more than 4 bones supported!"));
		todo_important(TXT("add support for more bones"));
		bonesRequired = 4;
	}
	if (vertexFormat.get_skinning_index_type() == ::System::DataFormatValueType::Uint8)
	{
		uint8* out = (uint8*)((int8*)get_current_vertex_data() + skinningOffset);
		for (int idx = 0; idx < bonesRequired; ++idx, ++out)
		{
			*out = NONE;
		}
	}
	else if (vertexFormat.get_skinning_index_type() == ::System::DataFormatValueType::Uint16)
	{
		uint16* out = (uint16*)((int8*)get_current_vertex_data() + skinningOffset);
		for (int idx = 0; idx < bonesRequired; ++idx, ++out)
		{
			*out = NONE;
		}
	}
	else if (vertexFormat.get_skinning_index_type() == ::System::DataFormatValueType::Uint32)
	{
		uint32* out = (uint32*)((int8*)get_current_vertex_data() + skinningOffset);
		for (int idx = 0; idx < bonesRequired; ++idx, ++out)
		{
			*out = NONE;
		}
	}
	else
	{
		todo_important(TXT("implement_ other skinning index type"));
	}
}

void Mesh3DBuilder::done_point()
{
	if (openPoint)
	{
		openPoint = false;
	}
}

void Mesh3DBuilder::add_triangle()
{
	an_assert(primitiveType == Primitive::Triangle, TXT("should be used for triangles"));
}

void Mesh3DBuilder::location(Vector3 const & _location)
{
	inputLocation = _location;
	if (openPoint)
	{
		load_location(_location);
	}
}

void Mesh3DBuilder::normal(Vector3 const & _normal)
{
	inputNormal = _normal.normal();
	if (openPoint)
	{
		load_normal(inputNormal);
	}
}

void Mesh3DBuilder::colour(Colour const & _colour)
{
	inputColour = _colour;
	if (openPoint)
	{
		load_colour(_colour);
	}
}

void Mesh3DBuilder::uv(Vector2 const & _uv)
{
	inputUV = _uv;
	if (openPoint)
	{
		load_uv(_uv);
	}
}

void Mesh3DBuilder::skin_to_single(int _boneIdx)
{
	skinToSingle = _boneIdx;
	if (openPoint)
	{
		load_skinning_single(skinToSingle.get());
	}
}

void Mesh3DBuilder::done_triangle()
{
	done_point();
	an_assert(primitiveType == Primitive::Triangle, TXT("should be used only for triangles"));
	if (primitiveType == Primitive::Triangle)
	{
		an_assert((numberOfVertices % 3) == 0, TXT("not enough points to close triangle"));
	}
}

void Mesh3DBuilder::done()
{
	done_point();
	if (primitiveType == Primitive::Triangle)
	{
		done_triangle();
	}
}

void Mesh3DBuilder::build()
{
	done();

	// prune data - we don't want to waste space
	vertexData.prune();

	// create at least one part
	if (parts.is_empty())
	{
		Part* part = new Part();
		part->address = 0;
		part->numberOfVertices = numberOfVertices;
		parts.push_back(part);
	}
}

void Mesh3DBuilder::clear()
{
	for_every_ptr(part, parts)
	{
		delete part;
	}
	parts.clear();

	vertexData.clear();
	numberOfVertices = 0;

	openPoint = false;
}

void Mesh3DBuilder::render(Video3D * _v3d, IMaterialInstanceProvider const * _materialProvider, Mesh3DRenderingContext const & _renderingContext) const
{
	Mesh3D::render_builder(_v3d, this, _materialProvider, _renderingContext);
}

void const * Mesh3DBuilder::get_data(int _part) const
{
	return parts.is_index_valid(_part)? &vertexData[parts[_part]->address] : nullptr;
}

uint32 Mesh3DBuilder::get_number_of_vertices(int _part) const
{
	return parts.is_index_valid(_part)? parts[_part]->numberOfVertices : 0;
}

uint32 Mesh3DBuilder::get_number_of_primitives(int _part) const
{
	return Primitive::vertex_count_to_primitive_count(primitiveType, get_number_of_vertices(_part));
}

int Mesh3DBuilder::calculate_triangles() const
{
	int triCount = 0;
	for_every_ptr(part, parts)
	{
		triCount += part->numberOfVertices / 3;
	}
	return triCount;
}

void Mesh3DBuilder::log(LogInfoContext& _log) const
{
	todo_implement;
}

#undef USE_OFFSET
