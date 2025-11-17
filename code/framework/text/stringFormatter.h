#pragma once

#include "..\time\dateTimeFormat.h"

#include "..\..\core\fastCast.h"
#include "..\..\core\types\name.h"
#include "..\..\core\containers\arrayStatic.h"
#include "..\..\core\memory\refCountObject.h"

#include "..\..\core\serialisation\serialiserBasics.h"

#include <functional>

#include "stringFormatterElement.h"

#include "localisedString.h"

namespace IO
{
	namespace XML
	{
		class Node;
	};
};

namespace Framework
{
	/**
	 *	String formatter is an utility to format strings using parameters and parameters providers.
	 *	Parameters can be serialised although with limitations.
	 *
	 *	StringFormatterParams are most important class here.
	 *	It stores:
	 *		any number of params
	 *			params can be of simple types (int, float, string) or custom (pointers to ICustomStringFormatter interface)
	 *		few additional formats - used during formatting to have sum of formats
	 *		few param providers - that can provide params during formatting, but only when not found
	 *
	 *	When serialising, it doesn't serialise additional formats (of course) and param providers - if there is need to serialise params
	 *	all params should be resolved and providers removed (check TextBlockInstance for an example).
	 *	One additional thing is while during normal gameplay we store raw (!) pointers to stuff, during deserialisation we may have
	 *	no idea where we were pointing. Thats' why we do this for custom data:
	 *		1.	We store (serialise) custom data right in the file
	 *		2.	When restoring (deserialising) we create temporary object that will hold data we deserialise, creating hard copy of our param
	 *			We use derivatives of CustomStringFormatterStored to handle that
	 *	To handle custom data we need to have special class that knows how to serialise such data
	 */
	class StringFormatter;
	struct LocalisedString;
	struct LocalisedStringForm;
	struct StringFormatterParam;
	struct StringFormatterParams;
	struct CustomStringFormatterStored;
	interface_class ICustomStringFormatter;
	interface_class IStringFormatterParamsProvider;
	interface_class ICustomStringFormatterParamSerialisationHandler;
	typedef std::function<bool(REF_ StringFormatterParams & _params, Name const & _param)> ProvideStringFormatterParams;

	void make_sure_string_format_is_valid(REF_ String & _format);
	void grow_format(REF_ String & _format, String const & _by);

	interface_class ICustomStringFormatter
	{
		FAST_CAST_DECLARE(ICustomStringFormatter);
		FAST_CAST_END();
	public:
		virtual ICustomStringFormatter const * get_hard_copy() const = 0; // allows to create hard copy (useful for local variables)
		virtual ICustomStringFormatter const * get_actual() const { return this; } // actual custom string formatter - for stored values it gets to the actual value
		virtual String format_custom(tchar const * _format, StringFormatterParams & _params) const = 0;
		virtual String const & get_form_for_format_custom(tchar const * _format, StringFormatterParams & _params) const { return String::empty(); }
	};

	// common base for custom string formatters stored in params - if no longer needed, is removed
	struct CustomStringFormatterStored
	: public RefCountObject
	, public ICustomStringFormatter
	{
		FAST_CAST_DECLARE(CustomStringFormatterStored);
		FAST_CAST_BASE(ICustomStringFormatter);
		FAST_CAST_END();
	public:
	};

	template<typename Class> // Class should derive from ICustomStringFormatter, this stores ICustomStringFormatter inside
	struct CustomStringFormatterStoredValue
	: public CustomStringFormatterStored
	{
		FAST_CAST_DECLARE(CustomStringFormatterStoredValue);
		FAST_CAST_BASE(CustomStringFormatterStored);
		FAST_CAST_END();
	public:
		CustomStringFormatterStoredValue() {}
		CustomStringFormatterStoredValue(Class const & _value) : stored(_value) {}

		override_ ICustomStringFormatter const * get_hard_copy() const { return new CustomStringFormatterStoredValue(stored); }
		override_ ICustomStringFormatter const * get_actual() const { return &stored; }
		override_ String format_custom(tchar const * _format, StringFormatterParams & _params) const { return get_actual()->format_custom(_format, _params); }
		override_ String const & get_form_for_format_custom(tchar const * _format, StringFormatterParams & _params) const { return get_actual()->get_form_for_format_custom(_format, _params); }

	public:
		Class stored;
	};
	template <typename Class>
	REGISTER_FOR_FAST_CAST(CustomStringFormatterStoredValue<Class>);

