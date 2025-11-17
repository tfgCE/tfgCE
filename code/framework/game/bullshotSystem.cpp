#include "bullshotSystem.h"

#include "game.h"

#include "..\ai\aiMindInstance.h"
#include "..\debug\previewAILogic.h"
#include "..\debug\testConfig.h"
#include "..\library\library.h"
#include "..\library\usedLibraryStored.inl"
#include "..\module\moduleAI.h"
#include "..\module\moduleAppearance.h"
#include "..\module\modulePresence.h"
#include "..\module\moduleTemporaryObjects.h"
#include "..\object\actor.h"
#include "..\object\item.h"
#include "..\object\scenery.h"
#include "..\world\world.h"

#include "..\..\core\debug\debug.h"
#include "..\..\core\other\parserUtils.h"
#include "..\..\core\system\video\renderTarget.h"
#include "..\..\core\system\video\video3dPrimitives.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace Framework;

//

BullshotSystem* BullshotSystem::s_bullshotSystem = nullptr;

void BullshotSystem::initialise_static()
{
	an_assert(s_bullshotSystem == nullptr);
	s_bullshotSystem = new BullshotSystem();
}

void BullshotSystem::close_static()
{
	delete_and_clear(s_bullshotSystem);
}

Name const & BullshotSystem::get_id()
{
	return s_bullshotSystem? s_bullshotSystem->id : Name::invalid();
}

bool BullshotSystem::is_active()
{
	return s_bullshotSystem? s_bullshotSystem->active : false;
}

bool BullshotSystem::does_want_to_draw()
{
	return s_bullshotSystem ? s_bullshotSystem->wantsToDraw : false;
}

float BullshotSystem::get_sub_step_details()
{
	an_assert(s_bullshotSystem);
	return s_bullshotSystem->subStepDetails;
}

Concurrency::SpinLock& BullshotSystem::access_lock()
{
	an_assert(s_bullshotSystem);
	return s_bullshotSystem->lock;
}

SimpleVariableStorage& BullshotSystem::access_variables()
{
	an_assert(s_bullshotSystem);
	an_assert(s_bullshotSystem->lock.is_locked_on_this_thread());
	return s_bullshotSystem->variables;
}

