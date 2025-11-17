#pragma once

#include "..\..\core\memory\softPooledObject.h"
#include "..\..\core\types\name.h"
#include "..\..\core\math\math.h"
#include "..\..\core\casts.h"
#include "..\interfaces\aiObject.h"
#include "aiParam.h"

namespace Framework
{
	class Room;
	class World;
	class SubWorld;
	interface_class IAIObject;

	namespace AI
	{
		struct HunchQueue;
		class Hunch;
		
		/**
		 *	Right now hunches are per AI
		 *	If there should be a need to issue a hunch to multiple AIs, multiple copies are required.
		 */
		class Hunch
		: public SoftPooledObject<Hunch>
		, public RefCountObject
		{
			typedef SoftPooledObject<Hunch> base;
		public:
			typedef RefCountObjectPtr<Hunch> Ptr;

		public:
			Hunch();
			virtual ~Hunch() {}

			bool is_valid() const { return ! expired; }
		
			bool is_consumed() const { return consumed; }
			void consume() { consumed = true; }

			Name const & get_name() const { return name; }
			Name const & get_reason() const { return reason; }
			Param & access_param(Name const & _name); // creates if doesn't exist
			Param const * get_param(Name const & _name) const;
			
			void name_hunch(Name const & _name) { name = _name; }
			void set_reason(Name const & _reason) { reason = _reason; }

			void set_issue_time(float _worldTime) { issueWorldTime = _worldTime; }
			void set_expire_by(float _worldTime) { expireBy = _worldTime; }

			void update_world_time(float _worldTime);

		protected: friend class SoftPooledObject<Hunch>;
			override_ void on_get();
			override_ void on_release();

		protected: friend class RefCountObject;
			override_ void destroy_ref_count_object() { SoftPooledObject<Hunch>::release(); }

		private:
			Name name;
			Name reason;
			float issueWorldTime; // when it was created
			Optional<float> expireBy;
			bool expired = false;
			bool consumed = false;

			Params params;

			void distribute_to_process() const;

			friend struct HunchQueue;
		};
	};
};

TYPE_AS_CHAR(Framework::AI::Hunch);