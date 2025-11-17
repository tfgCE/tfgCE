#include "teaForGodMain.h"

#include "teaRegisteredTypes.h"

#include "artDir.h"

#include "ai\aiCommon.h"
#include "ai\aiManagerData.h"

#include "gameScript\registeredGameScriptConditions.h"
#include "gameScript\registeredGameScriptElements.h"

#include "game\game.h"
#include "game\gameConfig.h"
#include "game\gameObjectPtrs.h"
#include "game\gameSettings.h"

#include "..\framework\game\gameMode.h"

#include "library\gameDefinition.h"
#include "library\gameModifier.h"

#include "regionGenerators\regionGeneratorBase.h"
#include "roomGenerators\roomGenerationInfo.h"

#include "serialisation\stringFormatterSerialisationHandler.h"

#include "..\core\mainConfig.h"

#include "ai\perceptionRequests\aipr_FindInVisibleRooms.h"

#include "meshGen\shapes\meshGenShapes.h"

#include "..\core\mainSettings.h"

#include "..\core\gameEnvironment\iGameEnvironment.h"


#ifdef AN_STEAM
#include "..\core\gameEnvironment\steamworks\steamworks.h"
#endif
#ifdef AN_VIVEPORT
#include "..\core\gameEnvironment\viveport\viveport.h"
#endif
#ifdef AN_OCULUS
#include "..\core\gameEnvironment\oculus\oculus_ge.h"
#endif
#ifdef AN_PICO
#include "..\core\gameEnvironment\pico\pico.h"
#endif

//

#include "..\core\collision\queryPrimitivePoint.h"
#include "..\core\collision\shapeSphere.h"

#include "..\core\buildInformation.h"
#include "..\core\coreInit.h"
#include "..\core\io\assetFile.h"
#include "..\core\other\parserUtils.h"
#include "..\core\splash\splashInfo.h"
#include "..\core\splash\splashLogos.h"
#include "..\core\splash\splashScreen.h"
#include "..\core\system\core.h"
#include "..\core\system\faultHandler.h"
#include "..\core\system\sound\soundSystem.h"
#include "..\core\system\video\shaderProgramCache.h"

#include "..\framework\debug\previewGame.h"
#include "..\framework\jobSystem\jobSystem.h"

#include "..\framework\loaders\loaderCircles.h"
#include "..\framework\loaders\loaderZX.h"
#include "..\framework\loaders\loaderVoidRoom.h"
#include "..\framework\loaders\loaderText.h"
#include "..\framework\loaders\loaderLoadingLogo.h"
#include "..\framework\loaders\loaderSequence.h"
#include "..\framework\loaders\loaderEmpty.h"

#include "..\framework\text\localisedCharacters.h"

#include "..\tools\tools.h"

#include "ai\logics\aiLogics.h"

#include "library\library.h"

#include "modules\modules.h"

#include "loaders\loaderArmActivator.h"
#include "loaders\loaderDisplay.h"
#include "loaders\loaderPlatformerGame.h"
#include "loaders\loaderStarField.h"
#include "loaders\hub\loaderHub.h"
#include "loaders\hub\loaderHubInSequence.h"
#include "loaders\hub\scenes\lhs_beAtRightPlace.h"
#include "loaders\hub\scenes\lhs_comfortMessageAndBasicOptions.h"
#include "loaders\hub\scenes\lhs_empty.h"
#include "loaders\hub\scenes\lhs_handyMessages.h"
#include "loaders\hub\scenes\lhs_test.h"
#include "loaders\hub\scenes\lhs_text.h"
#include "loaders\hub\scenes\lhs_message.h"
#include "loaders\hub\scenes\lhs_question.h"
#include "loaders\hub\scenes\lhs_vrModeSelection.h"
#include "loaders\hub\scenes\lhs_waitForScreen.h"
#include "loaders\hub\screens\lhc_handGestures.h"

#include "appearance\appearanceControllers.h"

#include "wheresMyPoint\wmp_tools.h"

#include "game\energy.h"

#include "reportBug.h"

#include "..\core\network\http.h"

// test includes BEGIN

// test bhaptics
#include "..\core\physicalSensations\physicalSensations.h"
#include "..\core\physicalSensations\bhapticsJava\bhj_bhapticsModule.h"

// test includes END

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

#ifdef AN_DEVELOPMENT_OR_PROFILER
#define AN_QUICK_START
#endif

#ifdef BUILD_NON_RELEASE
#define AN_QUICK_START_SKIP_SPLASH
#endif

//

// fading reasons
DEFINE_STATIC_NAME(quittingGame);

// screens
DEFINE_STATIC_NAME(loading);

// localised strings
DEFINE_STATIC_NAME_STR(lsLoading, TXT("hub; common; loading"));
DEFINE_STATIC_NAME_STR(lsPlayAreaNotDetected, TXT("hub; common; play area not detected"));
DEFINE_STATIC_NAME_STR(lsPlayAreaEnlarged, TXT("hub; common; play area enlarged"));
DEFINE_STATIC_NAME_STR(lsPlayAreaForcedScaled, TXT("hub; common; play area forced scaled"));
DEFINE_STATIC_NAME_STR(lsBoundaryUnavailable, TXT("hub; common; boundary unavailable"));
DEFINE_STATIC_NAME_STR(lsMayHaveInvalidBoundary, TXT("hub; common; may have invalid boundary"));
DEFINE_STATIC_NAME_STR(lsSoundNotInitialised, TXT("hub; common; sound not initialised"));
DEFINE_STATIC_NAME_STR(lsReportBugsQuestion, TXT("hub; common; report bugs question"));
//DEFINE_STATIC_NAME_STR(lsNewBuildAvailable, TXT("hub; common; new build available"));
//DEFINE_STATIC_NAME_STR(lsNewBuildAvailableYouHave, TXT("hub; common; new build available; you have"));
DEFINE_STATIC_NAME_STR(lsPleaseWait, TXT("hub; common; please wait"));
DEFINE_STATIC_NAME_STR(lsGameUnavailable, TXT("hub; game unavailable"));
	DEFINE_STATIC_NAME(gameEnvironment);
DEFINE_STATIC_NAME_STR(lsGameChecking, TXT("hub; game checking"));

// colours
DEFINE_STATIC_NAME(loader_hub_fade);

// shader options
DEFINE_STATIC_NAME(retro);

// video options
DEFINE_STATIC_NAME(bloom);

//

struct TeaForGodEmperor::CheckGameEnvironment
{
	bool confirmedOk = false;

	TeaForGodEmperor::Game* game = nullptr;
	tchar const* name = TXT("game environment");

	CheckGameEnvironment(bool _ignoreCheck)
	{
		if (_ignoreCheck)
		{
			// won't create game environment, will succeed
			return;
		}
#ifndef BUILD_PUBLIC_RELEASE
		if (false) // do game environment checks only in public releases, to test: comment this line
#endif
		{
#ifdef AN_STEAM
			{
				name = TXT("steam");
				GameEnvironment::Steamworks::init(
#ifdef BUILD_DEMO
					TXT("2075700")
#else
					TXT("1764400")
#endif
				);
			}
#endif
#ifdef AN_VIVEPORT
			{
				name = TXT("viveport");
				GameEnvironment::Viveport::init(
#ifdef AN_WINDOWS
#ifdef BUILD_DEMO
					TXT("f6f66cf2-5045-41e3-b033-f279ce37d80e"), TXT("MIGfMA0GCSqGSIb3DQEBAQUAA4GNADCBiQKBgQDGEH5wNmtgqyud9doB/UwsFHBBoNvH91a1Fiv6MUp5pWEa07CdgCbA3YqEZk7BfidDWTtvkOxUNRWetCkds+ia9YO4TAo5tZ/PJaT/TJyw4ZaTtS1YJWP/TFTLitYfUxAFVFs/Tzke8V+nRZ7G24qig6QD2qW4k+QKE+xf58jUkwIDAQAB")
#else
					TXT("5e1dacd1-efe7-4091-8745-1de90fdf74bc"), TXT("MIGfMA0GCSqGSIb3DQEBAQUAA4GNADCBiQKBgQCX55xwpUMQUNCEfBvNEfl/OuSMmDvRmvPB3M8QUdYv18fU4TW+42rh13FRYTWfFsMZT9nnsc8kXCSUdp5dcJFosFh18PWfxWeVrszi1Jv02RBTzSfV6HRN5TDrbMjrtnVZI5zoWf21rH2De6BpSBN/poBPRjE6xE0Y5topAHpUcQIDAQAB")
#endif
#else
#ifdef BUILD_DEMO
					TXT("f4c0883e-cf72-4962-81a9-511cfdc73ab2"), TXT("MIGfMA0GCSqGSIb3DQEBAQUAA4GNADCBiQKBgQCSvdZf019+KSeBMOeMEjI7wlX1UMgbbHN7LsjmNTobjvUDWRvkAhWXrsvxfyD86OOU17tx6f5Gl7X5Leb6KFOv+clPRNOmDINOvpA5BDGGF74ka3Vb+i+FTU7sKA6INgXJZqeyy1kHfkfOe+kYNSc1HtWIddEk3PAXxGCvGhWplwIDAQAB")
#else
					TXT("03cd1a1c-4999-4712-957c-b074e7acc15c"), TXT("MIGfMA0GCSqGSIb3DQEBAQUAA4GNADCBiQKBgQCV50T9osjJ4JMtUmXNn3UkbVOdrQ6YKDnAxSA04WmqKzd5/E3uofaIgA/s+BleeG6+vK9+ztSYs5mvFBKGmeXJ6s3FOIF5fcMjC2l8CaF++cDtFXCD8W9Uqlmn/ctnoLGqGvEkFtg3zO1NCDDhHWKm1TeEC4ijgMX0sBdse+cA2wIDAQAB")
#endif
#endif
				);
			}
#endif
#ifdef AN_OCULUS
			{
				name = TXT("oculus");
				GameEnvironment::Oculus::init();
			}
#endif
#ifdef AN_PICO
			{
				name = TXT("pico");
				GameEnvironment::Pico::init();
			}
#endif
		}
	}

