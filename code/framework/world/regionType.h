#pragma once

#include "doorHole.h"
#include "doorVRCorridor.h"
#include "worldSetting.h"
#include "worldSettingCondition.h"

#include "..\library\libraryStored.h"
#include "..\library\libraryStoredReplacer.h"

#include "..\..\core\wheresMyPoint\iWMPTool.h"

#include "regionGenerator.h"
#include "roomGeneratorInfo.h"

namespace Framework
{
	class EnvironmentType;
	class Reverb;
	class RegionGeneratorInfo;

	/*
	 *	Region type
	 *
	 *	Region types compose regions - (usually) separated parts of the world
	 *	Although region types can be nested (region type composed of different regions and rooms as connectors),
	 *	they create through pieceCombiner region
	 * 
	 *	Use canBeCreatedOnlyViaConnectorWithCreateNewPieceMark with createNewPieceMark and regionPieceInstances to create inner flow
	 */
	class RegionType
	: public LibraryStored
	{
		FAST_CAST_DECLARE(RegionType);
		FAST_CAST_BASE(LibraryStored);
		FAST_CAST_END();
		LIBRARY_STORED_DECLARE_TYPE();

		typedef LibraryStored base;
	public:
		struct Environments
		{
			struct Create
			{
				Name name;
				Name parentName;
				UsedLibraryStored<EnvironmentType> environmentType;
			};
			Array<Create> create;
			Name defaultEnvironmentName;

			bool load_from_xml(IO::XML::Node const * _node, Framework::LibraryLoadingContext & _lc);
			bool prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext);
		};

		template <typename Class>
		struct Included
		{
		public:
			Class const * get_included() const { return included.get(); }
			Random::Number<int> const * get_limit_count() const { return useLimitCount ? &limitCount : nullptr; }
			Name get_can_be_created_only_via_connector_with_new_piece_mark() const { return canBeCreatedOnlyViaConnectorWithCreateNewPieceMark; }
			Optional<float> const& get_probability_coef() const { return probabilityCoef; }
			Optional<float> const& get_mul_probability_coef() const { return mulProbabilityCoef; }

			void set_included(Class const * _included) { included = _included; }
			void set_limit_count(Random::Number<int> const& _limitCount) { limitCount = _limitCount; useLimitCount = true; }

		private:
			UsedLibraryStored<Class> included;
			Name canBeCreatedOnlyViaConnectorWithCreateNewPieceMark;
			Random::Number<int> limitCount;
			bool useLimitCount = false;
			Optional<float> probabilityCoef; // overrides
			Optional<float> mulProbabilityCoef;

			friend class RegionType;

