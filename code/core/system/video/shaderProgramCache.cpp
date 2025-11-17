#include "shaderProgramCache.h"
#include "video3d.h"

#include "..\..\buildInformation.h"
#include "..\..\mainSettings.h"
#include "..\..\io\assetDir.h"
#include "..\..\io\assetFile.h"
#include "..\..\io\dir.h"
#include "..\..\io\file.h"
#include "..\..\serialisation\serialiser.h"
#include "..\..\system\core.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

#define VER_BASE 0
#define VER_HOST_INCLUDED 1
#define CURRENT_VERSION VER_HOST_INCLUDED

//

using namespace System;

//

void ShaderProgramCached::set_with(String const& _id, ShaderProgramHostInfo const& _host, IO::Digested const& _digestedVertexShaderSource, IO::Digested const& _digestedFragmentShaderSource, int _binaryFormat, Array<int8> const& _binary, bool _autoCache)
{
	id = _id;
	host = _host;
	digestedVertexShaderSource = _digestedVertexShaderSource;
	digestedFragmentShaderSource = _digestedFragmentShaderSource;
	binaryFormat = _binaryFormat;
	binary = _binary;
	binary.prune();
	autoCache = _autoCache;
}

bool ShaderProgramCached::serialise(Serialiser & _serialiser)
{
	bool result = true;
	result &= serialise_data(_serialiser, id);
	result &= serialise_data(_serialiser, host);
	result &= serialise_data(_serialiser, digestedVertexShaderSource);
	result &= serialise_data(_serialiser, digestedFragmentShaderSource);
	result &= serialise_data(_serialiser, binaryFormat);
	result &= serialise_plain_data_array(_serialiser, binary);
	return result;
}

//

void ShaderProgramHostInfo::update(Video3D* _v3d)
{
	version = String::from_char8((char8 *)glGetString(GL_VERSION));
	vendor = String::from_char8((char8 *)glGetString(GL_VENDOR));
	renderer = String::from_char8((char8 *)glGetString(GL_RENDERER));
	buildNo = get_build_version_build_no();
}

bool ShaderProgramHostInfo::serialise(Serialiser & _serialiser)
{
	bool result = true;
	result &= serialise_data(_serialiser, version);
	result &= serialise_data(_serialiser, vendor);
	result &= serialise_data(_serialiser, renderer);
	result &= serialise_data(_serialiser, buildNo);
	return result;
}

//

ShaderProgramCache::ShaderProgramCache(Video3D* _v3d)
: v3d(_v3d)
{
	autoFullFilePath = IO::get_user_game_data_directory();
	autoFullFilePath += IO::get_auto_directory_name() + IO::get_directory_separator();
	autoFullFilePath += String(TXT("shaderProgramCache.")) + System::Core::get_system_precise_name();
	host.update(v3d);
}

ShaderProgramCache::~ShaderProgramCache()
{
}

bool ShaderProgramCache::load_from_files()
{
	if (MainSettings::global().should_force_reimporting())
	{
		// ignore loading shader program cache
		return true;
	}

	output_colour(0, 1, 1, 0);
	output(TXT("load shader program caches"));
	output_colour();

	bool result = true;

	{	// try to load all for this platform, if host mismatches, will be skipped}
		{	// first asset dir
			IO::AssetDir dir;
			String dirPath = IO::get_asset_data_directory();
			dirPath += String(TXT("shaderProgramCaches"));
			if (dir.list(dirPath))
			{
				for_every(file, dir.get_files())
				{
					if (file->does_contain(TXT("shaderProgramCache.")) &&
						! file->does_contain(TXT(".bat")))
					{
						String filePath = dirPath + IO::get_directory_separator();
						filePath += *file;
						result &= load_from_file(filePath, true, false);
					}
				}
			}
		}
		if (IO::get_user_game_data_directory() != IO::get_asset_data_directory())
		{	// then user dir
			IO::Dir dir;
			String dirPath = IO::get_user_game_data_directory();
			dirPath += String(TXT("shaderProgramCaches"));
			if (dir.list(dirPath))
			{
				for_every(file, dir.get_files())
				{
					if (file->does_contain(TXT("shaderProgramCache.")) &&
						! file->does_contain(TXT(".bat")))
					{
						String filePath = dirPath + IO::get_directory_separator();
						filePath += *file;
						result &= load_from_file(filePath, false, false);
					}
				}
			}
		}
	}

	{
		// auto at the end
		result &= load_from_file(autoFullFilePath, false, true);
	}

	output_colour(0, 1, 1, 0);
	output(TXT("loaded shader program caches"));
	output_colour();
	output(TXT("  shader programs: %i"), shaderPrograms.get_size());

	return result;
}