	bool check()
	{
		if (!confirmedOk)
		{
			if (auto* ge = GameEnvironment::IGameEnvironment::get())
			{
				auto isOk = ge->is_ok();
				if (isOk.is_set())
				{
					if (isOk.get())
					{
						confirmedOk = true;
					}
					else
					{
						String message = String(TXT("Game unavailable for this account")); // hack when the check is done before the game is loaded (eg. Steam)
						if (Framework::LocalisedStrings::get_entry(NAME(lsGameUnavailable), true))
						{
							message = Framework::StringFormatter::format_loc_str(NAME(lsGameUnavailable), Framework::StringFormatterParams()
								.add(NAME(gameEnvironment), String(name)));
						}
						error(TXT("[gameEnv init] %S"), message.to_char());
						if (game)
						{
							game->show_hub_scene((new Loader::HubScenes::Message(message))->requires_acceptance());
							::System::Core::quick_exit();
						}
						else
						{
#ifdef AN_WINDOWS

							MessageBox(nullptr, message.to_char(), TXT("Problem!"), MB_ICONEXCLAMATION | MB_OK);
							::System::Core::quick_exit();
#endif
						}
					}
				}
			}
			else
			{
				// no game engine - good to be
				confirmedOk = true;
			}
		}
		return confirmedOk;
	}
};

//

Loader::VoidRoomLoaderType::Type loaderType =
	Loader::VoidRoomLoaderType::None
	;

#ifdef CHECK_BUILD_AVAILABLE
struct DownloadLatestBuildVersionData
{
	String* buildAvailable;
	bool* processed;
	DownloadLatestBuildVersionData(String* _buildAvailable, bool* _processed) : buildAvailable(_buildAvailable), processed(_processed) {}
};

void download_latest_build_version(void* _data)
{
	DownloadLatestBuildVersionData* data = (DownloadLatestBuildVersionData*)_data;
	tchar const* addr =
#ifdef AN_ANDROID
		TXT("http://voidroom.com/download/teaQuest.buildInfo")
#else
		TXT("http://voidroom.com/download/tea.buildInfo")
#endif
		;
	output(TXT("[check latest build version]"));
	// check for build info
	String response = Network::HTTP::get(addr);
	List<String> lines;
	response.split(String(TXT("\n")), lines);
	if (lines.get_size() < 4)
	{
		error(TXT("[check latest build version] invalid build info received"));
	}
	else
	{
		output(TXT("[check latest build version] got available build version"));
		int versionMajor = ParserUtils::parse_int(lines[0]);
		int versionMinor = ParserUtils::parse_int(lines[1]);
		int versionSubMinor = ParserUtils::parse_int(lines[2]);
		int versionBuildNo = ParserUtils::parse_int(lines[3]);
		output(TXT("[check latest build version] %i.%i.%i (#%i)"), versionMajor, versionMinor, versionSubMinor, versionBuildNo);
		if (versionBuildNo > get_build_version_build_no())
		{
			*(data->buildAvailable) = get_build_version_short_custom(versionMajor, versionMinor, versionSubMinor, versionBuildNo);
		}
	}
	*(data->processed) = true;
	delete data;
}
#endif

#ifdef AN_CHECK_MEMORY_LEAKS
struct ScopedMemoryReport
{
	size_t memoryAllocated0 = 0;
	size_t memoryAllocated1 = 0;

	ScopedMemoryReport()
	{
		memoryAllocated0 = get_memory_allocated_checked_in();
	}

	~ScopedMemoryReport()
	{
		memoryAllocated1 = get_memory_allocated_checked_in();

		float amount = (float)(memoryAllocated1 - memoryAllocated0);
		output(TXT("memory report, allocated: %.2fMB (%.1fkB)"), amount / 1024.0f / 1024.0f, amount / 1024.0f);
	}
};

#define SCOPED_MEMORY_REPORT ScopedMemoryReport scopedMemoryReport;
#else
#define SCOPED_MEMORY_REPORT
#endif

REMOVE_AS_SOON_AS_POSSIBLE_
#ifdef PHYSICAL_SENSATIONS_BHAPTICS_JAVA
void test_bhaptics(void* _data = nullptr)
{	// test bhaptics
	output(TXT("[!@#] test bhaptics"));
	output(TXT("[!@#] create/get BhapticsModule"));
	bhapticsJava::BhapticsModule* bhm = new bhapticsJava::BhapticsModule();
	output(TXT("[!@#] created BhapticsModule"));
	{
		while (true)
		{
			{
				System::Core::sleep(1.0f);
				bhm->update_devices_with_pending_devices();
				Concurrency::ScopedSpinLock lock(bhm->access_devices_lock());
				for_every(device, bhm->get_devices())
				{
					if (device->isPaired &&
						device->isConnected)
					{
						bhm->ping(device->address);
					}
				}
			}
		}
	}
	output(TXT("[!@#] stop scanning"));
	delete bhm;
	output(TXT("[!@#] DONE"));
}
#endif

void TeaForGodEmperor::Main::init()
{
	TeaForGodEmperor::determine_demo();

	disallow_output();

#ifdef AN_CHECK_MEMORY_LEAKS
	mark_main_thread_running();
#endif
	::System::Core::set_app_name(TEA_FOR_GOD_NAME);

	Splash::Info::initialise_static();
	if (!silentRun)
	{
		Splash::Info::add_info(String::printf(TEA_FOR_GOD_NAME), 1, 1, 1, 1);
		Splash::Info::add_info(String::printf(TXT("by void room (2016-2024)")), 0, 0, 0, 1);
		Splash::Info::add_info(String::printf(TXT("build: %S"), get_build_information()), 0, 0, 0, 1);
		if (TeaForGodEmperor::is_demo())
		{
			Splash::Info::add_info(String::printf(TXT("(demo mode)")), 0, 0, 0, 1);
		}

		Splash::Screen::show();
	}

	core_initialise_static();

	Splash::Logos::add(TXT("credits"), 0, true);
}

void TeaForGodEmperor::Main::pre_init_game()
{
	if (::System::Core::restartRequired)
	{
		core_restart_static();
	}

	startGame = false;
	::System::Core::restartRequired = false;

	if (silentRun)
	{
		// do not start VR when dealing with
		MainConfig::access_global().vr_not_needed_at_all();
	}

	if (!forceVR.is_empty())
	{
		MainConfig::access_global().force_vr(forceVR);
	}

	System::FaultHandler::set_report_bug_log(TeaForGodEmperor::ReportBug::send_crash);

	runNormally = true;

	if (createCodeInfo || testWithCodeInfo)
	{
		allow_output();

		Splash::Info::send_to_output();
		output(TXT("now: %S"), Framework::DateTime::get_current_from_system().get_string_system_date_time().to_char());

		output(TXT("create code info"));

		IO::File file;
		if (file.create(codeInfoFilePath))
		{
			System::FaultHandler::create_code_info(file);
		}

		if (createCodeInfo)
		{
			runNormally = false;
		}
	}
}

void TeaForGodEmperor::Main::init_system_prior_to_game()
{
	Tools::initialise_static();

	Framework::initialise_static();

	NAME_POOLED_OBJECT(TeaForGodEmperor::AI::PerceptionRequests::FindInVisibleRooms);

	// register StringFormatterSerialisationHandler
	Framework::StringFormatterSerialisationHandlers::add_serialisation_handler(new TeaForGodEmperor::StringFormatterSerialisationHandler());

	TeaForGodEmperor::RegisteredTypes::initialise_static();
	TeaForGodEmperor::GameConfig::initialise_static();
	TeaForGodEmperor::Modules::initialise_static();
	TeaForGodEmperor::AppearanceControllers::initialise_static();
	TeaForGodEmperor::RegionGenerators::Base::initialise_static();
	TeaForGodEmperor::RoomGenerationInfo::initialise_static();
	TeaForGodEmperor::GameScript::RegisteredScriptConditions::initialise_static();
	TeaForGodEmperor::GameScript::RegisteredScriptElements::initialise_static();
	TeaForGodEmperor::LootSelector::initialise_static();
	TeaForGodEmperor::ProjectileSelector::initialise_static();
	TeaForGodEmperor::GameModifier::initialise_static();
	TeaForGodEmperor::GameDefinition::initialise_static();
	TeaForGodEmperor::WheresMyPointTools::initialise_static();
	TeaForGodEmperor::AI::Common::initialise_static();
	TeaForGodEmperor::AI::ManagerDatas::initialise_static();
	TeaForGodEmperor::AI::Logics::Managers::GlobalSpawnManagerInfo::initialise_static();
	TeaForGodEmperor::WeaponStatAffection::initialise_static();
	TeaForGodEmperor::PilgrimageInstance::initialise_static();

	TeaForGodEmperor::MeshGeneration::Shapes::add_shapes();

	allow_output();

	Splash::Info::send_to_output();
	output(TXT("now: %S"), Framework::DateTime::get_current_from_system().get_string_system_date_time().to_char());

	System::FaultHandler::output_modules_base_address(true);
	System::Core::sleep(0.1f);

	TeaForGodEmperor::AI::Logics::All::register_static();

	// check auto folder, if build number differs, remove auto folder altogether
	IO::validate_auto_directory();
}

