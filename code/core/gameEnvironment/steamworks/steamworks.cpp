#include "steamworks.h"

#include "..\..\io\file.h"
#include "..\..\memory\memory.h"

#include "steamworksImpl.h"

//

using namespace GameEnvironment;

//

void Steamworks::init(tchar const* _appId)
{
	new Steamworks(_appId);
}

Steamworks::Steamworks(tchar const* _appId)
{
	{
		output(TXT("check app id file"));
		bool createAppIdFile = true;
		String fileName = /* no dir! we assume it is the running directory */ String(TXT("steam_appid.txt"));
		if (IO::File::does_exist(fileName))
		{
			{
				IO::File f;
				if (f.open(fileName))
				{
					String appId;
					f.set_type(IO::InterfaceType::Text);
					f.read_into_string(appId);
					if (appId == _appId)
					{ 
						output(TXT("matches, skip"));
						createAppIdFile = false;
					}
				}
			}
			if (createAppIdFile)
			{
				output(TXT("remove old one"));
				IO::File::delete_file(fileName);
			}
		}
		if (createAppIdFile)
		{
			output(TXT("create new one"));
			output(TXT("\"%S\""), _appId);
			IO::File f;
			f.create(fileName);
			f.set_type(IO::InterfaceType::Text);
			f.write_text(String(_appId));
		}
	}

	isOk = SteamworksImpl::init();
}

Steamworks::~Steamworks()
{
	if (isOk)
	{
		SteamworksImpl::shutdown();
	}
}

void Steamworks::set_achievement(Name const& _achievementName)
{
	if (isOk)
	{
		Array<char> achievementName8;
		_achievementName.to_string().to_char8_array(achievementName8);
		SteamworksImpl::set_achievement(achievementName8.get_data());
	}
}

tchar const* Steamworks::get_language()
{
	if (isOk)
	{
		char const * language8 = SteamworksImpl::get_language();

		String language = String::from_char8(language8);

		// this is hardcoded steam code -> standard/web code
		if (language == TXT("schinese")) return TXT("zh-CN");
		if (language == TXT("tchinese")) return TXT("zh-TW");
		if (language == TXT("bulgarian")) return TXT("bg");
		if (language == TXT("czech")) return TXT("cs");
		if (language == TXT("german")) return TXT("de");
		if (language == TXT("danish")) return TXT("da");
		if (language == TXT("dutch")) return TXT("nl");
		if (language == TXT("english")) return TXT("en");
		if (language == TXT("spanish")) return TXT("es");
		if (language == TXT("finnish")) return TXT("fi");
		if (language == TXT("french")) return TXT("fr");
		if (language == TXT("greek")) return TXT("el");
		if (language == TXT("hungarian")) return TXT("hu");
		if (language == TXT("italian")) return TXT("it");
		if (language == TXT("japanese")) return TXT("ja");
		if (language == TXT("korean")) return TXT("ko");
		if (language == TXT("koreana")) return TXT("ko");
		if (language == TXT("norwegian")) return TXT("no");
		if (language == TXT("polish")) return TXT("pl");
		if (language == TXT("portuguese")) return TXT("pt");
		if (language == TXT("brazilian")) return TXT("pt-BR");
		if (language == TXT("romanian")) return TXT("ro");
		if (language == TXT("russian")) return TXT("ru");
		if (language == TXT("swedish")) return TXT("sv");
		if (language == TXT("thai")) return TXT("th");
		if (language == TXT("turkish")) return TXT("tr");
		if (language == TXT("ukrainian")) return TXT("uk");
		if (language == TXT("vietnamese")) return TXT("vn");

		return nullptr; // leave it for system
	}
	return nullptr;
}

