#include "loaderVoidRoom.h"

#include "..\library\library.h"
#include "..\render\renderCamera.h"
#include "..\render\renderContext.h"
#include "..\render\renderScene.h"

#include "..\..\core\debug\debugRenderer.h"
#include "..\..\core\mainConfig.h"
#include "..\..\core\math\math.h"
#include "..\..\core\splash\splashLogos.h"
#include "..\..\core\system\video\fragmentShaderOutputSetup.h"
#include "..\..\core\system\video\renderTarget.h"
#include "..\..\core\system\video\vertexFormatUtils.h"
#include "..\..\core\system\video\video3d.h"
#include "..\..\core\system\video\video3dPrimitives.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//#define STAY_HERE

//

using namespace Loader;

//

DEFINE_STATIC_NAME(colour);
DEFINE_STATIC_NAME(inData);
DEFINE_STATIC_NAME(logoAlpha);
DEFINE_STATIC_NAME(overrideColour);

//

VoidRoom::VoidRoom(VoidRoomLoaderType::Type _type, Optional<Colour> const & _backgroundColour, Optional<Colour> const & _letterColour, Optional<Colour> const & _fadeFrom, Optional<Colour> const& _fadeTo)
: type(_type)
, backgroundColour(_backgroundColour.get(Colour::black))
, letterColour(_letterColour.get(Colour::white))
, fadeFromColour(_fadeFrom.get(Colour::black))
, fadeToColour(_fadeTo.get(Colour::black))
{
	voidRoomSquareSize = letterSize * 4.0f + letterSpacing * 3.0f;

	construct_letters();

	construct_shaders();

	for_count(int, i, 8)
	{
		letters[i].visibilityBlend = 0.0f;// 0.25f;
		letters[i].delay = 0.0f;// 1.2f + 0.05f * (float)i;
	}
	logoShowDelay = 0.0f;// 1.5f;
	logoVisibilityBlend = 0.0f;// 0.5f;

	if (auto * vr = VR::IVR::get())
	{
		if (vr->is_render_view_valid())
		{
			auto viewTransform = vr->get_render_view();
			startingYaw = Rotator3::get_yaw(viewTransform.get_axis(Axis::Forward));
		}
	}
}

VoidRoom::~VoidRoom()
{
	logoShaderProgram = nullptr;
	logoVertexShader = nullptr;
	logoFragmentShader = nullptr;
	delete_and_clear(letterV);
	delete_and_clear(letterO);
	delete_and_clear(letterI);
	delete_and_clear(letterD);
	delete_and_clear(letterR);
	delete_and_clear(letterM);
	delete_and_clear(voidRoom);
	delete_and_clear(functionMesh);
	for_every(logoMesh, logoMeshes)
	{
		delete_and_clear(logoMesh->mesh);
		delete_and_clear(logoMesh->material);
	}
	for_every(logoMesh, otherLogoMeshes)
	{
		delete_and_clear(logoMesh->mesh);
		delete_and_clear(logoMesh->material);
	}
	::Splash::Logos::unload_logos(REF_ splashTextures, REF_ otherSplashTextures);
}

void VoidRoom::setup_static_display()
{
	activation = 1.0f;
	targetActivation = 1.0f;
}

void VoidRoom::add_point(REF_ Array<int8> & _vertexData, REF_ int & _pointIdx, ::System::VertexFormat const & _vertexFormat, Vector2 const & _point, Vector2 const& _uv)
{
	int8* currentVertexData = &_vertexData[_vertexFormat.get_stride() * _pointIdx];

	Vector3* location = (Vector3*)(currentVertexData + _vertexFormat.get_location_offset());
	*location = Vector3(_point.x, _point.y, 0.0f);

	if (_vertexFormat.get_texture_uv_offset() != NONE)
	{
		::System::VertexFormatUtils::store(currentVertexData + _vertexFormat.get_texture_uv_offset(), _vertexFormat.get_texture_uv_type(), _uv);
	}

	++_pointIdx;
}

void VoidRoom::add_triangle(REF_ Array<int32> & _elements, REF_ int & _elementIdx, int32 _a, int32 _b, int32 _c)
{
	_elements[_elementIdx++] = _a;
	_elements[_elementIdx++] = _b;
	_elements[_elementIdx++] = _c;
}

void VoidRoom::construct_logos()
{
	for_every(logoMesh, logoMeshes)
	{
		delete_and_clear(logoMesh->mesh);
	}
	for_every(logoMesh, otherLogoMeshes)
	{
		delete_and_clear(logoMesh->mesh);
	}
	::Splash::Logos::unload_logos(REF_ splashTextures, REF_ otherSplashTextures);

	if (functionType == ForInit)
	{
		::Splash::Logos::load_logos(REF_ splashTextures, REF_ otherSplashTextures);

		if (!splashTextures.is_empty())
		{
			System::IndexFormat indexFormat;
			indexFormat.with_element_size(System::IndexElementSize::FourBytes);
			indexFormat.calculate_stride();

			System::VertexFormat vertexFormat;
			vertexFormat.with_texture_uv().with_texture_uv_type(::System::DataFormatValueType::Float);
			vertexFormat.with_padding();
			vertexFormat.calculate_stride_and_offsets();

			{
				logoMeshes.set_size(splashTextures.get_size());

				float height = 0.0f;
				float spacing = letterSize * 0.5f;
				for_every_reverse_ptr(splashTexture, splashTextures)
				{
					LogoMesh& lm = logoMeshes[for_everys_index(splashTexture)];
					lm.mesh = new Meshes::Mesh3D();

					Array<int32> elements;
					Array<int8> vertexData;

					int pointCount = 4;
					int triangleCount = 2;

					vertexData.set_size(vertexFormat.get_stride() * pointCount);
					elements.set_size(3 * triangleCount);

					Vector2 size = splashTexture->get_size().to_vector2();
					float scale = (letterSize * 3.0f) / size.x;
					size *= scale;

					Vector2 halfSize = size * Vector2::half;

					int pointIdx = 0;
					add_point(REF_ vertexData, REF_ pointIdx, vertexFormat, Vector2(-halfSize.x, -halfSize.y), Vector2(0.0f, 0.0f));
					add_point(REF_ vertexData, REF_ pointIdx, vertexFormat, Vector2(-halfSize.x, halfSize.y), Vector2(0.0f, 1.0f));
					add_point(REF_ vertexData, REF_ pointIdx, vertexFormat, Vector2(halfSize.x, halfSize.y), Vector2(1.0f, 1.0f));
					add_point(REF_ vertexData, REF_ pointIdx, vertexFormat, Vector2(halfSize.x, -halfSize.y), Vector2(1.0f, 0.0f));

					int elementIdx = 0;
					add_triangle(REF_ elements, REF_ elementIdx, 0, 1, 2);
					add_triangle(REF_ elements, REF_ elementIdx, 0, 2, 3);

					lm.mesh->load_part_data(vertexData.get_data(), elements.get_data(), Meshes::Primitive::Triangle, vertexData.get_size() / vertexFormat.get_stride(), elements.get_size(), vertexFormat, indexFormat);

					lm.material = new ::System::Material();
					lm.material->set_shader(System::MaterialShaderUsage::Static, logoShaderProgram.get());
					lm.material->set_uniform(NAME(inData), splashTexture->get_texture_id());
					lm.material->set_src_blend_op(System::BlendOp::SrcAlpha);
					lm.material->set_dest_blend_op(System::BlendOp::OneMinusSrcAlpha);
					lm.material->allow_individual_instances(true);

					lm.meshInstance.set_placement(Transform(Vector3(0.0f, height + halfSize.y, 0.0f), Quat::identity));
					lm.meshInstance.set_mesh(lm.mesh);
					lm.meshInstance.set_material(0, lm.material);

					height += size.y;
					height += spacing;
				}

				logosHeight = height - spacing;
			}

			if (! Framework::is_preview_game())
			{
				otherLogoMeshes.set_size(otherSplashTextures.get_size());

				float height = 0.0f;
				float spacing = letterSize * 0.5f;
				for_every_reverse_ptr(splashTexture, otherSplashTextures)
				{
					LogoMesh& lm = otherLogoMeshes[for_everys_index(splashTexture)];
					lm.mesh = new Meshes::Mesh3D();

					Array<int32> elements;
					Array<int8> vertexData;

					int pointCount = 4;
					int triangleCount = 2;

					vertexData.set_size(vertexFormat.get_stride() * pointCount);
					elements.set_size(3 * triangleCount);

					Vector2 size = splashTexture->get_size().to_vector2();
					float scale = (letterSize * 6.0f) / size.x;
					size *= scale;

					Vector2 halfSize = size * Vector2::half;

					int pointIdx = 0;
					add_point(REF_ vertexData, REF_ pointIdx, vertexFormat, Vector2(-halfSize.x, -halfSize.y * 2.0f), Vector2(0.0f, 0.0f));
					add_point(REF_ vertexData, REF_ pointIdx, vertexFormat, Vector2(-halfSize.x, 0.0f), Vector2(0.0f, 1.0f));
					add_point(REF_ vertexData, REF_ pointIdx, vertexFormat, Vector2(halfSize.x, 0.0f), Vector2(1.0f, 1.0f));
					add_point(REF_ vertexData, REF_ pointIdx, vertexFormat, Vector2(halfSize.x, -halfSize.y * 2.0f), Vector2(1.0f, 0.0f));

					int elementIdx = 0;
					add_triangle(REF_ elements, REF_ elementIdx, 0, 1, 2);
					add_triangle(REF_ elements, REF_ elementIdx, 0, 2, 3);

					lm.mesh->load_part_data(vertexData.get_data(), elements.get_data(), Meshes::Primitive::Triangle, vertexData.get_size() / vertexFormat.get_stride(), elements.get_size(), vertexFormat, indexFormat);

					lm.material = new ::System::Material();
					lm.material->set_shader(System::MaterialShaderUsage::Static, logoShaderProgram.get());
					lm.material->set_uniform(NAME(inData), splashTexture->get_texture_id());
					lm.material->set_src_blend_op(System::BlendOp::SrcAlpha);
					lm.material->set_dest_blend_op(System::BlendOp::OneMinusSrcAlpha);
					lm.material->allow_individual_instances(true);

					lm.meshInstance.set_placement(Transform(Vector3(0.0f, height + halfSize.y, 0.0f), Quat::identity));
					lm.meshInstance.set_mesh(lm.mesh);
					lm.meshInstance.set_material(0, lm.material);

					height -= size.y;
					height -= spacing;
				}

				otherLogosHeight = height - spacing;
			}
		}

	}
}