void TeaForGodEmperor::Main::log_registered_types()
{
#ifdef AN_DEVELOPMENT
	LogInfoContext log;
	RegisteredType::log_all(log);
	log.output_to_output();
#endif
}

void TeaForGodEmperor::Main::test_code_info()
{
	Array<uint64> checkRelAddresses;
	//
	// put all the numbers you want to check here
	//
	int buildNo = 212;
	checkRelAddresses.push_back(0x0046C74B);
	//
	IO::File file;
	String fileName = String::printf(TXT("..\\..\\_pdbs\\tea_%i.codeInfo"), buildNo);
	if (file.open(fileName))
	{
		output(TXT("loading code info from \"%S\""), fileName.to_char());
		System::FaultHandler::load_code_info(file);
		for_every(relAddr, checkRelAddresses)
		{
			String info = String::printf(TXT("%p : "), *relAddr);
			info += System::FaultHandler::get_info(*relAddr, false);
			output(TXT("%S"), info.to_char());
		}
	}
	AN_BREAK;
	::System::Core::quick_exit(true);
}

void TeaForGodEmperor::Main::close_system_post_game()
{
	output(TXT("close low level systems..."));
	TeaForGodEmperor::PilgrimageInstance::close_static();
	TeaForGodEmperor::WeaponStatAffection::close_static();
	TeaForGodEmperor::AI::Logics::Managers::GlobalSpawnManagerInfo::close_static();
	TeaForGodEmperor::AI::ManagerDatas::close_static();
	TeaForGodEmperor::AI::Common::close_static();
	TeaForGodEmperor::GameDefinition::close_static();
	TeaForGodEmperor::GameModifier::close_static();
	TeaForGodEmperor::ProjectileSelector::close_static();
	TeaForGodEmperor::LootSelector::close_static();
	TeaForGodEmperor::GameScript::RegisteredScriptElements::close_static();
	TeaForGodEmperor::GameScript::RegisteredScriptConditions::close_static();
	TeaForGodEmperor::RoomGenerationInfo::close_static();
	TeaForGodEmperor::RegionGenerators::Base::close_static();
	TeaForGodEmperor::RegisteredTypes::close_static();

	output(TXT("...framework"));
	Framework::close_static();

	output(TXT("...tools"));
	Tools::close_static();
}

void TeaForGodEmperor::Main::close()
{
	allow_output();

	output(TXT("close core"));

	core_close_static();

	output(TXT("EOL"));

	RegisteredPool::prune_all();
}

void TeaForGodEmperor::Main::load_code_info()
{
	output(TXT("check for code info file"));

	if (!createCodeInfo || testWithCodeInfo)
	{
		IO::File file;
		if (file.open(codeInfoFilePath))
		{
			System::FaultHandler::load_code_info(file);
		}
	}
}

void TeaForGodEmperor::Main::load_localised_characters()
{
	// prefer user dir
	for_count(int, i, 2)
	{
		IO::Interface* io = nullptr;
		IO::File f;
		if (i == 0)
		{
			if (f.open(IO::get_user_game_data_directory() + localisedCharactersFilePath))
			{
				io = &f;
			}
		}
#ifdef AN_ANDROID
		IO::AssetFile af;
		if (i == 1)
		{
			if (af.open(IO::get_asset_data_directory() + localisedCharactersFilePath))
			{
				io = &af;
			}
		}
#endif
		if (io)
		{
			io->set_type(IO::InterfaceType::UTF8);
			Framework::LocalisedCharacters::load_from(io);
			break;
		}
	}
}