bool ShaderProgramCache::load_from_file(String const& fullFilePath, bool _assetFile, bool _autoCache)
{
	bool result = true;

	requiresSaving = false;

	IO::Interface* source = nullptr;
	IO::File file;
#ifdef AN_ANDROID
	IO::AssetFile assetFile;
	if (_assetFile)
	{
		assetFile.open(fullFilePath);
		assetFile.set_type(IO::InterfaceType::Binary);
		if (assetFile.is_open())
		{
			source = &assetFile;
		}
	}
	else
#endif
	{
		file.open(fullFilePath);
		file.set_type(IO::InterfaceType::Binary);
		if (file.is_open())
		{
			source = &file;
		}
	}

	if (source)
	{
		output(TXT("  load shader program cache from \"%S\""), fullFilePath.to_char());

		ShaderProgramHostInfo readHost;
		Serialiser serialiser = Serialiser::reader(source);
		uint16 version = 0;
		serialise_data(serialiser, version);
		if (version != CURRENT_VERSION)
		{
			// don't even try to read, we will be recompiling
			result = false;
		}
		else
		{
			if (readHost.serialise(serialiser))
			{
				if (ShaderProgramHostInfo::compare(host, readHost, false))
				{
					List<ShaderProgramCached> tempShaderPrograms;
					if (serialise_data(serialiser, tempShaderPrograms))
					{
						for_every(sp, tempShaderPrograms)
						{
							set_binary_internal(sp->id, sp->host, sp->digestedVertexShaderSource, sp->digestedFragmentShaderSource, sp->binaryFormat, sp->binary, false, _autoCache);
						}
					}
					else
					{
						result = false;
					}
				}
			}
			else
			{
				result = false;
			}
		}

		if (result)
		{
			output(TXT("    loaded shader program cache"));
		}
	}
	else
	{
		output(TXT("  no shader program cache to load at \"%S\""), fullFilePath.to_char());
	}

	if (!result)
	{
		output_colour(1, 0, 0, 1);
		output(TXT("    invalid shader program cache"));
		output_colour();
	}

	return result;
}

bool ShaderProgramCache::save_to_file(bool _force)
{
	if (!_force && !requiresSaving)
	{
		// no need to save
		return true;
	}

	output_colour(0, 1, 1, 0);
	output(TXT("save shader program cache (%i) to \"%S\""), shaderPrograms.get_size(), autoFullFilePath.to_char());
	output_colour();

	requiresSaving = false;

	bool result = true;

	IO::File file;
	file.create(autoFullFilePath);
	file.set_type(IO::InterfaceType::Binary);

	if (file.is_open())
	{
		Serialiser serialiser = Serialiser::writer(&file);
		uint16 version = CURRENT_VERSION;
		serialise_data(serialiser, version);

		result &= serialise_data(serialiser, host);

		List<ShaderProgramCached> tempShaderPrograms;
		for_every(sp, shaderPrograms)
		{
#ifdef BUILD_NON_RELEASE
			// for non release, check host info, to only save for the current platform (we use it to refresh shader cache)
			if (ShaderProgramHostInfo::compare(sp->host, host, false))
#else
			// for release, save only those who are part of auto cache (old ones or new ones, doesn't matter)
			if (sp->autoCache)
#endif
			{
				tempShaderPrograms.push_back(*sp);
			}
		}
		result &= serialise_data(serialiser, tempShaderPrograms);
	}
	else
	{
		error(TXT("could not save shader program cache to \"%S\""), (autoFullFilePath).to_char());
		result = false;
	}

	return result;
}