void VoidRoom::construct_letters()
{
	System::IndexFormat indexFormat;
	indexFormat.with_element_size(System::IndexElementSize::FourBytes);
	indexFormat.calculate_stride();

	System::VertexFormat vertexFormat;
	vertexFormat.with_padding();
	vertexFormat.calculate_stride_and_offsets();

	float height = this->letterSize; // h
	float halfHeight = 0.5f;
	float corner = 0.5f; // c
	float thickness = this->thickness;
	float halfThickness = thickness * 0.5f;
	int pointsOnArc = this->pointsOnArc;

	{	// V
		letterV = new Meshes::Mesh3D();

		Array<int32> elements;
		Array<int8> vertexData;

		int pointCount = 6;
		int triangleCount = 4;

		vertexData.set_size(vertexFormat.get_stride() * pointCount);
		elements.set_size(3 * triangleCount);

		/*
		 *    ^ A--*---
		 *    |	 \     \
		 *    h---\     \---
		 *    |	   \     \
		 *    v	D   C__B__\
		 *			 E
		 *	    <--c--->
		 *			<--fw->
		 *	<AEB is 90'
		 */
		float AB = sqrt(sqr(corner) + sqr(height)); // |AB|
		float aDAB = atan_deg(corner / height); // <DAB
		float aEAB = asin_deg(halfThickness / AB); // <EAB
		float aDAC = aDAB - aEAB; // <DAC
		float flatWidth = (corner - tan_deg(aDAC) * height) * 2.0f; // fw
		int pointIdx = 0;
		add_point(REF_ vertexData, REF_ pointIdx, vertexFormat, Vector2(-corner, halfHeight));
		add_point(REF_ vertexData, REF_ pointIdx, vertexFormat, Vector2(-corner + flatWidth, halfHeight));
		add_point(REF_ vertexData, REF_ pointIdx, vertexFormat, Vector2(corner - flatWidth, halfHeight));
		add_point(REF_ vertexData, REF_ pointIdx, vertexFormat, Vector2(corner, halfHeight));
		add_point(REF_ vertexData, REF_ pointIdx, vertexFormat, Vector2(-flatWidth * 0.5f, -halfHeight));
		add_point(REF_ vertexData, REF_ pointIdx, vertexFormat, Vector2(flatWidth * 0.5f, -halfHeight));

		int elementIdx = 0;
		add_triangle(REF_ elements, REF_ elementIdx, 0, 1, 4);
		add_triangle(REF_ elements, REF_ elementIdx, 4, 1, 5);
		add_triangle(REF_ elements, REF_ elementIdx, 4, 2, 5);
		add_triangle(REF_ elements, REF_ elementIdx, 5, 2, 3);

		letterV->load_part_data(vertexData.get_data(), elements.get_data(), Meshes::Primitive::Triangle, vertexData.get_size() / vertexFormat.get_stride(), elements.get_size(), vertexFormat, indexFormat);
	}

	{	// O
		letterO = new Meshes::Mesh3D();

		Array<int32> elements;
		Array<int8> vertexData;

		int pointCount = 32 * 4; // 4 arcs (two inside, two outside)
		int triangleCount = (pointsOnArc - 1) * 2 * 2; // 4 points make 2 triangles, we have two parts

		vertexData.set_size(vertexFormat.get_stride() * pointCount);
		elements.set_size(3 * triangleCount);

		/*
		 *				   |
		 *	^		  _--A |			|AF| = halfHeight
		 *	|		 /	 | |			|HF| = halfThickness
		 *	|	    /	-B |			|BF| = halfHeight - thickness
		 *  |      |   /   |
		 *	h	 --G   |-H-F-----
		 *	|      |   \   |
		 *	|	^	\	-C |
		 *	| 0.5*fw \   | |
		 *	v	v     '--D E
		 *				   |
		 *				 <-> 0.5 * fw
		 *		   <-0.5*h->
		 */
		float aHFA = acos_deg(halfThickness / halfHeight);
		float aHFB = acos_deg(halfThickness / (halfHeight - thickness));
		float invPointsOnArcF = 1.0f / (float)(pointsOnArc - 1);
		float AF = halfHeight;
		float BF = halfHeight - thickness;

		int pointIdx = 0;
		int elementIdx = 0;
		for (int sideIdx = 0; sideIdx < 2; ++sideIdx)
		{
			float side = sideIdx == 0 ? -1.0f : 1.0f;
			for (int p = 0; p < pointsOnArc; ++p)
			{
				float pt = ((float)p * invPointsOnArcF);
				float aOuter = aHFA - aHFA * 2.0f * pt;
				float aInner = aHFB - aHFB * 2.0f * pt;
				add_point(REF_ vertexData, REF_ pointIdx, vertexFormat, AF * Vector2(side * cos_deg(aOuter), sin_deg(aOuter)));
				add_point(REF_ vertexData, REF_ pointIdx, vertexFormat, BF * Vector2(side * cos_deg(aInner), sin_deg(aInner)));
				if (p > 0)
				{
					if (sideIdx == 0)
					{
						add_triangle(REF_ elements, REF_ elementIdx, pointIdx - 4, pointIdx - 3, pointIdx - 2);
						add_triangle(REF_ elements, REF_ elementIdx, pointIdx - 2, pointIdx - 3, pointIdx - 1);
					}
					else
					{
						add_triangle(REF_ elements, REF_ elementIdx, pointIdx - 3, pointIdx - 4, pointIdx - 2);
						add_triangle(REF_ elements, REF_ elementIdx, pointIdx - 3, pointIdx - 2, pointIdx - 1);
					}
				}
			}
		}

		letterO->load_part_data(vertexData.get_data(), elements.get_data(), Meshes::Primitive::Triangle, vertexData.get_size() / vertexFormat.get_stride(), elements.get_size(), vertexFormat, indexFormat);
	}

	{	// I
		letterI = new Meshes::Mesh3D();

		Array<int32> elements;
		Array<int8> vertexData;

		int pointCount = 4;
		int triangleCount = 2;

		vertexData.set_size(vertexFormat.get_stride() * pointCount);
		elements.set_size(3 * triangleCount);

		int pointIdx = 0;
		add_point(REF_ vertexData, REF_ pointIdx, vertexFormat, Vector2(-halfThickness, halfHeight));
		add_point(REF_ vertexData, REF_ pointIdx, vertexFormat, Vector2(halfThickness, halfHeight));
		add_point(REF_ vertexData, REF_ pointIdx, vertexFormat, Vector2(-halfThickness, -halfHeight));
		add_point(REF_ vertexData, REF_ pointIdx, vertexFormat, Vector2(halfThickness, -halfHeight));

		int elementIdx = 0;
		add_triangle(REF_ elements, REF_ elementIdx, 0, 1, 2);
		add_triangle(REF_ elements, REF_ elementIdx, 2, 1, 3);

		letterI->load_part_data(vertexData.get_data(), elements.get_data(), Meshes::Primitive::Triangle, vertexData.get_size() / vertexFormat.get_stride(), elements.get_size(), vertexFormat, indexFormat);
	}

	{	// D
		letterD = new Meshes::Mesh3D();

		Array<int32> elements;
		Array<int8> vertexData;

		int pointCount = 10 + pointsOnArc * 2;
		int triangleCount = 6 + (pointsOnArc - 1) * 2;

		vertexData.set_size(vertexFormat.get_stride() * pointCount);
		elements.set_size(3 * triangleCount);

		int pointIdx = 0;
		add_point(REF_ vertexData, REF_ pointIdx, vertexFormat, Vector2(-corner, -halfHeight));
		add_point(REF_ vertexData, REF_ pointIdx, vertexFormat, Vector2(-corner, halfHeight));
		add_point(REF_ vertexData, REF_ pointIdx, vertexFormat, Vector2(-corner + thickness, halfHeight - thickness));
		add_point(REF_ vertexData, REF_ pointIdx, vertexFormat, Vector2(-corner + thickness, -halfHeight));
		add_point(REF_ vertexData, REF_ pointIdx, vertexFormat, Vector2(-corner + thickness*2.0f, -halfHeight));
		add_point(REF_ vertexData, REF_ pointIdx, vertexFormat, Vector2(-corner + thickness*2.0f, -halfHeight + thickness));
		add_point(REF_ vertexData, REF_ pointIdx, vertexFormat, Vector2(0.0f, halfHeight));
		add_point(REF_ vertexData, REF_ pointIdx, vertexFormat, Vector2(0.0f, halfHeight - thickness));
		add_point(REF_ vertexData, REF_ pointIdx, vertexFormat, Vector2(0.0f, -halfHeight + thickness));
		add_point(REF_ vertexData, REF_ pointIdx, vertexFormat, Vector2(0.0f, -halfHeight));

		int elementIdx = 0;
		add_triangle(REF_ elements, REF_ elementIdx, 0, 1, 2);
		add_triangle(REF_ elements, REF_ elementIdx, 0, 2, 3);
		add_triangle(REF_ elements, REF_ elementIdx, 2, 1, 6);
		add_triangle(REF_ elements, REF_ elementIdx, 7, 2, 6);
		add_triangle(REF_ elements, REF_ elementIdx, 4, 5, 8);
		add_triangle(REF_ elements, REF_ elementIdx, 4, 8, 9);

		float invPointsOnArcF = 1.0f / (float)(pointsOnArc - 1);
		float rOuter = halfHeight;
		float rInner = halfHeight - thickness;
		for (int p = 0; p < pointsOnArc; ++p)
		{
			float pt = ((float)p * invPointsOnArcF);
			float angle = 90.0f - 180.0f * pt;
			add_point(REF_ vertexData, REF_ pointIdx, vertexFormat, rOuter * Vector2(cos_deg(angle), sin_deg(angle)));
			add_point(REF_ vertexData, REF_ pointIdx, vertexFormat, rInner * Vector2(cos_deg(angle), sin_deg(angle)));
			if (p > 0)
			{
				add_triangle(REF_ elements, REF_ elementIdx, pointIdx - 3, pointIdx - 4, pointIdx - 2);
				add_triangle(REF_ elements, REF_ elementIdx, pointIdx - 3, pointIdx - 2, pointIdx - 1);
			}
		}

		letterD->load_part_data(vertexData.get_data(), elements.get_data(), Meshes::Primitive::Triangle, vertexData.get_size() / vertexFormat.get_stride(), elements.get_size(), vertexFormat, indexFormat);
	}

	{	// R
		letterR = new Meshes::Mesh3D();

		Array<int32> elements;
		Array<int8> vertexData;

		int pointCount = 13 + pointsOnArc * 2;
		int triangleCount = 8 + (pointsOnArc - 1) * 2;

		vertexData.set_size(vertexFormat.get_stride() * pointCount);
		elements.set_size(3 * triangleCount);

		float shelfTop = 0.0f;
		float shelfBottom = -thickness;
		float shelfLeftTop = 0.0f;
		float shelfRightBottom = corner;

		/**
		 *					 Y
		 *			A____Bp	 .		A is at shelfLeftTop, shelfTop
		 *			 \ 	 \	 |
		 *			  \	  B-'		B is at shelfBottom
		 *			   \   \
		 *			.   \_._\
		 *			X	 CV	 D		D is at corner, halfHeight
		 *
		 *	A, D are known
		 *	<XAD = asin(|XD| / |AD|)
		 *	<CAD = asin(thickness / |AD|) 
		 *	<XAC = <YDB = <VBD = <XAD - <CAD
		 *	tg(<XAC) = XC / AX	->	XC = tg(<XAC) * AX
		 *	tg(<VBD) = VD / VB	->	VD = tg(<VBD) * VB
		 */
		Vector2 A = Vector2(shelfLeftTop, shelfTop);
		Vector2 D = Vector2(shelfRightBottom, -halfHeight);
		float AD = (D - A).length();
		float XD = shelfRightBottom - shelfLeftTop;
		float aXAD = asin_deg(clamp(XD / AD, 0.0f, 1.0f));
		float aCAD = asin_deg(clamp(thickness / AD, 0.0f, 1.0f));
		float aXAC = aXAD - aCAD;
		float tgXAC = tan_deg(aXAC);
		Vector2 C = Vector2(A.x + tgXAC * abs(D.y - shelfTop), -halfHeight);
		Vector2 B = Vector2(D.x - tgXAC * abs(D.y - shelfBottom), shelfBottom);
		Vector2 Bp = A + (D-C);
		float radiusOfR = (halfHeight - shelfBottom) * 0.5f;
		Vector2 centreOfR(corner - radiusOfR, halfHeight - radiusOfR);

		int pointIdx = 0;
		add_point(REF_ vertexData, REF_ pointIdx, vertexFormat, Vector2(-corner, -halfHeight));
		add_point(REF_ vertexData, REF_ pointIdx, vertexFormat, Vector2(-corner, halfHeight));
		add_point(REF_ vertexData, REF_ pointIdx, vertexFormat, Vector2(-corner + thickness, halfHeight - thickness));
		add_point(REF_ vertexData, REF_ pointIdx, vertexFormat, Vector2(-corner + thickness, -halfHeight));
		add_point(REF_ vertexData, REF_ pointIdx, vertexFormat, Vector2(centreOfR.x, halfHeight));
		add_point(REF_ vertexData, REF_ pointIdx, vertexFormat, Vector2(centreOfR.x, halfHeight - thickness));
		add_point(REF_ vertexData, REF_ pointIdx, vertexFormat, Vector2(centreOfR.x, shelfTop));
		add_point(REF_ vertexData, REF_ pointIdx, vertexFormat, Vector2(centreOfR.x, shelfBottom));
		add_point(REF_ vertexData, REF_ pointIdx, vertexFormat, B);
		add_point(REF_ vertexData, REF_ pointIdx, vertexFormat, D);
		add_point(REF_ vertexData, REF_ pointIdx, vertexFormat, C);
		add_point(REF_ vertexData, REF_ pointIdx, vertexFormat, A);
		add_point(REF_ vertexData, REF_ pointIdx, vertexFormat, Bp);

		int elementIdx = 0;
		add_triangle(REF_ elements, REF_ elementIdx, 0, 1, 2);
		add_triangle(REF_ elements, REF_ elementIdx, 0, 2, 3);
		add_triangle(REF_ elements, REF_ elementIdx, 2, 1, 4);
		add_triangle(REF_ elements, REF_ elementIdx, 5, 2, 4);

		add_triangle(REF_ elements, REF_ elementIdx, 6, 7, 8);
		add_triangle(REF_ elements, REF_ elementIdx, 6, 8, 11);

		add_triangle(REF_ elements, REF_ elementIdx, 12, 9, 11);
		add_triangle(REF_ elements, REF_ elementIdx, 9, 10, 11);

		float invPointsOnArcF = 1.0f / (float)(pointsOnArc - 1);
		float rOuter = radiusOfR;
		float rInner = rOuter - thickness;
		for (int p = 0; p < pointsOnArc; ++p)
		{
			float pt = ((float)p * invPointsOnArcF);
			float angle = 90.0f - 180.0f * pt;
			add_point(REF_ vertexData, REF_ pointIdx, vertexFormat, centreOfR + rOuter * Vector2(cos_deg(angle), sin_deg(angle)));
			add_point(REF_ vertexData, REF_ pointIdx, vertexFormat, centreOfR + rInner * Vector2(cos_deg(angle), sin_deg(angle)));
			if (p > 0)
			{
				add_triangle(REF_ elements, REF_ elementIdx, pointIdx - 3, pointIdx - 4, pointIdx - 2);
				add_triangle(REF_ elements, REF_ elementIdx, pointIdx - 3, pointIdx - 2, pointIdx - 1);
			}
		}

		letterR->load_part_data(vertexData.get_data(), elements.get_data(), Meshes::Primitive::Triangle, vertexData.get_size() / vertexFormat.get_stride(), elements.get_size(), vertexFormat, indexFormat);
	}

	if (true)
	{	// M
		letterM = new Meshes::Mesh3D();

		Array<int32> elements;
		Array<int8> vertexData;

		int pointCount = 14 + pointsOnArc * 2;
		int triangleCount = 8 + (pointsOnArc - 1) * 2;

		vertexData.set_size(vertexFormat.get_stride() * pointCount);
		elements.set_size(3 * triangleCount);

		float radiusOfM = min((halfThickness - (-corner + thickness)), thickness * 1.5f);

		int pointIdx = 0;
		add_point(REF_ vertexData, REF_ pointIdx, vertexFormat, Vector2(-corner, -halfHeight)); // left, left bottom
		add_point(REF_ vertexData, REF_ pointIdx, vertexFormat, Vector2(-corner, halfHeight)); // left, left top
		add_point(REF_ vertexData, REF_ pointIdx, vertexFormat, Vector2(-corner + thickness, -halfHeight)); // left, right bottom
		add_point(REF_ vertexData, REF_ pointIdx, vertexFormat, Vector2(-corner + thickness, halfHeight - thickness)); // left, right, at arc height
		add_point(REF_ vertexData, REF_ pointIdx, vertexFormat, Vector2(corner - radiusOfM, halfHeight)); // right, left top arc end
		add_point(REF_ vertexData, REF_ pointIdx, vertexFormat, Vector2(corner - radiusOfM, halfHeight - thickness)); // right, left below top, arc end
		add_point(REF_ vertexData, REF_ pointIdx, vertexFormat, Vector2(corner - thickness, -halfHeight)); // right, left bottom
		add_point(REF_ vertexData, REF_ pointIdx, vertexFormat, Vector2(corner, -halfHeight)); // right, right bottom
		add_point(REF_ vertexData, REF_ pointIdx, vertexFormat, Vector2(corner - thickness, halfHeight - radiusOfM)); // right, left top below arc
		add_point(REF_ vertexData, REF_ pointIdx, vertexFormat, Vector2(corner, halfHeight - radiusOfM)); // right, right top below arc
		add_point(REF_ vertexData, REF_ pointIdx, vertexFormat, Vector2(-halfThickness, -halfHeight)); // centre, left bottom
		add_point(REF_ vertexData, REF_ pointIdx, vertexFormat, Vector2(halfThickness, -halfHeight)); // centre, right bottom
		add_point(REF_ vertexData, REF_ pointIdx, vertexFormat, Vector2(-halfThickness, halfHeight - thickness * 2.0f)); // centre, left top below arc
		add_point(REF_ vertexData, REF_ pointIdx, vertexFormat, Vector2(halfThickness, halfHeight - thickness * 2.0f)); // centre, right top below arc

		int elementIdx = 0;
		add_triangle(REF_ elements, REF_ elementIdx, 3, 2, 0);
		add_triangle(REF_ elements, REF_ elementIdx, 3, 0, 1);
		add_triangle(REF_ elements, REF_ elementIdx, 3, 1, 4);
		add_triangle(REF_ elements, REF_ elementIdx, 3, 4, 5);
		add_triangle(REF_ elements, REF_ elementIdx, 6, 8, 9);
		add_triangle(REF_ elements, REF_ elementIdx, 6, 9, 7);
		add_triangle(REF_ elements, REF_ elementIdx, 10, 12, 13);
		add_triangle(REF_ elements, REF_ elementIdx, 10, 13, 11);

		float invPointsOnArcF = 1.0f / (float)(pointsOnArc - 1);
		float rOuter = radiusOfM;
		float rInner = rOuter - thickness;
		{
			Vector2 centreOfM(corner - radiusOfM, halfHeight - radiusOfM);
			float aOuterGoesFor = 90.0f;
			float aInnerGoesFor = 90.0f;
			for (int p = 0; p < pointsOnArc; ++p)
			{
				float pt = ((float)p * invPointsOnArcF);
				float aOuter = aOuterGoesFor * pt;
				float aInner = aInnerGoesFor * pt;
				add_point(REF_ vertexData, REF_ pointIdx, vertexFormat, centreOfM + rOuter * Vector2(cos_deg(aOuter), sin_deg(aOuter)));
				add_point(REF_ vertexData, REF_ pointIdx, vertexFormat, centreOfM + rInner * Vector2(cos_deg(aInner), sin_deg(aInner)));
				if (p > 0)
				{
					add_triangle(REF_ elements, REF_ elementIdx, pointIdx - 4, pointIdx - 3, pointIdx - 2);
					add_triangle(REF_ elements, REF_ elementIdx, pointIdx - 2, pointIdx - 3, pointIdx - 1);
				}
			}
		}

		letterM->load_part_data(vertexData.get_data(), elements.get_data(), Meshes::Primitive::Triangle, vertexData.get_size() / vertexFormat.get_stride(), elements.get_size(), vertexFormat, indexFormat);
	}
	else
	{	// M
		letterM = new Meshes::Mesh3D();

		Array<int32> elements;
		Array<int8> vertexData;

		int pointCount = 14 + pointsOnArc * 2 * 2;
		int triangleCount = 8 + (pointsOnArc - 1) * 2 * 2;

		vertexData.set_size(vertexFormat.get_stride() * pointCount);
		elements.set_size(3 * triangleCount);

		float radiusOfM = min((halfThickness - (-corner + thickness)), thickness * 1.5f);

		int pointIdx = 0;
		add_point(REF_ vertexData, REF_ pointIdx, vertexFormat, Vector2(-corner, -halfHeight)); // left, left bottom
		add_point(REF_ vertexData, REF_ pointIdx, vertexFormat, Vector2(-corner, halfHeight)); // left, left top
		add_point(REF_ vertexData, REF_ pointIdx, vertexFormat, Vector2(halfThickness - radiusOfM, halfHeight)); // centre, left top arc end
		add_point(REF_ vertexData, REF_ pointIdx, vertexFormat, Vector2(-corner + thickness, -halfHeight)); // left, right bottom
		add_point(REF_ vertexData, REF_ pointIdx, vertexFormat, Vector2(-halfThickness, -halfHeight)); // centre, left bottom
		add_point(REF_ vertexData, REF_ pointIdx, vertexFormat, Vector2(halfThickness, -halfHeight)); // centre, right bottom
		add_point(REF_ vertexData, REF_ pointIdx, vertexFormat, Vector2(-halfThickness, halfHeight - radiusOfM)); // centre, left top below arc
		add_point(REF_ vertexData, REF_ pointIdx, vertexFormat, Vector2(halfThickness, halfHeight - radiusOfM)); // centre, right top below arc
		add_point(REF_ vertexData, REF_ pointIdx, vertexFormat, Vector2(corner - thickness, -halfHeight)); // right, left bottom
		add_point(REF_ vertexData, REF_ pointIdx, vertexFormat, Vector2(corner, -halfHeight)); // right, right bottom
		add_point(REF_ vertexData, REF_ pointIdx, vertexFormat, Vector2(corner - thickness, halfHeight - radiusOfM)); // right, left top below arc
		add_point(REF_ vertexData, REF_ pointIdx, vertexFormat, Vector2(corner, halfHeight - radiusOfM)); // right, right top below arc
		add_point(REF_ vertexData, REF_ pointIdx, vertexFormat, Vector2(-corner + thickness, halfHeight - thickness)); // left, right, at arc height
		add_point(REF_ vertexData, REF_ pointIdx, vertexFormat, Vector2(halfThickness - radiusOfM, halfHeight - thickness)); // centre, left below top, arc end

		int elementIdx = 0;
		add_triangle(REF_ elements, REF_ elementIdx, 3, 0, 1);
		add_triangle(REF_ elements, REF_ elementIdx, 3, 1, 12);
		add_triangle(REF_ elements, REF_ elementIdx, 12, 1, 2);
		add_triangle(REF_ elements, REF_ elementIdx, 13, 12, 2);
		add_triangle(REF_ elements, REF_ elementIdx, 4, 6, 5);
		add_triangle(REF_ elements, REF_ elementIdx, 5, 6, 7);
		add_triangle(REF_ elements, REF_ elementIdx, 9, 8, 10);
		add_triangle(REF_ elements, REF_ elementIdx, 9, 10, 11);

		float invPointsOnArcF = 1.0f / (float)(pointsOnArc - 1);
		float rOuter = radiusOfM;
		float rInner = rOuter - thickness;
		for (int arcIdx = 0; arcIdx < 2; ++arcIdx)
		{
			Vector2 centreOfM(arcIdx == 0 ? halfThickness - radiusOfM : corner - radiusOfM, halfHeight - radiusOfM);
			Vector2 centreOfMInner = centreOfM;
			float stopAt = min(halfThickness + thickness, corner - thickness);
			float aOuterGoesFor = arcIdx == 0 || rOuter <= 0.0f ? 90.0f : clamp(acos_deg(clamp((stopAt - centreOfM.x) / rOuter, 0.0f, 1.0f)), 0.0f, 90.0f);
			float aInnerGoesFor = arcIdx == 0 || rInner <= 0.0f ? 90.0f : clamp(acos_deg(clamp((stopAt - centreOfM.x) / rInner, 0.0f, 1.0f)), 0.0f, 90.0f);
			float rInnerUse = rInner;
			if (arcIdx == 1)
			{
				centreOfMInner = Vector2(halfThickness - radiusOfM, halfHeight - radiusOfM);
				aOuterGoesFor = 90.0f;
				Vector2 rightBottomCorner(corner - thickness, halfHeight - radiusOfM);
				rInnerUse = (rightBottomCorner - centreOfMInner).length();
				aInnerGoesFor = clamp(asin_deg(clamp(rOuter / rInnerUse, 0.0f, 1.0f)), 0.0f, 90.0f);
			}
			for (int p = 0; p < pointsOnArc; ++p)
			{
				float pt = ((float)p * invPointsOnArcF);
				float aOuter = aOuterGoesFor * pt;
				float aInner = aInnerGoesFor * pt;
				add_point(REF_ vertexData, REF_ pointIdx, vertexFormat, centreOfM + rOuter * Vector2(cos_deg(aOuter), sin_deg(aOuter)));
				add_point(REF_ vertexData, REF_ pointIdx, vertexFormat, centreOfMInner + rInnerUse * Vector2(cos_deg(aInner), sin_deg(aInner)));
				if (p > 0)
				{
					add_triangle(REF_ elements, REF_ elementIdx, pointIdx - 4, pointIdx - 3, pointIdx - 2);
					add_triangle(REF_ elements, REF_ elementIdx, pointIdx - 2, pointIdx - 3, pointIdx - 1);
				}
			}
		}

		letterM->load_part_data(vertexData.get_data(), elements.get_data(), Meshes::Primitive::Triangle, vertexData.get_size() / vertexFormat.get_stride(), elements.get_size(), vertexFormat, indexFormat);
	}

	letters[0].offsetScale = Vector2(-1.5f, 0.5f);
	letters[1].offsetScale = Vector2(-0.5f, 0.5f);
	letters[2].offsetScale = Vector2(0.5f, 0.5f);
	letters[3].offsetScale = Vector2(1.5f, 0.5f);

	letters[4].offsetScale = Vector2(-1.5f, -0.5f);
	letters[5].offsetScale = Vector2(-0.5f, -0.5f);
	letters[6].offsetScale = Vector2(0.5f, -0.5f);
	letters[7].offsetScale = Vector2(1.5f, -0.5f);

	letters[0].letter = letterV;
	letters[1].letter = letterO;
	letters[2].letter = letterI;
	letters[3].letter = letterD;

	letters[4].letter = letterR;
	letters[5].letter = letterO;
	letters[6].letter = letterO;
	letters[7].letter = letterM;
	
	for_count(int, i, 8)
	{
		letters[i].letterInstance.set_mesh(letters[i].letter);
		letters[i].letterInstance.dont_provide_materials();
	}

	letters[5].angle = 90.0f;

	letters[1].rotates = true;
	letters[5].rotates = true;
	letters[6].rotates = true;

	{	// void room
		voidRoom = new Meshes::Mesh3D();

		Array<int32> elements;
		Array<int8> vertexData;

		int pointCount = 32;
		int triangleCount = 24;

		vertexData.set_size(vertexFormat.get_stride() * pointCount);
		elements.set_size(3 * triangleCount);

		float corner = voidRoomSquareSize * 0.5f;
		float height = voidRoomSquareSize;

		float innerCorner = corner * deepPT;
		float innerHeight = height * deepPT;
		float innerBottom = height * centrePT - 0.5f * innerHeight;
		float innerTop = height * centrePT + 0.5f * innerHeight;

		Vector2 edgeTop = Vector2(corner - innerCorner, height - innerTop);
		Vector2 thickTop = edgeTop.normal().rotated_right() * lineThickness * 0.5f;

		Vector2 edgeBottom = Vector2(corner - innerCorner, innerBottom);
		Vector2 thickBottom = edgeBottom.normal().rotated_right() * lineThickness * 0.5f;

		int pointIdx = 0;
		add_point(REF_ vertexData, REF_ pointIdx, vertexFormat, Vector2(-corner, 0.0f));
		add_point(REF_ vertexData, REF_ pointIdx, vertexFormat, Vector2(-corner, height));
		add_point(REF_ vertexData, REF_ pointIdx, vertexFormat, Vector2(corner, height));
		add_point(REF_ vertexData, REF_ pointIdx, vertexFormat, Vector2(corner, 0.0f));

		add_point(REF_ vertexData, REF_ pointIdx, vertexFormat, Vector2(-corner + lineThickness, lineThickness));
		add_point(REF_ vertexData, REF_ pointIdx, vertexFormat, Vector2(-corner + lineThickness, height - lineThickness));
		add_point(REF_ vertexData, REF_ pointIdx, vertexFormat, Vector2(corner - lineThickness, height - lineThickness));
		add_point(REF_ vertexData, REF_ pointIdx, vertexFormat, Vector2(corner - lineThickness, lineThickness));

		add_point(REF_ vertexData, REF_ pointIdx, vertexFormat, Vector2(-innerCorner, innerBottom));
		add_point(REF_ vertexData, REF_ pointIdx, vertexFormat, Vector2(-innerCorner, innerTop));
		add_point(REF_ vertexData, REF_ pointIdx, vertexFormat, Vector2(innerCorner, innerTop));
		add_point(REF_ vertexData, REF_ pointIdx, vertexFormat, Vector2(innerCorner, innerBottom));

		add_point(REF_ vertexData, REF_ pointIdx, vertexFormat, Vector2(-innerCorner + lineThickness, innerBottom + lineThickness));
		add_point(REF_ vertexData, REF_ pointIdx, vertexFormat, Vector2(-innerCorner + lineThickness, innerTop - lineThickness));
		add_point(REF_ vertexData, REF_ pointIdx, vertexFormat, Vector2(innerCorner - lineThickness, innerTop - lineThickness));
		add_point(REF_ vertexData, REF_ pointIdx, vertexFormat, Vector2(innerCorner - lineThickness, innerBottom + lineThickness));

		int elementIdx = 0;
		add_triangle(REF_ elements, REF_ elementIdx, 0, 1, 4);
		add_triangle(REF_ elements, REF_ elementIdx, 4, 1, 5);
		add_triangle(REF_ elements, REF_ elementIdx, 1, 2, 5);
		add_triangle(REF_ elements, REF_ elementIdx, 2, 6, 5);
		add_triangle(REF_ elements, REF_ elementIdx, 2, 3, 7);
		add_triangle(REF_ elements, REF_ elementIdx, 2, 7, 6);
		add_triangle(REF_ elements, REF_ elementIdx, 3, 4, 7);
		add_triangle(REF_ elements, REF_ elementIdx, 3, 0, 4);

		add_triangle(REF_ elements, REF_ elementIdx, 0 + 8, 1 + 8, 4 + 8);
		add_triangle(REF_ elements, REF_ elementIdx, 4 + 8, 1 + 8, 5 + 8);
		add_triangle(REF_ elements, REF_ elementIdx, 1 + 8, 2 + 8, 5 + 8);
		add_triangle(REF_ elements, REF_ elementIdx, 2 + 8, 6 + 8, 5 + 8);
		add_triangle(REF_ elements, REF_ elementIdx, 2 + 8, 3 + 8, 7 + 8);
		add_triangle(REF_ elements, REF_ elementIdx, 2 + 8, 7 + 8, 6 + 8);
		add_triangle(REF_ elements, REF_ elementIdx, 3 + 8, 4 + 8, 7 + 8);
		add_triangle(REF_ elements, REF_ elementIdx, 3 + 8, 0 + 8, 4 + 8);

		for (int i = 0; i < 4; ++i)
		{
			Vector2 side = Vector2::one;
			if (i < 2)
			{
				side.x = -1.0f;
			}
			if (i % 2)
			{
				side.y = -1.0f;
			}
			Vector2 thick = i % 2 ? thickBottom : thickTop;
			float innerTo = i % 2 ? height * 0.5f - innerBottom : innerTop - height * 0.5f;
			add_point(REF_ vertexData, REF_ pointIdx, vertexFormat, side * (Vector2(corner - lineThickness, corner - lineThickness) - thick) + Vector2(0.0f, height * 0.5f));
			add_point(REF_ vertexData, REF_ pointIdx, vertexFormat, side * (Vector2(corner - lineThickness, corner - lineThickness) + thick) + Vector2(0.0f, height * 0.5f));
			add_point(REF_ vertexData, REF_ pointIdx, vertexFormat, side * (Vector2(innerCorner - lineThickness, innerTo - lineThickness) + thick) + Vector2(0.0f, height * 0.5f));
			add_point(REF_ vertexData, REF_ pointIdx, vertexFormat, side * (Vector2(innerCorner - lineThickness, innerTo - lineThickness) - thick) + Vector2(0.0f, height * 0.5f));

			if (side.x * side.y < 0.0f)
			{
				add_triangle(REF_ elements, REF_ elementIdx, pointIdx - 3, pointIdx - 4, pointIdx - 2);
				add_triangle(REF_ elements, REF_ elementIdx, pointIdx - 2, pointIdx - 4, pointIdx - 1);
			}
			else
			{
				add_triangle(REF_ elements, REF_ elementIdx, pointIdx - 4, pointIdx - 3, pointIdx - 2);
				add_triangle(REF_ elements, REF_ elementIdx, pointIdx - 4, pointIdx - 2, pointIdx - 1);
			}
		}

		voidRoom->load_part_data(vertexData.get_data(), elements.get_data(), Meshes::Primitive::Triangle, vertexData.get_size() / vertexFormat.get_stride(), elements.get_size(), vertexFormat, indexFormat);

		voidRoomInstance.set_mesh(voidRoom);
		voidRoomInstance.dont_provide_materials();
	}
}