bool BullshotSystem::load_from_xml(IO::XML::Node const* _node, LibraryLoadingContext& _lc)
{
	an_assert(s_bullshotSystem);
	if (!s_bullshotSystem)
	{
		return true;
	}

	Concurrency::ScopedSpinLock lock(s_bullshotSystem->lock);

	s_bullshotSystem->history.clear();
	
	bool result = true;

	s_bullshotSystem->active = ! _node->get_bool_attribute_or_from_child_presence(TXT("appearNonActive"));
	s_bullshotSystem->wantsToDraw = true;

	s_bullshotSystem->id = _node->get_name_attribute_or_from_child(TXT("id"), Name::invalid());
	s_bullshotSystem->variables.clear();
	s_bullshotSystem->variables.load_from_xml_child_node(_node, TXT("variables"));
	s_bullshotSystem->variables.load_from_xml_child_node(_node, TXT("parameters"));

	s_bullshotSystem->subStepDetails = _node->get_float_attribute_or_from_child(TXT("subStepDetails"), 1.0f);
	
	s_bullshotSystem->offsetPlacementRelativelyToParam = Name::invalid();
	for_every(node, _node->children_named(TXT("offsetPlacementRelatively")))
	{
		s_bullshotSystem->offsetPlacementRelativelyToParam = node->get_name_attribute(TXT("toParam"));
	}

	s_bullshotSystem->settings.clear();
	for_every(settingNode, _node->children_named(TXT("setting")))
	{
		if (!settingNode->get_text().is_empty())
		{
			Name a = Name(settingNode->get_text());
			if (a.is_valid())
			{
				s_bullshotSystem->settings.push_back(a);
			}
		}
	}

	s_bullshotSystem->objects.clear();
	for_every(osnode, _node->children_named(TXT("objects")))
	{
		for_every(node, osnode->children_named(TXT("object")))
		{
			BullshotObject o;
			if (o.load_from_xml(node, _lc))
			{
				s_bullshotSystem->objects.push_back(o);
			}
			else
			{
				result = false;
			}
		}
	}

	s_bullshotSystem->cells.clear();
	for_every(osnode, _node->children_named(TXT("cells")))
	{
		for_every(node, osnode->children_named(TXT("cell")))
		{
			BullshotCell c;
			if (c.load_from_xml(node, _lc))
			{
				s_bullshotSystem->cells.push_back(c);
			}
			else
			{
				result = false;
			}
		}
	}

	s_bullshotSystem->logos.clear();
	for_every(osnode, _node->children_named(TXT("logos")))
	{
		for_every(node, osnode->children_named(TXT("logo")))
		{
			BullshotLogo l;
			if (l.load_from_xml(node, _lc))
			{
				s_bullshotSystem->logos.push_back(l);
			}
			else
			{
				result = false;
			}
		}
	}

	for_every(node, _node->children_named(TXT("testConfig")))
	{
		// we should be pass test config load from _devConfig
		TestConfig::load_from_xml(node);
	}

	s_bullshotSystem->vr = VRBullshot();
	for_every(node, _node->children_named(TXT("vr")))
	{
		s_bullshotSystem->vr.headBoneOS.load_from_xml(node, TXT("headBoneOS"));
		s_bullshotSystem->vr.handLeftBoneOS.load_from_xml(node, TXT("handLeftBoneOS"));
		s_bullshotSystem->vr.handRightBoneOS.load_from_xml(node, TXT("handRightBoneOS"));
	}

	s_bullshotSystem->cameraInRoom.clear();
	s_bullshotSystem->cameraPlacement.clear();
	s_bullshotSystem->cameraFOV.clear();
	s_bullshotSystem->nearZ.clear();
	s_bullshotSystem->cameraConsumed = false;
	for_every(node, _node->children_named(TXT("camera")))
	{
		s_bullshotSystem->cameraInRoom.load_from_xml_attribute(node, TXT("inRoom"));
		s_bullshotSystem->cameraPlacement.load_from_xml(node, TXT("placement"));
		s_bullshotSystem->cameraFOV.load_from_xml(node, TXT("fov"));
		s_bullshotSystem->nearZ.load_from_xml(node, TXT("nearZ"));

		if (node->get_bool_attribute_or_from_child_presence(TXT("keepCamera"), false))
		{
			s_bullshotSystem->cameraConsumed = true;
		}
	}

	s_bullshotSystem->soundCameraInRoom.clear();
	s_bullshotSystem->soundCameraPlacement.clear();
	s_bullshotSystem->soundCamera = false;
	for_every(node, _node->children_named(TXT("soundCamera")))
	{
		s_bullshotSystem->soundCamera = true;
		s_bullshotSystem->soundCameraInRoom.load_from_xml_attribute(node, TXT("inRoom"));
		s_bullshotSystem->soundCameraPlacement.load_from_xml(node, TXT("placement"));
	}

	s_bullshotSystem->showFrameAspectRatio.clear();
	s_bullshotSystem->showFrameOffsetPt.clear();
	s_bullshotSystem->showFrameRenderCentreOffsetPt.clear();
	s_bullshotSystem->showFrameTemplate = Name::invalid();
	s_bullshotSystem->showFrameSafeAreaPt = Range2(Range(0.0f, 1.0f), Range(0.0f, 1.0f));
	for_every(node, _node->children_named(TXT("showFrame")))
	{
		float width = node->get_float_attribute(TXT("width"));
		float height = node->get_float_attribute(TXT("height"));
		float aspectRatio = node->get_float_attribute(TXT("aspectRatio"));
		if (auto* a = node->get_attribute(TXT("size")))
		{
			VectorInt2 size = VectorInt2::zero;
			if (size.load_from_string(a->get_as_string()))
			{
				if (size.x != 0.0f && size.y != 0.0f)
				{
					width = (float)size.x;
					height = (float)size.y;
				}
			}
		}
		if (width != 0.0f && height != 0.0f)
		{
			aspectRatio = width / height;
		}
		if (aspectRatio == 0.0f && ! s_bullshotSystem->showFrameAspectRatio.is_set())
		{
			if (System::Video3D* v3d = System::Video3D::get())
			{
				auto ssize = v3d->get_screen_size().to_vector2();
				if (!ssize.is_zero())
				{
					aspectRatio = ssize.x / ssize.y;
				}
			}
		}

		if (aspectRatio != 0.0f)
		{
			s_bullshotSystem->showFrameAspectRatio = aspectRatio;
		}

		s_bullshotSystem->showFrameOffsetPt.load_from_xml(node, TXT("offsetPt"));
		s_bullshotSystem->showFrameRenderCentreOffsetPt.load_from_xml(node, TXT("renderCentreOffsetPt"));
		s_bullshotSystem->showFrameTemplate = node->get_name_attribute(TXT("template"), s_bullshotSystem->showFrameTemplate);

		for_every(n, node->children_named(TXT("safeAreaPt")))
		{
			s_bullshotSystem->showFrameSafeAreaPt.load_from_xml(n);
		}
	}

	{
		s_bullshotSystem->titles.clear();
		for_every(osnode, _node->children_named(TXT("titles")))
		{
			for_every(node, osnode->children_named(TXT("title")))
			{
				BullshotTitle bt;
				if (bt.load_from_xml(node, _lc))
				{
					s_bullshotSystem->titles.push_back(bt);
				}
				else
				{
					result = false;
				}
			}
		}
	}

	{
		s_bullshotSystem->showGameModifiers = ShowGameModifiers();
		auto& sgm = s_bullshotSystem->showGameModifiers;
		for_every(node, _node->children_named(TXT("showGameModifiers")))
		{
			sgm.show = true;
			sgm.atPt.load_from_xml_child_node(node, TXT("atPt"));
			sgm.dir.load_from_xml_child_node(node, TXT("dir"));
			sgm.dir.normalise();
			sgm.sizePt = node->get_float_attribute_or_from_child(TXT("sizePt"), sgm.sizePt);
			sgm.spacingPt = node->get_float_attribute_or_from_child(TXT("spacingPt"), sgm.spacingPt);
			sgm.speedPt = node->get_float_attribute_or_from_child(TXT("speedPt"), sgm.speedPt);
			sgm.backgroundColour.load_from_xml_child_or_attr(node, TXT("backgroundColour"));
			for_every(tnode, node->children_named(TXT("texturePart")))
			{
				UsedLibraryStored<TexturePart> tp;
				if (tp.load_from_xml(tnode, TXT("name"), _lc))
				{
					sgm.textures.push_back(tp);
				}
				else
				{
					result = false;
				}
			}
		}
	}

	return result;
}

RangeInt2 BullshotSystem::calculate_show_frame_for(VectorInt2 const& _size, Optional<VectorInt2> const& _centre)
{
	RangeInt2 result = RangeInt2::empty;
	auto aspectRatio = BullshotSystem::get_show_frame_aspect_ratio();
	auto offsetPt = BullshotSystem::get_show_frame_offset_pt();
	VectorInt2 half = _size / 2;
	VectorInt2 centre = _centre.get(half);
	if (aspectRatio.is_set())
	{
		Vector2 size = _size.to_vector2();
		if (size.x != 0.0f && size.y != 0.0f)
		{
			float viewportAspectRatio = size.x / size.y;

			Range2 rect = Range2::empty;
			bool udShadow = false;
			bool lrShadow = false;
			if (aspectRatio.get() >= viewportAspectRatio)
			{
				rect.x = Range(0.0f, size.x - 1.0f);
				rect.y = Range(size.y * 0.5f);
				float os = size.x / aspectRatio.get();
				rect.y.min -= os * 0.5f;
				rect.y.max += os * 0.5f;
				udShadow = true;
				if (offsetPt.is_set())
				{
					rect.y.min += os * offsetPt.get().y;
					rect.y.max += os * offsetPt.get().y;
				}
				rect.y.min += centre.y - half.y;
				rect.y.max += centre.y - half.y;
			}
			else
			{
				rect.y = Range(0.0f, size.y - 1.0f);
				rect.x = Range(size.x * 0.5f);
				float os = size.y * aspectRatio.get();
				rect.x.min -= os * 0.5f;
				rect.x.max += os * 0.5f;
				lrShadow = true;
				if (offsetPt.is_set())
				{
					rect.x.min += os * offsetPt.get().x;
					rect.x.max += os * offsetPt.get().x;
				}
				rect.x.min += centre.x - half.x;
				rect.x.max += centre.x - half.x;
			}

			result.x.min = TypeConversions::Normal::f_i_cells(rect.x.min);
			result.x.max = TypeConversions::Normal::f_i_cells(rect.x.max);
			result.y.min = TypeConversions::Normal::f_i_cells(rect.y.min);
			result.y.max = TypeConversions::Normal::f_i_cells(rect.y.max);

			result.x.min = max(result.x.min, 0);
			result.x.max = min(result.x.max, _size.x - 1);
			result.y.min = max(result.y.min, 0);
			result.y.max = min(result.y.max, _size.y - 1);
		}
	}

	return result;
}

