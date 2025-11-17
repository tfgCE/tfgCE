#pragma once

#include "room.h"
#include "environmentType.h"

namespace Framework
{
	class EnvironmentType;

	class Environment
	: public Room
	{
		FAST_CAST_DECLARE(Environment);
		FAST_CAST_BASE(Room);
		FAST_CAST_END();

		typedef Room base;
	public:
		Environment(Name const & _name, Name const & _parent, SubWorld* _inSubWorld, Region* _inRegion, EnvironmentType const * _environmentType, Random::Generator const& _rg);
		~Environment();

		Name const & get_name() const { return name; }

		void add_room(Room* _room);
		void remove_room(Room* _room);

		EnvironmentInfo const & get_info() const { return info; }
		EnvironmentInfo & access_info() { return info; }
		EnvironmentType const * get_environment_type() const { return environmentType; }

		EnvironmentParam const * get_param(Name const & _name) const;
		EnvironmentParam * access_param(Name const & _name);

		Environment* get_parent_environment() const { return parent; }
		void set_parent_environment(Environment* _parent);
		void set_parent_environment(Region* _inRegion, Name const & _parent);
		void update_parent_environment(Region* _inRegion);

		float get_time_before_usage_reset() const { return environmentType ? environmentType->get_time_before_usage_reset() : -1.0f; }

	public: // Room
		override_ void set_environment(Environment* _environment, Transform const& _environmentPlacement);
		override_ bool generate();

	protected:
		Name name;
		EnvironmentType const * environmentType;
		Array<Room*> rooms; // rooms attached to this environment (held to detach environment from room when environment is destroyed)
		Array<Environment*> children;
		Environment* parent; // to create chain of environments, by default params (through info) are not copied! those params have to be added (not done yet!)
		Name parentName;

		EnvironmentInfo info;

		friend class Render::EnvironmentProxy;

	private: // Room
		override_ void get_use_environment_from_type();
		override_ void setup_using_use_environment();

	};
};