void TeaForGodEmperor::Main::store_descriptions()
{
	struct WriteText
	{
		IO::File* f;
		bool fakeLineSeparatorsAreReal = false;
		void main_header(tchar const* _text)
		{
			f->write_text(TXT("========================================================= "));
			f->write_text(_text);
			f->write_next_line();
		}
		void header(Name const & _language,  tchar const* _text)
		{
			f->write_text(TXT("------------------- "));
			f->write_text(_language.to_char());
			f->write_text(TXT(" : "));
			f->write_text(_text);
			f->write_next_line();
		}
		void text(String const& _text)
		{
			String text = _text;
			while (!text.is_empty() &&
				text.get_data()[text.get_length() - 1] == '~')
			{
				text = text.get_left(text.get_length() - 1);
			}
			if (fakeLineSeparatorsAreReal)
			{
				while (!text.is_empty())
				{
					int at = text.find_first_of('~');
					if (at != NONE)
					{
						f->write_text(text.get_left(at));
						f->write_next_line();
						text = text.get_sub(at + 1);
					}
					else
					{
						f->write_text(text);
						break;
					}
				}
			}
			else
			{
				f->write_text(text.to_char());
			}
		}
		void fake_line_separator()
		{
			if (fakeLineSeparatorsAreReal)
			{
				f->write_next_line();
			}
			else
			{
				f->write_text(TXT("~"));
			}
		}
		void next_line()
		{
			f->write_next_line();
		}

		String bold_first_line(String const& _text)
		{
			int at = _text.find_first_of('~');
			if (at == NONE)
			{
				String result;
				result += TXT("[h2]");
				result += _text;
				result += TXT("[/h2]");
				return result;
			}
			else
			{
				String result;
				result += TXT("[h2]");
				result += _text.get_left(at);
				result += TXT("[/h2]");
				result += _text.get_sub(at);
				return result;
			}
		}
		String into_list(String const& _text, int _first, int _last)
		{
			String text = _text;
			String result;
			int at = 0;
			int lineIdx = 0;
			while (lineIdx < _first)
			{
				at = text.find_first_of('~');
				if (at == NONE)
				{
					break;
				}
				result += text.get_left(at + 1);
				text = text.get_sub(at + 1);
				++lineIdx;
			}
			result += TXT("[list]~");
			while (lineIdx <= _last)
			{
				at = text.find_first_of('~');
				if (at == NONE)
				{
					break;
				}
				result += TXT("[*]");
				result += text.get_left(at + 1);
				text = text.get_sub(at + 1);
				++lineIdx;
			}
			result += TXT("[/list]~");
			result += text;
			return result;
		}
		String first_sentence_into_italics(String const& _text, int _first, int _last)
		{
			String text = _text;
			String result;
			int at = 0;
			int lineIdx = 0;
			while (lineIdx < _first)
			{
				at = text.find_first_of('~');
				if (at == NONE)
				{
					break;
				}
				result += text.get_left(at + 1);
				text = text.get_sub(at + 1);
				++lineIdx;
			}
			while (lineIdx <= _last)
			{
				at = text.find_first_of('~');
				if (at == NONE)
				{
					break;
				}
				{
					String subText = text.get_left(at + 1);
					int at2 = subText.find_first_of('.');
					result += TXT("[i]");
					result += subText.get_left(at2+1);
					result += TXT("[/i]");
					result += subText.get_sub(at2 + 1);
				}
				text = text.get_sub(at + 1);
				++lineIdx;
			}
			result += text;
			return result;
		}
		String double_lines(String const& _text)
		{
			String result;
			String text = _text;
			while (!text.is_empty())
			{
				int at = text.find_first_of('~');
				if (at != NONE)
				{
					result += text.get_left(at);
					result += TXT("~~");
					text = text.get_sub(at + 1);
				}
				else
				{
					break;
				}
			}
			result += text;
			return result;
		}
		String put_at(String const& _text, int _at, String const& _add)
		{
			int at = 0;
			for_count(int, i, _at + 1)
			{
				at = _text.find_first_of('~', at);
				if (at == NONE)
				{
					break;
				}
			}
			if (at == NONE)
			{
				String result = _text;
				result += TXT("~");
				result += _add;
				return result;
			}
			else
			{
				String result = _text.get_left(at + 1); // with fake line separator
				result += _add;
				result += _text.get_sub(at); // with fake line separator
				return result;
			}
		}
	};
	Name currLang = Framework::LocalisedStrings::get_language();
	IO::File f;
	f.create(IO::get_user_game_data_directory() + IO::get_auto_directory_name() + IO::get_directory_separator() + String(TXT("store desc all.txt")));
	f.set_type(IO::InterfaceType::UTF8);
	WriteText write;
	write.f = &f;

	for_every(lang, Framework::LocalisedStrings::get_languages())
	{
		Framework::LocalisedStrings::set_language(lang->id);
		{
			write.main_header(lang->id.to_char());
		}
		{
			write.header(lang->id, TXT("title"));
			write.text(LOC_STR(Name(TXT("! game desc; ! title"))));
			write.next_line();
			write.next_line();
		}
		{
			write.header(lang->id, TXT("title-demo"));
			write.text(LOC_STR(Name(TXT("! game desc; ! title demo"))));
			write.next_line();
			write.next_line();
		}
		{
			write.header(lang->id, TXT("log-line"));
			write.text(LOC_STR(Name(TXT("! game desc; log line"))));
			write.next_line();
			write.next_line();
		}
		{
			write.header(lang->id, TXT("log-line (steam < 300)"));
			String logLine = LOC_STR(Name(TXT("! game desc; log line")));
			while (logLine.get_length() > 300)
			{
				int at = logLine.find_last_of('.', logLine.get_length() - 2);
				if (at != NONE)
				{
					logLine = logLine.get_left(at + 1);
				}
				else
				{
					break;
				}
			}
			write.text(logLine);
			write.next_line();
			write.next_line();
		}
		{
			write.header(lang->id, TXT("<300 (pico brief)"));
			write.text(LOC_STR(Name(TXT("! game desc; short to 300"))));
			write.next_line();
			write.next_line();
		}
		{
			write.fakeLineSeparatorsAreReal = true;
			write.header(lang->id, TXT("detailed long (steam)"));
			{
				String text = LOC_STR(Name(TXT("! game desc; long to 1000")));
				text = write.put_at(text, 0, String(TXT("[img]{STEAM_APP_IMAGE}/extras/earth.jpg[/img]")));
				text = write.double_lines(text);
				write.text(text);
			}
			write.fake_line_separator();
			write.fake_line_separator();
			write.text(String(TXT("[img]{STEAM_APP_IMAGE}/extras/airfighters.jpg[/img]")));
			write.fake_line_separator();
			write.fake_line_separator();
			{
				String text = LOC_STR(Name(TXT("! game desc; desc; key features")));
				text = write.bold_first_line(text);
				text = write.into_list(text, 1, 1000);
				write.text(text);
			}
			write.fake_line_separator();
			write.fake_line_separator();
			write.text(String(TXT("[img]{STEAM_APP_IMAGE}/extras/engine_room.jpg[/img]")));
			write.fake_line_separator();
			write.fake_line_separator();
			{
				String text = LOC_STR(Name(TXT("! game desc; desc; impossible spaces")));
				text = write.bold_first_line(text);
				write.text(text);
			}
			write.fake_line_separator();
			write.fake_line_separator();
			{
				String text = LOC_STR(Name(TXT("! game desc; desc; procedural generation")));
				text = write.bold_first_line(text);
				write.text(text);
			}
			write.fake_line_separator();
			write.fake_line_separator();
			{
				String text = LOC_STR(Name(TXT("! game desc; desc; play area")));
				text = write.bold_first_line(text);
				write.text(text);
			}
			write.fake_line_separator();
			write.fake_line_separator();
			write.text(String(TXT("[img]{STEAM_APP_IMAGE}/extras/map.jpg[/img]")));
			write.fake_line_separator();
			write.fake_line_separator();
			{
				String text = LOC_STR(Name(TXT("! game desc; desc; customizable experience")));
				text = write.bold_first_line(text);
				// change in reverse order!
				text = write.first_sentence_into_italics(text, 2, 4);
				text = write.into_list(text, 6, 8);
				text = write.into_list(text, 2, 4);
				write.text(text);
			}
			write.fake_line_separator();
			write.fake_line_separator();
			write.text(String(TXT("[img]{STEAM_APP_IMAGE}/extras/pesters.jpg[/img]")));
			write.fake_line_separator();
			write.fake_line_separator();
			{
				String text = LOC_STR(Name(TXT("! game desc; desc; world and story")));
				text = write.bold_first_line(text);
				write.text(text);
			}
			write.next_line();
			write.next_line();
			write.fakeLineSeparatorsAreReal = false;
		}
		{
			write.fakeLineSeparatorsAreReal = true;
			write.header(lang->id, TXT("detailed long (oculus, pico, vive)"));
			{
				String text = LOC_STR(Name(TXT("! game desc; long to 1000")));
				text = write.double_lines(text);
				write.text(text);
			}
			write.next_line();
			write.next_line();
			write.fakeLineSeparatorsAreReal = false;
		}
		{
			write.fakeLineSeparatorsAreReal = true;
			write.header(lang->id, TXT("detailed long demo (oculus, itch, pico, vive)"));
			{
				String text = LOC_STR(Name(TXT("! game desc; long to 1000")));
				text = write.double_lines(text);
				write.text(text);
			}
			write.fake_line_separator();
			write.fake_line_separator();
			write.text(LOC_STR(Name(TXT("! game desc; demo info"))));
			write.next_line();
			write.next_line();
			write.fakeLineSeparatorsAreReal = false;
		}
		write.next_line();
		write.next_line();
		write.next_line();
	}
	f.close();

	// output Oculus Descriptions TSV
	struct OutputOculusTSV
	{
		static String trim_end_line(String txt)
		{
			while (txt.get_sub(txt.get_length() - 1) == TXT("~"))
			{
				txt = txt.get_left(txt.get_length() - 1);
			}
			return txt;
		}
		static void perform(String const & fileName, bool demo, bool quest)
		{
			IO::File f;
			f.create(IO::get_user_game_data_directory() + IO::get_auto_directory_name() + IO::get_directory_separator() + fileName);
			f.set_type(IO::InterfaceType::UTF8);
			
			f.write_text(TXT("locale\tApp Name\tShort Description\tLong Description\tSearch Keyword 1\tSearch Keyword 2\tSearch Keyword 3\tSearch Keyword 4\tSearch Keyword 5"));
			f.write_line();

			for_every(lang, Framework::LocalisedStrings::get_languages())
			{
				Framework::LocalisedStrings::set_language(lang->id);
				{
					String locale = LOC_STR(Name(TXT("! language code; oculus")));
					if (locale == TXT("_") ||
						locale.is_empty())
					{
						continue;
					}

					f.write_text(locale.to_char());
					f.write_text(TXT("\t"));
				}
				{
					String txt = LOC_STR(Name(demo? TXT("! game desc; ! title demo") : TXT("! game desc; ! title")));
					txt = trim_end_line(txt);
					f.write_text(txt.to_char());
					f.write_text(TXT("\t"));
				}
				{
					String txt = LOC_STR(Name(TXT("! game desc; short to 500")));
					txt = trim_end_line(txt);
					f.write_text(TXT("\""));
					f.write_text(txt.replace(String(TXT("~")), String(TXT("\n\n"))));
					f.write_text(TXT("\""));
					f.write_text(TXT("\t"));
				}
				{
					String txt = LOC_STR(Name(TXT("! game desc; long to 1000")));
					txt = trim_end_line(txt);
					f.write_text(TXT("\""));
					f.write_text(txt.replace(String(TXT("~")), String(TXT("\n\n"))));
					if (demo)
					{
						f.write_text(String(TXT("\n\n")));
						String demotxt = quest? LOC_STR(Name(TXT("! game desc; demo info quest"))) : LOC_STR(Name(TXT("! game desc; demo info")));
						demotxt = trim_end_line(demotxt);
						f.write_text(demotxt.replace(String(TXT("~")), String(TXT("\n\n"))));
					}
					f.write_text(TXT("\""));
					f.write_text(TXT("\t"));
				}

				f.write_line();
			}

			f.close();
		}
	};

	OutputOculusTSV::perform(String(TXT("store desc oculus demo.tsv")), true, false);
	OutputOculusTSV::perform(String(TXT("store desc oculus.tsv")), false, false);
	OutputOculusTSV::perform(String(TXT("store desc oculus quest demo.tsv")), true, true);
	OutputOculusTSV::perform(String(TXT("store desc oculus quest.tsv")), false, true);

	Framework::LocalisedStrings::set_language(currLang);
}