	template<typename Class> // Class should be smart pointer (eg RefCountObjectPtr) to an object derived from ICustomStringFormatter
	struct CustomStringFormatterStoredPtr
	: public CustomStringFormatterStored
	{
		FAST_CAST_DECLARE(CustomStringFormatterStoredPtr);
		FAST_CAST_BASE(CustomStringFormatterStored);
		FAST_CAST_END();
	public:
		override_ ICustomStringFormatter const * get_hard_copy() const { return this; }
		override_ ICustomStringFormatter const * get_actual() const { return stored.get(); }
		override_ String format_custom(tchar const * _format, StringFormatterParams & _params) const { return get_actual()->format_custom(_format, _params); }
		override_ String const & get_form_for_format_custom(tchar const * _format, StringFormatterParams & _params) const { return get_actual()->get_form_for_format_custom(_format, _params); }

	public:
		Class stored;
	};
	template <typename Class>
	REGISTER_FOR_FAST_CAST(CustomStringFormatterStoredPtr<Class>);

	// ICustomStringFormatterPtr wraps around ICustomStringFormatter to allow auto ref manipulation for stored CSFs
	struct ICustomStringFormatterPtr
	{
		ICustomStringFormatterPtr() {}
		ICustomStringFormatterPtr(ICustomStringFormatter const * _iCustom) : iCustom(nullptr) { set(_iCustom); }
		ICustomStringFormatterPtr(ICustomStringFormatterPtr const & _other) : iCustom(nullptr) { set(_other.iCustom); }
		~ICustomStringFormatterPtr() { set(nullptr); }
		ICustomStringFormatterPtr& operator=(ICustomStringFormatter const * _iCustom) { set(_iCustom); return *this; }
		ICustomStringFormatterPtr& operator=(ICustomStringFormatterPtr const & _other) { set(_other.iCustom); return *this; }
		void set(ICustomStringFormatter const * _iCustom)
		{
			if (iCustomStored) iCustomStored->release_ref();
			iCustom = _iCustom;
			iCustomStored = fast_cast<CustomStringFormatterStored>(iCustom);
			if (iCustomStored) iCustomStored->add_ref();
		}
		inline ICustomStringFormatter const* operator * ()  const { an_assert(iCustom); return iCustom; }
		inline ICustomStringFormatter const* operator -> () const { an_assert(iCustom); return iCustom; }
		inline ICustomStringFormatter const* get() const { return iCustom; }
		inline bool is_set() const { return iCustom != nullptr; }
	private:
		ICustomStringFormatter const * iCustom = nullptr;
		CustomStringFormatterStored const * iCustomStored = nullptr;
	};

	/**
	 *	This is useful when we want to reference ICustomStringFormatter but we want to provide custom format for it, for example we have:
	 *		format A: Is there %character%?
	 *		with character being just pointer to ICustomStringFormatter character
	 *		result: Is there John Doe?
	 *		(default formatting for character's name)
	 *	but we would like to ask differently and not in a manner provided by chracter itself, then we have
	 *		format A: Is there %character%?
	 *		with character being CustomStringFormatterUsingFormat character, "%title% %firstName%"
	 *		result: Is there Mr John?
	 *	additionaly we want our new format to extend existing one, so we will have
	 *		Nie ma %character~dopelniacz%.
	 *		default result: Nie ma Jana Kowalskiego.
	 *		Using CustomStringFormatterUsingFormat we will effectively have "Nie ma %character~title~dopelniacz% %character~firstName~dopelniacz%."
	 *		with result: Nie ma pana Jana.
	 */
	struct CustomStringFormatterUsingFormat
	{
		CustomStringFormatterUsingFormat() {}
		CustomStringFormatterUsingFormat(ICustomStringFormatter const * _iCustom, tchar const * _format, bool _formatMemoryManaged = false) : iCustom(_iCustom), format(_format), formatMemoryManaged(_formatMemoryManaged) {}
		CustomStringFormatterUsingFormat(ICustomStringFormatter const * _iCustom, LocalisedString const & _localisedString) : iCustom(_iCustom), localisedString(_localisedString) {}
		~CustomStringFormatterUsingFormat() { if (formatMemoryManaged) delete[] format; }
		GS_ ICustomStringFormatterPtr iCustom;
		GS_ tchar const * format = nullptr;
		bool formatMemoryManaged = false; // used when deserialising
		GS_ LocalisedString localisedString;
		