void ShaderProgramCache::clear()
{
	requiresSaving = true;
	shaderPrograms.clear();
}

bool ShaderProgramCache::get_binary(String const & _id, OUT_ int & _binaryFormat, OUT_ Array<int8> & _binary) const
{
	// this function is to do an early check without processing shader sources
	// for release builds, we choose this path to avoid putting shader sources together to compare to shader program cache
	// as the game should be released only with proper shader program cache

	if (MainSettings::global().should_force_reimporting())
	{
		// never get binary if reimporting is forced
		return false;
	}
	
#ifdef BUILD_NON_RELEASE
	// if non release, allow to recompile things (we will compare sources)
	{
		return false;
	}
#endif
	// for release build, if host info matches, we should use it
	// release builds always have different buildNo, it doesn't have to match (hence false)
	{
		for_every(shaderProgram, shaderPrograms)
		{
			if (shaderProgram->id == _id &&
				ShaderProgramHostInfo::compare(shaderProgram->host, host, false))
			{
				_binaryFormat = shaderProgram->binaryFormat;
				_binary = shaderProgram->binary;
				return true;
			}
		}
	}

	return false;
}

bool ShaderProgramCache::get_binary(String const & _id, String const & _vertexShader, String const & _fragmentShader, OUT_ int & _binaryFormat, OUT_ Array<int8> & _binary) const
{
	for_every(shaderProgram, shaderPrograms)
	{
		if (shaderProgram->id == _id &&
			ShaderProgramHostInfo::compare(shaderProgram->host, host, false))
		{
			IO::Digested digestedVertexShaderSource;
			IO::Digested digestedFragmentShaderSource;
			digestedVertexShaderSource.digest(_vertexShader);
			digestedFragmentShaderSource.digest(_fragmentShader);
			if (shaderProgram->digestedVertexShaderSource == digestedVertexShaderSource &&
				shaderProgram->digestedFragmentShaderSource == digestedFragmentShaderSource)
			{
				_binaryFormat = shaderProgram->binaryFormat;
				_binary = shaderProgram->binary;
				return true;
			}
			else
			{
				return false;
			}
		}
	}

	return false;
}

void ShaderProgramCache::set_binary(String const& _id, String const& _vertexShader, String const& _fragmentShader, int _binaryFormat, Array<int8> const& _binary)
{
	IO::Digested digestedVertexShaderSource;
	IO::Digested digestedFragmentShaderSource;
	digestedVertexShaderSource.digest(_vertexShader);
	digestedFragmentShaderSource.digest(_fragmentShader);
	set_binary_internal(_id, host, digestedVertexShaderSource, digestedFragmentShaderSource, _binaryFormat, _binary, true, true);
}

void ShaderProgramCache::set_binary_internal(String const & _id, ShaderProgramHostInfo const& _host, IO::Digested const& _digestedVertexShaderSource, IO::Digested const& _digestedFragmentShaderSource, int _binaryFormat, Array<int8> const & _binary, bool _requiresSaving, bool _autoCache)
{
	requiresSaving = _requiresSaving;

	for_every(shaderProgram, shaderPrograms)
	{
		if (shaderProgram->id == _id &&
			ShaderProgramHostInfo::compare(shaderProgram->host, _host, false))
		{
			shaderProgram->set_with(_id, _host, _digestedVertexShaderSource, _digestedFragmentShaderSource, _binaryFormat, _binary, _autoCache);
			return;
		}
	}

	shaderPrograms.push_back(ShaderProgramCached());
	shaderPrograms.get_last().set_with(_id, _host, _digestedVertexShaderSource, _digestedFragmentShaderSource, _binaryFormat, _binary, _autoCache);
}