void VoidRoom::construct_function_mesh()
{
	System::IndexFormat indexFormat;
	indexFormat.with_element_size(System::IndexElementSize::FourBytes);
	indexFormat.calculate_stride();

	System::VertexFormat vertexFormat;
	vertexFormat.with_padding();
	vertexFormat.calculate_stride_and_offsets();

	if (functionType != NoFunction &&
		type != VoidRoomLoaderType::None)
	{	// function
		functionMesh = new Meshes::Mesh3D();

		Array<int32> elements;
		Array<int8> vertexData;

		tchar const * initPreviewGame[] = {
			TXT("           #      #  #             #          #  ."),
			TXT("  ##   ## # # # #   # # # # #     ###  #   #  #  ."),
			TXT("  # # #   ##  # # # ##  # # #      #  # # # # #  ."),
			TXT("  ##  #    ##  #  #  ##  # #       #   #   #  #  ."),
			TXT("  #                                              ."),
			TXT("          #           # #                        ."),
			TXT("          #  #   #   ##   ##   #                 ."),
			TXT("          # # # # # # # # # # # #                ."),
			TXT("          #  #   ##  ## # # #  ## # # #          ."),
			TXT("                                #                ."),
			TXT(".") };

		tchar const* stopPreviewGame[] = {
			TXT("           #      #  #             #          #  ."),
			TXT("  ##   ## # # # #   # # # # #     ###  #   #  #  ."),
			TXT("  # # #   ##  # # # ##  # # #      #  # # # # #  ."),
			TXT("  ##  #    ##  #  #  ##  # #       #   #   #  #  ."),
			TXT("  #                                              ."),
			TXT("                         #                       ."),
			TXT("         ##  # #  ##  #    ##   #                ."),
			TXT("         # # # # #   # # # # # # #               ."),
			TXT("         ##   ## #    ## # # #  ## # # #         ."),
			TXT("         #             #         #               ."),
			TXT(".") };

		/*
		tchar const* initSubspaceScavengersOld[] = {
			TXT("           ##  #           #  #                  ."),
			TXT("          ##  ###  #   ## ###   ##   #           ."),
			TXT("            #  #  # # #    #  # # # # #          ."),
			TXT("          ##   #   ## #    #  # # #  ##          ."),
			TXT("                                      #          ."),
			TXT("                                                 ."),
			TXT("      ### #  # ###   ### ###   ##   ### ####     ."),
			TXT("     ##   #  # #### ##   #  # #  # #    ###      ."),
			TXT("       ## #  # #  #   ## ###  #### #    #        ."),
			TXT("     ###   ##  ###  ###  #    #  #  ### ####     ."),
			TXT("                                                 ."),
			TXT(" ###  ###  ##  #  # #### #  #  ### #### ###   ###."),
			TXT("##   #    #  # #  # ##   ## # #    ##   #  # ##  ."),
			TXT("  ## #    ####  ##  #    # ## # ## #    ###    ##."),
			TXT("###   ### #  #  ##  #### #  #  ### #### #  # ### ."),
			TXT(".") };
		*/

		/*
		tchar const* initSubspaceScavengers[] = {
			//TXT("                     ##  #           #  #                           ."),
			//TXT("                    ##  ###  #   ## ###   ##   #                    ."),
			//TXT("                      #  #  # # #    #  # # # # #                   ."),
			//TXT("                    ##   #   ## #    #  # # #  ##                   ."),
			//TXT("                                                #                   ."),
			//TXT("                                                                    ."),
			TXT("          ##  #   #  ####      ##  ####    ###    ####   ####       ."),
			TXT("         #    #   #      #    #        #  #   #  #      #           ."),
			TXT("         #    #   #      #    #        #  #   #  #      #           ."),
			TXT("         #    #   #  ####     #    ####   #   #  #      # ###       ."),
			TXT("         #    #   #      #    #    #      #   #  #      #           ."),
			TXT("         #    #   #      #    #    #      #   #  #      #           ."),
			TXT("       ##      ###   ####   ##     #      #   #   ####   ####       ."),
			TXT("                                                                    ."),
			TXT("                                                                    ."),
			TXT("   ##   ####   ###   #   #   ####  ##  #   ####   ####  ####      ##."),
			TXT("  #    #      #   #  #   #  #      # # #  #      #          #    #  ."),
			TXT("  #    #      #   #  #   #  #      # # #  #      #          #    #  ."),
			TXT("  #    #      #   #  #   #  # ###  # # #  # ###  # ###  ####     #  ."),
			TXT("  #    #      #   #  #   #  #      # # #  #   #  #      #   #    #  ."),
			TXT("  #    #      #   #   # #   #      # # #  #   #  #      #   #    #  ."),
			TXT("##      ####  #   #    #     ####  #  ##   ####   ####  #   #  ##   ."),
			TXT(".") };
		*/

		tchar const* initTeaForGod[] = {
			TXT("##### #####   #  ."),
			TXT("             #   ."),
			TXT("  #   ###    # # ."),
			TXT("  #         #   #."),
			TXT("  #   ##### #   #."),
			TXT("                 ."),
			TXT("#####  ##   # ## ."),
			TXT("      #     #   #."),
			TXT("###   #   # # ## ."),
			TXT("          # #    ."),
			TXT("#       ##  #   #."),
			TXT("                 ."),
			TXT(" ##     ##  #### ."),
			TXT("#         #     #."),
			TXT("#  ## #   # #   #."),
			TXT("      #     #   #."),
			TXT("  ##   ##   # ## ."),
			TXT(".") };

		tchar const* initTeaForGodEmperor[] = {
			TXT("            ##### ##### #####       ##### ##### #####             ."),
			TXT("              #   #     #   #       #     #   # #   #             ."),
			TXT("              #   ###   #####       ###   #   # ####              ."),
			TXT("              #   #     #   #       #     #   # #   #             ."),
			TXT("              #   ##### #   #       #     ##### #   #             ."),
			TXT("                                                                  ."),
			TXT(" ##### ##### ####       ##### #   # ##### ##### ##### ##### ##### ."),
			TXT(" #     #   # #   #      #     ## ## #   # #     #   # #   # #   # ."),
			TXT(" # ### #   # #   #      ###   # # # ##### ###   ####  #   # ####  ."),
			TXT(" #   # #   # #   #      #     #   # #     #     #   # #   # #   # ."),
			TXT(" ##### ##### ####       ##### #   # #     ##### #   # ##### #   # ."),
			TXT(".") };

		tchar const* initNullPoint[] = {
			TXT("                      # #              #       #             ."),
			TXT("            ###  #  # # #    ###   ##    ###  ###            ."),
			TXT("            #  # #  # # #    #  # #  # # #  #  #             ."),
			TXT("            #  # #  # # #    #  # #  # # #  #  #             ."),
			TXT("            #  #  ### # #    ###   ##  # #  #   #            ."),
			TXT("                             #                               ."),
			TXT(".") };

		tchar const* initBHG[] = {
			TXT("                                #                             ."),
			TXT("       #                        #              #     ##       ."),
			TXT("       #                        #              #    #         ."),
			TXT("       #    #  #                ###            #     ##       ."),
			TXT("       ###  #  #  ##   ##       #  #  ##       # #     #      ."),
			TXT("       #  # #  # #  # #         #  # #  #  ### ##     #       ."),
			TXT("       #  #  ### #  #  ##       #  # #  # #    # #  ##        ."),
			TXT("       ###        ###    #   #        ### #    #  #           ."),
			TXT("                    # ###   #              ###                ."),
			TXT("                 ###           #                              ."),
			TXT("         #            #      #####      #            ##       ."),
			TXT("        # #      ##   #    #   #        #           #         ."),
			TXT("         #      #  #  #        #    ##  ###    ##    ##       ."),
			TXT("        # # #   #  #  #   ##   #   #  # #  #  #  #     #      ."),
			TXT("       #   #     ###  #    #    ## #    #   # ###      #      ."),
			TXT("        ### #      #  #   ###      #  # #   # #    ####       ."),
			TXT("                    #  ##           ##         ###            ."),
			TXT("                  ##                                          ."),
			TXT(".") };

		tchar const* stopEOL[] = {
			TXT("#### #  # ###        ##  ####      #    # #  # ####."),
			TXT("###  ## # #  #      #  # ###       #    # ## # ### ."),
			TXT("#    # ## #  #      #  # #         #    # # ## #   ."),
			TXT("#### #  # ###        ##  #         #### # #  # ####."),
			TXT(".") };

		/*
		tchar const* stopShutdown[] = {
			TXT(" #### ##  ## ##  ## #### ####  ##### ##   ## ##  ##."),
			TXT("###   ###### ##  ##  ##  ## ## ## ## ## # ## ### ##."),
			TXT("  ### ##  ## ##  ##  ##  ## ## ## ## ####### ## ###."),
			TXT("####  ##  ## ######  ##  ####  ##### ### ### ##  ##."),
			TXT(".") };
		*/

		/*
		tchar const* stopByeBye[] = {
			TXT("###  #   # ####      ###  #   # ####."),
			TXT("#  #  # #  #         #  #  # #  #   ."),
			TXT("###    #   ###       ###    #   ### ."),
			TXT("#  #   #   #         #  #   #   #   ."),
			TXT("###    #   ####      ###    #   ####."),
			TXT(".") };
		*/

		tchar const* loadingFailed[] = {
			TXT("#                # #          ."),
			TXT("#    ##  ##    ###   ###   ###."),
			TXT("#   #  #   #  #  # # #  # #  #."),
			TXT("#   #  # ###  #  # # #  #  ###."),
			TXT("###  ##  ####  ### # #  #    #."),
			TXT("                           ## ."),
			TXT("                              ."),
			TXT("     ##      # #         #    ."),
			TXT("    #   ##     #  ##   ###    ."),
			TXT("    ##    #  # # #### #  #    ."),
			TXT("    #   ###  # # #    #  #    ."),
			TXT("    #   #### # #  ##   ###    ."),
			TXT(".") };
		
		tchar const* endingGame[] = {
			TXT("             # #          ."),
			TXT(" ##  ###   ###   ###   ###."),
			TXT("#### #  # #  # # #  # #  #."),
			TXT("#    #  # #  # # #  #  ###."),
			TXT(" ##  #  #  ### # #  #    #."),
			TXT("                       ## ."),
			TXT("                          ."),
			TXT("    ### ##   ####   ##    ."),
			TXT("   #  #   #  # # # ####   ."),
			TXT("    ### ###  # # # #      ."),
			TXT("      # #### # # #  ##    ."),
			TXT("    ##                    ."),
			TXT(".") };

		tchar const** init = initPreviewGame;
		tchar const** stop = stopPreviewGame;
		/*
		if (type == VoidRoomLoaderType::SubspaceScavengers)
		{
			init = initSubspaceScavengers;
			stop = stopEOL;
		}
		*/

		if (type == VoidRoomLoaderType::TeaForGod)
		{
			init = initTeaForGod;
			stop = stopEOL;
		}

		if (type == VoidRoomLoaderType::TeaForGodEmperor)
		{
			init = initTeaForGodEmperor;
			stop = stopEOL;
		}

		if (type == VoidRoomLoaderType::NullPoint)
		{
			init = initNullPoint;
			stop = stopEOL;
		}		

		if (type == VoidRoomLoaderType::BHG)
		{
			init = initBHG;
			stop = stopEOL;
		}

		if (type == VoidRoomLoaderType::LoadingFailed)
		{
			init = loadingFailed;
			stop = loadingFailed;
			blockDeactivationFor = 5.0f; // whole thing should be displayed at least 5 seconds
		}

		if (type == VoidRoomLoaderType::EndingGame)
		{
			init = endingGame;
			stop = endingGame;
		}

		tchar const** use = init;
		if (functionType == ForShutdown)
		{
			use = stop;
		}

		int pixels = 0;
		int width = 0;
		functionMeshHeight = 0.0f;
		for (int l = 0; use[l][0]!='.'; ++l)
		{
			int n = 0;
			while (use[l][n] != '.')
			{
				if (use[l][n] == '#')
				{
					++pixels;
				}
				++n;
			}
			width = max(width, n);
			functionMeshHeight += 1.0f;
		}

		int pointCount = pixels * 4;
		int triangleCount = pixels * 2;

		vertexData.set_size(vertexFormat.get_stride() * pointCount);
		elements.set_size(3 * triangleCount);

		Vector2 start = Vector2(-functionMeshPixelSize.x * (float)width * 0.5f, 0.0f);

		int pointIdx = 0;
		int elementIdx = 0;

		for (int l = 0; use[l][0] != '.'; ++l)
		{
			int n = 0;
			while (use[l][n] != '.')
			{
				if (use[l][n] == '#')
				{
					Vector2 current = start + Vector2((float)n, -(float)(l + 1)) * functionMeshPixelSize;
					add_point(REF_ vertexData, REF_ pointIdx, vertexFormat, current + Vector2(0.0f, 0.0f) * functionMeshPixelSize);
					add_point(REF_ vertexData, REF_ pointIdx, vertexFormat, current + Vector2(0.0f, 1.0f) * functionMeshPixelSize);
					add_point(REF_ vertexData, REF_ pointIdx, vertexFormat, current + Vector2(1.0f, 1.0f) * functionMeshPixelSize);
					add_point(REF_ vertexData, REF_ pointIdx, vertexFormat, current + Vector2(1.0f, 0.0f) * functionMeshPixelSize);

					add_triangle(REF_ elements, REF_ elementIdx, pointIdx - 4, pointIdx - 3, pointIdx - 2);
					add_triangle(REF_ elements, REF_ elementIdx, pointIdx - 2, pointIdx - 1, pointIdx - 4);
				}
				++n;
			}
		}

		functionMesh->load_part_data(vertexData.get_data(), elements.get_data(), Meshes::Primitive::Triangle, vertexData.get_size() / vertexFormat.get_stride(), elements.get_size(), vertexFormat, indexFormat);

		functionMeshInstance.set_mesh(functionMesh);
		functionMeshInstance.dont_provide_materials();
	}
}