void BullshotSystem::update_objects()
{
	if (!s_bullshotSystem || !s_bullshotSystem->world)
	{
		return;
	}

	{
		Concurrency::ScopedSpinLock lock(s_bullshotSystem->lock);

		s_bullshotSystem->createdObjects.clear();
	}

	Game::get()->add_async_world_job_top_priority(TXT("update bullshot objects"),
		[]()
		{
			Concurrency::ScopedSpinLock lock(s_bullshotSystem->lock);

			{
				s_bullshotSystem->cameraInRoomPtr.clear();
				if (!s_bullshotSystem->cameraInRoom.is_empty())
				{
					Game::get()->perform_sync_world_job(TXT("find room"),
						[]()
						{
							for_every_ptr(r, s_bullshotSystem->world->get_rooms())
							{
								if (s_bullshotSystem->cameraInRoom.check(r->get_tags()))
								{
									s_bullshotSystem->cameraInRoomPtr = r;
								}
							}
						});
				}
			}

			{
				s_bullshotSystem->soundCameraInRoomPtr.clear();
				if (!s_bullshotSystem->soundCameraInRoom.is_empty())
				{
					Game::get()->perform_sync_world_job(TXT("find room"),
						[]()
						{
							for_every_ptr(r, s_bullshotSystem->world->get_rooms())
							{
								if (s_bullshotSystem->soundCameraInRoom.check(r->get_tags()))
								{
									s_bullshotSystem->soundCameraInRoomPtr = r;
								}
							}
						});
				}
			}

			s_bullshotSystem->offsetPlacement = Transform::identity;
			s_bullshotSystem->soundOffsetPlacement = Transform::identity;
			if (s_bullshotSystem->offsetPlacementRelativelyToParam.is_valid())
			{
				if (auto* r = s_bullshotSystem->cameraInRoomPtr.get())
				{
					Game::get()->perform_sync_world_job(TXT("get offset placement"),
						[r]()
						{
							s_bullshotSystem->offsetPlacement = r->get_variables().get_value<Transform>(s_bullshotSystem->offsetPlacementRelativelyToParam, Transform::identity);
						});
				}
				if (auto* r = s_bullshotSystem->soundCameraInRoomPtr.get())
				{
					Game::get()->perform_sync_world_job(TXT("get sound offset placement"),
						[r]()
						{
							s_bullshotSystem->soundOffsetPlacement = r->get_variables().get_value<Transform>(s_bullshotSystem->offsetPlacementRelativelyToParam, Transform::identity);
						});
				}
			}

			Game::get()->perform_sync_world_job(TXT("allow existing temporary objects to finish"),
				[]()
				{
#ifdef AN_ALLOW_BULLSHOTS
					for_every_ptr(to, s_bullshotSystem->subWorld->get_active_temporary_objects())
					{
						to->clear_bullshot_allow_advance_for();
					}
#else
					todo_implement;
#endif
				});

			for_every(obj, s_bullshotSystem->objects)
			{
				s_bullshotSystem->history.log(TXT("process object \"%S\""), obj->name.to_char());
				LOG_INDENT(s_bullshotSystem->history);

				IModulesOwner* existingObj = nullptr;
				if (!existingObj && !obj->existingTagged.is_empty())
				{
					Game::get()->perform_sync_world_job(TXT("find existing object"),
						[obj, &existingObj]()
						{
							for_every_ptr(o, s_bullshotSystem->world->get_objects())
							{
								if (obj->existingTagged.check(o->get_tags()))
								{
									existingObj = o;
								}
							}
						});
				}

				if (!existingObj && !obj->name.is_empty())
				{
					Game::get()->perform_sync_world_job(TXT("find existing object"),
						[obj, &existingObj]()
						{
							for_every_ptr(o, s_bullshotSystem->world->get_objects())
							{
								if (o->ai_get_name() == obj->name)
								{
									existingObj = o;
								}
							}
						});
				}

				Room* inRoom = nullptr;
				if (! obj->inRoom.is_empty())
				{
					Game::get()->perform_sync_world_job(TXT("find room"),
						[obj, &inRoom]()
						{
							for_every_ptr(r, s_bullshotSystem->world->get_rooms())
							{
								if (obj->inRoom.check(r->get_tags()))
								{
									inRoom = r;
								}
							}
						});
				}

				if (existingObj && !obj->rg.is_zero_seed() &&
					obj->existingTagged.is_empty() &&
					existingObj->get_individual_random_generator() != obj->rg)
				{
					if (auto* d = fast_cast<IDestroyable>(existingObj))
					{
						Game::get()->perform_sync_world_job(TXT("destruct"), [d]()
							{
								d->destruct_and_delete();
							});
					}
					existingObj = nullptr;
				}

				s_bullshotSystem->history.log(TXT("existingObj = %S"), existingObj? existingObj->ai_get_name().to_char() : TXT("--"));
				s_bullshotSystem->history.log(TXT("inRoom = %S"), inRoom ? inRoom->get_name().to_char() : TXT("--"));

				if (! existingObj && inRoom)
				{
					ObjectType* objectType = nullptr;
					if (auto* lib = Library::get_current())
					{
						obj->actorType.find_may_fail(lib);
						obj->itemType.find_may_fail(lib);
						obj->sceneryType.find_may_fail(lib);
						obj->mesh.find_may_fail(lib);
						obj->meshGenerator.find_may_fail(lib);
					}
					if (obj->actorType.get()) { objectType = obj->actorType.get(); }
					if (obj->itemType.get()) { objectType = obj->itemType.get(); }
					if (obj->sceneryType.get()) { objectType = obj->sceneryType.get(); }

					s_bullshotSystem->history.log(TXT("create %S"), objectType? objectType->get_name().to_string().to_char() : TXT("--"));

					if (objectType)
					{
						ScopedAutoActivationGroup saag(s_bullshotSystem->world, true);
						
						IModulesOwner* spawnedObject = nullptr;

						objectType->load_on_demand_if_required();

						Game::get()->perform_sync_world_job(TXT("spawn bullshot object"), [objectType, obj, &spawnedObject, inRoom]()
							{
								spawnedObject = objectType->create(obj->name.is_empty()? String::printf(TXT("unnamed bullshot object")) : obj->name);
								spawnedObject->init(inRoom->get_in_sub_world());
							});

						if (auto* o = fast_cast<Object>(spawnedObject))
						{
							o->access_tags().set_tags_from(obj->tags);
						}

						if (!obj->rg.is_zero_seed())
						{
							spawnedObject->access_individual_random_generator() = obj->rg;
						}
						spawnedObject->access_variables().set_from(s_bullshotSystem->variables);
						spawnedObject->access_variables().set_from(obj->parameters);
						spawnedObject->initialise_modules([obj](Framework::Module* _module)
							{
								if (auto* a = fast_cast<Framework::ModuleAppearance>(_module))
								{
									if (auto* m = obj->mesh.get())
									{
										a->use_mesh(m);
									}
									if (auto* m = obj->meshGenerator.get())
									{
										a->use_mesh_generator_on_setup(m);
									}
								}
							});
						spawnedObject->be_autonomous();
						if (obj->noAI)
						{
							if (auto* ai = spawnedObject->get_ai())
							{
								if (auto* mind = ai->get_mind())
								{
									mind->use_logic(new PreviewAILogic(mind));
									mind->access_locomotion().dont_control();
								}
							}
						}

						existingObj = spawnedObject;
					}
				}

				if (existingObj)
				{
					if (!inRoom)
					{
						inRoom = existingObj->get_presence()->get_in_room();
					}
					Game::get()->perform_sync_world_job(TXT("update bullshot object"),
						[existingObj, inRoom, obj]()
						{
							existingObj->access_variables().set_from(s_bullshotSystem->variables);
							existingObj->access_variables().set_from(obj->parameters);
							if (inRoom && obj->placement.is_set())
							{
								existingObj->get_presence()->place_in_room(inRoom, s_bullshotSystem->offsetPlacement.to_world(obj->placement.get()));
								existingObj->get_presence()->force_update_presence_links();
							}
#ifdef AN_ALLOW_BULLSHOTS
							if (auto* ap = existingObj->get_appearance())
							{
								if (!obj->poseLS.is_empty())
								{
									ap->use_bullshot_final_pose(true);
									auto& targetBonesLS = ap->acccess_bullshot_final_pose_ls();
									if (auto* s = ap->get_skeleton())
									{
										if (auto* ss = s->get_skeleton())
										{
											for_every(bd, obj->poseLS)
											{
												int idx = for_everys_index(bd);
												if (bd->boneName.is_valid())
												{
													idx = ss->find_bone_index(bd->boneName);
												}
												if (idx != NONE)
												{
													targetBonesLS.set_size(max(targetBonesLS.get_size(), idx + 1));
													if (bd->translationLS.is_set())
													{
														targetBonesLS[idx].translationLS = bd->translationLS;
													}
													if (bd->rotationLS.is_set())
													{
														targetBonesLS[idx].rotationLS = bd->rotationLS;
													}
													if (bd->scaleLS.is_set())
													{
														targetBonesLS[idx].scaleLS = bd->scaleLS;
													}
												}
											}
										}
									}
								}
								else
								{
									ap->use_bullshot_final_pose(false);
								}
							}
							if (obj->placement.is_set() && obj->lockPlacement)
							{
								existingObj->get_presence()->access_bullshot_placement() = s_bullshotSystem->offsetPlacement.to_world(obj->placement.get());
							}
							if (!obj->materialSetups.is_empty())
							{
								if (auto* ap = existingObj->get_appearance())
								{
									for_every(m, obj->materialSetups)
									{
										m->resolve_links();
									}
									ap->access_bullshot_materials() = obj->materialSetups;
								}
							}

#else
							todo_implement;
#endif
							if (!obj->triggerTemporaryObjects.is_empty())
							{
								if (auto* to = existingObj->get_temporary_objects())
								{
									Array<SafePtr<IModulesOwner>> spawnedTOs;
									for_every(tto, obj->triggerTemporaryObjects)
									{
										spawnedTOs.clear();
										to->spawn_all(tto->id, NP, &spawnedTOs);
										if (tto->advanceOnlyFor.is_set())
										{
											for_every_ref(sto, spawnedTOs)
											{
												sto->set_allow_rendering_since_core_time(tto->advanceOnlyFor.get());
											}
										}
									}
								}
							}
						});
				}

				s_bullshotSystem->createdObjects.push_back(SafePtr<IModulesOwner>(existingObj));
			}
		});
}

