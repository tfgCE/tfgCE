#pragma once

#include "aiPerceptionLock.h"
#include "aiPerceptionPause.h"

#include "..\..\core\globalDefinitions.h"
#include "..\..\core\latent\latent.h"

//

/**
 *	LATENT_COMMON_VAR_BEGIN();
 *	LATENT_COMMON_VAR(Framework::PresencePath, enemy);
 *	LATENT_COMMON_VAR(float, aggressiveness);
 *	LATENT_COMMON_VAR_END();
 */

#define LATENT_COMMON_VAR_BEGIN() \
	LATENT_VAR(bool, commonVarsInitialised);

#define	LATENT_COMMON_VAR(type, var) \
	LATENT_VAR(type*, var##Ptr); \
	if (! commonVarsInitialised) \
	{ \
		GetCommonVariable::var(mind, var##Ptr); \
	} \
	type & var = *(var##Ptr);

#define LATENT_COMMON_VAR_END() \
	commonVarsInitialised = true;

//

template <typename Class> struct SafePtr;
struct SimpleVariableInfo;

struct LogInfoContext;
struct Vector3;

namespace Framework
{
	interface_class IModulesOwner;
	class Room;
	struct PresencePath;
	struct RelativeToPresencePlacement;
	struct InRoomPlacement;
	
	namespace AI
	{
		class MindInstance;
	};
};

namespace TeaForGodEmperor
{
	namespace AI
	{
		/**
		 *	please check aiCommonVariablesList.h for more info
		 */
		class GetCommonVariable
		{
		public:
			#define VAR(type, var) static void var(::Framework::AI::MindInstance* _mind, OUT_ type*& _var);
			#include "aiCommonVariablesList.h"
			#undef VAR
		};

		void log_common_variables(LogInfoContext & _log, ::Framework::AI::MindInstance* _mind);

		class Mobility
		{
		public:
			static bool should_remain_stationary(Framework::IModulesOwner* _imo);
			static bool may_leave_room(Framework::IModulesOwner* _imo, Framework::Room* _room);
			static bool may_leave_room_to_wander(Framework::IModulesOwner* _imo, Framework::Room* _room);
			static bool may_leave_room_to_investigate(Framework::IModulesOwner* _imo, Framework::Room* _room);
			static bool may_leave_room_when_attacking(Framework::IModulesOwner* _imo, Framework::Room* _room);
			static bool is_in_transport_room(Framework::IModulesOwner* _imo);
			static bool is_transport_room(Framework::Room* _room);
		};

		class Investigate
		{
		public:
			static bool is_close_and_in_front(Framework::IModulesOwner* _imo, float _dist, float _inFrontAngle);
			static bool is_in_worth_scanning(Framework::IModulesOwner* _imo);
			static bool is_worth_scanning(Framework::Room* _room);
		};

		class Targetting
		{
		public:
			static Vector3 get_enemy_targetting_in_owner_room(Framework::IModulesOwner* _imo);
			static Vector3 get_enemy_centre_of_presence_in_owner_room(Framework::IModulesOwner* _imo);

			static Vector3 get_enemy_targetting_offset_os(Framework::IModulesOwner* _imo);
			static Vector3 get_enemy_centre_of_presence_offset_os(Framework::IModulesOwner* _imo);
		};

	};

};