		// temporary, this also limits calling just one per whole game in the same time
		bool inside = false; // to know that we have stepped inside, when entering into this parameter, default parameter id will be stored and will redirect from format inside into params

		void hard_copy();

	public:
		bool serialise(Serialiser& _serialiser);
	};

	/**
	 *	String formatting takes string and params and combines them to give result
	 *	It is slightly more sophisticated than printf as it has named params and has ability to use custom formatter
	 *	Example:
	 *
	 *		%person~formal% went to %place% and took %amount~:.2% kg of flour.
	 *
	 *		format starting with : means exact string format - also used to round numbers that are passed to localised string			%amount~:.2%			->	1.23
	 *		format starting with : and followed by auto - will use auto formatting/decimals												%amount~:auto%			->  1.23
	 *		format %% starts with ^ - first letter uppercase																			%^item%					->	Table
	 *		format %% starts with ^^ - all letters uppercase																			%^^item%				->	TABLE
	 *		format @ include localised string, can leave param, ie %@string name% - should be placed BEFORE ~							%amount@number; amount%	->	processes through "number; amount" format
	 *		- before param name - negate param value
	 *		| before param name - use absolute param value
	 *		` before param name - value as 100% percentage (*100 + % added)
	 *
	 *		will have parameters:
	 *			person - custom defining name, accepts format "~formal"
	 *			place - localised string
	 *			amount - float
	 *
	 *		%object~format1~format2% everything that starts with ~ is part of format
	 *
	 *	Note:
	 *		Strings should exist as long as format operation is processed
	 *
	 *		%percentage-sign% ----> %
	 */
	struct StringFormatParser
	{
		StringFormatParser(tchar const * _format);
		~StringFormatParser();

		tchar const * get_next();
	private:
		tchar * current;
		tchar * format;
		bool hasNext;
	};

	struct StringFormatterParam
	{
	public:
		enum Type
		{
			String,
			Float,
			Int,
			Custom,
			CustomUsingFormat
		};
	public:
		StringFormatterParam() {}
		StringFormatterParam(StringFormatterParam const& _other) { *this = _other; }
		StringFormatterParam(Name const & _id, int _value): id(_id), type(Int), asInt(_value) {}
		StringFormatterParam(Name const & _id, float _value) : id(_id), type(Float), asFloat(_value) {}
		StringFormatterParam(Name const & _id, ::String const & _string) : id(_id), type(String), asString(_string) {}
		StringFormatterParam(Name const & _id, ICustomStringFormatter const * _customFormatter) : id(_id), type(Custom), iCustom(_customFormatter) {}
		StringFormatterParam(Name const & _id, ICustomStringFormatter const & _customFormatter) : id(_id), type(Custom), iCustom(&_customFormatter) {}
		StringFormatterParam(Name const & _id, CustomStringFormatterUsingFormat const & _customUsingFormat) : id(_id), type(CustomUsingFormat), customUsingFormat(_customUsingFormat) {}

	public:
		Name const & get_id() const { return id; }
		Type get_type() const { return type; }
		::String const & get_as_string() const { an_assert(type == String); return asString; }
		int get_as_int() const { an_assert(type == Int); return asInt; }
		float get_as_float() const { an_assert(type == Float); return asFloat; }
		ICustomStringFormatter const* get_as_custom() const { an_assert(type == Custom); return iCustom.get(); }

