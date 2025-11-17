#pragma once

#include "..\..\core\memory\pooledObject.h"
#include "..\..\core\memory\refCountObject.h"

namespace Framework
{
	class SoundSource;

	class SoundSourceModifier
	: public PooledObject<SoundSourceModifier>
	, public RefCountObject
	{
	public:
		SoundSourceModifier(SoundSource* _soundSource);
		virtual ~SoundSourceModifier();

		virtual void advance(float _deltaTime) {}

		SoundSource & access_sound_source() { return *soundSource; }

	private: friend class SoundSource;
		void added_to(SoundSource* _soundSource);
		void removed_from(SoundSource* _soundSource);

	private:
		SoundSource* soundSource;
	};

};