void VoidRoom::construct_shaders()
{
	{	// logos
		String vertexShaderSource
#ifdef AN_GL
			= String(TXT("#version 150\n"))
#endif
#ifdef AN_GLES
			= String(TXT("#version 300 es\n"))
#endif
			+ String(TXT("\n"))
			+ String(TXT("// uniforms\n"))
			+ String(TXT("uniform mat4 modelViewMatrix;\n"))
			+ String(TXT("uniform mat4 projectionMatrix;\n"))
			+ String(TXT("\n"))
			+ String(TXT("in vec2 inAPosition;\n"))
			+ String(TXT("in vec2 inOUV;\n"))
			+ String(TXT("in vec4 inOColour;\n"))
			+ String(TXT("\n"))
			+ String(TXT("// output\n"))
			+ String(TXT("out vec2 varUV;\n"))
			+ String(TXT("out vec4 varColour;\n"))
			+ String(TXT("\n"))
			+ String(TXT("void main()\n"))
			+ String(TXT("{\n"))
			+ String(TXT("	vec4 prcPosition;\n"))
			+ String(TXT("	\n"))
			+ String(TXT("	prcPosition.xy = inAPosition;\n"))
			+ String(TXT("	prcPosition.z = 0.0;\n"))
			+ String(TXT("	prcPosition.w = 1.0;\n"))
			+ String(TXT("	\n"))
			+ String(TXT("	prcPosition = modelViewMatrix * prcPosition;\n"))
			+ String(TXT("	\n"))
			+ String(TXT("	// fill output\n"))
			+ String(TXT("	gl_Position = projectionMatrix * prcPosition;\n"))
			+ String(TXT("	\n"))
			+ String(TXT("	// just copy\n"))
			+ String(TXT("	varUV = inOUV;\n"))
			+ String(TXT("  varColour = inOColour;\n"))
			+ String(TXT("}\n"))
			;

		String fragmentShaderSource
#ifdef AN_GL
			= String(TXT("#version 150\n"))
#endif
#ifdef AN_GLES
			= String(TXT("#version 300 es\n"))
#endif
			+ String(TXT("\n"))
			+ String(TXT("#ifdef GL_ES\n"))
			+ String(TXT("precision mediump float;\n"))
			+ String(TXT("precision mediump int;\n"))
			+ String(TXT("#endif\n"))
			+ String(TXT("\n"))
			+ String(TXT("#define PROCESSING_TEXTURE_SHADER\n"))
			+ String(TXT("\n"))
			+ String(TXT("// uniform\n"))
			+ String(TXT("uniform sampler2D inData;\n"))
			+ String(TXT("uniform float logoAlpha;\n"))
			+ String(TXT("uniform float activation;\n"))
			+ String(TXT("uniform vec4 overrideColour;\n"))
			+ String(TXT("\n"))
			+ String(TXT("// input\n"))
			+ String(TXT("in vec2 varUV;\n"))
			+ String(TXT("in vec4 varColour;\n"))
			+ String(TXT("\n"))
			+ String(TXT("// output\n"))
			+ String(TXT("out vec4 colour;\n"))
			+ String(TXT("\n"))
			+ String(TXT("void main()\n"))
			+ String(TXT("{\n"))
			+ String(TXT("    colour.rgba = texture(inData, varUV).rgba;\n"))
			+ String(TXT("    colour.rgba *= varColour;\n"))
			+ String(TXT("    colour.rgb = colour.rgb * (1.0 - overrideColour.a) + overrideColour.rgb * overrideColour.a;\n"))
			+ String(TXT("    colour.a *= logoAlpha;\n"))
			+ String(TXT("}\n"));

		logoVertexShader = new System::Shader(System::ShaderSetup::from_string(System::ShaderType::Vertex, vertexShaderSource));
		logoFragmentShader = new System::Shader(System::ShaderSetup::from_string(System::ShaderType::Fragment, fragmentShaderSource));

		logoFragmentShader->access_fragment_shader_output_setup().allow_default_output();

		if (logoVertexShader.get() && logoFragmentShader.get())
		{
			logoShaderProgram = new System::ShaderProgram(logoVertexShader.get(), logoFragmentShader.get());
		}
	}
}

