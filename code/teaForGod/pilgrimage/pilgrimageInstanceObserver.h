#pragma once

#include "..\teaEnums.h"
#include "..\..\core\fastCast.h"

namespace TeaForGodEmperor
{
	class PilgrimageInstance;

	interface_class IPilgrimageInstanceObserver
	{
		FAST_CAST_DECLARE(IPilgrimageInstanceObserver);
		FAST_CAST_END();

	public:
		static void pilgrimage_instance_switch(PilgrimageInstance const* _from, PilgrimageInstance const* _to);
		static void pilgrimage_instance_end(PilgrimageInstance const* _pilgrimage, PilgrimageResult::Type _pilgrimageResult);

	public:

		IPilgrimageInstanceObserver() { pio_add(); }
		virtual ~IPilgrimageInstanceObserver() { pio_remove(); }

	protected:
		virtual void on_pilgrimage_instance_switch(PilgrimageInstance const* _from, PilgrimageInstance const* _to) = 0;
		virtual void on_pilgrimage_instance_end(PilgrimageInstance const* _pilgrimage, PilgrimageResult::Type _pilgrimageResult) = 0;

		void pio_add();
		void pio_remove();

	private:
		static IPilgrimageInstanceObserver* s_pio_first;
		IPilgrimageInstanceObserver* pio_next = nullptr;
		IPilgrimageInstanceObserver* pio_prev = nullptr;
	};

};
