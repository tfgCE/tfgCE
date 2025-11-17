#include "meshGenElement.h"

#include "meshGenElementAppearanceController.h"
#include "meshGenElementBone.h"
#include "meshGenElementDoWheresMyPoint.h"
#include "meshGenElementDropMeshNodes.h"
#include "meshGenElementForMeshNodes.h"
#include "meshGenElementGetFromMeshNode.h"
#include "meshGenElementInclude.h"
#include "meshGenElementIncludeMesh.h"
#include "meshGenElementIncludeStack.h"
#include "meshGenElementInfo.h"
#include "meshGenElementLoop.h"
#include "meshGenElementMeshNode.h"
#include "meshGenElementMeshProcessor.h"
#include "meshGenElementModifier.h"
#include "meshGenElementNamedCheckpoint.h"
#include "meshGenElementPointOfInterest.h"
#include "meshGenElementRandomSet.h"
#include "meshGenElementRayCast.h"
#include "meshGenElementSet.h"
#include "meshGenElementShape.h"
#include "meshGenElementSocket.h"
#include "meshGenGenerationContext.h"
#include "meshGenUtils.h"

#include "meshGenParamImpl.inl"

#include "..\framework.h"
#include "..\library\libraryLoadingContext.h"
#include "..\world\pointOfInterestTypes.h"

#include "..\..\core\collision\model.h"
#include "..\..\core\concurrency\scopedSpinLock.h"
#include "..\..\core\io\xml.h"
#include "..\..\core\mesh\mesh3dPart.h"
#include "..\..\core\mesh\socketDefinitionSet.h"
#include "..\..\core\system\timeStamp.h"

#ifdef AN_CLANG
#include "..\library\usedLibraryStored.inl"
#endif

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace Framework;
using namespace MeshGeneration;

//

DEFINE_STATIC_NAME(materialIdx);
DEFINE_STATIC_NAME(materialIndex);

//

REGISTER_FOR_FAST_CAST(ElementInstance);

ElementInstance::ElementInstance(GenerationContext & _context, Element const * _element, ElementInstance* _parent)
: context(_context)
, element(_element)
, parent(_parent)
{
	todo_note(TXT("instantiate random?!"));
}

