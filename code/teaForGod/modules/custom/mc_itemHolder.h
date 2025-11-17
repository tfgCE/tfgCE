#pragma once

#include "..\..\..\framework\module\moduleCustom.h"
#include "..\..\..\framework\module\moduleData.h"
#include "..\..\..\core\concurrency\scopedSpinLock.h"
#include "..\..\..\core\types\colour.h"

namespace TeaForGodEmperor
{
	namespace CustomModules
	{
		/**
		 *	Allows holding an item
		 */
		class ItemHolder
		: public Framework::ModuleCustom
		{
			FAST_CAST_DECLARE(ItemHolder);
			FAST_CAST_BASE(Framework::ModuleCustom);
			FAST_CAST_END();

			typedef Framework::ModuleCustom base;
		public:
			static Framework::RegisteredModule<Framework::ModuleCustom> & register_itself();

			ItemHolder(Framework::IModulesOwner* _owner);
			virtual ~ItemHolder();

			Name const & get_hold_socket() const { return holdSocket; }
			Name const & get_held_item_socket() const { return heldItemSocket; }

			void set_locked(bool _locked) { locked = _locked; }

			bool is_locked() const { return locked; }

			TagCondition const& get_hold_only_tagged() const { return holdOnlyTagged; }

		public:
			bool hold(Framework::IModulesOwner* _item, bool _force = false); // if not forced, will check for locks and conditions

			Framework::IModulesOwner* get_held() const;

		public:	// Module
			override_ void setup_with(::Framework::ModuleData const * _moduleData);

		protected: // ModuleCustom
			override_ void advance_post(float _deltaTime);

		private:
			SafePtr<Framework::IModulesOwner> heldItem;

			bool locked = false; // may start locked
			bool lockWhenEmptied = false; // when emptied, won't accept any new items

			Name holdSocket;
			Name heldItemSocket;

			TagCondition holdOnlyTagged;

			bool does_still_hold_object() const;
		};
	};
};