void BullshotSystem::log(LogInfoContext& _log)
{
	an_assert(s_bullshotSystem);

	Concurrency::ScopedSpinLock lock(s_bullshotSystem->lock);
	{
		_log.log(TXT("objects"));
		LOG_INDENT(_log);
		for_every(co, s_bullshotSystem->createdObjects)
		{
			if (auto* o = co->get())
			{
				_log.log(TXT("[%S]"), o->ai_get_name().to_char());
				LOG_INDENT(_log);
				if (auto* ob = fast_cast<Object>(o))
				{
					_log.log(TXT("tagged: %S"), ob->get_tags().to_string().to_char());
				}
				if (o->get_presence()->get_in_room())
				{
					_log.log(TXT("in %S @ %S"),
						o->get_presence()->get_in_room()->get_name().to_char(),
						o->get_presence()->get_placement().get_translation().to_string().to_char());
				}
			}
			else
			{
				_log.log(TXT("[--]"));
			}
		}
	}
	{
		_log.log(TXT("history"));
		LOG_INDENT(_log);
		_log.log(s_bullshotSystem->history);
	}
}

RegionType* BullshotSystem::get_cell_region_at(VectorInt2 const& _at)
{
	an_assert(s_bullshotSystem);

	Concurrency::ScopedSpinLock lock(s_bullshotSystem->lock);

	for_every(c, s_bullshotSystem->cells)
	{
		if (c->at == _at)
		{
			if (auto* lib = Library::get_current())
			{
				c->regionType.find_may_fail(lib);
			}

			return c->regionType.get();
		}
	}
	return nullptr;
}

