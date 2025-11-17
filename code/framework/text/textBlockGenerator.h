#pragma once

#include "textBlockRef.h"

#include "..\..\core\pieceCombiner\pieceCombiner.h"
#include "..\..\framework\library\usedLibraryStored.h"

namespace Framework
{
	struct StringFormatterParams;

	class TextBlockGenerator
	{
	public: // classes for piece combiner
		struct LoadingContext
		: public PieceCombiner::DefaultCustomGenerator::LoadingContext
		{};

		struct PreparingContext
		: public PieceCombiner::DefaultCustomGenerator::PreparingContext
		{};

		struct GenerationContext
		: public PieceCombiner::DefaultCustomGenerator::GenerationContext
		{
			StringFormatterParams* params = nullptr;
		};

		struct PieceDef
		: public PieceCombiner::DefaultCustomGenerator::PieceDef
		{
			TextBlockPtr textBlock;

			bool prepare(PieceCombiner::Piece<TextBlockGenerator> * _piece, PreparingContext * _context = nullptr) { return true; }

			String get_desc() const;
		};

		struct ConnectorDef
		: public PieceCombiner::DefaultCustomGenerator::ConnectorDef
		{
			Name paramId; // if set, will provide data for owning text block instance

			bool prepare(PieceCombiner::Piece<TextBlockGenerator> * _piece, PieceCombiner::Connector<TextBlockGenerator> * _connector, PreparingContext * _context = nullptr) { return true; }
		};

		struct PieceInstanceData
		: public PieceCombiner::DefaultCustomGenerator::PieceInstanceData
		{
			bool isStarting = false;
			PieceInstanceData(bool _isStarting = false) : isStarting(_isStarting) {}
		};

		struct ConnectorInstanceData
		: public PieceCombiner::DefaultCustomGenerator::ConnectorInstanceData
		{};

		struct Utils
		{
		public:
			static bool should_use_connector(PieceCombiner::Generator<TextBlockGenerator> const & _generator, PieceCombiner::PieceInstance<TextBlockGenerator> const & _pieceInstance, PieceCombiner::Connector<TextBlockGenerator> const & _connector);
		};
	};
};
