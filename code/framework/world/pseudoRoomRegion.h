#pragma once

#include "region.h"

#include "..\..\core\other\simpleVariableStorage.h"
#include "..\..\core\wheresMyPoint\iWMPOwner.h"

namespace Framework
{
	class Region;
	class RegionType;
	class Room;
	class RoomType;

	/**
	 *	This is a class that might be used to process setup_parameters wmp processor to mimic values that would be done during the actual generation
	 */
	class PseudoRoomRegion
	: public WheresMyPoint::IOwner
	{
		FAST_CAST_DECLARE(PseudoRoomRegion);
		FAST_CAST_BASE(WheresMyPoint::IOwner);
		FAST_CAST_END();
	public:
		PseudoRoomRegion(Region* _inRegion, RegionType const * _regionType);
		PseudoRoomRegion(Region* _inRegion, RoomType const * _roomType);
		virtual ~PseudoRoomRegion();

		Region* get_in_region() const { return inRegion; }

		SimpleVariableStorage& access_variables() { return variables; }
		SimpleVariableStorage const & get_variables() const { return variables; }
		void collect_variables(REF_ SimpleVariableStorage& _variables) const;

		Random::Generator const& get_individual_random_generator() const { return individualRandomGenerator; }
		void set_individual_random_generator(Random::Generator const& _individualRandomGenerator);

		void run_wheres_my_point_processor_setup_parameters();

	public: // WheresMyPoint::IOwner
		override_ bool store_value_for_wheres_my_point(Name const & _byName, WheresMyPoint::Value const & _value);
		override_ bool restore_value_for_wheres_my_point(Name const & _byName, OUT_ WheresMyPoint::Value & _value) const;
		override_ bool store_convert_value_for_wheres_my_point(Name const& _byName, TypeId _to);
		override_ bool rename_value_forwheres_my_point(Name const& _from, Name const& _to);
		override_ bool store_global_value_for_wheres_my_point(Name const & _byName, WheresMyPoint::Value const & _value);
		override_ bool restore_global_value_for_wheres_my_point(Name const & _byName, OUT_ WheresMyPoint::Value & _value) const;
		override_ bool get_point_for_wheres_my_point(Name const & _byName, OUT_ Vector3 & _point) const; // shouldn't be called!

		override_ WheresMyPoint::IOwner* get_wmp_owner() { return inRegion; }

	private:
		Region* inRegion = nullptr;
		RegionType const * regionType = nullptr;
		RoomType const * roomType = nullptr;

		Random::Generator individualRandomGenerator;
		SimpleVariableStorage variables;

	};
};
