// main app

#include "buildVerUpdate.h"
#include <string>
#include <iterator>
#include <iostream>
#include <time.h>

#include "..\core\buildVer.h"
#include "..\core\io\file.h"
#include "..\core\other\parserUtils.h"

static void t2c(_TCHAR* _in, tchar* _out, int _max)
{
	tchar* inc = (tchar*)_in;
	int i = 0;
	while (*inc != 0 && i < _max)
	{
		*_out = *inc;
		inc += 2;
		++_out;
	}
	*_out = 0;
}

void show_file_contents(String const& _fileName)
{
#ifndef AN_CLANG
	IO::File fileVerify;
	if (fileVerify.open(_fileName))
	{
		fileVerify.set_type(IO::InterfaceType::Text);
		List<String> lines;
		fileVerify.read_lines(lines);
		tprintf(TXT("written to \"%s\":\n"), _fileName.to_char());
		for_every(line, lines)
		{
			tprintf(TXT("  %s\n"), line->to_char());
		}
		tprintf(TXT("end of file\n"));
	}
#endif
}

enum BuildVerHVersion
{
	Old,
	New
};

#if AN_RELEASE || AN_PROFILER
int main(int argc, char *argv[])
#else
int _tmain(int argc, _TCHAR* argv[])
#endif
{
	String::initialise_static();

	bool buildDemo = false;
	bool buildPreview = false;
	bool buildPreviewRelease = false;
	bool buildPreviewPublicReleaseCandidate = false;
	bool buildPublicRelease = true;
	bool updateMajor = false;
	bool updateMinor = false;
	bool updateSubMinor = false;
	bool updateBuildNo = false; // separate! should be updated only when actual build is performed
	bool copyFile = false;
	bool forCopyFileUseBuildNo = false;
	bool createBuildInfoFile = false;

	// get file name
	String copyFileName; // won't update headers and properties
	String buildInfoFileName;
	String fileNameHeader;
	String fileNameProjectHeader;
	String fileNameAndroidProperties;
	String fileNameAndroidManifest;
	String androidPackageName;
	String setActiveProject;
	Array<String> additionalBuildSettings;
	bool keepAdditionalBuildSettings = true;
	int argIdx = 1;

	String androidPackageNameParam(TXT("--androidPackageName="));
	while (argc > argIdx)
	{
		String arg = String::convert_from(argv[argIdx]);
		if (arg.has_prefix(androidPackageNameParam))
		{ 
			argIdx++;
			androidPackageName = arg.get_sub(androidPackageNameParam.get_length());
			if (androidPackageName[0] == '\"')
			{
				// remove quotes
				androidPackageName = androidPackageName.get_sub(1, androidPackageName.get_length() - 2);
			}
			continue;
		}
		if (arg == TXT("--buildPublicDemoRelease"))
		{ 
			argIdx++;
			buildDemo = true;
			buildPreview = false;
			buildPreviewRelease = false;
			buildPreviewPublicReleaseCandidate = false;
			buildPublicRelease = true;
			continue;
		}
		if (arg == TXT("--buildPreview"))
		{
			argIdx++;
			buildDemo = false;
			buildPreview = true;
			buildPreviewRelease = false;
			buildPreviewPublicReleaseCandidate = false;
			buildPublicRelease = false;
			continue;
		}
		if (arg == TXT("--buildPreviewRelease"))
		{ 
			argIdx++;
			buildDemo = false;
			buildPreview = true;
			buildPreviewRelease = true;
			buildPreviewPublicReleaseCandidate = false;
			buildPublicRelease = false;
			continue;
		}
		if (arg == TXT("--buildPreviewPublicReleaseCandidate"))
		{
			argIdx++;
			buildDemo = false;
			buildPreview = true;
			buildPreviewRelease = true;
			buildPreviewPublicReleaseCandidate = true;
			buildPublicRelease = false;
			continue;
		}
		if (arg == TXT("--buildPublicRelease"))
		{ 
			argIdx++;
			buildDemo = false;
			buildPreview = false;
			buildPreviewRelease = false;
			buildPreviewPublicReleaseCandidate = false;
			buildPublicRelease = true;
			continue;
		}
		if (arg == TXT("--updateBuildNo"))
		{
			argIdx++;
			updateBuildNo = true;
			continue;
		}
		if (arg == TXT("--updateSubMinor"))
		{
			argIdx++;
			updateSubMinor = true;
			continue;
		}
		if (arg == TXT("--updateMinor"))
		{
			argIdx++;
			updateMinor = true;
			continue;
		}
		if (arg == TXT("--updateMajor"))
		{
			argIdx++;
			updateMajor = true;
			continue;
		}
		if (arg == TXT("--copyFileWithBuildName"))
		{
			argIdx++;
			copyFile = true;
			forCopyFileUseBuildNo = false;
			continue;
		}
		if (arg == TXT("--copyFileWithBuildNo"))
		{
			argIdx++;
			copyFile = true;
			forCopyFileUseBuildNo = true;
			continue;
		}
		if (arg == TXT("--createBuildInfoFile"))
		{
			argIdx++;
			createBuildInfoFile = true;
			continue;
		}
		if (arg == TXT("--setActiveProject"))
		{
			argIdx++;
			if (argIdx < argc)
			{
				setActiveProject = String::convert_from(argv[argIdx]);
				argIdx++;
			}
			continue;
		}
		if (arg == TXT("--additionalBuildSetting"))
		{
			keepAdditionalBuildSettings = false;
			argIdx++;
			if (argIdx < argc)
			{
				String argV = String::convert_from(argv[argIdx]);
				if (argV == TXT("none"))
				{
					// should be okay not to load it
				}
				else
				{
					additionalBuildSettings.push_back(argV);
				}
				argIdx++;
			}
			continue;
		}
		if (copyFile && copyFileName.is_empty())
		{
			copyFileName = arg;
			if (copyFileName[0] == '\"')
			{
				// remove quotes
				copyFileName = copyFileName.get_sub(1, copyFileName.get_length() - 2);
			}
			argIdx++;
			continue;
		}
		if (createBuildInfoFile && buildInfoFileName.is_empty())
		{
			buildInfoFileName = arg;
			if (buildInfoFileName[0] == '\"')
			{
				// remove quotes
				buildInfoFileName = buildInfoFileName.get_sub(1, buildInfoFileName.get_length() - 2);
			}
			argIdx++;
			continue;
		}
		if (fileNameHeader.is_empty())
		{
			fileNameHeader = arg;
			if (fileNameHeader[0] == '\"')
			{
				// remove quotes
				fileNameHeader = fileNameHeader.get_sub(1, fileNameHeader.get_length() - 2);
			}
			argIdx++;
			continue;
		}
		if (fileNameAndroidProperties.is_empty())
		{
			fileNameAndroidProperties = arg;
			if (fileNameAndroidProperties[0] == '\"')
			{
				// remove quotes
				fileNameAndroidProperties = fileNameAndroidProperties.get_sub(1, fileNameAndroidProperties.get_length() - 2);
			}
			argIdx++;
			continue;
		}
		if (fileNameAndroidManifest.is_empty())
		{
			fileNameAndroidManifest = arg;
			if (fileNameAndroidManifest[0] == '\"')
			{
				// remove quotes
				fileNameAndroidManifest = fileNameAndroidManifest.get_sub(1, fileNameAndroidManifest.get_length() - 2);
			}
			argIdx++;
			continue;
		}
		break;
	}

	if (fileNameHeader.is_empty())
	{
#ifndef AN_CLANG
		tprintf(TXT("no header provided to modify"));
#endif
		String::close_static();

		return 1;
	}

	// values
	int buildNo = 0;
	String activeProject;
	List<String> allProjects;
	String buildVerSuffix;

	// read from separate file
	int buildVerMajor = 0;
	int buildVerMinor = 0;
	int buildVerSubMinor = 0;

	BuildVerHVersion buildVerHVersion = BuildVerHVersion::Old;

	{
#ifndef AN_CLANG
		if (!copyFile)
		{
			tprintf(TXT("updating \"%s\" (build ver)\n"), fileNameHeader.to_char());
		}
#endif
		IO::File file;

		// header
		// read values from file
		if (file.open(fileNameHeader))
		{
			file.set_type(IO::InterfaceType::Text);
			/** requires specific format:
				// %ACTIVE_PROJECT% by hand
				// %BUILD_VER_SUFFIX% suffix
				// <- empty!
				// projects:
				//  %project 0%
				//  %project 1%
				//  %...other projects...%
				// <- empty!
				// additional build settings:
				//  %setting 0%
				//  %setting 1%
				// <- empty!
				%...information...%
				%...actual code...%
			 */
			List<String> lines;
			file.read_lines(lines);
			if (lines.get_size() > 3)
			{
				int i = 3;
				if (lines[i].get_sub(2).trim() == TXT("projects:"))
				{
					// new version
					buildVerHVersion = BuildVerHVersion::New;
					if (lines.get_size() > 1) activeProject = lines[0].get_sub(2).trim();
					if (lines.get_size() > 2) buildVerSuffix = lines[1].get_sub(2).trim();
					++i;
					while (true && i < lines.get_size())
					{
						String project = lines[i].get_sub(2).trim();
						if (project.is_empty())
						{
							++i;
							break;
						}
						allProjects.push_back(project);
						++i;
					}
					if (i <= lines.get_size() - 1  &&
						lines[i].get_sub(2).trim() == TXT("additional build settings:"))
					{
						++i; // eat "additional build settings"
						while (true && i < lines.get_size())
						{
							String abs = lines[i].get_sub(2).trim();
							if (abs.is_empty())
							{
								break;
							}
							if (keepAdditionalBuildSettings)
							{
								additionalBuildSettings.push_back(abs);
							}
							++i;
						}
					}
				}
				else
				{
					// old version
					buildVerHVersion = BuildVerHVersion::Old;
					if (lines.get_size() > 0) buildNo = ParserUtils::parse_int(lines[0].get_sub(2));
					if (lines.get_size() > 1) buildVerMajor = ParserUtils::parse_int(lines[1].get_sub(2));
					if (lines.get_size() > 2) buildVerMinor = ParserUtils::parse_int(lines[2].get_sub(2));
					if (lines.get_size() > 3) buildVerSubMinor = ParserUtils::parse_int(lines[3].get_sub(2));
					if (lines.get_size() > 4) buildVerSuffix = lines[4].get_sub(2).trim();
				}
			}
			file.close();
		}

		if (!setActiveProject.is_empty())
		{
			activeProject = setActiveProject;
		}

		if (!activeProject.is_empty())
		{
			if (fileNameHeader.get_right(2) == TXT(".h"))
			{
				fileNameProjectHeader = fileNameHeader.get_left(fileNameHeader.get_length() - 2) + String::printf(TXT("_%S.h"), activeProject.to_char());
			}
		}
		
		if (! fileNameProjectHeader.is_empty() &&
			buildVerHVersion == BuildVerHVersion::New)
		{
			if (file.open(fileNameProjectHeader))
			{
				file.set_type(IO::InterfaceType::Text);
				/** requires specific format:
					// %BUILD_NO% auto
					// %BUILD_VER_MAJOR% auto
					// %BUILD_VER_MINOR% auto
					// %BUILD_VER_SUBMINOR% auto
					%...information...%
					%...actual code...%
				 */
				List<String> lines;
				file.read_lines(lines);
				if (lines.get_size() > 0) buildNo = ParserUtils::parse_int(lines[0].get_sub(2));
				if (lines.get_size() > 1) buildVerMajor = ParserUtils::parse_int(lines[1].get_sub(2));
				if (lines.get_size() > 2) buildVerMinor = ParserUtils::parse_int(lines[2].get_sub(2));
				if (lines.get_size() > 3) buildVerSubMinor = ParserUtils::parse_int(lines[3].get_sub(2));
				file.close();
			}
		}

		if (createBuildInfoFile)
		{
			IO::File file;
			file.create(buildInfoFileName);
			file.set_type(IO::InterfaceType::Text);

			file.write_line(String::printf(TXT("%i"), buildVerMajor));
			file.write_line(String::printf(TXT("%i"), buildVerMinor));
			file.write_line(String::printf(TXT("%i"), buildVerSubMinor));
			file.write_line(String::printf(TXT("%i"), buildNo));
			file.close();
			tprintf(TXT("created build info file \"%s\" (ver \"%s\")\n"), buildInfoFileName.to_char(), fileNameHeader.to_char());
		}
		else if (copyFile)
		{
			IO::File fileSource;
			fileSource.open(copyFileName);
			int dotAt = copyFileName.find_last_of('.');
			if (dotAt != NONE)
			{
				String fileNameSuffix;
				if (forCopyFileUseBuildNo)
				{
					fileNameSuffix = String::printf(TXT("_%i"), buildNo);
				}
				else
				{
					fileNameSuffix = String::printf(TXT("_v_%i_%i_%i_%i"), buildVerMajor, buildVerMinor, buildVerSubMinor, buildNo);
				}
				if (!buildVerSuffix.is_empty())
				{
					fileNameSuffix += TXT("_");
					fileNameSuffix += buildVerSuffix;
				}
				String copyTo = copyFileName.get_left(dotAt) + fileNameSuffix + copyFileName.get_sub(dotAt);
				tprintf(TXT("copying \"%s\" to \"%s\" (ver \"%s\")\n"), copyFileName.to_char(), copyTo.to_char(), fileNameHeader.to_char());
				IO::File fileDest;
				fileDest.create(copyTo);
				byte buffer[16384];
				while (int readCount = fileSource.read(buffer, 16384))
				{
					fileDest.write(buffer, readCount);
				}
			}
		}
		else
		{
			if (updateMajor)
			{
				buildVerMajor++;
				buildVerMinor = 0;
				buildVerSubMinor = 0;
			}
			else if (updateMinor)
			{
				buildVerMinor++;
				buildVerSubMinor = 0;
			}
			else if (updateSubMinor)
			{
				buildVerSubMinor++;
			}

			if (updateBuildNo)
			{
				++buildNo;
			}

			String dateAndTime;
			int buildMoment = 0;
			{
				time_t currentTime = time(0);
				tm currentTimeInfo;
				tm* pcurrentTimeInfo = &currentTimeInfo;
#ifdef AN_CLANG
				pcurrentTimeInfo = localtime(&currentTime);
#else
				localtime_s(&currentTimeInfo, &currentTime);
#endif
				dateAndTime += String::printf(TXT("%04i-%02i-%02i"), pcurrentTimeInfo->tm_year + 1900, pcurrentTimeInfo->tm_mon + 1, pcurrentTimeInfo->tm_mday);
				dateAndTime += String::printf(TXT(" %02i:%02i:%02i"), pcurrentTimeInfo->tm_hour, pcurrentTimeInfo->tm_min, pcurrentTimeInfo->tm_sec);

				buildMoment = (int)(currentTime / (60 * 60 * 24));
			}

			buildVerSuffix = buildPreviewPublicReleaseCandidate ? TXT("candidate") : (buildPreview ? TXT("preview") : TXT(""));
			if (buildDemo)
			{
				buildVerSuffix = (String(TXT("demo ")) + buildVerSuffix).trim();
			}

			// write file
			if (file.create(fileNameHeader))
			{
				file.set_type(IO::InterfaceType::Text);
				// header in specific format (check above)
				if (buildVerHVersion == BuildVerHVersion::New)
				{
					file.write_text(String::printf(TXT("// %S\n"), activeProject.to_char()));
				}
				else
				{
					file.write_text(String::printf(TXT("// %i\n"), buildNo));
					file.write_text(String::printf(TXT("// %i\n"), buildVerMajor));
					file.write_text(String::printf(TXT("// %i\n"), buildVerMinor));
					file.write_text(String::printf(TXT("// %i\n"), buildVerSubMinor));
				}
				file.write_text(String::printf(TXT("// %S\n"), buildVerSuffix.to_char()));
				if (buildVerHVersion == BuildVerHVersion::New)
				{
					file.write_text(String(TXT("//\n")));
					file.write_text(String(TXT("// projects:\n")));
					for_every(project, allProjects)
					{
						file.write_text(String::printf(TXT("//  %S\n"), project->to_char()));
					}
					file.write_text(String(TXT("//\n")));
					file.write_text(String(TXT("// additional build settings:\n")));
					for_every(abs, additionalBuildSettings)
					{
						file.write_text(String::printf(TXT("//  %S\n"), abs->to_char()));
					}
					file.write_text(String(TXT("//\n")));
				}
				// information
				{
					String updatedOn;
					updatedOn += TXT("// updated on ");
					updatedOn += dateAndTime;
					updatedOn += TXT("\n");
					file.write_text(updatedOn);
				}
				String useBuildVerSuffix = buildVerSuffix;
				if (buildPreview)
				{
					useBuildVerSuffix += TXT(" ");
					useBuildVerSuffix += dateAndTime;
				}
				// actual code
				if (buildVerHVersion == BuildVerHVersion::Old)
				{
					file.write_text(String::printf(TXT("#define BUILD_NO %i\n"), buildNo));
					file.write_text(String::printf(TXT("#define BUILD_VER_MAJOR %i\n"), buildVerMajor));
					file.write_text(String::printf(TXT("#define BUILD_VER_MINOR %i\n"), buildVerMinor));
					file.write_text(String::printf(TXT("#define BUILD_VER_SUBMINOR %i\n"), buildVerSubMinor));
				}
				file.write_text(String::printf(TXT("#define BUILD_VER_SUFFIX \"%S\"\n"), useBuildVerSuffix.to_char()));
				file.write_text(String::printf(TXT("#define BUILD_DATE \"%S\"\n"), dateAndTime.to_char()));
				file.write_text(String::printf(TXT("#define BUILD_MOMENT %i\n"), buildMoment));
				file.write_text(String(TXT("\n")));
				if (!activeProject.is_empty() &&
					buildVerHVersion == BuildVerHVersion::New)
				{
					file.write_text(String(TXT("// additional build settings\n")));
					for_every(abs, additionalBuildSettings)
					{
						file.write_text(String::printf(TXT("#define %S\n"), abs->to_char()));
					}
					file.write_text(String(TXT("\n")));
					file.write_text(String(TXT("// active project\n")));
					file.write_text(String::printf(TXT("#define AN_%S\n"), activeProject.to_upper().to_char()));
					file.write_text(String::printf(TXT("#include \"buildVer_%S.h\"\n"), activeProject.to_char()));
					file.write_text(String::printf(TXT("#include \"buildVer_%S_definitions.h\"\n"), activeProject.to_char()));
					file.write_text(String(TXT("\n")));
				}
				file.write_text(String(TXT("// demo release\n")));
				file.write_text(String::printf(TXT("%S#define BUILD_DEMO\n"), buildDemo ? TXT("") : TXT("//")));
				file.write_text(String(TXT("\n")));
				file.write_text(String(TXT("// non release build for development\n")));
				file.write_text(String::printf(TXT("%S#define BUILD_NON_RELEASE\n"), !buildPreviewRelease && !buildPreviewPublicReleaseCandidate && !buildPublicRelease ? TXT("") : TXT("//")));
				file.write_text(String(TXT("\n")));
				file.write_text(String(TXT("// preview build for development\n")));
				file.write_text(String::printf(TXT("%S#define BUILD_PREVIEW\n"), buildPreview ? TXT("") : TXT("//")));
				file.write_text(String(TXT("\n")));
				file.write_text(String(TXT("// preview build available publicly\n")));
				file.write_text(String::printf(TXT("%S#define BUILD_PREVIEW_RELEASE\n"), buildPreviewRelease ? TXT("") : TXT("//")));
				file.write_text(String(TXT("\n")));
				file.write_text(String(TXT("// public release (preview) candidate\n")));
				file.write_text(String::printf(TXT("%S#define BUILD_PREVIEW_PUBLIC_RELEASE_CANDIDATE\n"), buildPreviewPublicReleaseCandidate ? TXT("") : TXT("//")));
				file.write_text(String(TXT("\n")));
				file.write_text(String(TXT("// public release, official build\n")));
				file.write_text(String::printf(TXT("%S#define BUILD_PUBLIC_RELEASE\n"), buildPublicRelease ? TXT("") : TXT("//")));
				file.write_text(String(TXT("\n")));
				file.write_text(String(TXT("// seeing this version means that all changes are done for this version\n")));
				file.write_text(String(TXT("// the version number gets updated after a build\n")));
				file.write_text(String(TXT("\n")));
				file.close();

				show_file_contents(fileNameHeader);
			}

			// write file
			if (!fileNameProjectHeader.is_empty() &&
				buildVerHVersion == BuildVerHVersion::New &&
				file.create(fileNameProjectHeader))
			{
				file.set_type(IO::InterfaceType::Text);
				// header in specific format (check above)
				file.write_text(String::printf(TXT("// %i\n"), buildNo));
				file.write_text(String::printf(TXT("// %i\n"), buildVerMajor));
				file.write_text(String::printf(TXT("// %i\n"), buildVerMinor));
				file.write_text(String::printf(TXT("// %i\n"), buildVerSubMinor));
				// information
				{
					String updatedOn;
					updatedOn += TXT("// updated on ");
					updatedOn += dateAndTime;
					updatedOn += TXT("\n");
					file.write_text(updatedOn);
				}
				// actual code
				file.write_text(String::printf(TXT("#define BUILD_NO %i\n"), buildNo));
				file.write_text(String::printf(TXT("#define BUILD_VER_MAJOR %i\n"), buildVerMajor));
				file.write_text(String::printf(TXT("#define BUILD_VER_MINOR %i\n"), buildVerMinor));
				file.write_text(String::printf(TXT("#define BUILD_VER_SUBMINOR %i\n"), buildVerSubMinor));
				file.close();

				show_file_contents(fileNameHeader);
			}

		}

	}

	if (!fileNameAndroidProperties.is_empty() && !copyFile)
	{
#ifndef AN_CLANG
		tprintf(TXT("updating \"%s\" (properties)\n"), fileNameAndroidProperties.to_char());
#endif
		IO::File file;

		// header
		// read values from file
		if (file.open(fileNameAndroidProperties))
		{
			List<String> lines;

			file.set_type(IO::InterfaceType::Text);
			file.read_lines(lines);

			file.close();

			bool hasVersionCode = false;
			bool hasVersionName = false;
			String versionCodeString = String::printf(TXT("version.code=%i"), buildNo);
			String versionNameString = String::printf(TXT("version.name=%i.%i.%i%S"), buildVerMajor, buildVerMinor, buildVerSubMinor, buildVerSuffix.to_char()).trim();

			// replace values in lines
			for_every(line, lines)
			{
				if (line->has_prefix(TXT("version.code=")))
				{
					*line = versionCodeString;
					hasVersionCode = true;
				}
				if (line->has_prefix(TXT("version.name=")))
				{
					*line = versionNameString;
					hasVersionName = true;
				}
			}

			// remove last empty lines
			while (!lines.is_empty() && lines.get_last().is_empty())
			{
				lines.pop_back();
			}

			if (!hasVersionCode)
			{
				lines.push_back(versionCodeString);
				hasVersionCode = true;
			}
			if (!hasVersionName)
			{
				lines.push_back(versionNameString);
				hasVersionName = true;
			}

			// add one empty line at the end
			lines.push_back(String::empty());

			if (file.create(fileNameAndroidProperties))
			{
				file.set_type(IO::InterfaceType::Text);
				for_every(line, lines)
				{
					file.write_line(*line);
				}
				file.close();

				show_file_contents(fileNameAndroidProperties);
			}

		}

	}

	if (!fileNameAndroidManifest.is_empty() && !copyFile)
	{
#ifndef AN_CLANG
		tprintf(TXT("updating \"%s\" (manifest)\n"), fileNameAndroidManifest.to_char());
#endif
		IO::File file;

		// header
		// read values from file
		if (file.open(fileNameAndroidManifest))
		{
			List<String> lines;

			file.set_type(IO::InterfaceType::Text);
			file.read_lines(lines);

			file.close();

			String packageString(TXT("package=\""));

			bool hasPackage = false;
			// replace values in lines
			for_every(line, lines)
			{
				if (line->does_contain(packageString) && !androidPackageName.is_empty())
				{
					int startsAt = line->find_first_of(packageString);
					if (startsAt != NONE)
					{
						startsAt += packageString.get_length();
						if (startsAt < line->get_length())
						{
							int endsAt = line->find_first_of('"', startsAt);
							if (endsAt != NONE)
							{
								*line = line->get_left(startsAt) + androidPackageName + line->get_sub(endsAt);
								hasPackage = true;
							}
						}
					}
				}
			}

			// remove last empty lines
			while (!lines.is_empty() && lines.get_last().is_empty())
			{
				lines.pop_back();
			}

			// add one empty line at the end
			lines.push_back(String::empty());

			if (!hasPackage && !androidPackageName.is_empty())
			{
#ifndef AN_CLANG
				tprintf(TXT("could not find \"package\" info in \"%s\"\n"), fileNameAndroidManifest.to_char());
#endif
			}
			if (file.create(fileNameAndroidManifest))
			{
				file.set_type(IO::InterfaceType::Text);
				for_every(line, lines)
				{
					file.write_line(*line);
				}
				file.close();

				show_file_contents(fileNameAndroidManifest);
			}

		}

	}

	String::close_static();

	return 0;
}