void BullshotSystem::hard_copy(IModulesOwner* _imo)
{
	if (!_imo)
	{
		return;
	}

	static int bullshotHardCopyIdx = 0;
	String dirName = String(get_log_file_name()).replace(String(TXT(".log")), String(TXT("")));
	String fileName = ParserUtils::void_to_hex((_imo)) + String::space() + _imo->ai_get_name();
	fileName += String::printf(TXT("%i"), bullshotHardCopyIdx);
	++bullshotHardCopyIdx;
	String fileNameCorrected;
	for_every(ch, fileName.get_data())
	{
		if (*ch != 0)
		{
			if (*ch == ' ' ||
				(*ch >= '0' && *ch <= '9') ||
				(*ch >= 'a' && *ch <= 'z') ||
				(*ch >= 'A' && *ch <= 'Z') ||
				*ch == '-')
			{
				fileNameCorrected += *ch;
			}
			else
			{
				fileNameCorrected += ' ';
			}
		}
	}

	IO::XML::Document doc;

	if (auto* n_library = doc.get_root()->add_node(TXT("library")))
	{
		n_library->set_attribute(TXT("group"), TXT("bullshots"));
		if (auto* n_bullshot= n_library->add_node(TXT("bullshot")))
		{
			if (_imo->get_presence()->is_vr_movement())
			{
				if (auto* n_vr = n_bullshot->add_node(TXT("vr")))
				{
					_imo->get_presence()->get_requested_relative_look_for_vr().save_to_xml_child_node(n_vr, TXT("headBoneOS"));
					_imo->get_presence()->get_requested_relative_hand_for_vr(0).save_to_xml_child_node(n_vr, TXT("handLeftBoneOS"));
					_imo->get_presence()->get_requested_relative_hand_for_vr(1).save_to_xml_child_node(n_vr, TXT("handRightBoneOS"));
				}
			}
			if (auto* n_object = n_bullshot->add_node(TXT("object")))
			{
				String safeName;
				for_every(ch, _imo->ai_get_name().get_data())
				{
					if (*ch != 0)
					{
						if (*ch == ' ' ||
							(*ch >= '0' && *ch <= '9') ||
							(*ch >= 'a' && *ch <= 'z') ||
							(*ch >= 'A' && *ch <= 'Z') ||
							*ch == '-')
						{
							safeName += *ch;
						}
						else
						{
							safeName += ' ';
						}
					}
				}

				n_object->set_attribute(TXT("name"), safeName);
				if (auto* a = fast_cast<Actor>(_imo))
				{
					n_object->set_attribute(TXT("actorType"), a->get_object_type()->get_name().to_string());
				}
				if (auto* a = fast_cast<Item>(_imo))
				{
					n_object->set_attribute(TXT("itemType"), a->get_object_type()->get_name().to_string());
				}
				if (auto* a = fast_cast<Scenery>(_imo))
				{
					n_object->set_attribute(TXT("sceneryType"), a->get_object_type()->get_name().to_string());
				}

				if (auto* n_rg = n_object->add_node(TXT("randomGenerator")))
				{
					_imo->get_individual_random_generator().save_to_xml(n_rg);
				}

				/*
				if (auto* n_parameters = n_object->add_node(TXT("parameters")))
				{
					_imo->get_variables().save_to_xml(n_parameters);
				}
				*/

				if (auto* ap = _imo->get_appearance())
				{
					if (auto* n_pose = n_object->add_node(TXT("pose")))
					{
						if (auto* poseLS = ap->get_final_pose_LS())
						{
							for_every(bone, poseLS->get_bones())
							{
								if (auto* n_bone = n_pose->add_node(TXT("bone")))
								{
									if (auto* s = ap->get_skeleton())
									{
										if (auto* ss = s->get_skeleton())
										{
											int idx = for_everys_index(bone);
											if (ss->get_bones().is_index_valid(idx))
											{
												n_bone->set_attribute(TXT("name"), ss->get_bones()[idx].get_name().to_char());
											}
										}
									}
									bone->save_to_xml(n_bone);
								}
							}
						}
					}
				}
			}
		}
	}

	{
		IO::File file;
		file.create(dirName + IO::get_directory_separator() + fileNameCorrected.compress().trim() + TXT(".bullshotObject"));
		file.set_type(IO::InterfaceType::Text);
		doc.save_xml(&file);
	}
}

void BullshotSystem::get_camera(OUT_ Room*& _inRoom, OUT_ Transform& _placement, OUT_ float& _fov, bool _force)
{
	an_assert(s_bullshotSystem);

	if (s_bullshotSystem->cameraConsumed && !_force)
	{
		return;
	}

	if (s_bullshotSystem->cameraInRoomPtr.is_set())
	{
		_inRoom = s_bullshotSystem->cameraInRoomPtr.get();
	}
	if (s_bullshotSystem->cameraPlacement.is_set())
	{
		_placement = s_bullshotSystem->offsetPlacement.to_world(s_bullshotSystem->cameraPlacement.get());
	}
	if (s_bullshotSystem->cameraFOV.is_set())
	{
		_fov = s_bullshotSystem->cameraFOV.get();
	}

	s_bullshotSystem->cameraConsumed = true;
}

void BullshotSystem::set_camera(Optional<Room*> const& _inRoom, Optional<Transform> const & _placement, Optional<float> const & _fov)
{
	an_assert(s_bullshotSystem);

	s_bullshotSystem->cameraConsumed = false;

	if (_inRoom.is_set() && _inRoom.get())
	{
		s_bullshotSystem->cameraInRoomPtr = _inRoom.get();
	}
	if (_placement.is_set())
	{
		s_bullshotSystem->cameraPlacement = _placement.get();
	}
	if (_fov.is_set())
	{
		s_bullshotSystem->cameraFOV = _fov.get();
	}
}

bool BullshotSystem::get_sound_camera(OUT_ Room*& _inRoom, OUT_ Transform& _placement)
{
	an_assert(s_bullshotSystem);

	if (s_bullshotSystem->soundCamera &&
		s_bullshotSystem->soundCameraInRoomPtr.is_set() &&
		s_bullshotSystem->soundCameraPlacement.is_set())
	{
		_inRoom = s_bullshotSystem->soundCameraInRoomPtr.get();
		_placement = s_bullshotSystem->soundCameraPlacement.get();
		return _inRoom != nullptr;
	}
	else
	{
		return false;
	}
}

