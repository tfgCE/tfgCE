#include "fbxAnimationSetExtractor.h"

#include "fbxScene.h"
#include "fbxUtils.h"

#include "fbxImporter.h"

#include "..\..\core\debug\extendedDebug.h"
#include "..\..\core\memory\memory.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

#ifdef USE_FBX
using namespace FBX;

//

void AnimationSetExtractor::initialise_static()
{
	Meshes::AnimationSet::importer().register_file_type_with_options(String(TXT("fbx")),
		[](String const & _fileName, Meshes::AnimationSetImportOptions const & _options)
		{
			Concurrency::ScopedSpinLock lock(FBX::Importer::importerLock);
			Meshes::AnimationSet* animationSet = nullptr;
			if (FBX::Scene* scene = FBX::Importer::import(_fileName))
			{
				animationSet = AnimationSetExtractor::extract_from(scene, _options);
			}
			else
			{
				error(TXT("problem importing fbx from file \"%S\""), _fileName.to_char());
			}
			return animationSet;
		}
	);
}

void AnimationSetExtractor::close_static()
{
	Meshes::AnimationSet::importer().unregister(String(TXT("fbx")));
}

Meshes::AnimationSet* AnimationSetExtractor::extract_from(Scene* _scene, Meshes::AnimationSetImportOptions const & _options)
{
	AnimationSetExtractor extractor(_scene);
	extractor.extract_animation_set(_options);
	return extractor.animationSet;
}

AnimationSetExtractor::AnimationSetExtractor(Scene* _scene)
: scene(_scene)
, animationSet(nullptr)
{
	an_assert(scene);
}

AnimationSetExtractor::~AnimationSetExtractor()
{
}

