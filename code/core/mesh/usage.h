#pragma once

#include "..\system\video\material.h"

#include "..\serialisation\serialiserBasics.h"

namespace Meshes
{
	namespace Usage
	{
		enum Type
		{
			Static,
			Skinned,
			SkinnedToSingleBone // every vertex is skinned to single bone only - it's used for statics and whenever something says that it might be used with static, it also might be used with skinned-to-single-bone
		};

		inline tchar const * to_char(Type const & _type)
		{
			if (_type == Static) return TXT("static");
			if (_type == Skinned) return TXT("skinned");
			if (_type == SkinnedToSingleBone) return TXT("skinned to single bone");
			return TXT("<unknown>");
		}

		inline bool is_static(Type const & _a)
		{
			return _a == Static;
		}

		inline bool is_skinned(Type const & _a)
		{
			return _a == Skinned ||
				   _a == SkinnedToSingleBone;
		}

		inline bool are_of_same_kind(Type const & _a, Type const & _b)
		{
			if (_a == Static && _b == Static)
			{
				return true;
			}
			if ((_a == Skinned ||
				 _a == SkinnedToSingleBone) &&
				(_b == Skinned ||
				 _b == SkinnedToSingleBone))
			{
				return true;
			}
			return false;
		}

		inline ::System::MaterialShaderUsage::Type usage_to_material_usage(Type const & _type)
		{
			if (_type == Usage::Static)
			{
				return ::System::MaterialShaderUsage::Static;
			}
			if (_type == Usage::Skinned)
			{
				return ::System::MaterialShaderUsage::Skinned;
			}
			if (_type == Usage::SkinnedToSingleBone)
			{
				return ::System::MaterialShaderUsage::SkinnedToSingleBone;
			}
			an_assert(false, TXT("unknown type"));
			return ::System::MaterialShaderUsage::Static;
		}
	};

};

DECLARE_SERIALISER_FOR(Meshes::Usage::Type);
