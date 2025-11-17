#pragma once

#include "..\..\framework\library\usedLibraryStored.h"

namespace Framework
{
	class ObjectType;
	interface_class IModulesOwner;
};

namespace TeaForGodEmperor
{
	struct MissionDefinitionObjectives;
	struct MissionState;
	struct PilgrimOverlayInfoRenderableData;

	/*
		This structure stores randomised mission objectives.
		This might be filled before the actual mission state is created.
	*/
	struct MissionStateObjectives
	{
		static bool compare(MissionStateObjectives const& _a, MissionStateObjectives const& _b);
		MissionStateObjectives() {}
		MissionStateObjectives(MissionStateObjectives const& _other) { operator=(_other); }
		MissionStateObjectives & operator=(MissionStateObjectives const& _other) { copy_from(_other); return *this; }

		bool is_item_required(Framework::IModulesOwner* _imo) const;

	public:
		void reset();
		void copy_from(MissionStateObjectives const& _other);

		void prepare_for_gameplay();

	public:
		void prepare_to_render(PilgrimOverlayInfoRenderableData& _rd, Vector3 const& _xAxis, Vector3 const& _yAxis, Vector2 const& _startingOffset, float _scaleLineModel) const;

	public:
		bool save_to_xml(IO::XML::Node* _node) const;
		bool load_from_xml(IO::XML::Node const* _node);

	private:
		mutable Concurrency::MRSWLock lock = Concurrency::MRSWLock(TXT("TeaForGodEmperor.MissionStateObjectives.lock"));
		Array<Framework::UsedLibraryStored<Framework::ObjectType>> bringItemsTypes;

		friend struct MissionDefinitionObjectives;
		friend struct MissionState;
	};

};