void TeaForGodEmperor::Main::update_localised_characters()
{
	Framework::LocalisedCharacters::update_from_localised_strings();

	// store in memory as UTF8
	IO::MemoryStorage ms;
	ms.set_type(IO::InterfaceType::UTF8);
	Framework::LocalisedCharacters::save_to(&ms);

	// compare with existing file to check if we should update it, do it by comparing binary data

	bool requiresSaving = true;

	int const bufferSize = 256;

	// store always to user game data directory, we can't store to asset

	{
		IO::File f;
		if (f.open(IO::get_user_game_data_directory() + localisedCharactersFilePath))
		{
			requiresSaving = false;
			ms.set_type(IO::InterfaceType::Binary);
			f.set_type(IO::InterfaceType::Binary);

			ms.go_to(0);

			// compare files
			byte msData[bufferSize];
			byte fData[bufferSize];
			while (true)
			{
				int msRead = ms.read(msData, bufferSize);
				int fRead = f.read(fData, bufferSize);

				if (msRead != fRead)
				{
					requiresSaving = true;
					break;
				}
				if (!msRead)
				{
					break;
				}
				if (!memory_compare(msData, fData, msRead))
				{
					requiresSaving = true;
					break;
				}
			}
		}
	}

	// if we need to update it, do so now

	if (requiresSaving)
	{
		IO::File f;
		if (f.create(IO::get_user_game_data_directory() + localisedCharactersFilePath))
		{
			ms.go_to(0);
			ms.set_type(IO::InterfaceType::Binary);
			f.set_type(IO::InterfaceType::Binary);
			byte msData[bufferSize];
			while (true)
			{
				int msRead = ms.read(msData, bufferSize);
				if (!msRead)
				{
					break;
				}

				f.write(msData, msRead);
			}
		}
	}
}

bool TeaForGodEmperor::Main::run_other_game_if_requested()
{
	bool otherGameDone = !loadLibraryAndExit && Framework::PreviewGame::run_if_requested<TeaForGodEmperor::Library>(String(TXT("_preview.xml")),
		[](Framework::Game* _game)
		{
			TeaForGodEmperor::ArtDir::setup_rendering();
			TeaForGodEmperor::ArtDir::add_custom_build_in_uniforms();
			TeaForGodEmperor::RoomGenerationInfo::access().setup_to_use(false); // why here?
			TeaForGodEmperor::GameSettings::create(fast_cast<TeaForGodEmperor::Game>(_game)); // it will be nullptr, right?
		});
	
	return otherGameDone;
}

bool TeaForGodEmperor::Main::load_system_library()
{
	bool loaded = false;

	//loaded = game->load_library(game->get_system_library_directory(), ::Framework::LibraryLoadLevel::System, &(Loader::VoidRoom(loaderType, Colour::black, Colour::white).for_video()));
	//loaded = game->load_library(game->get_system_library_directory(), ::Framework::LibraryLoadLevel::System, &(Loader::VoidRoom(loaderType, game->does_use_vr() ? Colour::lerp(0.5f, Colour::greyLight, Colour::whiteWarm) : Colour::greyLight, game->does_use_vr() ? Colour::blackWarm : Colour::blackWarm).for_init()));
	{
		Loader::LoadingLogo loader = Loader::LoadingLogo(true);
		SCOPED_MEMORY_REPORT;
		loaded = game->load_library(game->get_system_library_directory(), ::Framework::LibraryLoadLevel::System, &loader);
	}

	{
		DEFINE_STATIC_NAME_STR(lsDegree, TXT("chars; degree"));
		String::use_degree(LOC_STR(NAME(lsDegree)));
	}

	return loaded;
}


bool TeaForGodEmperor::Main::load_actual_library()
{
	bool loaded = false;

	// before we start, load play area before we start loading stuff and showing hub
	if (VR::IVR::can_be_used())
	{
		if (auto* vr = VR::IVR::get())
		{
			vr->load_play_area_rect(true);

			// as we loaded, update scene mesh to match
			TeaForGodEmperor::RoomGenerationInfo::access().setup_to_use(false); // do not output here, we will be outputting later on
			game->update_hub_scene_meshes();
		}
	}
	Loader::Sequence seq;
		// all shared variables should be placed here (or at least not inside blocks)
		// test
		/*
		Loader::StarField sf;
		seq.add(&sf, 100000.0f);
		*/
		/*
		// "be at right place" to show boundary
		Loader::HubInSequence barp(&(game->access_main_hub_loader()), new Loader::HubScenes::BeAtRightPlace(false, true), false);
		seq.add(&barp);
		*/

#ifndef AN_QUICK_START_SKIP_SPLASH
		Loader::LoadingLogo loader = Loader::LoadingLogo(true, 4.0f /* force showing for this time */, true /*wait for vr*/);
		if (::System::Core::is_vr_paused() ||
			::System::Core::is_rendering_paused())
		{
			seq.add([]() {Game::get()->fade_in_immediately(); /* to show loader fully */});
			// if we didn't show at all, use waiting logo loader to show the game's logo, then menu
			seq.add(&loader);
		}
		// logos and fade out info
		Loader::VoidRoom logos(loaderType, Colour::black, Colour::white, NP, RegisteredColours::get_colour(NAME(loader_hub_fade)));
		logos.for_init();
		seq.add([]() {Game::get()->fade_in_immediately(); /* to show loader fully */});
		//
		seq.add(&logos, 7.0f); // 3.0 but to show credits, more
#endif
		seq.add([]() {Game::get()->fade_out_immediately(); /* to fade in via hub loader*/});

		// do this always, we need to be in VR to reset forward properly
		Loader::HubInSequence waitForValidVR(&(game->access_main_hub_loader()), new Loader::HubScenes::Empty(true), false);
		seq.add(&waitForValidVR);
		seq.add([this]()
			{
				game->access_main_hub_loader().force_reset_and_update_forward();
			});

#ifndef AN_QUICK_START
		// messages
		Loader::HubInSequence loadPlayArea(&(game->access_main_hub_loader()), (new Loader::HubScenes::Text(LOC_STR(NAME(lsPlayAreaNotDetected)), 0.5f))->close_on_condition([]()
			{
				if (VR::IVR::can_be_used())
				{
					if (auto* vr = VR::IVR::get())
					{
						// reload just in any case
						vr->load_play_area_rect(true);

						if (vr->has_loaded_play_area_rect())
						{
							return true;
						}
						else
						{
							vr->load_play_area_rect(true);
							return vr->has_loaded_play_area_rect();
						}
					}
				}
				return true;
			}), false);
		Loader::HubInSequence comfortMessage(&(game->access_main_hub_loader()), new Loader::HubScenes::ComfortMessageAndBasicOptions(0.05f /* so we can have valid head read */), false);
		Loader::HubInSequence vrModeSelection(&(game->access_main_hub_loader()), new Loader::HubScenes::VRModeSelection(0.05f /* so we can have valid head read */), false);
		Loader::HubInSequence boundaryUnavailable(&(game->access_main_hub_loader()), new Loader::HubScenes::Message(LOC_STR(NAME(lsBoundaryUnavailable))), false);
		Loader::HubInSequence mayHaveInvalidBoundary(&(game->access_main_hub_loader()), new Loader::HubScenes::Message(LOC_STR(NAME(lsMayHaveInvalidBoundary))), false);
		Loader::HubInSequence playAreaEnlarged(&(game->access_main_hub_loader()), new Loader::HubScenes::Message(LOC_STR(NAME(lsPlayAreaEnlarged))), false);
		Loader::HubInSequence playAreaForcedScaled(&(game->access_main_hub_loader()), new Loader::HubScenes::Message(LOC_STR(NAME(lsPlayAreaForcedScaled))), false);
		Loader::HubInSequence handGestures(&(game->access_main_hub_loader()), new Loader::HubScenes::WaitForScreen([](Loader::Hub* _hub) { return Loader::HubScreens::HandGestures::create(_hub, nullptr, false); }), false);
		// add them in sequence
		{
			if (!loadLibraryAndExit &&
				!createMainConfigFileAndExit)
			{
				if (VR::IVR::can_be_used())
				{
					if (auto* vr = VR::IVR::get())
					{
						seq.add(&loadPlayArea);
						seq.add([this, vr]()
						{
							output(TXT("[start] check if we should use immobile vr auto"));
							TeaForGodEmperor::RoomGenerationInfo::access().setup_to_use(); // we should have proper vr space at this time
							if (vr->is_boundary_unavailable())
							{
								output(TXT("[start] boundary not available"));
								MainConfig::access_global().set_immobile_vr_auto(true, true);
								output(TXT("[start] was marked as auto, redo RGI"));
								TeaForGodEmperor::RoomGenerationInfo::access().setup_to_use();
							}

							if (TeaForGodEmperor::RoomGenerationInfo::get().was_play_area_forced_scaled() ||
								TeaForGodEmperor::RoomGenerationInfo::get().was_play_area_enlarged())
							{
								output(TXT("[start] should use immobile vr auto"));
								MainConfig::access_global().set_immobile_vr_auto(true);
								if (MainConfig::global().get_immobile_vr() == Option::Auto)
								{
									output(TXT("[start] switching to immobile vr (because auto is accepted)"));
									output(TXT("[start] was marked as auto, redo RGI"));
									TeaForGodEmperor::RoomGenerationInfo::access().setup_to_use();
								}
							}
							else
							{
								output(TXT("[start] no need to use immobile vr auto"));
							}
							if (MainConfig::global().should_be_immobile_vr())
							{
								output(TXT("[start] using immobile vr"));
							}
							else
							{
								output(TXT("[start] using roomscale vr"));
							}

							game->update_hub_scene_meshes(); /* post load - to make sure we have the right size */
						});
						seq.add(&boundaryUnavailable, NP, [vr]() { return !MainConfig::global().should_be_immobile_vr() && vr->is_boundary_unavailable() && ! vr->is_invalid_boundary_replacement_available(); });
						seq.add(&mayHaveInvalidBoundary, NP, [vr]() { return !MainConfig::global().should_be_immobile_vr() && vr->may_have_invalid_boundary() && !vr->is_invalid_boundary_replacement_available(); });
					}
				}
				seq.add(&vrModeSelection);
				seq.add(&comfortMessage);
				{
					seq.add(&playAreaForcedScaled, NP, []() { return !MainConfig::global().should_be_immobile_vr() && TeaForGodEmperor::RoomGenerationInfo::get().was_play_area_forced_scaled(); });
					seq.add(&playAreaEnlarged, NP, []() { return !MainConfig::global().should_be_immobile_vr() && TeaForGodEmperor::RoomGenerationInfo::get().was_play_area_enlarged(); });
				}
				if (auto* vr = VR::IVR::get())
				{
					seq.add(&handGestures, NP, [vr]() {return vr->is_using_hand_tracking(); });
				}
			}
		}
#endif
		//
		//
		Loader::HubInSequence loadingScene(&(game->access_main_hub_loader()), game->create_loading_hub_scene(LOC_STR(NAME(lsLoading)), 0.5f, NAME(loading), LoadingHubScene::InitialLoading), false /* no fade out, we jump straight to whatever's next */);
		seq.add(&loadingScene);
		//
	SCOPED_MEMORY_REPORT;
	{
#ifdef AN_DEVELOPMENT_OR_PROFILER
		if (importLanguages)
		{
			importLanguages = is_there_anything_for_import_languages();
		}
#endif
		if (!loadLibraryAndExit &&
			!createMainConfigFileAndExit &&
			!dumpLanguages &&
			!importLanguages &&
			!translateLocalisedStrings &&
			!updateHashesForLocalisedStrings)
			// no need for updateLocalisedCharacters as localised characters require localised strings which are always loaded
		{
			::Framework::Library::allow_loading_on_demand();
		}
		loaded = game->load_library(game->get_library_directory(), ::Framework::LibraryLoadLevel::Main, &seq);
	}

	return loaded;
}