void BullshotSystem::get_vr_placements(OUT_ Transform& headBoneOS, OUT_ Transform& handLeftBoneOS, OUT_ Transform& handRightBoneOS)
{
	if (s_bullshotSystem)
	{
		if (s_bullshotSystem->vr.headBoneOS.is_set())
		{
			headBoneOS = s_bullshotSystem->vr.headBoneOS.get();
		}
		if (s_bullshotSystem->vr.handLeftBoneOS.is_set())
		{
			handLeftBoneOS = s_bullshotSystem->vr.handLeftBoneOS.get();
		}
		if (s_bullshotSystem->vr.handRightBoneOS.is_set())
		{
			handRightBoneOS = s_bullshotSystem->vr.handRightBoneOS.get();
		}
	}
}

void BullshotSystem::change_title(int _changeBy)
{
	an_assert(s_bullshotSystem);

	Concurrency::ScopedSpinLock lock(s_bullshotSystem->lock);

	s_bullshotSystem->titleIdx = clamp(s_bullshotSystem->titleIdx + _changeBy, NONE, s_bullshotSystem->titles.get_size());
	s_bullshotSystem->titleTime = 0.0f;
}

bool BullshotSystem::is_setting_active(Name const& _what)
{
	return !s_bullshotSystem || s_bullshotSystem->settings.does_contain(_what);
}

void BullshotSystem::set_title_time(float _time)
{
	an_assert(s_bullshotSystem);

	Concurrency::ScopedSpinLock lock(s_bullshotSystem->lock);

	s_bullshotSystem->titleTime = _time;
}

void BullshotSystem::advance(float _deltaTime, bool _skippable)
{
	an_assert(s_bullshotSystem);

	if (_skippable)
	{
		s_bullshotSystem->lock.acquire_if_not_locked();
	}
	else
	{
		s_bullshotSystem->lock.acquire();
	}

	s_bullshotSystem->titleTime += _deltaTime;

	if (s_bullshotSystem->showGameModifiers.show)
	{
		auto& sgm = s_bullshotSystem->showGameModifiers;
		bool allFound = true;
		for_every(tp, sgm.textures)
		{
			if (!tp->get())
			{
				allFound = false;
				tp->find_may_fail(Library::get_current());
			}
		}
		if (allFound)
		{
			if (sgm.available.is_empty() && sgm.elements.is_empty())
			{
				sgm.available = sgm.textures;
			}
			while (!sgm.available.is_empty())
			{
				float alongDir = -2.0f;
				if (!sgm.elements.is_empty())
				{
					alongDir = sgm.elements.get_last().alongDir - sgm.spacingPt;
				}
				ShowGameModifiers::Element e;
				e.alongDir = alongDir;
				{
					int idx = Random::get_int(sgm.available.get_size());
					e.show = sgm.available[idx];
					sgm.available.remove_fast_at(idx);
				}
				sgm.elements.push_back(e);
			}
			for_every(e, sgm.elements)
			{
				e->alongDir += sgm.speedPt * _deltaTime;
			}
			if (!sgm.elements.is_empty())
			{
				auto & f = sgm.elements.get_first();
				if (f.alongDir > 2.0f) // went out
				{
					sgm.available.push_back(sgm.elements.get_first().show);
					sgm.elements.pop_front();
				}
			}
		}
	}

	s_bullshotSystem->lock.release();
}

void BullshotSystem::render_logos_and_titles(::System::RenderTarget* _rt, Optional<VectorInt2> const& _centre)
{
	an_assert(s_bullshotSystem);

	System::Video3D* v3d = System::Video3D::get();

	VectorInt2 rtSize = v3d->get_screen_size();

	if (_rt)
	{
		rtSize = _rt->get_size();
	}

	v3d->set_viewport(VectorInt2::zero, rtSize);
	v3d->setup_for_2d_display();
	v3d->set_2d_projection_matrix_ortho(rtSize.to_vector2());
	v3d->access_model_view_matrix_stack().clear();

	Vector2 centre = _centre.get(rtSize / 2).to_vector2();
	RangeInt2 frameI = calculate_show_frame_for(rtSize, _centre);

	Range2 frame;
	frame.x.min = (float)frameI.x.min;
	frame.x.max = (float)frameI.x.max;
	frame.y.min = (float)frameI.y.min;
	frame.y.max = (float)frameI.y.max;

	int frameX = frameI.x.length();
	int frameY = frameI.y.length();

	Concurrency::ScopedSpinLock lock(s_bullshotSystem->lock);

	if (s_bullshotSystem->showGameModifiers.show)
	{
		auto& sgm = s_bullshotSystem->showGameModifiers;
	
		if (! sgm.backgroundColour.is_none())
		{
			::System::Video3DPrimitives::fill_rect_2d(sgm.backgroundColour, Vector2::zero, rtSize.to_vector2(), true);
		}

		for_every(e, sgm.elements)
		{
			if (auto* tp = e->show.get())
			{
				Vector2 at = frame.get_at(sgm.atPt + sgm.dir * e->alongDir);
				Vector2 tpSize = tp->get_size();
				float minSize = (float)min(frameX, frameY);
				float reqSize = minSize * sgm.sizePt;
				float tpFitSize = frameX <= frameY ? tpSize.x : tpSize.y;
				TexturePartDrawingContext tpdc;
				//tpdc.colour = ...;
				tpdc.scale = Vector2::one * (reqSize / tpFitSize);
				tp->draw_at(System::Video3D::get(), at, tpdc);
			}
		}
	}

	for_every(logo, s_bullshotSystem->logos)
	{
		if (!logo->texturePart.get())
		{
			if (auto* lib = Library::get_current())
			{
				logo->texturePart.find_may_fail(lib);
			}
		}

		if (auto* tp = logo->texturePart.get())
		{
			Vector2 at = frame.get_at(logo->atPt);
			Vector2 tpSize = tp->get_size();
			float minSize = (float)min(frameX, frameY);
			float reqSize = minSize * logo->sizePt;
			float tpFitSize = frameX <= frameY? tpSize.x: tpSize.y;
			TexturePartDrawingContext tpdc;
			tpdc.colour = logo->colour;
			tpdc.scale = Vector2::one * (reqSize / tpFitSize);
			tp->draw_at(System::Video3D::get(), at, tpdc);
		}
	}

	if (s_bullshotSystem->titleIdx >= 0 && s_bullshotSystem->titleIdx < s_bullshotSystem->titles.get_size())
	{
		System::Video3D* v3d = System::Video3D::get();

		auto& t = s_bullshotSystem->titles[s_bullshotSystem->titleIdx];

		float scale = min((float)frameX / t.doneForSize.x, (float)frameY / t.doneForSize.y);
		Vector2 scalev = Vector2::one * scale;

		float time = s_bullshotSystem->titleTime;

		if (t.paper.a > 0.0f)
		{
			::System::Video3DPrimitives::fill_rect_2d(t.paper, Vector2::zero, rtSize.to_vector2(), true);
		}

		if (t.font.get())
		{
			Vector2 doneForCentre = t.doneForSize * 0.5f;

			System::FontDrawingContext fdc;
			fdc.blend = true;

			for_every(e, t.elements)
			{
				float atT = 0.0f; // stay (-1->0 come, 0->1 away)
				if (time < e->comeTimeOffset)
				{
					float t__ = clamp((time - (e->comeTimeOffset - e->comeTime)) / e->comeTime, 0.0f, 1.0f);
					float t_ = BezierCurveTBasedPoint<float>::find_t_at(t__, t.comeCurve, nullptr, 0.0000001f);
					atT = -t.comeCurve.calculate_at(t_).p;
				}
				if (time > e->comeTime + t.stayTime)
				{
					float timeRel = time - (e->comeTime + t.stayTime);
					float t__ = clamp(timeRel / t.awayTime, 0.0f, 1.0f);
					float t_ = BezierCurveTBasedPoint<float>::find_t_at(t__, t.awayCurve, nullptr, 0.0000001f);
					atT = t.awayCurve.calculate_at(t_).p;
				}

				if (abs(atT) < 1.0f)
				{
					Colour colour = Colour::lerp(1.0f - sqr(1.0f - abs(atT)), t.ink, t.paper);

					Vector2 at = (e->at - doneForCentre) * scalev + centre;
					if (atT < 0.0f)
					{
						at += (-atT) * (e->comeLoc - e->at) * scalev;
					}
					if (atT > 0.0f)
					{
						at += (atT) * (e->awayLoc - e->at) * scalev;
					}
					t.font->draw_text_at(v3d, e->text.to_char(), colour, at, t.fontScale * scalev * t.finalFontScale, Vector2::zero, NP, fdc);
				}
			}
		}
	}

}


