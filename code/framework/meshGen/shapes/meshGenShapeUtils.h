#pragma once

#include <functional>

#include "..\meshGenUVDef.h"
#include "..\meshGenValueDef.h"

#include "..\..\..\core\globalDefinitions.h"
#include "..\..\..\core\containers\array.h"
#include "..\..\..\core\math\math.h"
#include "..\..\..\core\types\name.h"
#include "..\..\..\core\types\optional.h"

//

namespace Meshes
{
	namespace Builders
	{
		class IPU;
	};
	struct VertexSkinningInfo;
};

namespace Framework
{
	namespace MeshGeneration
	{
		struct CreateCollisionInfo;
		struct GenerationContext;
		struct ElementInstance;
		struct SubStepDef;
		
		namespace ShapeUtils
		{
			enum Corner
			{
				LB, RB, LT, RT
			};
			struct CrossSection
			{
				Name id;
				Array<Vector2> points;
				Array<UVDef> us;
			};

			struct PointOnCurve
			{
				Optional<float> pt;
				Optional<float> dist;

				float get_pos() const;
				float get_pos_not_clamped() const;
				void set_pos(float _pos); // no const as we really mean to set pos here!
				bool calculate_pos(Range const & _ptRange, float _curveLength) const; // returns false if clamped outside

				void reverse_single(float _curveLength);
				bool load_single_from_xml(IO::XML::Node const * _node);

			protected:
				mutable CACHED_ float posNotClamped = 0.0f; // calculated position
				mutable CACHED_ float pos = 0.0f; // calculated position (clamped)
#ifdef AN_ASSERT
				mutable bool posCalculated = false;
#endif
			};

			struct CrossSectionOnCurve
			: public PointOnCurve
			{
				Name id; // cross section id
				ArrayStatic<Name, 8> altId; // overrides id, the first found overrides
				Optional<Name> uId; // cross section id used for u
				// scale and scaleCurve accumulate (!)
				ValueDef<float> scaleFloat;
				ValueDef<Vector2> scale;
				Optional<BezierCurve<BezierCurveTBasedPoint<float>>> scaleFloatCurve; // prev to this one
				Optional<BezierCurve<BezierCurveTBasedPoint<Vector2>>> scaleCurve; // prev to this one
				Optional<BezierCurve<BezierCurveTBasedPoint<float>>> morphCurve; // prev to this one
				ValueDef<float> morphCurveStepLength;

				// these are when we fill
				CACHED_ Name prevId; // to be used
				CACHED_ Optional<Vector2> finalScale; // scale calculated, to be used
				CACHED_ Optional<float> finalMorph; // morph calculated, to be used

				CrossSectionOnCurve()
				{
					SET_EXTRA_DEBUG_INFO(altId, TXT("CrossSectionOnCurve.altId"));
				}

				void reverse_single(float _curveLength);
				bool load_single_from_xml(IO::XML::Node const * _node);

				static void insert(ShapeUtils::CrossSectionOnCurve const & _cs, Array<ShapeUtils::CrossSectionOnCurve> & _into);
				static void reverse(Array<CrossSectionOnCurve> & _csOnCurve, float _curveLength);
				static bool load_from_xml(IO::XML::Node const * _node, Array<CrossSectionOnCurve> & _csOnCurve, tchar const * const _childName);

				Vector2 calculate_final_scale(GenerationContext const & _context, CrossSectionOnCurve const * _prev, float _t, bool _tIsOuter = false) const;
				float calculate_final_morph(GenerationContext const & _context, CrossSectionOnCurve const * _prev, float _t, bool _tIsOuter = false) const;
				float transform_t(float _t) const;
			};

			struct CustomOnCurve
			: public PointOnCurve
			{
				Name createMeshNode;

				static void insert(ShapeUtils::CustomOnCurve const & _cs, Array<ShapeUtils::CustomOnCurve> & _into);
				static void reverse(Array<CustomOnCurve> & _csOnCurve, float _curveLength);
			};

			typedef std::function< Vector3(BezierCurve<Vector3> const & _curve, float _atT, Vector3 const & _tangent) > CalculateNormal;
			typedef std::function< Meshes::VertexSkinningInfo(BezierCurve<Vector3> const & _curve, float _atT) > CalculateBones;

			CrossSection const & get_cross_section(Array<CrossSection> const & _cs, Name const & _id);
			Name const & get_alt_cross_section_id(Array<CrossSection> const & _cs, CrossSectionOnCurve const & _csOnCurve);
			Name const & get_alt_cross_section_id(Array<CrossSection> const & _cs, Name const& _id, ArrayStatic<Name, 8> const & _altIds);
			void morph_cross_section(Array<CrossSection> const & _cs, OUT_ CrossSection & _csOut, Name const & _prevId, Name const & _id, float _morph);

