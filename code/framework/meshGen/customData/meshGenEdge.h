#pragma once

#include "meshGenSplineRef.h"
#include "..\meshGenGenerationCondition.h"
#include "..\meshGenSubStepDef.h"
#include "..\shapes\meshGenShapeUtils.h"

#include "..\..\library\customLibraryDatas.h"

#include "..\..\..\core\math\math.h"

namespace Framework
{
	class MeshGenerator;

	namespace MeshGeneration
	{
		class Element;

		namespace CustomData
		{
			template <typename Point> class Spline;

			namespace EdgeNormalAlignment
			{
				enum Type
				{
					AlignWithNormals, // up will be calculated by average of both normals
					AlignWithLeftNormal, // up is taken from normal of surface on left
					AlignWithRightNormal, // up is taken from normal of surface on right
					AlongLeftSurface, // along left surface - up is along surface on left
					AlongRightSurface, // along right surface - up is along surface on right
					AlignWithLeftSurfaceEdge, // opposite of along left surface - up is facing away from surface on left
					AlighWithRightSurfaceEdge, // opposite of along right surface - up is facing away from surface on right
					TowardsRefPoint, // normal will be pointing at ref point
					AwayFromRefPoint, // normal will be pointing away from ref point
					RefDir, // in ref dir (ref point)
				};

				EdgeNormalAlignment::Type parse(String const & _text);
			};

			struct EdgeCap
			{
			public:
				~EdgeCap();

				bool load_from_xml(IO::XML::Node const * _node, tchar const * _childName, LibraryLoadingContext & _lc);
				bool prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext);
#ifdef AN_DEVELOPMENT
				void for_included_mesh_generators(std::function<void(MeshGenerator*)> _do) const;
#endif

				bool is_valid() const { return element.is_set(); }

				Element const * get_element() const { return element.get(); }
				ValueDef<float> const & get_shorten_edge() const { return shortenEdge; }
				bool should_move_by_shorten_edge() const { return moveByShortenEdge; }

			private:
				GenerationCondition generationCondition;
				RefCountObjectPtr<Element> element;

				ValueDef<float> shortenEdge = ValueDef<float>(0.0f); // this is to shorten edge on which cap is placed - if it covers whole part of edge, why not shorten it?
				bool moveByShortenEdge = false; // to move cap by shortened distance
			};

			/**
			 *	Edge that contains cross section or refers to it
			 */
			class Edge
			: public CustomLibraryData
			{
				FAST_CAST_DECLARE(Edge);
				FAST_CAST_BASE(CustomLibraryData);
				FAST_CAST_END();
				typedef CustomLibraryData base;
			public:
				struct CrossSection
				{
					Name id;
					SplineRef<Vector2> spline;
				};
				struct AutoCrossSectionOnCurve
				{
					Name id;
					bool atEdgesCentreOnly = false; // will add one random sequence at the centre only
					bool addOnePerEdge = false;
					bool notAtCorners = false;
					Optional<float> minLength;
					Optional<Range> spacing; // will be randomised for every call, distance between ends of csoc sequences
					Optional<Range> border; // does not include csoc sequences
					Optional<Range> cornerBorder; // like border but at every (!) corner
					struct Sequence
					{
						float probCoef = 1.0f;
						Array<ShapeUtils::CrossSectionOnCurve> csocs;
						Name createMeshNode;
						bool enforceAtEnds = false;
					};
					List<Sequence> sequences;

					bool load_from_xml(IO::XML::Node const * _node);
				};

				~Edge();

				SubStepDef const & get_edge_sub_step() const { return edgeSubStep; }
				EdgeNormalAlignment::Type const & get_normal_alignment() const { return normalAlignment; }
				float get_corner_radius() const { return cornerRadius; }
				bool get_cut_corner() const { return cutCorner; }

				Array<CrossSection> const & get_all_cross_sections() const { return crossSections; }

				SplineRef<Vector2> const & get_cross_section_spline(Name const & _id = Name::invalid()) const; // returns default if not found
				SubStepDef const & get_cross_section_sub_step() const { return crossSectionSubStep; }

				AutoCrossSectionOnCurve const * get_auto_csoc(Name const & _id) const;

				EdgeCap const * get_cap_start() const { return capStart.is_valid() ? &capStart : (cap.is_valid() ? &cap : nullptr); }
				EdgeCap const * get_cap_end() const { return capEnd.is_valid() ? &capEnd : (cap.is_valid() ? &cap : nullptr); }

#ifdef AN_DEVELOPMENT
				void for_included_mesh_generators(std::function<void(MeshGenerator*)> _do) const;
#endif

			public: // CustomLibraryData
#ifdef AN_DEVELOPMENT
				override_ void ready_for_reload();
#endif
				override_ bool load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc);
				override_ bool prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext);

			private:
				SubStepDef edgeSubStep;
				EdgeNormalAlignment::Type normalAlignment = EdgeNormalAlignment::AlignWithNormals;
				float cornerRadius = 0.0f;
				bool cutCorner = false;

				Array<CrossSection> crossSections;
				SubStepDef crossSectionSubStep;

				List<AutoCrossSectionOnCurve> autoCSOCs;

				EdgeCap cap; // used for both start and end, works as fallback if others are not set/valid
				EdgeCap capStart;
				EdgeCap capEnd;

				CrossSection & access_cross_section(Name const & _id);
				AutoCrossSectionOnCurve & access_auto_csoc(Name const & _id);
			};
		};
	};
};
