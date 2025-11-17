#pragma once

#include "string.h"
#include "..\containers\arrayStatic.h"
#include "..\concurrency\mrswLock.h"
#include "..\concurrency\scopedMRSWLock.h"
#include "..\defaults.h"

// .inl
#include "..\utils.h"

class NameStorage;
class Serialiser;

#define DEFINE_STATIC_NAME(_name) static Name staticName##_name(TXT(#_name));
#define DEFINE_STATIC_NAME_AS(_name, _actualName) static Name staticName##_name(TXT(#_actualName));
#define DEFINE_STATIC_NAME_STR(_name, _asString) static Name staticName##_name(_asString);
#define NAME(_name) staticName##_name

#define MAX_NAME_COUNT 30000
#define MAX_NAME_LENGTH 63

typedef Array<NameStorage> WholeNameStorage;

struct Name
{
public:
	static Name const & invalid() { return c_invalid; }

	static void initialise_static();
	static void close_static();
	
#ifdef BUILD_NON_RELEASE
	static void dev_auto_check();
#endif

	inline Name();
	inline explicit Name(String const & _name);
	inline explicit Name(tchar const * _name);
	inline explicit Name(int _index);

	inline String to_string() const;
	inline tchar const * to_char() const;
	inline int get_index() const { return index; }

	inline bool is_valid() const;
	inline static int find(tchar const* _name);

	inline bool operator == (Name const & _other) const { return index == _other.index; }
	inline bool operator != (Name const & _other) const { return index != _other.index; }

	inline bool operator == (tchar const * _other) const { return String::compare_icase(to_char(), _other); }
	inline bool operator != (tchar const * _other) const { return ! String::compare_icase(to_char(), _other); }

	inline bool operator < (Name const & _other) const { return index < _other.index; }

	bool serialise(Serialiser & _serialiser);

	bool save_to_xml(IO::XML::Node* _node) const;

private:
	static Name c_invalid;

	int index;

#ifdef AN_NAT_VIS
	WholeNameStorage* natVisNameStorage;
#endif
};

class NameStorage
{
	friend struct Name;

public:
	inline NameStorage();
	inline ~NameStorage();

private:
	struct Keeper
	{
		Keeper();
		~Keeper();

		void clear();
	};

	static tchar const * s_none;
	static Keeper keeper;
	static WholeNameStorage* storage;

	tchar name[MAX_NAME_LENGTH + 1];
	int icaseNegIdx = NONE;
	int icasePosIdx = NONE;
	inline NameStorage(tchar const* _name);

	inline static WholeNameStorage& access_storage(); // to allow creation on demand
	inline static Concurrency::MRSWLock& access_storage_lock(); // to allow creation on demand

	inline static bool is_name_valid(tchar const* _name);

	inline static int find(tchar const * _name);
	inline static int find_no_lock(tchar const* _name, OPTIONAL_ OUT_ int * _lastValidIndex = nullptr, OPTIONAL_ OUT_ int * _lastICase = nullptr);
	inline static int find_or_create(tchar const* _name);
	static int create_at_no_lock(tchar const* _name, int _lastValidIndex, int _lastICase);

	inline static tchar const* get_as_char(int _index);

	inline static bool is_index_valid(int _index);

	inline static bool is_storage_valid_no_lock();

#ifdef AN_DEVELOPMENT_OR_PROFILER
	static bool shouldLogCreatingNames;
	static bool should_log_creating_names();
public:
	static void set_should_log_creating_names(bool _should);
#endif

};

#include "name.inl"

DECLARE_REGISTERED_TYPE(Name);
