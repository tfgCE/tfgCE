#include "fbx.h"

#ifdef USE_FBX
#include "..\..\core\splash\splashInfo.h"
#endif

void FBX::initialise_static()
{
#ifdef USE_FBX
	Splash::Info::add_info(String(TXT("This software contains Autodesk(R) FBX(R) code developed by Autodesk, Inc.Copyright 2014 Autodesk, Inc.All rights, reserved. Such code is provided \"as is\" and Autodesk, Inc.disclaims any and all warranties, whether express or implied, including without limitation the implied warranties of merchantability, fitness for a particular purpose or non - infringement of third party rights. In no event shall Autodesk, Inc.be liable for any direct, indirect, incidental, special, exemplary, or consequential damages (including, but not limited to, procurement of substitute goods or services; loss of use, data, or profits; or business interruption) however caused and on any theory of liability, whether in contract, strict liability, or tort (including negligence or otherwise) arising in any way out of such code.")));

	FBX::Manager::initialise_static();
	an_assert(FBX::Manager::is_valid());

	FBX::AnimationSetExtractor::initialise_static();
	FBX::Mesh3DExtractor::initialise_static();
	FBX::SocketsExtractor::initialise_static();
	FBX::SkeletonExtractor::initialise_static();
#endif
}

void FBX::close_static()
{
#ifdef USE_FBX
	FBX::Importer::finished_importing();

	FBX::AnimationSetExtractor::close_static();
	FBX::Mesh3DExtractor::close_static();
	FBX::SocketsExtractor::close_static();
	FBX::SkeletonExtractor::close_static();

	FBX::Manager::close_static();
#endif
}