		void hard_copy(); // it's important only for custom and only if it points to actual value (not pointer) but whether to create copy depends is handled in ICustomStringFormatter class

	public:
		bool serialise(Serialiser& _serialiser);

	private:
		::String format(tchar const * _format, Name const & _useLocalisedStringId, StringFormatterParams & _params, bool _negate, bool _absolute, bool _percentage);

	private:
		GS_ Name id; // if empty - default
		GS_ Type type = Int;
		GS_ ::String asString;
		GS_ float asFloat;
		GS_ int asInt;
		GS_ ICustomStringFormatterPtr iCustom;
		GS_ CustomStringFormatterUsingFormat customUsingFormat;

		friend struct StringFormatterParams;
		friend class StringFormatter;
	};

	/**
	 *	To keep params as they were. Allowing to alter params in a scope
	 */
	struct ForStringFormatterParams
	{
		ForStringFormatterParams(StringFormatterParams& _params);
		~ForStringFormatterParams();

		void keep_params(); // keep params count

		void add_params_provider(IStringFormatterParamsProvider const * _provider);
		void add_params_provider(ProvideStringFormatterParams _provider);

		void push_additional_format(tchar const * _format);

	private:
		StringFormatterParams & params;

		bool keepParams = false;
		int keepParamsCount;

		bool addedProviders = false;
		int keepProvidersCount;

		int pushedAdditionalFormats = 0;
	};

	struct StringFormatterParams
	{
	public:
		StringFormatterParams();
		StringFormatterParams & temp_ref() { return *this; }

		StringFormatterParams & date_time_format(DateTimeFormat const & _dateTimeFormat) { dateTimeFormat = _dateTimeFormat; return *this; }
		StringFormatterParams & add_params_from(StringFormatterParams const & _params);
		StringFormatterParams & add(StringFormatterParam const & _param) { params.push_back(_param); return *this; }
		StringFormatterParams & add(Name const & _id, int _value) { params.push_back(StringFormatterParam(_id, _value)); return *this; }
		StringFormatterParams & add(Name const & _id, float _value) { params.push_back(StringFormatterParam(_id, _value)); return *this; }
		StringFormatterParams & add(Name const & _id, String const & _string) { params.push_back(StringFormatterParam(_id, _string)); return *this; }
		StringFormatterParams & add(Name const & _id, ICustomStringFormatter const & _customFormatter) { params.push_back(StringFormatterParam(_id, &_customFormatter)); return *this; }
		StringFormatterParams & add(Name const & _id, ICustomStringFormatter const * _customFormatter) { params.push_back(StringFormatterParam(_id, _customFormatter)); return *this; }
		StringFormatterParams & add(int _value) { return add(Name::invalid(), _value); }
		StringFormatterParams & add(float _value) { return add(Name::invalid(), _value); }
		StringFormatterParams & add(String const & _string) { return add(Name::invalid(), _string); }
		StringFormatterParams & add(ICustomStringFormatter const & _customFormatter) { return add(Name::invalid(), _customFormatter); }
		StringFormatterParams & add(ICustomStringFormatter const * _customFormatter) { return add(Name::invalid(), _customFormatter); }
		StringFormatterParams & add_params_provider(IStringFormatterParamsProvider const * _provider) { an_assert(_provider, TXT("maybe we should not add it at all?")); paramsProviders.push_back(_provider); return *this; }
		StringFormatterParams & add_params_provider(ProvideStringFormatterParams _provider) { an_assert(_provider, TXT("maybe we should not add it at all?")); paramsProviders.push_back(_provider); return *this; }

		void clear();
		void prune();

		DateTimeFormat const & get_date_time_format() const { return dateTimeFormat; }

		int get_param_count() const { return params.get_size(); }
		void keep_params(int _count) { params.set_size(min(_count, get_param_count())); }
		void remove_params_at(int _at, int _count = 1);