int TeaForGodEmperor::Main::run()
{
	init();

	// various tests
	/*
	{
		allow_output();
		output(TXT("run convex hull test"));

		Random::Generator rg;
		for_count(int, testIdx, 100000)
		{
			ConvexHull ch;
			Vector3 s;
			s.x = rg.get_float(0.03f, 2.0f);
			s.y = rg.get_float(0.03f, 2.0f);
			s.z = rg.get_float(0.03f, 2.0f);
			Range3 r(Range(-s.x, s.x), Range(-s.y, s.y), Range(-s.z, s.z));
			float roundTo = 0.0f;
			for_count(int, vertexIdx, 10000)
			{
				Vector3 v;
				v.x = rg.get(r.x);
				v.y = rg.get(r.y);
				v.z = rg.get(r.z);

				if (roundTo != 0.0f)
				{
					v.x = v.x / roundTo;
					v.x = round(v.x);
					v.x = v.x * roundTo;
					v.y = v.y / roundTo;
					v.y = round(v.y);
					v.y = v.y * roundTo;
					v.z = v.z / roundTo;
					v.z = round(v.z);
					v.z = v.z * roundTo;
				}

				ch.add(v);
			}

			ch.build();

			output(TXT("test %i done"), testIdx);
		}
	}
	*/
	/*
	{
		allow_output();
		output(TXT("run float to string test"));

		{ float v = 1.0f; output(TXT("%.10f, %S"), v, ParserUtils::float_to_string_auto_decimals(v).to_char()); }
		{ float v = 2.1f; output(TXT("%.10f, %S"), v, ParserUtils::float_to_string_auto_decimals(v).to_char()); }
		{ float v = 3.01f; output(TXT("%.10f, %S"), v, ParserUtils::float_to_string_auto_decimals(v).to_char()); }
		{ float v = 4.001f; output(TXT("%.10f, %S"), v, ParserUtils::float_to_string_auto_decimals(v).to_char()); }
		{ float v = 5.0001f; output(TXT("%.10f, %S"), v, ParserUtils::float_to_string_auto_decimals(v).to_char()); }
		{ float v = 6.00001f; output(TXT("%.10f, %S"), v, ParserUtils::float_to_string_auto_decimals(v).to_char()); }
	}
	*/

	codeInfoFilePath = String::printf(TXT("system%ScodeInfo"), IO::get_directory_separator());
	localisedCharactersFilePath = String::printf(TXT("system%SlocalisedCharacters"), IO::get_directory_separator());
	
	while (startGame || ::System::Core::restartRequired)
	{
		pre_init_game();

		if (runNormally || loadLibraryAndExit || createMainConfigFile || createMainConfigFileAndExit)
		{
			init_system_prior_to_game();

			{	// test things here
			}

			log_registered_types();

#ifdef AN_DEVELOPMENT
			if (0)
			{
				test_code_info();
			}
#endif

			load_code_info();

			// TeaForGodEmperor::ArtDir::setup_rendering() was moved to be most init, as we require settings to be loaded to properly setup everything

			// useful for preview too!

			output(TXT("preview game?"));

			bool otherGameDone = false;
#ifdef AN_DEVELOPMENT_OR_PROFILER
			otherGameDone = run_other_game_if_requested();
#endif

			if (otherGameDone)
			{
				TeaForGodEmperor::GameSettings::destroy();
			}
			else
			{
				output(TXT("create actual game"));

#ifdef AN_WINDOWS
				if (createMainConfigFile || createMainConfigFileAndExit)
				{
					String forcedSystem = MainConfig::global().get_forced_system();
					if (forcedSystem != TXT("quest") &&
						forcedSystem != TXT("quest2") &&
						forcedSystem != TXT("quest3") &&
						forcedSystem != TXT("questPro") &&
						forcedSystem != TXT("vive") &&
						forcedSystem != TXT("pico"))
					{
						VR::IVR::create_config_files();
					}
				}
#endif

				checkGameEnvironment = new CheckGameEnvironment(createMainConfigFileAndExit || loadLibraryAndExit);

				checkGameEnvironment->check(); // before game is created

				game = new TeaForGodEmperor::Game();
				checkGameEnvironment->game = game;

				game->add_execution_tags(executionTags);
				game->set_to_create_main_config_only(createMainConfigFile || createMainConfigFileAndExit);

				scoped_call_stack_info(TXT("main thread"));

				game->start(createMainConfigFileAndExit);

				if (createMainConfigFileAndExit && !loadLibraryAndExit)
				{
					game->save_config_file();
					::System::Core::quick_exit(true);
				}

				{
					// sky mesh has radius 13000, is now more round so we may lower far Z, just above it
					Game::s_renderingFarZ = 13500.0f;
	
					// enough for VR - lesser values result in z fighting, changing this may result in front plane not rendered
					Game::s_renderingNearZ = 0.025f;
					if (::System::Core::get_system_tags().has_tag(Name(TXT("quest2"))) ||
						::System::Core::get_system_tags().has_tag(Name(TXT("quest3"))) ||
						::System::Core::get_system_tags().has_tag(Name(TXT("questPro"))))
					{
						// a bit further to avoid z fighting
						Game::s_renderingNearZ = 0.045f;
					}
				}

				TeaForGodEmperor::ArtDir::setup_rendering();

				{	// test things here

					/*
					{	// test random generators
						Random::Generator rg;
						rg.set_seed(12345, 67890);
						int ri = 0;
						float rf = 0.0f;
						for_count(int, i, 20)
						{
							int ni = rg.get_int(100000);
							float nf = rg.get_float(0.0f, 5.0f);

							ri += ni;
							rf += nf;
							output(TXT("%03i ][ ri+%i rf+%.8f  "), i, ni, nf);
						}

						output(TXT("    ][ ri=%i rf=%.8f  "),ri, rf);
					}
					*/
					/*
					{	// test fault handler
						int* a = (int*)239408;
						int b = 3;
						output(TXT("test crash - start"));
						int c = b + *a;
						*a = c;
						output(TXT("test crash - done"));
					}
					*/
					/*
					{	// test fault handler
						int* a = (int*)239408;
						output(TXT("test crash - start"));
						delete a;
						output(TXT("test crash - done"));
					}
					*/
				}

				if (drawVRControls)
				{
					game->draw_vr_controls();
				}

#ifdef CHECK_BUILD_AVAILABLE
				// get latest build info
				String buildAvailable; // only if different
				bool buildAvailableProcessed = false;
				System::TimeStamp buildCheckTS;
#ifdef AN_ANDROID
#ifndef AN_OCULUS_ONLY
				//buildAvailableProcessed = true; // hack for skipping this when debugging?
#endif
#endif
				if (!buildAvailableProcessed)
				{
#ifndef AN_DEVELOPMENT_OR_PROFILER
					game->get_job_system()->do_asynchronous_off_system_job(download_latest_build_version, new DownloadLatestBuildVersionData(&buildAvailable, &buildAvailableProcessed));
#else
					buildAvailableProcessed = true;
#endif
				}
#endif

				TeaForGodEmperor::ArtDir::add_custom_build_in_uniforms();
				TeaForGodEmperor::RoomGenerationInfo::access().setup_to_use(false); // do not output values here, just run it to get some of the stuff ready, we will be doing setup when we load stuff

#ifdef NO_MUSIC
				if (loadLibraryAndExit) // only preparing, remove music as we don't want to have music in the build
				{
					IO::Dir::delete_dir(game->get_system_library_directory() + IO::get_directory_separator() + String(TXT("music")));
					IO::Dir::delete_dir(game->get_library_directory() + IO::get_directory_separator() + String(TXT("music")));
				}
#endif

				{
					Name langId = Framework::LocalisedStrings::get_system_language();
					if (!langId.is_valid())
					{
						langId = Framework::LocalisedStrings::get_default_language();
					}
					if (Framework::LocalisedStrings::get_suggested_default_language().is_valid())
					{
						langId = Framework::LocalisedStrings::get_suggested_default_language();
					}
					output(TXT("system language detected: %S"), langId.to_char());
					Framework::LocalisedStrings::set_language(langId, true);
				}

				output(TXT("load localised characters"));
				load_localised_characters();

#ifdef AN_DEVELOPMENT_OR_PROFILER
				Framework::Library::load_global_config_from_file(String(TXT("_loadSetup.xml")), Framework::LibrarySource::Files);
#endif

				bool loaded = true;
				loaded &= load_system_library();

				{
					bool loaded = game->load_existing_player_profile(false /* do not load game slots */); // to get language setting and other settings, we may save it without the game slots
					if (loaded)
					{
						if (auto* ps = PlayerSetup::get_current_if_exists())
						{
							Name langId = ps->get_preferences().get_language();
							output(TXT("use language from player profile: %S"), langId.to_char());
							Framework::LocalisedStrings::set_language(langId, true);
						}
					}
					else
					{
						// reset language if couldn't load player profile
						Name langId = Framework::LocalisedStrings::get_system_language();
						if (!langId.is_valid())
						{
							langId = Framework::LocalisedStrings::get_default_language();
						}
						if (Framework::LocalisedStrings::get_suggested_default_language().is_valid())
						{
							langId = Framework::LocalisedStrings::get_suggested_default_language();
						}
						output(TXT("system language detected post system library load: %S"), langId.to_char());
						Framework::LocalisedStrings::set_language(langId, true);
					}
				}

				if (MainSettings::global().get_shader_options().get_tag(NAME(retro)))
				{
					MainConfig::access_global().access_video().fxaa = false;
					MainConfig::access_global().access_video().options.set_tag(NAME(bloom), 0.0f);
					if (MainConfig::global().get_video().msaa)
					{
						// off
						MainConfig::access_global().access_video().msaa = false;
						bool softReinitialise = true;
						bool newRenderTargetsNeeded = true;
						TeaForGodEmperor::Game::get()->reinitialise_video(softReinitialise, newRenderTargetsNeeded);
					}
				}

#ifdef RENDER_HITCH_FRAME_INDICATOR
				if (auto* vr = VR::IVR::get())
				{
					if (vr->get_controls().is_button_pressed(VR::Input::Button::WithSource(VR::Input::Button::Type::Grip, VR::Input::Devices::all, Hand::Left)) &&
						vr->get_controls().is_button_pressed(VR::Input::Button::WithSource(VR::Input::Button::Type::Grip, VR::Input::Devices::all, Hand::Right)))
					{
						game->showHitchFrameIndicator = true;
					}
				}
#endif

				REMOVE_AS_SOON_AS_POSSIBLE_
#ifdef PHYSICAL_SENSATIONS_BHAPTICS_JAVA
					//game->get_job_system()->do_asynchronous_off_system_job(test_bhaptics);
					//test_bhaptics();
#endif

				// we're "faded out" as the last screen was faded out but here we could get paused for a while, avoid that by showing nothing
				output(TXT("init all we require to show hub loader"));
				game->fade_out_immediately(); // hub loader will fade in
				game->add_async_world_job(TXT("pre game init"),
					[this]()
					{
						TeaForGodEmperor::PlayerPreferences::lookup_defaults();
						if (!createMainConfigFile && !createMainConfigFileAndExit)
						{
							game->update_player_profiles_list(true /* select one */);
						}
						game->init_main_hub_loader();
						game->set_allow_saving_player_profile(false);
						if (!createMainConfigFile && !createMainConfigFileAndExit)
						{
							output(TXT("load player profile (without game slots)"));
							freshStart = !game->load_existing_player_profile(false /* do not load game slots */); // to get language setting and other settings, we may save it without the game slots
							if (freshStart)
							{
								output(TXT("create and choose default player and start with welcome scene"));
								// we need to create a default player as we will be changing settings
								game->start_with_welcome_scene();
								game->create_and_choose_default_player(false /* do not load game slots */);
							}
						}
					});
				{
					Loader::Empty* le = new Loader::Empty();
					game->force_advance_world_manager(le); // just show blackness
					delete le;
				}
				output(TXT("ready to load library"));
				if (!loadLibraryAndExit &&
					!createMainConfigFileAndExit)
				{
					checkGameEnvironment->check();

#ifdef CHECK_BUILD_AVAILABLE
					if (!buildAvailableProcessed)
					{
						game->show_hub_scene(new Loader::HubScenes::Text(LOC_STR(NAME(lsPleaseWait))), NP, [&buildAvailableProcessed, &buildCheckTS]() { return buildAvailableProcessed || buildCheckTS.get_time_since() > 2.0f; }); // wait max 3 seconds
					}
					if (!buildAvailable.is_empty())
					{
						output(TXT("new build available: %S"), buildAvailable.to_char());
						game->show_hub_scene(new Loader::HubScenes::Message(String::printf(TXT("%S~%S~~%S~%S"), LOC_STR(NAME(lsNewBuildAvailable)).to_char(), buildAvailable.to_char(), LOC_STR(NAME(lsNewBuildAvailableYouHave)).to_char(), get_build_version_short())));
					}
#endif
					if (!::System::Sound::get())
					{
						warn(TXT("no sound system"));
#ifndef AN_DEVELOPMENT_OR_PROFILER
						game->show_hub_scene(new Loader::HubScenes::Message(LOC_STR(NAME(lsSoundNotInitialised))));
#endif
					}
					if (MainConfig::global().get_report_bugs() == Option::Unknown)
					{
#ifdef AN_ASSUME_AUTO_REPORT_BUGS
						an_assert(false, TXT("we should not end here!"));
						MainConfig::access_global().set_report_bugs(Option::True);
#else
						game->show_hub_scene(new Loader::HubScenes::Question(LOC_STR(NAME(lsReportBugsQuestion)),
							[] {MainConfig::access_global().set_report_bugs(Option::Ask); },
							[] {MainConfig::access_global().set_report_bugs(Option::False); }));
						// updated main config
						game->save_config_file();
#endif
					}
				}

				if (! loadLibraryAndExit)
				{
					game->play_loading_music_via_non_game_music_player();
				}

				REMOVE_AS_SOON_AS_POSSIBLE_
#ifdef PHYSICAL_SENSATIONS_BHAPTICS_JAVA
				//game->get_job_system()->do_asynchronous_off_system_job(test_bhaptics);
#endif

				// load actual library (but right before there are test scenes)
				if (false)
				{
					// try test scene
					SCOPED_MEMORY_REPORT;
					loaded &= game->load_library(game->get_library_directory(), ::Framework::LibraryLoadLevel::Main, &(game->access_main_hub_loader().set_scene(new Loader::HubScenes::Test())));
				}
				else if (false)
				{
					SCOPED_MEMORY_REPORT;
					loaded &= game->load_library(game->get_library_directory(), ::Framework::LibraryLoadLevel::Main, &(game->access_main_hub_loader().set_scene(new Loader::HubScenes::HandyMessages())));
				}
				else
				{
					loaded &= load_actual_library();
				}

				if (updateLocalisedCharacters)
				{
					update_localised_characters();
				}

				if (storeDescriptions)
				{
					store_descriptions();
				}

				/*
				{
					// test starfield
					if (auto* lib = TeaForGodEmperor::Library::get_current_as<TeaForGodEmperor::Library>())
					{
						if (auto* p = lib->get_pilgrimages().find(Framework::LibraryName(String(TXT("pilgrimage, 01 arrival:pilgrimage")))))
						{
							Loader::ILoader* sf = p->create_custom_loader();
							game->show_loader_and_perform_background(sf, []() { while (true) { ::System::Core::sleep(1.0f); }});
						}
					}
				}
				*/
			
				// final check for game environment
				if (!loadLibraryAndExit &&
					!createMainConfigFileAndExit)
				{
					if (!checkGameEnvironment->check())
					{
						output(TXT("we're still waiting for the game environment check to complete"));
						if (game)
						{
							String message = Framework::StringFormatter::format_loc_str(NAME(lsGameChecking), Framework::StringFormatterParams()
								.add(NAME(gameEnvironment), String(checkGameEnvironment->name)));
							game->show_hub_scene((new Loader::HubScenes::Text(message)), NP,
								[this]()
								{
									return checkGameEnvironment->check();
								});
						}
					}
				}

				// to have a proper forward when we reach the menu
				output(TXT("get ready to reach the menu!"));
				Loader::Hub::reset_last_forward();

				output_colour(1, 0, 1, 1);
				output(TXT("tea library loaded!"));
				output_colour();

				if (auto* vr = VR::IVR::get())
				{
					output(TXT("pre game play area info"));
					vr->output_play_area();
				}

				TeaForGodEmperor::PlayerPreferences::lookup_defaults();

				::Framework::Library::get_current()->set_current_library_load_level(::Framework::LibraryLoadLevel::PostLoad);

				if (!loaded)
				{
#ifdef AN_DEVELOPMENT
					error_stop(TXT("game not loaded properly. what now?"));
					loaded = true;
#else
#ifdef AN_WINDOWS
					MessageBox(nullptr, TXT("Could not load. Please send a log to contact@voidroom.com"), TXT("Problem!"), MB_ICONEXCLAMATION | MB_OK);
					::System::Core::quick_exit();
#endif
#endif
				}

				if (loadLibraryAndExit)
				{
					do_lists_imports_and_dumps();
					game->save_config_file();
					::System::Core::quick_exit(true);
				}

				if (loaded)
				{
					{	// test post load
					}

					game->show_hub_scene_waiting_for_world_manager(nullptr /* will use empty scene */, NP,
						[this]()
					{
						do_lists_imports_and_dumps();
						
						output(TXT("last minute things, before game starts"));

						if (!createMainConfigFile && !createMainConfigFileAndExit)
						{
							if (game->legacy__does_require_updating_player_profiles())
							{
								output(TXT(" - update player profiles"));
								game->legacy__update_player_profiles();
							}
						}

						// doesn't make sense to do this earlier - load whole player profile
						output(TXT(" - player profile"));
						game->set_allow_saving_player_profile(true); // right before we require to save
						game->save_player_profile(false); // save if exists, note we're saving just profile, without game slots
						game->load_existing_player_profile(); // load with game slots or mark as not loaded
#ifdef AN_DEVELOPMENT_OR_PROFILER
						if (auto* ps = TeaForGodEmperor::PlayerSetup::access_current_if_exists())
						{
							ps->access_preferences().subtitles = true;
							ps->get_preferences().apply(); // to setup input, subtitles etc
						}
#endif
						::System::Core::on_quick_exit(TXT("save player profile"), [this]() { game->save_player_profile(); });

						output(TXT(" - before game starts"));
						game->before_game_starts();

						output(TXT(" - init other hub loaders"));
						game->init_other_hub_loaders();

						output(TXT("done, ready to go"));
					});

					// --- game has started

					output(TXT("ready for continuous advancement"));
					game->ready_for_continuous_advancement();
					output(TXT("go!"));

					bool running = true;
#ifdef AN_DEVELOPMENT_OR_PROFILER
#ifdef AN_STANDARD_INPUT
					float exitHeldFor = 0.0f;
#endif
#endif
					while ((running || game->get_fade_out() < 0.999f) && !game->does_want_to_exit() && !::System::Core::restartRequired)
					{
						game->advance();

						//System::Core::sleep(5);
#ifdef AN_DEVELOPMENT_OR_PROFILER
#ifdef AN_STANDARD_INPUT
						todo_note(TXT("this is temporary, this is only to allow checking for memory leaks"));
						if (System::Input::get()->is_key_pressed(System::Key::Esc))
						{
							exitHeldFor += ::System::Core::get_raw_delta_time();
							if (exitHeldFor > 1.0f)
							{
								if (running)
								{
									game->set_wants_to_exit();
									output_colour_system();
									output(TXT("exit requested"));
									output_colour();
								}
								running = false;
							}
						}
						else
						{
							exitHeldFor = 0.0f;
						}
#endif
#endif

						if (!running)
						{
							// order to fade out
							game->fade_out(0.1f, NAME(quittingGame), 0.0f, true);
						}
					}

					// cancel all jobs and wait till current one ended
					// in the mean time provide void room loader
					game->cancel_world_managers_jobs(&(Loader::VoidRoom(Loader::VoidRoomLoaderType::EndingGame, Colour::black, Colour(0.4f, 0.4f, 0.4f), RegisteredColours::get_colour(NAME(loader_hub_fade))).for_shutdown()));

					game->end_mode(true);

					todo_note(TXT("this is hack to allow to exit game and test memory leaks"));
					game->access_player().bind_actor(nullptr);

					// destroy world
#ifdef DEBUG_DESTROY_WORLD
					output(TXT("main, game ends"));
#endif
					game->request_destroy_world();
					game->force_advance_world_manager(&(Loader::VoidRoom(loaderType, Colour::black, Colour(0.4f, 0.4f, 0.4f), RegisteredColours::get_colour(NAME(loader_hub_fade))).for_shutdown()));

#ifdef AN_DEVELOPMENT
					// log all game objects
					int gameObjectsCount = TeaForGodEmperor::log_game_objects();
					an_assert(gameObjectsCount == 0, TXT("there should be no game objects at this point!\n\nthis is first chance point to catch huge memory leaks"));
#endif
					// --- game has ended

					::System::Core::on_quick_exit_no_longer(TXT("save player profile"));
					game->save_player_profile();

					game->after_game_ended();
				}
				else
				{
					game->stop_non_game_music_player();
					//TeaForGodEmperor::Library::get_current()->list_all_objects();
					//todo_important(TXT("game not loaded properly. display message"));
					float keepRunningFor = 0.5f;
					bool running = true;
					while (running && keepRunningFor > 0.0f)
					{
						::System::Core::set_time_multiplier(); 
						System::Core::advance();
#ifdef AN_STANDARD_INPUT
						running = !System::Input::get()->is_key_pressed(System::Key::Esc);
#endif
						keepRunningFor -= System::Core::get_delta_time();
						System::Core::sleep(0.005f);
					}
				}
				game->stop_non_game_music_player();

				// drop everything that might require the library
				TeaForGodEmperor::PlayerPreferences::drop_defaults();
				Framework::LocalisedStrings::drop_assets();
				game->close_hub_loaders();

				// empty library before closing game
				//game->close_library(Framework::LibraryLoadLevel::Close, &Loader::Circles(Colour::black, Colour(0.0f, 0.0f, 0.4f)));
				if (loaded)
				{
					game->close_library(Framework::LibraryLoadLevel::Close, &(Loader::VoidRoom(loaderType, Colour::black, Colour(0.0f, 0.0f, 0.4f), RegisteredColours::get_colour(NAME(loader_hub_fade))).for_shutdown()));
				}
				else
				{
					game->close_library(Framework::LibraryLoadLevel::Close, &(Loader::VoidRoom(Loader::VoidRoomLoaderType::LoadingFailed, Colour(0.75f, 0.0f, 0.0f), Colour(0.1f, 0.0f, 0.0f)).for_shutdown()));
				}

				// close game
				game->close_and_delete();

				delete_and_clear(checkGameEnvironment);
			}

			close_system_post_game();
		}
	}

	close();

	return 0;
}