bool ElementInstance::process()
{
	if (!element->generationCondition.check(*this))
	{
		// skip this one!
		return true;
	}

#ifdef AN_DEVELOPMENT
	int performanceForElementIdx = context.add_performance_for_element(element);
	::System::TimeStamp totalTime;
#endif

	Concurrency::ScopedSpinLock elementLock(element->processLock, element->is_stateless()); // we can only enter the same element if it is stateless

	bool result = true;

	context.push_element(element);

	if (element->useStack)
	{
		context.push_stack();
	}
	if (element->useCheckpoint)
	{
		context.push_checkpoint(Checkpoint(context));
	}

#ifdef AN_DEVELOPMENT_OR_PROFILER
#ifndef AN_CLANG
	Random::Generator const storedRandomGenerator = context.get_random_generator();
#endif
#endif
	Random::Generator randomisedRandomGenerator = Random::Generator(Random::next_uint(), Random::next_uint());

	if (element->randomiseSeed)
	{
		context.use_random_generator(randomisedRandomGenerator);
	}

	context.advance_random_generator(23948, 239057);

	if (element->useStack)
	{
		invalidate_standard_parameters(context, *this);
	}

	context.set_parameters(element->parameters);

	for (int i = 0; i < 2; ++i)
	{
		SimpleVariableStorage const & params = i == 0 ? element->requiredParameters : element->optionalParameters;
		for_every(parameter, params.get_all())
		{
			if (!context.has_parameter(parameter->get_name(), parameter->type_id()))
			{
				if (i == 0)
				{
					error_generating_mesh(*this, TXT("element requires parameter \"%S\" of type %S"), parameter->get_name().to_char(), RegisteredType::get_name_of(parameter->type_id()));
				}
				if (void const * value = params.get_raw(parameter->get_name(), parameter->type_id()))
				{
					context.set_parameter(parameter->get_name(), parameter->type_id(), value);
				}
			}
		}
	}

#ifdef AN_DEVELOPMENT
	updatedWMPDirectly = false;
#endif

#ifdef AN_DEVELOPMENT
	DebugRendererFrameCheckpoint drfCheckpoint;
	if (auto * drf = context.debugRendererFrame)
	{
		drfCheckpoint = drf->get_checkpoint();
	}
#endif

	// ----------------------------------------------------------------------------------------------------------------------------------------------------

	if (element->forceUseRandomSeed.is_set())
	{
		context.push_random_generator_stack();
		int seed = element->forceUseRandomSeed.get(&context, 0);
		context.access_random_generator().set_seed(seed, 0);
	}
	if (element->updateWMPPhase == Element::UpdateWMPPreProcess)
	{
		result &= element->update_wmp(*this);
	}
#ifdef AN_DEVELOPMENT
	::System::TimeStamp timeProcess;
#endif
	result &= element->process(context, *this);
#ifdef AN_DEVELOPMENT
	context.performanceForElements[performanceForElementIdx].timeProcess = timeProcess.get_time_since();
#endif
	if (element->updateWMPPhase == Element::UpdateWMPPostProcess)
	{
		result &= element->update_wmp(*this);
	}
	result &= element->update_wmp(*this, &element->wheresMyPointProcessorPost);
	result &= element->post_process(context, *this);
	if (element->forceUseRandomSeed.is_set())
	{
		context.pop_random_generator_stack();
	}

	// ----------------------------------------------------------------------------------------------------------------------------------------------------

#ifdef AN_DEVELOPMENT
	if (element->updateWMPPhase == Element::UpdateWMPDirectly)
	{
		an_assert(updatedWMPDirectly, TXT("this element should udpate wmp processor by direct call"));
	}
#endif

#ifdef AN_DEVELOPMENT
	if (auto * drf = context.debugRendererFrame)
	{
		Transform usePlacement = Transform::identity;
		Vector3 useScale = Vector3::one;
		if (get_auto_placement_and_auto_scale(context, *this, OUT_ usePlacement, useScale))
		{
			usePlacement.set_scale(max(useScale.x, max(useScale.y, useScale.z)));
			drf->apply_transform(drfCheckpoint, usePlacement);
		}
	}
#endif

	Checkpoint checkpoint = context.get_checkpoint();

	if (context.get_parts().is_index_valid(checkpoint.partsSoFarCount))
	{
		RefCountObjectPtr<::Meshes::Mesh3DPart> const * part = &context.get_parts()[checkpoint.partsSoFarCount];
		for (int idx = checkpoint.partsSoFarCount; idx < context.get_parts().get_size(); ++idx, ++part)
		{
			apply_standard_parameters((*part)->access_vertex_data(), (*part)->get_number_of_vertices(), context, *this);
		}
	}

	// drop (remember to delete them!)
	if (element->dropMovementCollision)
	{
		for_range(int, i, checkpoint.movementCollisionPartsSoFarCount, context.access_movement_collision_parts().get_size() - 1)
		{
			delete context.access_movement_collision_parts()[i];
		}
		context.access_movement_collision_parts().set_size(checkpoint.movementCollisionPartsSoFarCount);
	}
	if (element->dropPreciseCollision)
	{
		for_range(int, i, checkpoint.preciseCollisionPartsSoFarCount, context.access_precise_collision_parts().get_size() - 1)
		{
			delete context.access_precise_collision_parts()[i];
		}
		context.access_precise_collision_parts().set_size(checkpoint.preciseCollisionPartsSoFarCount);
	}

	if (context.get_movement_collision_parts().is_index_valid(checkpoint.movementCollisionPartsSoFarCount))
	{
		::Collision::Model* const * part = &context.get_movement_collision_parts()[checkpoint.movementCollisionPartsSoFarCount];
		for (int idx = checkpoint.movementCollisionPartsSoFarCount; idx < context.get_movement_collision_parts().get_size(); ++idx, ++part)
		{
			apply_standard_parameters((*part), context, *this);
		}
	}

	if (context.get_precise_collision_parts().is_index_valid(checkpoint.preciseCollisionPartsSoFarCount))
	{
		::Collision::Model* const * part = &context.get_precise_collision_parts()[checkpoint.preciseCollisionPartsSoFarCount];
		for (int idx = checkpoint.preciseCollisionPartsSoFarCount; idx < context.get_precise_collision_parts().get_size(); ++idx, ++part)
		{
			apply_standard_parameters((*part), context, *this);
		}
	}

	if (checkpoint.socketsGenerationIdSoFar != context.get_current_sockets_generation_id())
	{
		for_every(socket, context.access_sockets().access_sockets())
		{
			if (socket->get_generation_id() > checkpoint.socketsGenerationIdSoFar)
			{
				apply_standard_parameters(socket, context, *this);
			}
		}
	}

	if (context.get_mesh_nodes().is_index_valid(checkpoint.meshNodesSoFarCount))
	{
		MeshNodePtr* meshNode = &context.access_mesh_nodes()[checkpoint.meshNodesSoFarCount];
		for (int idx = checkpoint.meshNodesSoFarCount; idx < context.get_mesh_nodes().get_size(); ++idx, ++meshNode)
		{
			apply_standard_parameters(meshNode->get(), context, *this);
		}
	}

	if (context.get_pois().is_index_valid(checkpoint.poisSoFarCount))
	{
		PointOfInterestPtr* poi = &context.access_pois()[checkpoint.poisSoFarCount];
		for (int idx = checkpoint.poisSoFarCount; idx < context.get_pois().get_size(); ++idx, ++poi)
		{
			apply_standard_parameters(poi->get(), context, *this);
		}
	}

	if (context.get_space_blockers().blockers.is_index_valid(checkpoint.spaceBlockersSoFarCount))
	{
		SpaceBlocker* sb = &context.access_space_blockers().blockers[checkpoint.spaceBlockersSoFarCount];
		for (int idx = checkpoint.spaceBlockersSoFarCount; idx < context.get_space_blockers().get_size(); ++idx, ++sb)
		{
			apply_standard_parameters(sb, context, *this);
		}
	}

	if (context.access_generated_bones().is_index_valid(checkpoint.bonesSoFarCount))
	{
		Bone* bone = &context.access_generated_bones()[checkpoint.bonesSoFarCount];
		for (int idx = checkpoint.bonesSoFarCount; idx < context.get_generated_bones().get_size(); ++idx, ++bone)
		{
			apply_standard_parameters(*bone, context, *this);
		}
	}

	if (context.access_appearance_controllers().is_index_valid(checkpoint.appearanceControllersSoFarCount))
	{
		AppearanceControllerDataPtr* ac = &context.access_appearance_controllers()[checkpoint.appearanceControllersSoFarCount];
		for (int idx = checkpoint.appearanceControllersSoFarCount; idx < context.get_appearance_controllers().get_size(); ++idx, ++ac)
		{
			apply_standard_parameters(ac->get(), context, *this);
		}
	}

	// get output parameters
	{
		int allocatedOnStackSize = 2048;
		int allocatedOnStackAlignment = 256;
		Array<byte> buffer;
		buffer.make_space_for(allocatedOnStackSize + allocatedOnStackAlignment);
		// makes sense only if we use stack, if we do not use, there will be available anyway
		// for non stack - only if storeAs is different, otherwise they are already available
		for_every(op, element->outputParameters)
		{
			if (element->useStack || op->storeAs != op->name)
			{
				int size;
				int alignment;
				RegisteredType::get_size_and_alignment_of(op->type, size, alignment);
				an_assert(size > 0);
				buffer.make_space_for(size + alignment);
				void* buff = align_memory(buffer.get_data(), alignment);
				context.get_parameter(op->name, op->type, buff);
				context.set_parameter(op->storeAs, op->type, buff, 1);
			}
		}
	}

	if (element->useCheckpoint)
	{
		context.pop_checkpoint();
	}
	if (element->useStack)
	{
		context.pop_stack();
	}

	context.pop_element();

#ifdef AN_DEVELOPMENT
	context.performanceForElements[performanceForElementIdx].timeTotal = totalTime.get_time_since();
	context.end_performance_for_element();
#endif

	return result;
}

