#include "splashLogos.h"

//

#include "..\random\random.h"

#include "..\system\video\texture.h"
#include "..\system\video\imageLoaders\il_tga.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

struct SplashLogo
{
	tchar const* logo;
	int priority;
	int subOrder;
	bool other = false; // if set, won't be used on the logo list
	SplashLogo() {}
	SplashLogo(tchar const* _logo, int _priority, int _subOrder, bool _other = false) : logo(_logo), priority(_priority), subOrder(_subOrder), other(_other) {}

	static int compare(void const* _a, void const* _b)
	{
		SplashLogo const* a = plain_cast<SplashLogo>(_a);
		SplashLogo const* b = plain_cast<SplashLogo>(_b);
		if (a->priority < b->priority) return B_BEFORE_A;
		if (a->priority > b->priority) return A_BEFORE_B;
		if (a->subOrder < b->subOrder) return B_BEFORE_A;
		if (a->subOrder > b->subOrder) return A_BEFORE_B;
		return A_AS_B;
	}

	bool operator == (SplashLogo const & _other) const
	{
		return tstrcmp(logo, _other.logo) == 0;
	}
};

ArrayStatic<SplashLogo, 32> logos;

//

using namespace Splash;

//

void Logos::add(tchar const* _fileName, int _priority, bool _other)
{
	SET_EXTRA_DEBUG_INFO(logos, TXT("Logos.logos"));
	logos.push_back_unique(SplashLogo(_fileName, _priority, logos.get_size(), _other));
}

void Logos::load_logos(REF_ Array<::System::Texture*>& _splashTextures, REF_ Array<::System::Texture*>& _otherSplashTextures, bool _randomisedOrder)
{
	ArrayStatic<SplashLogo, 32> orderedLogos; SET_EXTRA_DEBUG_INFO(orderedLogos, TXT("Logos::load_logos.orderedLogos"));
	for_every(logo, logos)
	{
		if (!logo->other)
		{
			orderedLogos.push_back(*logo);
		}
	}

	if (_randomisedOrder)
	{
		Random::Generator rg;
		for_every(logo, orderedLogos)
		{
			logo->subOrder = rg.get_int();
		}
	}

	sort(orderedLogos);

	for_every(logo, orderedLogos)
	{
		String path = String::printf(TXT("system%S%S"), IO::get_directory_separator(), logo->logo);
		::System::TextureImportSetup tis;
		tis.ignoreIfNotAbleToLoad = true;
		tis.sourceFile = path;
		tis.forceType = TXT("tga");
		tis.autoMipMaps = true;
		tis.wrapU = System::TextureWrap::clamp;
		tis.wrapV = System::TextureWrap::clamp;
		::System::Texture* texture = new ::System::Texture();
		if (texture->init_with_setup(tis))
		{
			_splashTextures.push_back(texture);
		}
		else
		{
			delete texture;
		}
	}

	for_every(logo, logos)
	{
		if (logo->other)
		{
			String path = String::printf(TXT("system%S%S"), IO::get_directory_separator(), logo->logo);
			::System::TextureImportSetup tis;
			tis.ignoreIfNotAbleToLoad = true;
			tis.sourceFile = path;
			tis.forceType = TXT("tga");
			tis.autoMipMaps = true;
			tis.wrapU = System::TextureWrap::clamp;
			tis.wrapV = System::TextureWrap::clamp;
			::System::Texture* texture = new ::System::Texture();
			if (texture->init_with_setup(tis))
			{
				_otherSplashTextures.push_back(texture);
			}
			else
			{
				delete texture;
			}
		}
	}
}

void Logos::unload_logos(REF_ Array<::System::Texture*>& _splashTextures, REF_ Array<::System::Texture*>& _otherSplashTextures)
{
	for_every_ptr(texture, _splashTextures)
	{
		delete texture;
	}
	_splashTextures.clear();
	for_every_ptr(texture, _otherSplashTextures)
	{
		delete texture;
	}
	_otherSplashTextures.clear();
}