			// uses curve to calculate location, tangent, normal and right and use it to add cross section
			// this will end with cross section aligning always with tangent and may result in sharp turns
			void build_cross_section_mesh(::Meshes::Builders::IPU & _ipu, GenerationContext & _context, ElementInstance & _elementInstance, CreateCollisionInfo const & _createMovementCollisionBoxes, CreateCollisionInfo const & _createPreciseCollisionBoxes, bool _debugDraw, Array<CrossSection> const & _cs, Vector2 const & _scaleCS, Vector2 const & _offsetCS,
				BezierCurve<Vector3> const & _onCurve, float _startAtT, float _endAtT, bool _forward, SubStepDef const & _curveSubStep, float _curveDetailsCoef,
				Array<CrossSectionOnCurve> const * _csOnCurve, OUT_ Array<CrossSectionOnCurve>* _usedCSOnCurve,
				Array<CustomOnCurve> const* _customOnCurve,
				CalculateNormal _calculate_normal, CalculateBones _calculate_bones);

			int add_single_cross_section_points(::Meshes::Builders::IPU & _ipu, GenerationContext const & _context, bool _debugDraw, CrossSection const & _cs, Vector2 const & _scaleCS, Vector2 const & _offsetCS,
				BezierCurve<Vector3> const & _onCurve, float _atT, bool _forward,
				CalculateNormal _calculate_normal, CalculateBones _calculate_bones,
				OPTIONAL_ Matrix44* _transform = nullptr);

			// uses four curves on window range (max of cross section) and stretches cross section on them at each "t"
			// better on narrower corners (corners: LB, RB, LT, RT)
			void build_cross_section_mesh_using_window_corners(::Meshes::Builders::IPU & _ipu, GenerationContext & _context, ElementInstance & _elementInstance, CreateCollisionInfo const & _createMovementCollisionBoxes, CreateCollisionInfo const & _createPreciseCollisionBoxes, bool _debugDraw, Array<CrossSection> const & _cs, Vector2 const & _scaleCS, Vector2 const & _offsetCS,
				Range2 const & _curveWindow, BezierCurve<Vector3> const * _onCurveWindowCorners,
				BezierCurve<Vector3> const & _onCurve, float _startAtT, float _endAtT, SubStepDef const & _curveSubStep, float _curveDetailsCoef, Array<CrossSectionOnCurve> const * _csOnCurve,
				CalculateBones _calculate_bones);

			// (corners: LB, RB, LT, RT)
			int add_single_cross_section_points_using_window_corners(::Meshes::Builders::IPU & _ipu, GenerationContext const & _context, bool _debugDraw, CrossSection const & _cs, Vector2 const & _scaleCS, Vector2 const & _offsetCS,
				Range2 const & _curveWindow, Vector3 const * _corners,
				Meshes::VertexSkinningInfo const * _skinningInfo,
				OPTIONAL_ Matrix44* _transform);

			int add_single_cross_section_points(::Meshes::Builders::IPU & _ipu, GenerationContext const & _context, bool _debugDraw, CrossSection const & _cs, Vector2 const & _scaleCS, Vector2 const & _offsetCS,
				Vector3 const & _location, Vector3 const & _tangent, Vector3 const & _normal, Vector3 const & _right,
				Meshes::VertexSkinningInfo const * _skinningInfo,
				OPTIONAL_ Matrix44* _transform = nullptr);

			void calculate_axes_for(BezierCurve<Vector3> const & _onCurve, float _atT, bool _forward,
				OUT_ Vector3 & _location, OUT_ Vector3 & _tangent, OUT_ Vector3 & _normal, OUT_ Vector3 & _right,
				CalculateNormal _calculate_normal);

			void calculate_matrix_for(BezierCurve<Vector3> const & _onCurve, float _atT, bool _forward, OUT_ Matrix44 & _matrix,
				CalculateNormal _calculate_normal);

			void calculate_transform_for(BezierCurve<Vector3> const & _onCurve, float _atT, bool _forward, OUT_ Transform & _transform,
				CalculateNormal _calculate_normal);
	
			// util
			void setup_cs_on_curve(OUT_ Array<CrossSectionOnCurve> & csOnCurve, GenerationContext const & _context, Array<ShapeUtils::CrossSection> const& _cs,
				BezierCurve<Vector3> const & _onCurve, float _startAtT, float _endAtT, SubStepDef const & _curveSubStep, float _curveDetailsCoef, Array<CrossSectionOnCurve> const * _csOnCurve);
		};
	};
};