void VoidRoom::update(float _deltaTime)
{
	if (functionType == ForVideo)
	{
		targetActivation = 1.0f;
	}
	blockDeactivationFor = max(0.0f, blockDeactivationFor - _deltaTime);
#ifdef STAY_HERE
	blockDeactivationFor = 1.0f;
#endif
	float currentTargetActivation = blockDeactivationFor > 0.0f ? 1.0f : targetActivation;
	activation = blend_to_using_speed_based_on_time(activation, currentTargetActivation, currentTargetActivation > activation ? 0.6f : 0.45f, min(_deltaTime, 0.03f));
	if (activation <= 0.0f && targetActivation <= 0.0f)
	{
		base::deactivate();
	}

	bool allVisible = true;
	for (int i = 0; i < 8; ++i)
	{
		Letter & l = letters[i];
		l.delay = max(0.0f, l.delay - _deltaTime);
		if (l.delay == 0.0f)
		{
			l.visible = blend_to_using_speed_based_on_time(l.visible, 1.0f, l.visibilityBlend, _deltaTime);
		}
		if (l.visible < 0.95f)
		{
			allVisible = false;
		}
		if (l.rotates)
		{
			if (l.rotationPT < 1.0f)
			{
				l.rotationPT = min(1.0f, l.rotationPT + _deltaTime / l.rotationLength);
				l.angle = l.rotationStartedAt + l.rotateBy * BlendCurve::cubic(l.rotationPT);
				if (l.rotationPT >= 1.0f)
				{
					if (l.rotationsLeft > 1)
					{
						l.timeToRotation = Random::get_float(0.0f, 0.2f);
					}
					else
					{
						l.timeToRotation = Random::get_float(0.1f, 1.2f);
					}
				}
			}
			else if (l.timeToRotation >= 0.0f)
			{
				l.timeToRotation -= _deltaTime;
				if (l.timeToRotation <= 0.0f)
				{
					l.rotationStartedAt = l.angle;
					l.rotationPT = 0.0f;
					--l.rotationsLeft;
					if (l.rotationsLeft <= 0)
					{
						l.rotateBy = Random::get_bool() ? 90.0f : -90.0f;
						l.rotationLength = Random::get_float(0.4f, 1.0f);
						if (Random::get_int(3) == 0)
						{
							l.rotationsLeft = Random::get_int_from_range(1, 4);
						}
						else
						{
							l.rotationsLeft = 1;
						}
					}
				}
			}
		}
	}

	if (allVisible)
	{
		logoShowDelay = max(0.0f, logoShowDelay - _deltaTime);
		if (logoShowDelay == 0.0f)
		{
			for_every(lm, logoMeshes)
			{
				lm->visible = blend_to_using_speed_based_on_time(lm->visible, 1.0f, logoVisibilityBlend, _deltaTime);
				if (lm->visible < 0.5f)
				{
					blockDeactivationFor = max(blockDeactivationFor, 1.0f);
					break; // wait with other logos to appear
				}
			}
		}

		for_every(lm, otherLogoMeshes)
		{
			lm->visible = blend_to_using_speed_based_on_time(lm->visible, 1.0f, logoVisibilityBlend, _deltaTime);
		}
	}
}