void AnimationSetExtractor::extract_animation_set(Meshes::AnimationSetImportOptions const & _options)
{
	an_assert(scene);
	an_assert(!animationSet);

	animationSet = Meshes::AnimationSet::create_new();

	if (auto* fbxScene = scene->get_scene())
	{
		if (FbxNode* rootNode = scene->get_node(_options.importFromNode))
		{
			boneExtractor.extract_from_root_node(rootNode, _options);
		}

		Optional<int> rootMotionTrackIdx;

		// prepare all tracks
		{
			if (!_options.importRootMotionFromNode.is_empty())
			{
				rootMotionTrackIdx = animationSet->add_root_motion_track(Name(_options.importRootMotionFromNode));
				an_assert(rootMotionTrackIdx.get() >= 0);
			}
			for_every(boneInfo, boneExtractor.extracted)
			{
				if (_options.importRootMotionFromNode.is_empty() ||
					_options.importRootMotionFromNode != boneInfo->boneName.to_string())
				{
					animationSet->add_track(_options.importRotationOnly ? Meshes::AnimationTrack::Type::Quat : Meshes::AnimationTrack::Type::Transform, boneInfo->boneName);
				}
			}
		}

		animationSet->prepare_to_use();

		for (int animIdx = 0; animIdx < fbxScene->GetSrcObjectCount<FbxAnimStack>(); ++animIdx)
		{
			FbxAnimStack* animStack = fbxScene->GetSrcObject<FbxAnimStack>(animIdx);
			String animationName = String::from_char8(animStack->GetName());
			if (!_options.importRemovePrefixFromAnimationNames.is_empty() &&
				animationName.has_prefix(_options.importRemovePrefixFromAnimationNames))
			{
				animationName = animationName.get_sub(_options.importRemovePrefixFromAnimationNames.get_length());
			}

#ifdef AN_ALLOW_EXTENDED_DEBUG
			IF_EXTENDED_DEBUG(FbxImport)
			{
				output(TXT("import anim \"%S\""), animationName.to_char());
			}
#endif

			int animLayersCount = animStack->GetMemberCount<FbxAnimLayer>();

			float timeStart = 10000.0f;
			float timeEnd = 0.0f;

			struct Utils
			{
				enum CurveFlags
				{
					X           = 0x0001,
					Y           = 0x0002,
					Z           = 0x0004,
					Translation = 0x0100,
					Rotation    = 0x0200,
					Scaling     = 0x0400,
				};

				static void for_every_anim_curve(FbxNode* node, FbxAnimLayer* animLayer, std::function<void(FbxAnimCurve* _curve, int _curveFlags)> _do)
				{
					FbxAnimCurve* animCurve = nullptr;
					if (animCurve = node->LclTranslation.GetCurve(animLayer, FBXSDK_CURVENODE_COMPONENT_X))
					{
						_do(animCurve, CurveFlags::Translation | CurveFlags::X);
					}
					if (animCurve = node->LclTranslation.GetCurve(animLayer, FBXSDK_CURVENODE_COMPONENT_Y))
					{
						_do(animCurve, CurveFlags::Translation | CurveFlags::Y);
					}
					if (animCurve = node->LclTranslation.GetCurve(animLayer, FBXSDK_CURVENODE_COMPONENT_Z))
					{
						_do(animCurve, CurveFlags::Translation | CurveFlags::Z);
					}
					if (animCurve = node->LclRotation.GetCurve(animLayer, FBXSDK_CURVENODE_COMPONENT_X))
					{
						_do(animCurve, CurveFlags::Rotation | CurveFlags::X);
					}
					if (animCurve = node->LclRotation.GetCurve(animLayer, FBXSDK_CURVENODE_COMPONENT_Y))
					{
						_do(animCurve, CurveFlags::Rotation | CurveFlags::Y);
					}
					if (animCurve = node->LclRotation.GetCurve(animLayer, FBXSDK_CURVENODE_COMPONENT_Z))
					{
						_do(animCurve, CurveFlags::Rotation | CurveFlags::Z);
					}
					if (animCurve = node->LclScaling.GetCurve(animLayer, FBXSDK_CURVENODE_COMPONENT_X))
					{
						_do(animCurve, CurveFlags::Scaling | CurveFlags::X);
					}
					if (animCurve = node->LclScaling.GetCurve(animLayer, FBXSDK_CURVENODE_COMPONENT_Y))
					{
						_do(animCurve, CurveFlags::Scaling | CurveFlags::Y);
					}
					if (animCurve = node->LclScaling.GetCurve(animLayer, FBXSDK_CURVENODE_COMPONENT_Z))
					{
						_do(animCurve, CurveFlags::Scaling | CurveFlags::Z);
					}
				}
			};

			// go through all layers and nodes to get the length of the anim - how much keys do we have
			float autoFramesPerSecond = 30.0f;
			{
				float lowestKeyInterval = 1000.0f;
				for (int animLayerIdx = 0; animLayerIdx < animLayersCount; ++animLayerIdx)
				{
					if (FbxAnimLayer* animLayer = animStack->GetMember<FbxAnimLayer>(animLayerIdx))
					{
						for_every(bone, boneExtractor.extracted)
						{
							if (auto* node = bone->node)
							{
								Utils::for_every_anim_curve(node, animLayer, [&timeStart, &timeEnd, node, &lowestKeyInterval](FbxAnimCurve* _curve, int _curveNode)
									{
										//String nodeName = String::from_char8(node->GetName());
										//String curveName = String::from_char8(_curve->GetName());
										//output(TXT(" +- %S:%S %X"), nodeName.to_char(), curveName.to_char(), _curveNode);
										float prevKeyTime = 0.0f;
										for_count(int, keyIdx, _curve->KeyGetCount())
										{
											auto& key = _curve->KeyGet(keyIdx);
											{
												float keyTime = (float)key.GetTime().GetSecondDouble();
												timeStart = min(timeStart, keyTime);
												timeEnd = max(timeEnd, keyTime);
												float keyInterval = abs(keyTime - prevKeyTime);
												if (keyInterval > 0.0f)
												{
													lowestKeyInterval = min(keyInterval, lowestKeyInterval);
												}
												prevKeyTime = keyTime;
											}
										}
									});
							}
						}
					}
				}
				if (lowestKeyInterval > 0.0f)
				{
					autoFramesPerSecond = 1.0f / lowestKeyInterval;
				}
			}

			int overrideCount = 1;
			for (int overrideIdx = 0; overrideIdx < overrideCount; ++overrideIdx)
			{
				float framesPerSecond = _options.importFramesPerSecond;
				if (framesPerSecond == 0.0f)
				{
					framesPerSecond = autoFramesPerSecond;
				}

				int frameCount = 2;
				String newAnimationName = animationName;

				// read animation details overrides
				{
					int readingOverrideIdx = 0;
					for_every(ad, _options.animationDetails)
					{
						if (String::compare_icase(ad->id, animationName))
						{
							if (overrideIdx == readingOverrideIdx)
							{
								if (!ad->importAs.is_empty())
								{
									newAnimationName = ad->importAs;
								}
								if (!ad->time.is_empty())
								{
									timeStart = ad->time.min;
									timeEnd = ad->time.max;
								}
								if (ad->framesPerSecond > 0.0f)
								{
									framesPerSecond = ad->framesPerSecond;
								}
							}
							++readingOverrideIdx;
						}
					}
					overrideCount = readingOverrideIdx;
					frameCount = TypeConversions::Normal::f_i_closest((timeEnd - timeStart) * framesPerSecond + 1.0f);
					frameCount = max(2, frameCount);
					framesPerSecond = timeEnd > timeStart? (float)(frameCount - 1) / (timeEnd - timeStart) : framesPerSecond;
#ifdef AN_ALLOW_EXTENDED_DEBUG
					IF_EXTENDED_DEBUG(FbxImport)
					{
						output(TXT(" time %8.3f -> %8.3f, frames: %3i (fps:%6.3f) : %S"), timeStart, timeEnd, frameCount, framesPerSecond, newAnimationName.to_char());
					}
#endif
				}

				int animationSetAnimIdx = animationSet->add_animation(Name(newAnimationName), frameCount, timeEnd - timeStart);

				// use global anim evaluator to stop at specific points in time to evaluate bones in root space
				{
					fbxScene->SetCurrentAnimationStack(animStack);

					auto* animEvaluator = fbxScene->GetAnimationEvaluator();

					FbxNode* rootMotionNode = nullptr;
					if (rootMotionTrackIdx.is_set() &&
						!_options.importRootMotionFromNode.is_empty())
					{
						for_every(boneInfo, boneExtractor.extracted)
						{
							if (_options.importRootMotionFromNode == boneInfo->boneName.to_string())
							{
								rootMotionNode = boneInfo->node;
							}
						}
					}

					// import per track
					for_every(boneInfo, boneExtractor.extracted)
					{
						int readIntoTrackIdx = NONE;
						if (!_options.importRootMotionFromNode.is_empty() &&
							_options.importRootMotionFromNode == boneInfo->boneName.to_string())
						{
							if (rootMotionTrackIdx.is_set())
							{
								readIntoTrackIdx = rootMotionTrackIdx.get();
							}
						}
						else
						{
							readIntoTrackIdx = animationSet->find_track_idx(boneInfo->boneName);
						}
						if (readIntoTrackIdx == NONE)
						{
							continue;
						}

						auto& track = animationSet->get_track(readIntoTrackIdx);

						bool isRootMotion = rootMotionTrackIdx.is_set() && rootMotionTrackIdx.get() == readIntoTrackIdx;

						auto* useRootNode = (rootMotionTrackIdx.is_set() && !isRootMotion) ? rootMotionNode : boneInfo->rootNode;
						an_assert(useRootNode);

						struct Utils
						{
							static FbxTime prepare_time(int frameIdx, int frameCount, float timeStart, float timeEnd)
							{
								double timePt = clamp((double)frameIdx / (double)(frameCount - 1), 0.0, 1.0);
								double atTime = timeStart + (timeEnd - timeStart) * timePt;

								FbxTime fbxAtTime;
								fbxAtTime.SetSecondDouble(atTime);
								return fbxAtTime;
							}
						};
						for (int frameIdx = 0; frameIdx < frameCount; ++frameIdx)
						{
							if (isRootMotion)
							{
								int prevFrameIdx = max(0, frameIdx - 1);
								int currFrameIdx = prevFrameIdx + 1;
								auto& prevAtTime = Utils::prepare_time(prevFrameIdx, frameCount, timeStart, timeEnd);
								auto& currAtTime = Utils::prepare_time(currFrameIdx, frameCount, timeStart, timeEnd);

								Transform prevNodeRS = BoneExtractor::get_placement_rs(
									fbx_matrix_to_matrix_44(animEvaluator->GetNodeGlobalTransform(useRootNode, prevAtTime)),
									fbx_matrix_to_matrix_44(animEvaluator->GetNodeGlobalTransform(boneInfo->node, prevAtTime)),
									_options);

								Transform currNodeRS = BoneExtractor::get_placement_rs(
									fbx_matrix_to_matrix_44(animEvaluator->GetNodeGlobalTransform(useRootNode, currAtTime)),
									fbx_matrix_to_matrix_44(animEvaluator->GetNodeGlobalTransform(boneInfo->node, currAtTime)),
									_options);

								Transform nodeRel = prevNodeRS.to_local(currNodeRS);

								if (track.get_type() == Meshes::AnimationTrack::Type::RootMotion)
								{
									animationSet->set_data(animationSetAnimIdx, readIntoTrackIdx, frameIdx, 1, &nodeRel);
								}
								else
								{
									todo_implement;
								}
							}
							else
							{
								auto& atTime = Utils::prepare_time(frameIdx, frameCount, timeStart, timeEnd);

								Transform nodeRS = BoneExtractor::get_placement_rs(
									fbx_matrix_to_matrix_44(animEvaluator->GetNodeGlobalTransform(useRootNode, atTime)),
									fbx_matrix_to_matrix_44(animEvaluator->GetNodeGlobalTransform(boneInfo->node, atTime)),
									_options);

								Transform parentRS = BoneExtractor::get_placement_rs(
									fbx_matrix_to_matrix_44(animEvaluator->GetNodeGlobalTransform(useRootNode, atTime)),
									fbx_matrix_to_matrix_44(animEvaluator->GetNodeGlobalTransform(boneInfo->parentNode? boneInfo->parentNode : useRootNode, atTime)),
									_options);

								Transform nodeLS = parentRS.to_local(nodeRS);

								if (track.get_type() == Meshes::AnimationTrack::Type::Transform)
								{
									animationSet->set_data(animationSetAnimIdx, readIntoTrackIdx, frameIdx, 1, &nodeLS);
								}
								else if (track.get_type() == Meshes::AnimationTrack::Type::Quat)
								{
									animationSet->set_data(animationSetAnimIdx, readIntoTrackIdx, frameIdx, 1, &nodeLS.get_orientation());
								}
								else
								{
									todo_implement;
								}
							}
						}
					}
				}
			}
		}
	}

	animationSet->post_import();

	animationSet->prepare_to_use();
}


#endif