bool ElementInstance::update_wmp_directly()
{
	an_assert(element->updateWMPPhase == Element::UpdateWMPDirectly)
	bool result = element->update_wmp(*this);
#ifdef AN_DEVELOPMENT
	updatedWMPDirectly = true;
#endif
	return result;
}

bool ElementInstance::store_value_for_wheres_my_point(Name const & _byName, WheresMyPoint::Value const & _value)
{
	return context.store_value_for_wheres_my_point(_byName, _value);
}

bool ElementInstance::restore_value_for_wheres_my_point(Name const & _byName, OUT_ WheresMyPoint::Value & _value) const
{
	return context.restore_value_for_wheres_my_point(_byName, OUT_ _value);
}

bool ElementInstance::store_convert_value_for_wheres_my_point(Name const& _byName, TypeId _to)
{
	return context.store_convert_value_for_wheres_my_point(_byName, _to);
}

bool ElementInstance::rename_value_forwheres_my_point(Name const& _from, Name const& _to)
{
	return context.rename_value_forwheres_my_point(_from, _to);
}

bool ElementInstance::store_global_value_for_wheres_my_point(Name const & _byName, WheresMyPoint::Value const & _value)
{
	return context.store_global_value_for_wheres_my_point(_byName, _value);
}

bool ElementInstance::restore_global_value_for_wheres_my_point(Name const & _byName, OUT_ WheresMyPoint::Value & _value) const
{
	return context.restore_global_value_for_wheres_my_point(_byName, OUT_ _value);
}

