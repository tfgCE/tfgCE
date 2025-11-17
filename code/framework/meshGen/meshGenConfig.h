#pragma once

#include "..\..\core\containers\arrayStatic.h"

namespace IO
{
	namespace XML
	{
		class Node;
	};
};

namespace Framework
{
	namespace MeshGeneration
	{
		class Config
		{
		public:
			static Config& access() { an_assert(s_config); return *s_config; }
			static Config const & get() { an_assert(s_config); return *s_config; }

			static void initialise_static();
			static void close_static();

		public:
			float get_sub_step_def_modify_divider(int _lod, float _useFully, float _use1AsBase = 0.0f) const;
			float get_min_size(int _lod, float _useFully) const;
			bool should_smooth_everything_at_lod(int _lod) const { return smoothEverythingAtLOD <= _lod; }
			int get_lod_index_multiplier() const { return lodIndexMultiplier; }

		public:
			bool load_from_xml(IO::XML::Node const* _node);
			void save_to_xml(IO::XML::Node * _node, bool _userSpecific = false) const;

		private:
			static Config* s_config;

			ArrayStatic<float, 32> subStepDefModifyDividers;
			ArrayStatic<float, 32> minSizes;
			int smoothEverythingAtLOD = 3;
			int lodIndexMultiplier = 1;

			Config();
		};

	};
};