void VoidRoom::display(System::Video3D * _v3d, bool _vr)
{
	if (!VR::IVR::get() ||
		VR::IVR::get()->get_available_functionality().renderToScreen)
	{
		display_at(_v3d, false);
	}
	if (_vr)
	{
		display_at(_v3d, _vr);
	}
}

void VoidRoom::display_at(::System::Video3D * _v3d, bool _vr, Vector2 const & _at, float _scale)
{
	rt->bind();
	_v3d->set_default_viewport();
	_v3d->set_near_far_plane(0.02f, 100.0f);

	// ready sizes
	Vector2 size(12.0f, 12.0f);
	size.x = size.y * _v3d->get_aspect_ratio();
	Vector2 centre = size * 0.5f / _scale;
	Vector2 actualCentre = centre;

	float centreToLogoMeshDistance = letterSize * 7.0f;
	if (_vr)
	{
		centre = Vector2::zero;
	}
	else if (! logoMeshes.is_empty())
	{
		centre.x -= centreToLogoMeshDistance * 0.4f;
	}

	/*
	 *								__  vrLogoHalf + vrLogoAndFuncOffsetY
	 *	"void room"		(sq*4+sp*3)
	 *	empty			(sp)
	 *	letters			(sq*2+sp)   __ -vrLogoHalf + vrLogoAndFuncOffsetY
	 *	empty			(fmSpacing) __ vrLogoAndFuncOffsetY
	 *	function mesh	(fmHeight)
	 *
	 *	whole thing is sq*6 + sp*5
	 */
	// main top at vrLogoHalf
	// main bottom at -vrLogoHalf
	float vrLogoHalf = (letterSize * 6.0f + letterSpacing * 5.0f) * 0.5f;
	float fmHeight = functionMeshHeight * functionMeshPixelSize.y;
	float fmSpacing = functionMeshHeight > 0.0f? letterSize * 0.5f : 0.0f;
	float vrLogoAndFuncHeight = vrLogoHalf * 2.0f + (fmHeight + fmSpacing);
	float vrLogoAndFuncOffsetY = (fmHeight + fmSpacing) * 0.5f;
	float functionMeshOffsetY = -vrLogoHalf - fmSpacing + vrLogoAndFuncOffsetY; // top
	float vrLogoBottom = -vrLogoHalf; // requires applying vrLogoAndFuncOffsetY
	float lettersSize = letterSize * 2.0f + letterSpacing * 1.0f;
	float lettersY = vrLogoBottom + letterSize * 1.0f + letterSpacing * 0.5f; // requires applying vrLogoAndFuncOffsetY
	float lettersOffsetScale = letterSize + letterSpacing;
	float logosCentreVertically = logosHeight < vrLogoAndFuncHeight? 0.4f : 1.0f; // if doesn't fit, always centre
	float logosBottom = (1.0f - logosCentreVertically) * (-vrLogoAndFuncHeight * 0.5f) + logosCentreVertically * (-logosHeight * 0.5f);
	float otherLogosTop = vrLogoBottom - letterSize;

	Vector2 globalOffset = Vector2::zero;
	globalOffset.y = vrLogoHalf * 0.2f;
	if (_vr)
	{
		globalOffset.y = vrLogoHalf * 0.4f;
	}

	::System::VideoMatrixStack& matrixStack = _v3d->access_model_view_matrix_stack();

	::Framework::Render::Scene* scenes[2];

	Transform vrPlacement;

	Colour currentBackgroundColour = MainConfig::global().get_video().forcedBlackColour.get(Colour::lerp(BlendCurve::cubic(activation), (blockDeactivationFor > 0.0f ? 1.0f : targetActivation) > 0.0f? fadeFromColour : fadeToColour, backgroundColour));
	Colour mainColour = Colour::lerp(BlendCurve::cubic(activation), currentBackgroundColour, letterColour);

	if (_vr)
	{
		auto* vr = VR::IVR::get();
		Vector3 eyeLoc = Vector3(0.0f, 0.0f, 1.65f);
		Transform eyePlacement = look_at_matrix(eyeLoc, eyeLoc + Vector3(0.0f, 2.0f, 0.0f), Vector3::zAxis).to_transform();
		if (vr->is_render_view_valid())
		{
			eyePlacement = vr->get_render_view();
		}
		eyeLoc = eyePlacement.get_translation();

		update_vr_centre(eyeLoc);
		Vector3 vrLoc = vrCentre.get() + 5.0f * Vector3::yAxis + Vector3(0.0f, 0.0f, 1.3f);
		Vector3 vrAtLoc = vrLoc + Vector3(0.0f, 0.0f, 1.0f);
		vrPlacement = look_at_matrix(vrLoc, vrAtLoc, -Vector3::yAxis).to_transform();
		vrPlacement.set_scale(0.5f);

		::Framework::Render::Camera camera;
		camera.set_placement(nullptr, eyePlacement);
		camera.set_near_far_plane(0.02f, 30000.0f);
		camera.set_background_near_far_plane();

		scenes[0] = Framework::Render::Scene::build(camera, NP, nullptr, NP, Framework::Render::SceneMode::VR_Left);
		scenes[1] = Framework::Render::Scene::build(camera, NP, nullptr, NP, Framework::Render::SceneMode::VR_Right);

		scenes[0]->set_background_colour(currentBackgroundColour);
		scenes[1]->set_background_colour(currentBackgroundColour);
	}
	else
	{
		_v3d->setup_for_2d_display();
		_v3d->set_2d_projection_matrix_ortho(size);
		matrixStack.clear();
		matrixStack.push_to_world(translation_scale_xy_matrix(-_at * _scale, Vector2(_scale, _scale)));

		_v3d->clear_colour_depth_stencil(currentBackgroundColour);

		_v3d->set_fallback_colour(mainColour);
	}

	// display letters void room

	int dirCount = 5;
	float dirAngle = 360.0f / (float)dirCount;

	for (int lIdx = 0; lIdx < 8; ++lIdx)
	{
		Letter & l = letters[lIdx];

		Matrix44 letterPlacement = translation_matrix(Vector3(centre.x + lettersOffsetScale * l.offsetScale.x + globalOffset.x,
															  centre.y + lettersOffsetScale * l.offsetScale.y + globalOffset.y + lettersY + vrLogoAndFuncOffsetY, 0.0f));
		letterPlacement = letterPlacement.to_world(rotation_xy_matrix(l.angle));
		if (_vr)
		{
			Transform letterTransform = vrPlacement.to_world(letterPlacement.to_transform());
			l.letterInstance.set_fallback_colour(Colour::lerp(BlendCurve::cubic(activation * l.visible), currentBackgroundColour, letterColour));
			for_count(int, i, dirCount)
			{
				Transform rotate(Vector3::zero, Rotator3(0.0f, startingYaw + dirAngle * (float)i, 0.0f).to_quat());
				scenes[0]->add_extra(&l.letterInstance, rotate.to_world(letterTransform));
				scenes[1]->add_extra(&l.letterInstance, rotate.to_world(letterTransform));
			}
		}
		else
		{
			_v3d->set_fallback_colour(Colour::lerp(BlendCurve::cubic(activation * l.visible), currentBackgroundColour, letterColour));
			matrixStack.push_to_world(letterPlacement);
			if (l.letter)
			{
				l.letter->render(_v3d);
			}
			matrixStack.pop();
		}
	}

	// display void room logo and function mesh

	_v3d->set_fallback_colour(mainColour);

	Matrix44 voidRoomPlacement = translation_matrix(Vector3(centre.x + globalOffset.x,
															centre.y + globalOffset.y + vrLogoBottom + lettersSize + letterSpacing + vrLogoAndFuncOffsetY, 0.0f));
	Matrix44 functionMeshPlacement = translation_matrix(Vector3(centre.x + globalOffset.x,
																centre.y + globalOffset.y + functionMeshOffsetY, 0.0f));
	if (_vr)
	{
		Transform voidRoomTransform = vrPlacement.to_world(voidRoomPlacement.to_transform());
		voidRoomInstance.set_fallback_colour(mainColour);
		for_count(int, i, dirCount)
		{
			Transform rotate(Vector3::zero, Rotator3(0.0f, startingYaw + dirAngle * (float)i, 0.0f).to_quat());
			scenes[0]->add_extra(&voidRoomInstance, rotate.to_world(voidRoomTransform));
			scenes[1]->add_extra(&voidRoomInstance, rotate.to_world(voidRoomTransform));
		}

		if (functionMesh)
		{
			Transform functionMeshTransform = vrPlacement.to_world(functionMeshPlacement.to_transform());
			functionMeshInstance.set_fallback_colour(Colour::lerp(BlendCurve::cubic(activation) * 0.5f, currentBackgroundColour, letterColour));
			for_count(int, i, dirCount)
			{
				Transform rotate(Vector3::zero, Rotator3(0.0f, startingYaw + dirAngle * (float)i, 0.0f).to_quat());
				scenes[0]->add_extra(&functionMeshInstance, rotate.to_world(functionMeshTransform));
				scenes[1]->add_extra(&functionMeshInstance, rotate.to_world(functionMeshTransform));
			}
		}
	}
	else
	{
		matrixStack.push_to_world(voidRoomPlacement);
		voidRoom->render(_v3d);
		matrixStack.pop();

		if (functionMesh)
		{
			_v3d->set_fallback_colour(Colour::lerp(BlendCurve::cubic(activation) * 0.5f, currentBackgroundColour, letterColour));

			matrixStack.push_to_world(functionMeshPlacement);
			functionMesh->render(_v3d);
			matrixStack.pop();
		}

	}

	// display logos

	_v3d->set_fallback_colour(Colour::white); // full colour

	Colour useBlack = MainConfig::global().get_video().forcedBlackColour.get(Colour::black);
	Colour overrideColour = useBlack.with_alpha(BlendCurve::cubic(1.0f - activation));

	if (MainConfig::global().get_video().forcedBlackColour.is_set())
	{
		overrideColour = activation > 0.5f ? useBlack.with_alpha(0.0f) : useBlack;
	}

	for (int lIdx = 0; lIdx < logoMeshes.get_size(); ++lIdx)
	{
		LogoMesh& l = logoMeshes[lIdx];

		float useVisible = BlendCurve::cubic(l.visible);
		if (MainConfig::global().get_video().forcedBlackColour.is_set())
		{
			useVisible = round(useVisible);
		}

		if (_vr)
		{
			Matrix44 logoPlacement = translation_matrix(Vector3(0.0f + globalOffset.x, logosBottom + globalOffset.y, 0.0f));

			Transform logoTransform = vrPlacement.to_world(logoPlacement.to_transform());
			for_count(int, i, dirCount)
			{
				Transform rotate(Vector3::zero, Rotator3(0.0f, startingYaw + dirAngle * 0.5f + dirAngle * (float)i, 0.0f).to_quat());
				l.meshInstance.set_fallback_colour(Colour::white.with_alpha(l.visible));
				l.meshInstance.access_material_instance(0)->set_uniform(NAME(logoAlpha), useVisible);
				l.meshInstance.access_material_instance(0)->set_uniform(NAME(overrideColour), overrideColour.to_vector4());
				scenes[0]->add_extra(&l.meshInstance, rotate.to_world(logoTransform));
				scenes[1]->add_extra(&l.meshInstance, rotate.to_world(logoTransform));
			}
		}
		else
		{
			Matrix44 logoPlacement = translation_matrix(Vector3(centre.x + globalOffset.x + centreToLogoMeshDistance, centre.y + globalOffset.y + logosBottom, 0.0f));

			::Meshes::Mesh3DRenderingContext renderingContext;

			matrixStack.push_to_world(logoPlacement);
			l.meshInstance.set_fallback_colour(Colour::white.with_alpha(l.visible));
			l.meshInstance.access_material_instance(0)->set_uniform(NAME(logoAlpha), useVisible);
			l.meshInstance.access_material_instance(0)->set_uniform(NAME(overrideColour), overrideColour.to_vector4());
			l.meshInstance.render(_v3d, renderingContext);
			matrixStack.pop();
		}
	}
	
	for (int lIdx = 0; lIdx < otherLogoMeshes.get_size(); ++lIdx)
	{
		LogoMesh& l = otherLogoMeshes[lIdx];

		float useVisible = BlendCurve::cubic(l.visible);
		if (MainConfig::global().get_video().forcedBlackColour.is_set())
		{
			useVisible = round(useVisible);
		}

		if (_vr)
		{
			Matrix44 logoPlacement = translation_matrix(Vector3(0.0f + globalOffset.x, otherLogosTop + globalOffset.y, 0.0f));

			Transform logoTransform = vrPlacement.to_world(logoPlacement.to_transform());
			for_count(int, i, dirCount)
			{
				Transform rotate(Vector3::zero, Rotator3(0.0f, startingYaw + dirAngle * (float)i, 0.0f).to_quat());
				l.meshInstance.set_fallback_colour(Colour::white.with_alpha(l.visible));
				l.meshInstance.access_material_instance(0)->set_uniform(NAME(logoAlpha), useVisible);
				l.meshInstance.access_material_instance(0)->set_uniform(NAME(overrideColour), overrideColour.to_vector4());
				scenes[0]->add_extra(&l.meshInstance, rotate.to_world(logoTransform));
				scenes[1]->add_extra(&l.meshInstance, rotate.to_world(logoTransform));
			}
		}
		else
		{
			Matrix44 logoPlacement = translation_matrix(Vector3(actualCentre.x + globalOffset.x, centre.y + globalOffset.y + otherLogosTop, 0.0f));

			::Meshes::Mesh3DRenderingContext renderingContext;

			matrixStack.push_to_world(logoPlacement);
			l.meshInstance.set_fallback_colour(Colour::white.with_alpha(l.visible));
			l.meshInstance.access_material_instance(0)->set_uniform(NAME(logoAlpha), useVisible);
			l.meshInstance.access_material_instance(0)->set_uniform(NAME(overrideColour), overrideColour.to_vector4());
			l.meshInstance.render(_v3d, renderingContext);
			matrixStack.pop();
		}
	}

	if (_vr)
	{
		scenes[0]->prepare_extras();
		scenes[1]->prepare_extras();

		Framework::Render::Context rc(_v3d);

		todo_note(TXT("store default texture somewhere"));
		//rc.set_default_texture_id(Framework::Library::get_current()->get_default_texture()->get_texture()->get_texture_id());

		_v3d->set_fallback_colour(Colour::lerp(BlendCurve::cubic(activation), currentBackgroundColour, letterColour));

		for_count(int, i, 2)
		{
			scenes[i]->render(rc, rt.get());
			scenes[i]->release();
		}

		VR::IVR::get()->copy_render_to_output(_v3d);
	}
	else
	{
		matrixStack.pop();

		_v3d->set_fallback_colour(Colour::white);

		System::RenderTarget::bind_none();

		rt->resolve();

		matrixStack.clear();
		matrixStack.force_requires_send_all();

		render_rt(_v3d);
	}
}

