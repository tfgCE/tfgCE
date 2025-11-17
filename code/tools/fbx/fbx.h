#pragma once

namespace FBX
{
	void initialise_static();
	void close_static();
};

#include "fbxLib.h"

#include "fbxUtils.h"

#include "fbxManager.h"
#include "fbxScene.h"
#include "fbxImporter.h"
#include "fbxAnimationSetExtractor.h"
#include "fbxMesh3dExtractor.h"
#include "fbxSocketsExtractor.h"
#include "fbxSkeletonExtractor.h"