bool ElementInstance::get_point_for_wheres_my_point(Name const & _byName, OUT_ Vector3 & _point) const
{
	return context.get_point_for_wheres_my_point(_byName, OUT_ _point);
}

WheresMyPoint::IOwner* ElementInstance::get_wmp_owner()
{
	if (parent)
	{
		return parent;
	}
	else
	{
		return &context;
	}
}

int ElementInstance::get_material_index_from_params() const
{
	int usingMaterialIndex = 0;
	{
		FOR_PARAM(get_context(), int, materialIndex)
		{
			usingMaterialIndex = *materialIndex;
		}
		FOR_PARAM(get_context(), int, materialIdx)
		{
			usingMaterialIndex = *materialIdx;
		}
	}
	return usingMaterialIndex;
}

//

REGISTER_FOR_FAST_CAST(Element);

Element* Element::create_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc)
{
	// those types are hardcoded but I do not expect them growing at insane rate so it should be ok, I hope
	if (_node->get_name() == TXT("shape"))
	{
		return new ElementShape();
	}
	if (_node->get_name() == TXT("modifier"))
	{
		return new ElementModifier();
	}
	if (_node->get_name() == TXT("namedCheckpoint"))
	{
		return new ElementNamedCheckpoint();
	}
	if (_node->get_name() == TXT("include"))
	{
		return new ElementInclude();
	}
	if (_node->get_name() == TXT("includeMesh"))
	{
		return new ElementIncludeMesh();
	}
	if (_node->get_name() == TXT("includeStack"))
	{
		return new ElementIncludeStack();
	}
	if (_node->get_name() == TXT("info"))
	{
		return new ElementInfo();
	}
	if (_node->get_name() == TXT("set"))
	{
		return new ElementSet();
	}
	if (_node->get_name() == TXT("socket"))
	{
		return new ElementSocket();
	}
	if (_node->get_name() == TXT("meshNode"))
	{
		return new ElementMeshNode();
	}
	if (_node->get_name() == TXT("meshProcessor"))
	{
		return new ElementMeshProcessor();
	}
	if (_node->get_name() == TXT("forMeshNodes"))
	{
		return new ElementForMeshNodes();
	}
	if (_node->get_name() == TXT("getFromMeshNodes"))
	{
		warn_loading_xml(_node, TXT("\"getFromMeshNode\", without \"s\"!"));
		return new ElementGetFromMeshNode();
	}
	if (_node->get_name() == TXT("getFromMeshNode"))
	{
		return new ElementGetFromMeshNode();
	}
	if (_node->get_name() == TXT("dropMeshNodes"))
	{
		return new ElementDropMeshNodes();
	}
	if (_node->get_name() == TXT("pointOfInterest") ||
		_node->get_name() == TXT("poi"))
	{
		return new ElementPointOfInterest();
	}
	if (_node->get_name() == TXT("bone"))
	{
		return new ElementBone();
	}
	if (_node->get_name() == TXT("appearanceController"))
	{
		return new ElementAppearanceController();
	}
	if (_node->get_name() == TXT("doWheresMyPoint"))
	{
		return new ElementDoWheresMyPoint();
	}
	if (_node->get_name() == TXT("rayCast"))
	{
		return new ElementRayCast();
	}
	if (_node->get_name() == TXT("loop"))
	{
		return new ElementLoop();
	}
	if (_node->get_name() == TXT("randomSet"))
	{
		return new ElementRandomSet();
	}
	error_loading_xml(_node, TXT("element not recognised \"%S\""), _node->get_name().to_char());
	return nullptr;
}

