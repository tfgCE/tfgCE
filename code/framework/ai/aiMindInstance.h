#pragma once

#include "..\library\libraryStored.h"
#include "..\library\usedLibraryStored.h"

#include "..\interfaces\aiMindOwner.h"

#include "aiMind.h"
#include "aiMindExecution.h"
#include "aiLocomotion.h"
#include "aiPerception.h"
#include "aiNavigation.h"

class SimpleVariableStorage;

namespace Framework
{
	interface_class IModulesOwner;

	namespace AI
	{
		class Logic;
		class Social;

		/**
		 *	Instance to wrap pointer to mind, owner, logic and execution.
		 *	All work is handled by execution.
		 */
		class MindInstance
		{
		public:
			MindInstance(IAIMindOwner* _owner);
			~MindInstance();

			bool is_enabled() const { return mindEnabled; }
			void set_enabled(bool _enabled);

			Logic* get_logic() const { return logic; }
			bool is_ending_logic() const { return endingLogic; }

			Social& access_social() { return *social; }
			Social const & get_social() const { return *social; }

			Nav::Flags const & get_nav_flags() const { return navFlags; }

			void use_mind(Mind * _mind);
			Mind* get_mind() const { return mind.get(); }

			void use_logic(Logic* _logic);

			void learn_from(SimpleVariableStorage & _parameters); // not const as it should be possible to modify this storage (for any reason)
			void ready_to_activate();

			void advance_logic(float _deltaTime);
			void advance_locomotion(float _deltaTime);
			void advance_perception();

			IAIMindOwner* get_owner() const { return owner; }
			IModulesOwner* get_owner_as_modules_owner() const { return ownerAsModulesOwner; }

			MindExecution & access_execution() { return execution; }
			Locomotion & access_locomotion() { return locomotion; }
			Perception & access_perception() { return perception; }
			Perception const & get_perception() const { return perception; }
			Navigation & access_navigation() { return navigation; }

		private:
			bool mindEnabled = true;

			IAIMindOwner* owner;
			IModulesOwner* ownerAsModulesOwner;
			UsedLibraryStored<Mind> mind;

			Nav::Flags navFlags = Nav::Flags::none(); // at least one is required - this is to determine the whole path

			MindExecution execution;
			Locomotion locomotion;
			Perception perception;
			Navigation navigation;

			Logic* logic = nullptr;
			Social* social = nullptr;
			bool endingLogic = false;

			void end_logic();
		};
	};
};

DECLARE_REGISTERED_TYPE(Framework::AI::MindInstance*);
