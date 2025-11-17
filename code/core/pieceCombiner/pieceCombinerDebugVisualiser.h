#pragma once

#include "..\concurrency\threadManager.h"
#include "..\debug\debugVisualiser.h"
#include "..\system\video\video3d.h"

namespace PieceCombiner
{
	template <typename CustomType> class Generator;
	template <typename CustomType> struct GenerationSpace;

	template <typename CustomType>
	class DebugVisualiser
	: public ::DebugVisualiser
	{
		typedef ::DebugVisualiser base;
	public:
		DebugVisualiser(Generator<CustomType> * _generator);
		virtual ~DebugVisualiser();

		void show(Colour const & _colour, String const & _message);
		void mark_finished() { markFinished = true; }

	public: // DebugVisualiser
		override_ void advance();
		override_ void render();
		override_ void use_font(::System::Font* _font, float _fontHeight = 0.0f);

	private:
		Generator<CustomType> * generator = nullptr;

		Colour messageColour;
		String message;

		// copied to allow rendering
		Colour displayMessageColour;
		String displayMessage;

		Concurrency::SpinLock generatorLock = Concurrency::SpinLock(SYSTEM_SPIN_LOCK); // it is used to separate generator doing its stuff from visualiser using stuff directly from generator (!) to advance and display

		bool markFinished = false;
		bool allowedNextStep = false;
		bool allowedToEnd = false;

		float const defaultPieceRadius = 1.0f;
		float const minPieceSeparation = 2.0f;
		float const pieceSeparation_strength = 1.0f;
		float const maxConnectorSeparation = 6.0f;
		float const maxConnectorSeparationCutOff = 12.0f;
		float const connectorSeparation_strength = 0.4f;
		float const connectorRadius = 0.2f;

		void move_things_around();
		void update_inner_radius_for_pieces_in(GenerationSpace<CustomType>* _generationSpace);
		void add_things_to_display(GenerationSpace<CustomType>* _generationSpace, Vector2 const & _zoomRefPoint = Vector2::zero, float _zoom = 1.0f);
	};
};
