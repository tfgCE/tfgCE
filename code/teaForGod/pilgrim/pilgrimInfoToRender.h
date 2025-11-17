#pragma once

#include "..\teaEnums.h"

#include "..\game\energy.h"

#include "..\..\core\memory\safeObject.h"
#include "..\..\core\types\hand.h"

#include "..\..\framework\library\usedLibraryStored.h"

namespace System
{
	class RenderTarget;
};

namespace Framework
{
	interface_class IModulesOwner;
	class Font;
	class TexturePart;
};

namespace TeaForGodEmperor
{
	struct PilgrimInfoToRender
	{
		struct VariableValue
		{
			int value = 0;
			float highlight = 0.0f; // >0 if went up, <0 if went down

			VariableValue() {}
			VariableValue(int _v) : value(_v) {}

			VariableValue& operator=(int _v) { if (_v < value) highlight = -1.0f; if (_v > value) highlight = 1.0f; value = _v; return *this; }

			void advance(float _deltaTime);
			void clear_highlight() { highlight = 0.0f; }
		};

		struct EquipmentInfo
		{
			SafePtr<::Framework::IModulesOwner> equipment;
			VariableValue ammo = 0; // if makes sense for given equipment
			VariableValue magazineAmmo = 0; // magazine + chamber

			void clear();
			void clear_highlight();
			void update(::Framework::IModulesOwner* _equipment, Energy const& _availableEnergyInMainStorage, bool _inUse);
			void advance(float _deltaTime);
		};

		struct Hand
		{
			VariableValue energy = 0;
			EquipmentInfo mainEquipment;
			EquipmentInfo inHandEquipment;

			void clear();
			void clear_highlight();
			void update(::Framework::IModulesOwner* _pilgrim, ::Hand::Type _hand);
			void advance(float _deltaTime);
		};

		struct Info
		{
			SafePtr<::Framework::IModulesOwner> pilgrim;

			//
			VariableValue health = 0;
			VariableValue healthTotal = 0;

			//
			Hand hand[::Hand::MAX];

			void clear();
			void clear_highlight();
			void update(::Framework::IModulesOwner* _pilgrim);
			void advance(float _deltaTime);
		};

		Info curr;

		bool isVisible = false; // we want to make it visible only when rendering world

		Framework::UsedLibraryStored<Framework::Font> fontBigNumbers;
		Framework::UsedLibraryStored<Framework::Font> fontMidNumbers;
		Framework::UsedLibraryStored<Framework::Font> fontText;
		Framework::UsedLibraryStored<Framework::TexturePart> leftHand;
		Framework::UsedLibraryStored<Framework::TexturePart> rightHand;

		float get_best_border_width(Vector2 const& _size);

		void clear_highlight();

		void advance(::Framework::IModulesOwner* _pilgrim, float _deltaTime);

		void update(::Framework::IModulesOwner* _pilgrim);

		void render(::System::RenderTarget* _rt, float _borderWidth);

		void update_assets();
	};
};