//

bool BullshotObject::load_from_xml(IO::XML::Node const* _node, LibraryLoadingContext& _lc)
{
	bool result = true;

	name = _node->get_string_attribute(TXT("name"), name);

	actorType.load_from_xml(_node, TXT("actorType"), _lc);
	itemType.load_from_xml(_node, TXT("itemType"), _lc);
	sceneryType.load_from_xml(_node, TXT("sceneryType"), _lc);
	mesh.load_from_xml(_node, TXT("mesh"), _lc);
	meshGenerator.load_from_xml(_node, TXT("meshGenerator"), _lc);

	noAI = _node->get_bool_attribute_or_from_child_presence(TXT("noAI"), false);

	existingTagged.load_from_xml_attribute(_node, TXT("existingTagged"));
	
	tags.load_from_xml_attribute_or_child_node(_node, TXT("tags"));
	
	inRoom.load_from_xml_attribute(_node, TXT("inRoom"));
	placement.load_from_xml(_node, TXT("placement"));
	lockPlacement = _node->get_bool_attribute_or_from_child_presence(TXT("lockPlacement"), false);

	parameters.clear();
	parameters.load_from_xml_child_node(_node, TXT("variables"));
	parameters.load_from_xml_child_node(_node, TXT("parameters"));

	rg.be_zero_seed();
	for_every(node, _node->children_named(TXT("randomGenerator")))
	{
		rg.load_from_xml(node);
	}
	
	poseLS.clear();
	for_every(node, _node->children_named(TXT("pose")))
	{
		for_every(boneNode, node->all_children())
		{
			if (boneNode->get_name() == TXT("bone"))
			{
				BoneData bd;
				bd.boneName = boneNode->get_name_attribute(TXT("name"));
				if (auto* n = boneNode->first_child_named(TXT("translation")))
				{
					bd.translationLS.load_from_xml(n);
				}
				if (auto* n = boneNode->first_child_named(TXT("rotation")))
				{
					bd.rotationLS.load_from_xml(n);
				}
				if (auto* n = boneNode->first_child_named(TXT("scale")))
				{
					bd.scaleLS.load_from_xml(n);
				}
				poseLS.push_back(bd);
			}
			else if (boneNode->get_name() == TXT("noBone"))
			{
				poseLS.push_back(BoneData());
			}
		}
	}

	materialSetups.clear();
	{
		for_every(node, _node->children_named(TXT("materials")))
		{
			for_every(materialNode, node->children_named(TXT("material")))
			{
				MaterialSetup ms;
				ms.load_from_xml(materialNode, _lc);
				materialSetups.push_back(ms);
			}
		}
	}

	triggerTemporaryObjects.clear();
	{
		for_every(node, _node->children_named(TXT("trigger")))
		{
			TriggerTemporaryObject tto;
			tto.id = node->get_name_attribute(TXT("temporaryObject"));
			tto.advanceOnlyFor.load_from_xml(node, TXT("advanceOnlyFor"));
			if (tto.id.is_valid())
			{
				triggerTemporaryObjects.push_back(tto);
			}
		}
	}

	return result;
}

//

bool BullshotCell::load_from_xml(IO::XML::Node const* _node, LibraryLoadingContext& _lc)
{
	bool result = true;

	if (auto* a = _node->get_attribute(TXT("at")))
	{
		if (at.load_from_string(a->get_as_string()))
		{
			// ok
		}
		else
		{
			error_loading_xml(_node, TXT("no valid \"at\" attribute"));
			result = false;
		}
	}
	else
	{
		error_loading_xml(_node, TXT("no valid \"at\" attribute"));
		result = false;
	}

	regionType.load_from_xml(_node, TXT("regionType"), _lc);

	return result;
};

//

bool BullshotLogo::load_from_xml(IO::XML::Node const* _node, LibraryLoadingContext& _lc)
{
	bool result = true;

	atPt.load_from_xml_child_node(_node, TXT("atPt"));
	sizePt = _node->get_float_attribute_or_from_child(TXT("sizePt"), sizePt);

	texturePart.load_from_xml(_node, TXT("texturePart"), _lc);

	colour.load_from_xml_child_or_attr(_node, TXT("colour"));

	return result;
};

//