			bool load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc, tchar const * _name);
			bool prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext);
		};

		struct IncludedWorldSettingCondition
		{
		public:
			WorldSettingCondition const * get_setting_condition() const { return worldSettingCondition.get(); }
			Name get_can_be_created_only_via_connector_with_new_piece_mark() const { return canBeCreatedOnlyViaConnectorWithCreateNewPieceMark; }
			Optional<float> const& get_probability_coef() const { return probabilityCoef; }
			Optional<float> const& get_mul_probability_coef() const { return mulProbabilityCoef; }

		private:
			RefCountObjectPtr<WorldSettingCondition> worldSettingCondition;
			Name canBeCreatedOnlyViaConnectorWithCreateNewPieceMark;
			Optional<float> probabilityCoef; // overrides
			Optional<float> mulProbabilityCoef;

			friend class RegionType;

			bool load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc);
		};

		RegionType(Library * _library, LibraryName const & _name);

		RegionGeneratorInfo const * get_region_generator_info() const { return regionGeneratorInfo.get(); }

		SimpleVariableStorage const & get_required_variables() const { return requiresVariables; }

		override_ bool load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc);
		override_ bool prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext);
		override_ void prepare_to_unload();

		WheresMyPoint::ToolSet const & get_wheres_my_point_processor_setup_parameters() const { return wheresMyPointProcessorSetupParameters; }
		WheresMyPoint::ToolSet const & get_wheres_my_point_processor_on_setup_generator() const { return wheresMyPointProcessorOnSetupGenerator; }
		WheresMyPoint::ToolSet const & get_wheres_my_point_processor_on_generate() const { return wheresMyPointProcessorOnGenerate; }

		WorldSetting const * get_setting() const { return setting.get(); }

		void setup_region_generator_add_pieces(REF_ PieceCombiner::Generator<Framework::RegionGenerator> & _generator, Region * _region, Random::Generator & _randomGenerator) const;

		Array<Included<RoomType> > const & get_included_room_types() const { return includedRoomTypes.included; }
		Array<Included<RegionType> > const & get_included_region_types() const { return includedRegionTypes.included; }
		Array<IncludedWorldSettingCondition> const & get_included_room_type_setting_conditions() const { return includedRoomTypes.includedSettingConditions; }
		Array<IncludedWorldSettingCondition> const & get_included_region_type_setting_conditions() const { return includedRegionTypes.includedSettingConditions; }

		PieceCombiner::GenerationRules<RegionGenerator> const * get_generation_rules() const { return &generationRules; }
		Array<RefCountObjectPtr<PieceCombiner::Piece<RegionGenerator>>> const & get_inside_region_pieces() const { return insideRegionPieces; }
		Array<RefCountObjectPtr<PieceCombiner::Connector<RegionGenerator>>> const & get_inside_outer_region_connectors() const { return insideOuterRegionConnectors; }
		Array<RefCountObjectPtr<PieceCombiner::Connector<RegionGenerator>>> const & get_inside_slot_connectors() const { return insideSlotConnectors; }
		PieceCombiner::Connector<RegionGenerator> const * find_inside_outer_region_connector(Name const & _outerName) const;
		PieceCombiner::Connector<RegionGenerator> const * find_inside_slot_connector(Name const & _slotName) const;

		PieceCombiner::Piece<RegionGenerator>* get_as_region_piece() { return regionPiece.get(); }
		PieceCombiner::Piece<RegionGenerator> const * get_as_region_piece() const { return regionPiece.get(); }

		Environments const & get_environments() const { return environments; }

		Reverb* get_reverb() const { return useReverb.get(); }

		LibraryStoredReplacer const & get_library_stored_replacer() const { return libraryStoredReplacer; }

		RoomType const * get_door_vr_corridor_room_room_type(Random::Generator& _rg) const { return doorVRCorridor.get_room(_rg); }
		RoomType const * get_door_vr_corridor_elevator_room_type(Random::Generator& _rg) const { return doorVRCorridor.get_elevator(_rg); }
		bool get_door_vr_corridor_priority(Random::Generator& _rg, OUT_ float & _result) const { return doorVRCorridor.get_priority(_rg, OUT_ _result); }
		bool get_door_vr_corridor_priority_range(OUT_ Range& _result) const { return doorVRCorridor.get_priority_range(OUT_ _result); }

	protected:
		~RegionType();

	private:
		struct Set
		{
			float chance = 1.0f;
			RangeInt count = RangeInt::empty; // include count of these below, or all if 0

			TagCondition generationTagsCondition; // against GenerationContext::generationTags
			
			SimpleVariableStorage setVariables; // sets variables if chosen

			template <typename Class>
			struct IncludeInstance
			{
				UsedLibraryStored<Class> include;
				float chance = 1.0f; // for count!
				TagCondition generationTagsCondition;
				bool mayFail = false;
			};
			Array<IncludeInstance<Framework::RegionType>> includeInstanceRegionTypes;
			Array<IncludeInstance<Framework::RoomType>> includeInstanceRoomTypes;
			Array<RefCountObjectPtr<PieceCombiner::Piece<RegionGenerator>>> insideRegionPieceInstances; // these pieces are created in region generator - can be used to provide extra connectors, to set shape to generation, to limit generated pieces etc

			bool load_from_xml(IO::XML::Node const * _node, Framework::LibraryLoadingContext & _lc, RegionGenerator::LoadingContext * _regionLoadingContext);
			bool prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext, RegionGenerator::PreparingContext & _preparingContext);
		};

		template <typename Class>
		struct Include
		{
			Name typeName;
			Array<Included<Class>> included;
			Array<IncludedWorldSettingCondition> includedSettingConditions; // shared between template and created from template
			
			Include(Name const & _typeName);
			~Include();
			
			bool load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc, tchar const * _nameIncludePlainTypeNode, tchar const * _nameIncludePlainTypeAttr, tchar const * _nameIncludeSettingConditionNode);
			bool prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext);
			
			bool is_empty() const { return included.is_empty() && includedSettingConditions.is_empty(); }
		};
		
		SimpleVariableStorage requiresVariables;

		WheresMyPoint::ToolSet wheresMyPointProcessorSetupParameters; // to allow setup of parameters randomly (!)

		// to allow setting variables in generator to use them when we setup generator and generate
		// this is called BEFORE setup_generator, allowing even generate_piece_for_region_generator to use values that result here
		WheresMyPoint::ToolSet wheresMyPointProcessorOnSetupGenerator; // to allow setup of parameters randomly (!)

		// to allow modification of parameters when generating region
		// this is called just before the actual generation of the region (post setup_generator)
		WheresMyPoint::ToolSet wheresMyPointProcessorOnGenerate;

		WorldSettingPtr setting;
		CopyWorldSettings copySettings;

		LibraryStoredReplacer libraryStoredReplacer;

		RefCountObjectPtr<RegionGeneratorInfo> regionGeneratorInfo;

		Environments environments;
		PieceCombiner::GenerationRules<RegionGenerator> generationRules;
		Array<RefCountObjectPtr<PieceCombiner::Piece<RegionGenerator>>> insideRegionPieces;
		// insideOuterRegionConnectors - how outside connectors (as region piece, connectors of that region piece) are understood as inside connectors (external for region generator,
		// region pieces inside), this just translates from outside to inside, there can be more defined than there really are
		// this is to connect already existing connector on the outside (nested generation)
		Array<RefCountObjectPtr<PieceCombiner::Connector<RegionGenerator>>> insideOuterRegionConnectors;
		// slot connectors that can be added as outer connectors to generation - mockup connectors for doors
		// this should be used at the top level of generation to get exits, to connect a region with something outside the scope of generation (start/exit, other regions in open world)
		Array<RefCountObjectPtr<PieceCombiner::Connector<RegionGenerator>>> insideSlotConnectors;
		Include<RoomType> includedRoomTypes; // included room types
		Include<RegionType> includedRegionTypes; // included region types
		List<Set> obligatorySets; // sets that are always included
		List<Set> sets;

		UsedLibraryStored<Reverb> useReverb;

		DoorVRCorridor doorVRCorridor;

		RefCountObjectPtr<PieceCombiner::Piece<RegionGenerator>> regionPiece; // it can be piece for generator too - not only as thing providing data for generation

		bool load_region_generation_from_xml(IO::XML::Node const * _node, Framework::LibraryLoadingContext & _lc, RegionGenerator::LoadingContext * _regionLoadingContext);

		Set const * choose_set(Random::Generator & _generator, Tags const & _generationTags) const;
		static void add_set(RegionType::Set const * regionTypeSet, REF_ PieceCombiner::Generator<Framework::RegionGenerator> & _generator, Region * _region, REF_ Random::Generator & _randomGenerator);
	};
};