String const & Element::get_location_info() const
{
#ifdef AN_DEVELOPMENT
	return locationInfo;
#endif
	return String::empty();
}

bool Element::is_xml_node_handled(IO::XML::Node const * _node) const
{
	if (_node->get_name() == TXT("parameters")) return true;
	if (_node->get_name() == TXT("requiredParameters")) return true;
	if (_node->get_name() == TXT("optionalParameters")) return true;
	if (_node->get_name() == TXT("outputParameters")) return true;
	if (_node->get_name() == TXT("wheresMyPoint")) return true;
	if (_node->get_name() == TXT("generationCondition")) return true;
	return false;
}

bool Element::load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc)
{
	bool result = true;

	useStack = _node->get_bool_attribute_or_from_child(TXT("useStack"), useStack);
	useCheckpoint = _node->get_bool_attribute_or_from_child(TXT("useCheckpoint"), useCheckpoint);

	result &= generationCondition.load_from_xml(_node);

#ifdef AN_DEVELOPMENT
	locationInfo = _node->get_location_info();
#endif

	randomiseSeed = _node->get_bool_attribute_or_from_child_presence(TXT("randomiseSeed"), randomiseSeed);

	dropMovementCollision = _node->get_bool_attribute_or_from_child_presence(TXT("dropMovementCollision"), dropMovementCollision);
	dropPreciseCollision = _node->get_bool_attribute_or_from_child_presence(TXT("dropPreciseCollision"), dropPreciseCollision);

	for_every(node, _node->children_named(TXT("parameters")))
	{
		result &= parameters.load_from_xml(node);
		_lc.load_group_into(parameters);
	}

	for_every(node, _node->children_named(TXT("requiredParameters")))
	{
		result &= requiredParameters.load_from_xml(node);
		_lc.load_group_into(requiredParameters);
	}

	for_every(node, _node->children_named(TXT("optionalParameters")))
	{
		result &= optionalParameters.load_from_xml(node);
		_lc.load_group_into(optionalParameters);
	}

	for_every(node, _node->children_named(TXT("outputParameters")))
	{
		for_every(n, node->all_children())
		{
			OutputParameter op;
			op.type = RegisteredType::get_type_id_by_name(n->get_name().to_char());
			op.name = n->get_name_attribute(TXT("name"));
			op.storeAs = n->get_name_attribute(TXT("as"), op.name);
			if (op.type == NONE)
			{
				error_loading_xml(n, TXT("not recognised type \"%S\""), n->get_name().to_char());
				result = false;
			}
			else if (! op.name.is_valid())
			{
				error_loading_xml(n, TXT("no name provided for output parameter"));
				result = false;
			}
			else
			{
				if (!op.storeAs.is_valid())
				{
					op.storeAs = op.name;
				}
				outputParameters.push_back(op);
			}
		}
	}

	for_every(node, _node->children_named(TXT("wheresMyPoint")))
	{
		result &= wheresMyPointProcessor.load_from_xml(node);
	}
	
	for_every(node, _node->children_named(TXT("wheresMyPointPost")))
	{
		result &= wheresMyPointProcessorPost.load_from_xml(node);
	}	

	forceUseRandomSeed.load_from_xml(_node, TXT("forceUseRandomSeed"));
	
	return result;
}