bool BullshotTitle::load_from_xml(IO::XML::Node const* _node, LibraryLoadingContext& _lc)
{
	bool result = true;

	auto* parentNode = _node->get_parent();

	text = _node->get_string_attribute_or_from_child(TXT("text"));

	{
		BezierCurve<BezierCurveTBasedPoint<float>> c;
		c.p0.t = 0.0f;
		c.p0.p = 0.0f;
		c.p3.t = 1.0f;
		c.p3.p = 1.0f;
		c.make_linear();
		c.make_roundly_separated();
		comeCurve = c;
		awayCurve = c;
	}

	for_count(int, loadFrom, 2)
	{
		auto* node = loadFrom == 0 ? parentNode : _node;
		fontHeightPt = node->get_float_attribute_or_from_child(TXT("fontHeightPt"), fontHeightPt);
		finalFontScale = node->get_float_attribute_or_from_child(TXT("finalFontScale"), finalFontScale);
		lineSpacingPt = node->get_float_attribute_or_from_child(TXT("lineSpacingPt"), lineSpacingPt);
		atPt.load_from_xml_child_node(node, TXT("atPt"));
		textPt.load_from_xml_child_node(node, TXT("textPt"));
		result &= font.load_from_xml(node, TXT("font"), _lc);
		stayTime = node->get_float_attribute_or_from_child(TXT("stayTime"), stayTime);
		comeTime = node->get_float_attribute_or_from_child(TXT("comeTime"), comeTime);
		perWordComeTimeOffset.load_from_xml(node, TXT("perWordComeTimeOffset"));
		awayTime = node->get_float_attribute_or_from_child(TXT("awayTime"), awayTime);
		comeMask.load_from_xml_child_node(node, TXT("comeMask"));
		awayMask.load_from_xml_child_node(node, TXT("awayMask"));
		comeDistPt.load_from_xml_child_node(node, TXT("comeDistPt"));
		awayDistPt.load_from_xml_child_node(node, TXT("awayDistPt"));
		comeUnifiedDir.load_from_xml(node, TXT("comeUnifiedDir"));
		awayUnifiedDir.load_from_xml(node, TXT("awayUnifiedDir"));
		if (auto const* n = node->first_child_named(TXT("comeCurve")))
		{
			comeCurve.load_from_xml(n);
		}
		if (auto const* n = node->first_child_named(TXT("awayCurve")))
		{
			awayCurve.load_from_xml(n);
		}
		rg.load_from_xml(node->first_child_named(TXT("randomGenrator")));

		ink.load_from_xml_child_or_attr(node, TXT("ink"));
		paper.load_from_xml_child_or_attr(node, TXT("paper"));
	}

	break_into_elements();

	return result;
};

void BullshotTitle::break_into_elements()
{
	elements.clear();

	System::Video3D* v3d = System::Video3D::get();

	auto ssize = v3d->get_screen_size().to_vector2();

	if (!font.get())
	{
		font.find_may_fail(Library::get_current());
	}

	if (!font.get())
	{
		return;
	}

	float actualFontHeight = font->get_height();
	float scale = (fontHeightPt * ssize.y) / actualFontHeight;
	Vector2 scalev = Vector2::one * scale;
	float spaceWidth = font->calculate_char_size().x * scale;

	List<String> lines;
	text.split(String(TXT("~")), lines);

	float atY = ssize.y * (atPt.y + (float)(lines.get_size() - 1) * lineSpacingPt * (1.0f - textPt.y)) - actualFontHeight * textPt.y;

	Range2 r = Range2::empty;
	for_every(line, lines)
	{
		float lineLength = 0.0f;
		List<String> words;
		Array<float> wordWidths;
		line->split(String::space(), words);
		int firstEIdx = elements.get_size();
		for_every(word, words)
		{
			Element e;
			e.text = *word;
			elements.push_back(e);
			if (lineLength > 0.0f) lineLength += spaceWidth;
			float wordWidth = font->get_font()->calculate_text_size(*word, scalev).x;
			wordWidths.push_back(wordWidth);
			lineLength += wordWidth;
		}

		float atX = ssize.x * atPt.x - lineLength * textPt.x;
		for (int i = firstEIdx; i < elements.get_size(); ++i)
		{
			auto& e = elements[i];
			e.at.x = atX;
			e.at.y = atY;
			e.size.x = wordWidths[i - firstEIdx];
			e.size.y = actualFontHeight;
			atX += e.size.x;
			atX += spaceWidth;

			r.x.include(e.at.x);
			r.x.include(e.at.x + e.size.x);
			r.y.include(e.at.y);
			r.y.include(e.at.y + e.size.y);
		}

		atY -= ssize.y * lineSpacingPt;
	}

	Vector2 centre = r.get_at(Vector2::half + (textPt - Vector2::half) * 0.5f);

	struct CalculateLoc
	{
		Vector2 ssize;
		Random::Generator urg;
		Vector2 centre;
		CalculateLoc(Vector2 const& _ssize, Random::Generator const & _urg, Vector2 const& _centre)
			: ssize(_ssize)
			, urg(_urg)
			, centre(_centre)
		{}

		Vector2 calculate(Vector2 const& _at, Vector2 const& _size, Optional<Vector2> const& _unifiedDir, Vector2 const& _dirMask, Range const& _distPt)
		{
			Vector2 dir = (_unifiedDir.get((_at + _size * 0.5f - centre)) * _dirMask).normal();
			while (dir.is_zero())
			{
				dir = (Vector2(urg.get_float(-1.0f, 1.0f), urg.get_float(-1.0f, 1.0f)) * _dirMask).normal();
			}
			Vector2 off = dir * urg.get(_distPt) * ssize;
			return _at + off;
		}
	} calculateLoc(ssize, rg, centre);

	for_every(e, elements)
	{
		e->comeLoc = calculateLoc.calculate(e->at, e->size, comeUnifiedDir, comeMask, comeDistPt);
		e->awayLoc = calculateLoc.calculate(e->at, e->size, awayUnifiedDir, awayMask, awayDistPt);
	}

	Random::Generator urg = rg;

	for_every(e, elements)
	{
		e->comeTime = comeTime;
		e->comeTimeOffset = comeTime;
		if (perWordComeTimeOffset.is_set())
		{
			e->comeTimeOffset += urg.get(perWordComeTimeOffset.get());
		}
	}

	// store cached
	fontScale = scalev;
	doneForSize = ssize;
}
