// main app

#include "../teaForGod/teaForGodMain.h"

#include "../core/mainConfig.h"
#include "../core/utils.h"
#include "../core/io/dir.h"
#include "../core/system/core.h"
#include "../core/types/string.h"

#include "SDL.h" // to get main properly defined

//

//#define TEST_CREATE_DEPLOY

//

#if AN_RELEASE || AN_PROFILER
int main(int argc, char *argv[])
#else
int _tmain(int argc, _TCHAR* argv[])
#endif
{
#ifdef AN_CHECK_MEMORY_LEAKS
	mark_static_allocations_done_for_memory_leaks();
#endif

	/*
	TeaForGodEmperor::Energy mag(9);
	TeaForGodEmperor::Energy chamber(20);

	for_count(int, i, 20)
	{
		output(TXT("%S / %S = %S"),
			mag.format(String(TXT("%.0f"))).to_char(),
			chamber.format(String(TXT("%.0f"))).to_char(),
			(mag / chamber).format(String(TXT("%.0f"))).to_char());
		output(TXT("%i / %i = %i"),
			mag.energy,
			chamber.energy,
			(mag / chamber).energy);
		mag += TeaForGodEmperor::Energy(0, 10);
	}
	*/

	/*
	float epsilon = 0.005f;
	Collision::QueryPrimitivePoint point(Vector3(4.0f, 0.0f, 0.0f));
	Collision::CheckGradientPoints<Collision::QueryPrimitivePoint> points(point, Matrix44::identity, epsilon);
	Collision::CheckGradientDistances distances(1.0f, epsilon);
	Collision::CheckGradientContext<Collision::QueryPrimitivePoint> cgc(points, distances);

	Array<Matrix44> bones;
	bones.push_back(Matrix44::identity);
	bones[0] = scale_matrix(2.0f).to_world(bones[0]);
	//bones[0] = rotation_xy_matrix(90.0f).to_world(bones[0]);
	bones[0] = translation_matrix(Vector3(1.0f, 0.0f, 0.0f)).to_world(bones[0]);

	Matrix44 b0i = bones[0].inverted();
	Matrix44 b0fi = bones[0].full_inverted();

	Vector3 loc = Vector3(2.0f, 0.0f, 0.0f);
	Vector3 l_b0i = b0i.location_to_world(loc);
	Vector3 l_b0fi = b0fi.location_to_world(loc);

	Meshes::PoseMatBonesRef matBonesRef(nullptr, bones);

	Collision::Sphere sphere;
	sphere.set(Sphere(Vector3(0.0f, 0.0f, 0.0f), 1.0f));
	Collision::update_check_gradient_for_shape<Collision::QueryPrimitivePoint, Collision::Sphere>(cgc, nullptr, matBonesRef, NP, sphere, PlaneSet::empty(), 1.0f);
	*/

	TeaForGodEmperor::Main* teaMain = new TeaForGodEmperor::Main();

	teaMain->executionTags.clear();
	teaMain->listLibrary = false;
	teaMain->listLibraryType = Name::invalid();
	teaMain->dumpLanguages = false; // will also dump info about if they should be translated or not, dumps in xml and tsv files
	teaMain->importLanguages = false; // check teaForGodMain for import directory
#ifdef AN_DEVELOPMENT_OR_PROFILER
	teaMain->importLanguages = true; // also produces altered tsv file that won't be imported but can be used as input for translation files (gets new phrases, context info etc), if nothing to load will be cleared, only in dev and profiler
#endif
	teaMain->translateLocalisedStrings = false; // to translate all localised strings that require translation
	teaMain->updateHashesForLocalisedStrings = false; // use only if we are going to keep existing translations
//#define AN_TRANSLATE_NOW
#ifdef AN_TRANSLATE_NOW
	// this is a set to import and dump to tsv files
	teaMain->importLanguages = true;
	teaMain->translateLocalisedStrings = true;
#endif
	teaMain->createCodeInfo = false;
	teaMain->testWithCodeInfo = false;
	teaMain->loadLibraryAndExit = false; // useful to reload/reimport all assets (especially music that is loaded on demand)
	teaMain->updateLocalisedCharacters = false;
	teaMain->storeDescriptions = false; // output descriptions for stores (steam, oculus, pico, viveport)
	teaMain->createMainConfigFile = false;
	teaMain->createMainConfigFileAndExit = false;
	teaMain->silentRun = false;
	teaMain->drawVRControls = false;
	teaMain->forceVR = String();

#ifdef BUILD_NON_RELEASE
	// force to always update
	teaMain->updateLocalisedCharacters = true;
#endif

	{
		/*
			--listLibrary --loadLibraryAndExit
			--listLibraryType doorType --loadLibraryAndExit
			--dumpLanguages --updateLocalisedCharacters --loadLibraryAndExit
			--listLibrary --dumpLanguages --loadLibraryAndExit			
		*/
		Array<String> args;
		for_range(int, i, 1, argc - 1)
		{
			args.push_back(String::convert_from(argv[i]));
		}
		for(int i = 0; i < args.get_size(); ++ i)
		{
			String const & arg = args[i];
			if (arg.has_prefix(TXT("--")) && arg.get_length() > 2)
			{
				if (arg == TXT("--listLibrary"))
				{
					teaMain->listLibrary = true;
				}
				if (arg == TXT("--listLibraryType"))
				{
					if (args.is_index_valid(i + 1) &&
						!args[i + 1].has_prefix(TXT("--")))
					{
						teaMain->listLibraryType = Name(args[i + 1]);
						++i;
					}
					else
					{
						error(TXT("provide a valid type for --listLibraryType"));
					}
				}
				if (arg == TXT("--dumpLanguages"))
				{
					teaMain->dumpLanguages = true;
				}
				if (arg == TXT("--translateLocalisedStrings"))
				{
					teaMain->translateLocalisedStrings = true;
				}
				if (arg == TXT("--updateHashesForLocalisedStrings"))
				{
					teaMain->updateHashesForLocalisedStrings = true;
				}
				if (arg == TXT("--createCodeInfo"))
				{
					teaMain->createCodeInfo = true;
					teaMain->silentRun = true;
				}
				if (arg == TXT("--drawVRControls"))
				{
					teaMain->drawVRControls = true;
				}
				if (arg == TXT("--loadLibraryAndExit"))
				{
					teaMain->loadLibraryAndExit = true;
					teaMain->silentRun = true;
				}
				if (arg == TXT("--updateLocalisedCharacters"))
				{
					teaMain->updateLocalisedCharacters = true;
				}
				if (arg == TXT("--storeDescriptions"))
				{
					teaMain->storeDescriptions = true;
				}
				if (arg == TXT("--createMainConfigFile") ||
					arg == TXT("--createMainConfigFileAndExit"))
				{
					teaMain->silentRun = true;
					teaMain->createMainConfigFile = true;
					teaMain->createMainConfigFileAndExit = arg == TXT("--createMainConfigFileAndExit");
					if (args.is_index_valid(i + 1) &&
						!args[i + 1].has_prefix(TXT("--")))
					{
						MainConfig::access_global().force_system(args[i + 1]);
						++i;
					}
				}
				if (arg == TXT("--pretendQuest"))
				{
					MainConfig::access_global().force_system(String(TXT("quest")));
				}
				if (arg == TXT("--pretendQuest2"))
				{
					MainConfig::access_global().force_system(String(TXT("quest2")));
				}
				if (arg == TXT("--pretendQuest3"))
				{
					MainConfig::access_global().force_system(String(TXT("quest3")));
				}
				if (arg == TXT("--pretendQuestPro"))
				{
					MainConfig::access_global().force_system(String(TXT("questPro")));
				}
				if (arg == TXT("--pretendVive"))
				{
					MainConfig::access_global().force_system(String(TXT("vive")));
				}
				if (arg == TXT("--pretendPico"))
				{
					MainConfig::access_global().force_system(String(TXT("pico")));
				}
				if (arg == TXT("--forceOculus"))
				{
					teaMain->forceVR = TXT("Oculus");
				}
				if (arg == TXT("--forceSteamVR"))
				{
					teaMain->forceVR = TXT("SteamVR");
				}
				if (arg == TXT("--forceOpenVR"))
				{
					teaMain->forceVR = TXT("OpenVR");
				}
				if (arg == TXT("--forceOpenXR"))
				{
					teaMain->forceVR = TXT("OpenXR");
				}
				teaMain->executionTags.set_tag(Name(arg.get_sub(2)));
			}
		}
	}

#ifdef TEST_CREATE_DEPLOY
	{
		teaMain->executionTags.set_tag(Name(String(TXT("loadLibraryAndExit"))));
		teaMain->executionTags.set_tag(Name(String(TXT("createMainConfigFile"))));
		teaMain->silentRun = true;
		teaMain->loadLibraryAndExit = true;
		teaMain->createMainConfigFile = true;
		MainConfig::access_global().force_system(String(TXT("quest")));
	}
#endif

#ifdef AN_WINDOWS
#ifdef AN_SDL
	if (false
#ifndef AN_DEVELOPMENT_OR_PROFILER
		|| (!teaMain->createMainConfigFile && // preparing a build
			!IO::File::does_exist(String(TXT("mainConfig.xml"))) && // there is main config - deployed game
			!IO::File::does_exist(String(TXT("_baseConfig.xml")))) // there is base config - development
#endif
		|| !IO::Dir::does_exist(String(TXT("library")))
		|| !IO::Dir::does_exist(String(TXT("system")))
		)
	{
		MessageBox(nullptr, TXT("Could not find some of the required files.\n\nMake sure that the game has been unpacked from the zip file."), TXT("Problem!"), MB_ICONEXCLAMATION | MB_OK);
		::System::Core::quick_exit();
	}
#endif
#endif

#ifdef AN_DEVELOPMENT_OR_PROFILER
	// to test on PC
	//MainConfig::access_global().force_system(String(TXT("quest")));
	//MainConfig::access_global().force_system(String(TXT("quest2")));
	//MainConfig::access_global().force_system(String(TXT("quest3")));
	//MainConfig::access_global().force_system(String(TXT("questPro")));
	//MainConfig::access_global().force_system(String(TXT("vive")));
	//MainConfig::access_global().force_system(String(TXT("pico")));
	
	// for test purposes run this or this
	// we want to see how the game looks like in both cases
#ifdef AN_DEVELOPMENT
	// for time being, dev builds get quest2 graphics always
	if (false)
#else
	//if (false) // should be disabled for preview games
	//if (Random::get_bool())
#endif
	{
		MainConfig::access_global().force_system(String(TXT("quest2")));
	}

	// force test retro
	//::System::Core::set_extra_requested_system_tags(TXT("retro"));
#endif

#ifdef AN_DEVELOPMENT_OR_PROFILER
	// to test demo
	//TeaForGodEmperor::be_demo();
#endif

	int result = teaMain->run();

	delete teaMain;

#ifdef AN_CHECK_MEMORY_LEAKS
	mark_static_allocations_left_for_memory_leaks();
#endif

	return result;
}
