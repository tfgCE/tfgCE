#pragma once

#include "..\globalDefinitions.h"
#include "..\casts.h"

#include "..\containers\array.h"
#include "..\containers\arrayStatic.h"

#include "..\memory\objectHelpers.h"
#include "..\memory\pooledObject.h"

#include "..\math\mathUtils.h"

#include "..\other\registeredType.h"

#define LATENT_VARIABLE_DESTRUCT_FUNCTION(_func) void (*_func)(void* _data)
#define LATENT_VARIABLE_COPY_CONSTRUCTOR_FUNCTION(_func) void (*_func)(void* _data, void const * _source)

namespace Latent
{
	class StackVariablesBase;

	// this is slightly different to simple variable storage as it has fixed memory size (to stay nicely in continous memory and to skip moving variables around)
	// was, simple variable storage is as of 2017.07.22 turned into buffers
	struct StackDataBuffer
	: public PooledObject<StackDataBuffer>
	{
	private: friend class StackVariablesBase;
		static const int DATA_SIZE = 16384;

		StackVariablesBase* inStackVariables;
		StackDataBuffer* prev;
		StackDataBuffer* next;
		ArrayStatic<int8, DATA_SIZE> data;

		StackDataBuffer(StackVariablesBase* _inStackVariables);
		~StackDataBuffer();

		friend struct StackVariableEntry;
	};

	struct StackVariableEntry
	{
		StackDataBuffer* inDataBuffer; //
		int at; // in data
		int size; // should never be zero as it may confuse destroy function (that will think that buffer is zero)
		enum Type
		{
			// begin and end are put automatically by StackVariablesTemplate when pushing var or param but when getting, they have to be traversed (LatentScope does that)
			None,
			// it is always in this order, params first, end params, vars
			BeginParams,
			Param,
			ParamNotProvided,
			EndParams,
			Var,
		};
		Type type = Var;
#ifdef AN_INSPECT_LATENT
		int typeId; // if available
		tchar const* name = nullptr;
#endif
		LATENT_VARIABLE_DESTRUCT_FUNCTION(destructFunc);
		LATENT_VARIABLE_COPY_CONSTRUCTOR_FUNCTION(copyConstructFunc);
		StackVariableEntry(StackDataBuffer* _inDataBuffer = nullptr) : inDataBuffer(_inDataBuffer), at(NONE), size(NONE), destructFunc(nullptr) {}
		StackVariableEntry(StackDataBuffer* _inDataBuffer, int _at, int _size, LATENT_VARIABLE_DESTRUCT_FUNCTION(_destructFunc), LATENT_VARIABLE_COPY_CONSTRUCTOR_FUNCTION(_copyConstructFunc)) : inDataBuffer(_inDataBuffer), at(_at), size(_size), destructFunc(_destructFunc), copyConstructFunc(_copyConstructFunc) {}

		void copy_from(StackVariableEntry const & _variable);
		void destroy();
		void* get_data() const;
	};

	class StackVariablesBase
	{
	public:
		StackVariablesBase();
		virtual ~StackVariablesBase();

		virtual bool get_variables(OUT_ StackVariableEntry const *& _first, OUT_ int & _count) const = 0;

	protected:
		StackDataBuffer* dataBuffer;

		void find_place_for(int _size, OUT_ StackDataBuffer* & _dataBuffer, OUT_ int & _at);

		friend struct StackVariableEntry;
		friend struct StackDataBuffer;
	};

	template <typename VariablesArray>
	class StackVariablesTemplate
	: public StackVariablesBase
	{
	public:
		StackVariablesTemplate();
		~StackVariablesTemplate();

		template <typename Class>
		void push_var(Class const & _var);

		void push_var(StackVariableEntry const & _variable);

		template <typename Class>
		Class & push_or_get_var(int _varIndex, tchar const * _name);

		template <typename Class>
		void push_param(Class const & _var);
		
		void push_param_optional_not_provided();

		void push_param(StackVariableEntry const & _variable);

		template <typename Class>
		Class & get_param(int _varIndex, tchar const * _name);

		template <typename Class>
		Class * get_param_optional(int _varIndex, tchar const * _name);

		StackVariableEntry::Type get_type(int _varIndex) const;
		bool is_begin_params(int _varIndex) const { return get_type(_varIndex) == StackVariableEntry::BeginParams; }
		bool is_end_params(int _varIndex) const { return get_type(_varIndex) == StackVariableEntry::EndParams; }
		bool is_param(int _varIndex) const { auto tp = get_type(_varIndex); return tp == StackVariableEntry::Param || tp == StackVariableEntry::ParamNotProvided; }
		void mark_begin_params();
		void mark_end_params();
		int make_sure_params_began();
		int make_sure_params_ended();

		void remove_variables_to_keep(int _howManyShouldBeLeft);
		void remove_all_variables();

		int get_variable_count() const { return variables.get_size(); }

	public: // StackVariablesBase
		implement_ bool get_variables(OUT_ StackVariableEntry const *& _first, OUT_ int & _count) const { _first = variables.get_data(); _count = variables.get_size(); return !variables.is_empty(); }

	protected:
		VariablesArray variables;

		friend struct StackVariableEntry;
		friend struct StackDataBuffer;
	};

	// actual classes to use:
	class StackVariables
	: public StackVariablesTemplate<Array<StackVariableEntry>>
	{
	};

	template <unsigned int VariableCount>
	class StackVariablesStatic
	: public StackVariablesTemplate<ArrayStatic<StackVariableEntry, VariableCount>>
	{
	public:
		StackVariablesStatic()
		{
			SET_EXTRA_DEBUG_INFO(this->variables, TXT("StackVariablesStatic.variables"));
		}
	};

	#include "latentStackVariables.inl"
};