String TeaForGodEmperor::Main::get_language_import_directory()
{
	return IO::get_user_game_data_directory() + IO::get_auto_directory_name() + IO::get_directory_separator() + String(TXT("languages.import"));
}

bool TeaForGodEmperor::Main::is_there_anything_for_import_languages()
{
	String languageImportDirectory = get_language_import_directory();
	IO::Dir dir;
	dir.list(languageImportDirectory);
	bool thereAreSomeFiles = ! dir.get_files().is_empty();
	return thereAreSomeFiles;
}

void TeaForGodEmperor::Main::do_lists_imports_and_dumps()
{
	if (listLibrary)
	{
		LogInfoContext log;
		TeaForGodEmperor::Library::get_current()->list_all_objects(log);
		log.output_to_output();
	}
	if (listLibraryType.is_valid())
	{
		LogInfoContext log;
		TeaForGodEmperor::Library::get_current()->get_store(listLibraryType)->list_all_objects(log);
		log.output_to_output();
	}
	String languageDumpDirectory = IO::get_user_game_data_directory() + IO::get_auto_directory_name() + IO::get_directory_separator() + String(TXT("languages"));
	String languageImportDirectory = get_language_import_directory();
	if (translateLocalisedStrings)
	{
		// dump all languages automatically
		// commented out as we don't want to dump it every time
		Framework::LocalisedStrings::translate_all_requiring(languageDumpDirectory);
	}
	if (importLanguages)
	{
		// import languages (this will also create an additional file in the same dir with updated strings)
		Framework::LocalisedStrings::load_all_languages_from_dir_tsv_and_save(languageImportDirectory, languageDumpDirectory);
	}
	if (updateHashesForLocalisedStrings)
	{
		Framework::LocalisedStrings::translate_all_requiring(languageDumpDirectory, true);
	}
	if (dumpLanguages)
	{
		// dump all languages automatically
		// commented out as we don't want to dump it every time
		Framework::LocalisedStrings::save_all_languages_to_dir(languageDumpDirectory);
	}
}
