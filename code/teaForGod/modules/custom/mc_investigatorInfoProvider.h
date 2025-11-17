#pragma once

#include "..\..\..\framework\appearance\socketID.h"
#include "..\..\..\framework\module\moduleCustom.h"
#include "..\..\..\framework\text\localisedString.h"

#include "..\..\..\core\types\colour.h"

//

#define INVESTIGATOR_INFO_PROVIDER_INFO_LIMIT 8

//

namespace TeaForGodEmperor
{
	namespace CustomModules
	{
		class InvestigatorInfoProviderData;

		// this structure is stored in info user to know if we should update showed string or not
		struct InvestigatorInfoCachedData
		{
			ArrayStatic<byte, 64> data; // just packed as data, only plain data!

			inline void clear() { data.clear(); }

			template<typename Class>
			void add(Class const& c)
			{
				int idx = data.get_size();
				int s = sizeof(c);
				an_assert(data.get_space_left() >= s);
				data.grow_size(s);
				memory_copy(&data[idx], &c, s);
			}

			template<typename Class>
			bool check_update(Class const& c)
			{
				int s = sizeof(c);
				if (data.get_size() == s &&
					memory_compare(&c, data.get_data(), s))
				{
					return false; // no update required
				}
				data.set_size(s);
				memory_copy(&data[0], &c, s);
				return true;
			}

			inline bool check_update_empty()
			{
				if (data.is_empty())
				{
					return false; // no update required
				}
				data.set_size(0);
				return true;
			}

			void add(SimpleVariableInfo const& _svi);
			void add(Framework::Module * _module, Name const & _variable, TypeId _typeId);

			bool operator == (InvestigatorInfoCachedData const& o) const { return data == o.data; }
			bool operator != (InvestigatorInfoCachedData const& o) const { return !(data == o.data); }
		};

		struct InvestigatorInfo
		{
			Name id; // used mostly for localised strings

			// conditions
			Optional<Name> healthRuleActive;
			Optional<Name> healthRuleInactive;
			Optional<Name> extraEffectActive;
			Optional<Name> extraEffectInactive;
			Optional<Name> variableTrue;
			Optional<Name> variableFalse;

			// provided info (uses id)
			Name iconAtSocket; // if at socket, won't be displayed in the main line
			Framework::LocalisedString icon; // this is only used for icon, only first character is used
			Framework::LocalisedString info; // this is only used for long info

			// if info requires values, this is how we get them
			struct ValueForInfo
			{
				Name provide;
				Optional<Name> fromTemporaryObject;
				Optional<Name> fromCustomModule;
				TypeId fromVariableType = type_id_none();
				Optional<Name> fromVariable;
			};
			ArrayStatic<ValueForInfo, 8> valuesForInfo;

			bool load_from_xml(IO::XML::Node const* _node, Framework::LibraryLoadingContext& _lc);

			struct Instance
			{
				Optional<Name> healthRuleActive;
				Optional<Name> healthRuleInactive;
				Optional<Name> extraEffectActive;
				Optional<Name> extraEffectInactive;
				Optional<SimpleVariableInfo> variableTrue;
				Optional<SimpleVariableInfo> variableFalse;

				Framework::SocketID iconAtSocket;
				Framework::LocalisedString icon; // this is only used for icon, only first character is used
				Framework::LocalisedString info; // this is only used for long info

				// if info requires values, this is how we get them
				struct ValueForInfo
				{
					Name provide;
					Optional<Name> fromTemporaryObject;
					Optional<Name> fromCustomModule;
					TypeId fromVariableType = type_id_none();
					Optional<Name> fromTemporaryObjectVariable;
					Optional<Name> fromCustomModuleVariable;
					Optional<SimpleVariableInfo> fromVariable;
				};
				ArrayStatic<ValueForInfo, 8> valuesForInfo;
			};

			Instance create_instance_for(Framework::IModulesOwner* imo) const;
		};

		class InvestigatorInfoProvider
		: public Framework::ModuleCustom
		{
			FAST_CAST_DECLARE(InvestigatorInfoProvider);
			FAST_CAST_BASE(Framework::ModuleCustom);
			FAST_CAST_END();

			typedef Framework::ModuleCustom base;
		public:
			static Framework::RegisteredModule<Framework::ModuleCustom> & register_itself();

			InvestigatorInfoProvider(Framework::IModulesOwner* _owner);
			virtual ~InvestigatorInfoProvider();

		public: // investigator info
			int get_info_count() const; // all, even if empty
			bool does_require_update(int _infoIdx, InvestigatorInfoCachedData & _cachedData) const; // updates cached data immediately
			bool is_active(int _infoIdx) const;
			bool has_icon(int _infoIdx) const;
			Optional<Framework::SocketID> get_icon_at_socket(int _infoIdx) const;
			Optional<tchar> get_icon(int _infoIdx) const;
			String get_info(int _infoIdx) const;
			bool should_ignore_social_info() const { return noSocialInfo; }

		public:	// Module
			override_ void setup_with(::Framework::ModuleData const * _moduleData);

		protected:
			InvestigatorInfoProviderData const * investigatorInfoProviderData = nullptr;

			ArrayStatic<InvestigatorInfo::Instance, INVESTIGATOR_INFO_PROVIDER_INFO_LIMIT> infos;
			bool noSocialInfo = false;
		};

		class InvestigatorInfoProviderData
		: public Framework::ModuleData
		{
			FAST_CAST_DECLARE(InvestigatorInfoProviderData);
			FAST_CAST_BASE(Framework::ModuleData);
			FAST_CAST_END();

			typedef Framework::ModuleData base;
		public:
			InvestigatorInfoProviderData(Framework::LibraryStored* _inLibraryStored);
			virtual ~InvestigatorInfoProviderData();

		public: // Framework::ModuleData
			override_ bool load_from_xml(IO::XML::Node const * _node, Framework::LibraryLoadingContext & _lc);
			override_ bool prepare_for_game(Framework::Library* _library, Framework::LibraryPrepareContext& _pfgContext);

		private:
			ArrayStatic<InvestigatorInfo, INVESTIGATOR_INFO_PROVIDER_INFO_LIMIT> infos;

			friend class InvestigatorInfoProvider;
		};
	};
};