bool Element::prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext)
{
	bool result = true;

	//

	return result;
}

bool Element::process(GenerationContext & _context, ElementInstance & _instance) const
{
	todo_important(TXT("implement_ in derived class!"));
	return true;
}

bool Element::post_process(GenerationContext & _context, ElementInstance & _instance) const
{
	// by default nothing here is done
	return true;
}

bool Element::load_single_element_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc, tchar const * const _childContainerName, RefCountObjectPtr<Element> & _element)
{
	bool result = true;

	if (IO::XML::Node const * node = _childContainerName? _node->first_child_named(_childContainerName) : _node)
	{
		for_every(child, node->all_children())
		{
			result &= load_single_element_from_xml(child, _lc, _element);
		}
	}
	else
	{
		result = false;
	}

	return result;
}

bool Element::load_single_element_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc, RefCountObjectPtr<Element> & _element)
{
	bool result = true;

	if (_node)
	{
		if (_element.is_set())
		{
			if (Framework::is_preview_game())
			{
				warn_loading_xml(_node, TXT("element \"%S\" already loaded when loading a single element - removing and reloading (special case for preview game, maybe we load it twice)"), _node->get_name().to_char());
				_element.clear();
			}
			else
			{
				error_loading_xml(_node, TXT("element \"%S\" already loaded when loading a single element"), _node->get_name().to_char());
				result = false;
				return false;
			}
		}
		if (_node->get_name() == TXT("empty"))
		{
			// just skip it
		}
		else
		{
			_element = MeshGeneration::Element::create_from_xml(_node, _lc);
			if (_element.is_set())
			{
				if (!_element->load_from_xml(_node, _lc))
				{
					error_loading_xml(_node, TXT("error loading element \"%S\""), _node->get_name().to_char());
					result = false;
				}
			}
			else
			{
				error_loading_xml(_node, TXT("could not load element \"%S\""), _node->get_name().to_char());
				result = false;
			}
		}
	}
	else
	{
		error_loading_xml(_node, TXT("no node"));
		result = false;
	}

	return result;
}

bool Element::update_wmp(ElementInstance & _instance, WheresMyPoint::ToolSet const * _processor) const
{
	if (_processor == nullptr)
	{
		_processor = &wheresMyPointProcessor;
	}
	WheresMyPoint::Value tempValue;
	WheresMyPoint::Context context(&_instance);
	context.set_random_generator(_instance.access_context().access_random_generator());
	if (_processor->update(tempValue, context))
	{
		return true;
	}
	else
	{
		error_generating_mesh(_instance, TXT("error processing \"wheresMyPoint\" processor for element"));
		return false;
	}
}
