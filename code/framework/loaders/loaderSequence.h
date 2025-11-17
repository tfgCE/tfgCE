#pragma once

#include "iLoader.h"

#include "..\..\core\containers\array.h"
#include "..\..\core\random\random.h"
#include "..\..\core\types\colour.h"

struct Vector2;
struct VectorInt2;

namespace System
{
	class Video3D;
};

namespace Loader
{
	class Sequence
	: public ILoader
	{
		typedef ILoader base;
	public:
		Sequence() {}
		virtual ~Sequence() {}

		void add(ILoader* _loader, Optional<float> const& _time = NP, std::function<bool()> _should_show = nullptr, Optional<bool> const & _passDeactivationThrough = NP);
		void add(std::function<void()> _perform = nullptr);

	public: // ILoader
		implement_ void update(float _deltaTime);
		implement_ void display(::System::Video3D * _v3d, bool _vr);
		implement_ bool activate();
		implement_ void deactivate();

	private:
		struct Element
		{
			ILoader* loader = nullptr;
			Optional<float> timeLeft;
			std::function<bool()> should_show = nullptr;
			std::function<void()> perform = nullptr;
			bool passDeactivationThrough = false;
		};
		Array<Element> loaders;
		ILoader* validLoaderForDisplay = nullptr;

		bool shouldDeactivate = false;

		void do_perform(int _leaveAmount = 0);
	};
};
