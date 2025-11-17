#pragma once

#include "..\..\core\casts.h"
#include "..\..\core\containers\arrayStatic.h"
#include "..\..\core\types\name.h"
#include "..\..\core\other\registeredType.h"

struct SimpleVariableInfo;

namespace Framework
{
	namespace AI
	{
		struct Param
		{
		private:
			static const int DATASIZE = sizeof(float) * 16; // matrix
			Name name;
			TypeId type = type_id_none();
			int8 data[DATASIZE];

		public:
			Param();
			Param(Name const& _name);
			Param(Param const & _other);
			Param& operator=(Param const & _other);
			~Param();

			Name const & get_name() const { return name; }
			TypeId get_type() const { return type; }

			void set(SimpleVariableInfo const& _svi);
			void set(TypeId _type, void const* _raw);

			void clear();

			template <typename Class>
			Class const & get_as() const;

			template <typename Class>
			Class & access_as();
		};

		struct Params
		{
		public:
			Params();
			Param & access(Name const & _name);
			Param const * get(Name const & _name) const;

			void clear();

		private:
			static const int PARAMLIMIT = 64;
			ArrayStatic<Param, PARAMLIMIT> params;
		};

		#include "aiParam.inl"
	};
};
