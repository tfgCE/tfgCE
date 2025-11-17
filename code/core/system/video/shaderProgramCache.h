#pragma once

#include "..\..\globalDefinitions.h"
#include "..\..\containers\array.h"
#include "..\..\types\string.h"
#include "..\..\io\digested.h"

class Serialiser;

namespace System
{
	class Video3D;

	struct ShaderProgramHostInfo
	{
		String version;
		String vendor;
		String renderer;
		int buildNo;

		void update(Video3D* _v3d);
		bool serialise(Serialiser& _serialiser);
		static bool compare(ShaderProgramHostInfo const& _a, ShaderProgramHostInfo const& _b, bool _buildNoHasToMatch) { return _a.version == _b.version && _a.vendor == _b.vendor && _a.renderer == _b.renderer && (!_buildNoHasToMatch || _a.buildNo == _b.buildNo); }
	};

	struct ShaderProgramCached
	{
		String id;
		ShaderProgramHostInfo host;
		IO::Digested digestedVertexShaderSource;
		IO::Digested digestedFragmentShaderSource;
		int binaryFormat;
		Array<int8> binary;
		bool autoCache = false; // this means that it should be stored in the auto cache

		void set_with(String const& _id, ShaderProgramHostInfo const& _host, IO::Digested const& _digestedVertexShaderSource, IO::Digested const& _digestedFragmentShaderSource, int _binaryFormat, Array<int8> const& _binary, bool _autoCache);
		bool serialise(Serialiser& _serialiser);
	};

	class ShaderProgramCache
	{
	public:
		ShaderProgramCache(Video3D* _v3d);
		~ShaderProgramCache();

		bool is_empty() const { return shaderPrograms.is_empty(); }

		bool load_from_files();
		bool save_to_file(bool _force = false);
		void clear();

		bool get_binary(String const & _id, OUT_ int & _binaryFormat, OUT_ Array<int8> & _binary) const; // returns true if shader program was found (doesn't check for vertex/fragment compatibility/validity)
		bool get_binary(String const & _id, String const & _vertexShader, String const & _fragmentShader, OUT_ int & _binaryFormat, OUT_ Array<int8> & _binary) const; // returns true if shader program was found
		void set_binary(String const & _id, String const & _vertexShader, String const & _fragmentShader, int _binaryFormat, Array<int8> const & _binary); // called only for compiled right now -> auto cache

	private:
		Video3D* v3d;

		String autoFullFilePath;

		ShaderProgramHostInfo host; // current host

		List<ShaderProgramCached> shaderPrograms;
		bool requiresSaving = true;

		bool load_from_file(String const & fullFilePath, bool _assetFile, bool _autoCache);

		void set_binary_internal(String const& _id, ShaderProgramHostInfo const& _host, IO::Digested const& _digestedVertexShaderSource, IO::Digested const& _digestedFragmentShaderSource, int _binaryFormat, Array<int8> const& _binary, bool _requiresSaving, bool _autoCache);
	};

};

 