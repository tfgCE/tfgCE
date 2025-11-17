#pragma once

#include "fbxLib.h"

#include "..\..\core\mesh\animationSet.h"

#ifdef USE_FBX

#include "fbxSkeletonExtractor.h"

namespace FBX
{
	class Scene;

	class AnimationSetExtractor
	{
	public:
		static void initialise_static();
		static void close_static();

		static Meshes::AnimationSet* extract_from(Scene* _scene, Meshes::AnimationSetImportOptions const & _options = Meshes::AnimationSetImportOptions());

	protected:
		AnimationSetExtractor(Scene* _scene);
		~AnimationSetExtractor();

		void extract_animation_set(Meshes::AnimationSetImportOptions const & _options);

	private:
		Scene* scene = nullptr;

		Meshes::AnimationSet* animationSet = nullptr;

		BoneExtractor boneExtractor;
	};

};
#endif