		StringFormatterParam const * find_existing_only(Name const & _id) const; // only existing in params array
		StringFormatterParam * find(Name const & _id); // from params array and from providers
		StringFormatterParam * find(tchar const * _id);
		StringFormatterParam * find_or_add(Name const & _id); // if doesn't exist, add new one (int 0)
		StringFormatterParam * add_from_providers(Name const & _id);

		void set_default_param_id(Name const & _defaultParamId) { defaultParamId = _defaultParamId; }
		Name get_default_param_id() const { return defaultParamId; }
		Name replace_default_param_id(Name const & _with) { Name prev = defaultParamId;  defaultParamId = _with; return prev; }

		void push_additional_format(tchar const * _format) { additionalFormatStack.push_back(_format); }
		void pop_additional_format() { additionalFormatStack.pop_back(); }

		int get_params_providers_count() const { return paramsProviders.get_size(); }
		void keep_params_providers(int _count) { paramsProviders.set_size(min(_count, get_params_providers_count())); }
		void remove_params_providers() { paramsProviders.clear(); }

		bool serialise(Serialiser& _serialiser);

		void hard_copy(); // creates hard copies of params and removes params providers

	private:
		GS_ DateTimeFormat dateTimeFormat;
		GS_ Name defaultParamId;
		GS_ Array<StringFormatterParam> params; // we should go through this array from the end to get the most recent one
		// additional format stack actually consists of all formats extended on the way and only last one should be added,
		// it's only used by StringFormatter as it propagates format to params and beyond
		ArrayStatic<tchar const *, 10> additionalFormatStack; // might be null, it will still work, not serialised!
		struct ParamsProvider
		{
			IStringFormatterParamsProvider const * provider = nullptr;
			ProvideStringFormatterParams provideFunc = nullptr;
			ParamsProvider() {}
			// not explicit as we just use it internally
			ParamsProvider(IStringFormatterParamsProvider const * _provider) : provider(_provider) {}
			ParamsProvider(ProvideStringFormatterParams _provideFunc) : provideFunc(_provideFunc) {}
		};
		ArrayStatic<ParamsProvider, 10> paramsProviders; // not serialised!

		friend class StringFormatter;
	};

	class StringFormatter
	{
	public:
		static void extract_params(String const & _string, OUT_ Array<Name> const & _paramNames);

		static String format_sentence_loc_str(LocalisedString const& _localisedString, StringFormatterParams & _params, int _formatParagraphs = NONE);
		static String format_loc_str(LocalisedString const& _localisedString, StringFormatterParams & _params);
		static String format_sentence_loc_str(Name const & _localisedStringID, StringFormatterParams & _params, int _formatParagraphs = NONE);
		static String format_loc_str(Name const & _localisedStringID, StringFormatterParams & _params);
		static String format(LocalisedStringForm const * _form, StringFormatterParams & _params);

		// with format
		static String format_sentence_loc_str(Name const & _localisedStringID, tchar const * _format, StringFormatterParams & _params, int _formatParagraphs = NONE);
		static String format_loc_str(Name const & _localisedStringID, tchar const * _format, StringFormatterParams & _params);
		static String format_loc_str(LocalisedString const & _localisedString, tchar const * _format, StringFormatterParams & _params);

		// avoid, slow
		AVOID_CALLING_ static String format_sentence(String const& _string, StringFormatterParams& _params, int _formatParagraphs = NONE);
		AVOID_CALLING_ static String format(String const& _string, StringFormatterParams& _params);

	private:
		//
		static String format(Array<StringFormatterElement> const & _elements, tchar const * _format, StringFormatterParams & _params);
		inline static void process_element(StringFormatterElement const & _element, OUT_ String & _result, StringFormatterParams & _params);
	};

	interface_class IStringFormatterParamsProvider
	{
		FAST_CAST_DECLARE(IStringFormatterParamsProvider);
		FAST_CAST_END();
	public:
		virtual bool requested_string_formatter_param(REF_ StringFormatterParams & _params, Name const & _param) const = 0;
	};
};

DECLARE_SERIALISER_FOR(Framework::StringFormatterParam::Type);
