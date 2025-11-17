#pragma once

#include "..\..\core\memory\refCountObject.h"
#include "..\..\core\memory\refCountObjectPtr.h"

#include "stringFormatter.h"

#include "textBlock.h"

namespace Framework
{
	class TextBlock;
	struct TextBlockInstance;
	typedef RefCountObjectPtr<TextBlockInstance> TextBlockInstancePtr;

	/**
	 *	This is pure instance.
	 *	Each text block instance points at text block it represents and provides additional text blocks by paramId (from text).
	 *	This is used to create string formatter params and finally format this as a string
	 *
	 *	Bigger picture:
	 *	We create instance to allow to save it to file and load it from file (savegames!) and have same result.
	 */
	struct TextBlockInstance
	: public RefCountObject
	, public ICustomStringFormatter
	{
	public:
		TextBlockInstance();
		virtual ~TextBlockInstance();

		String format() const; // format using existing params

		bool serialise(Serialiser & _serialise);

	protected: // ICustomStringFormatter
		implement_ ICustomStringFormatter const * get_hard_copy() const { return this; }
		implement_ String format_custom(tchar const * _format, StringFormatterParams & _params) const;

	private:
		struct Element;
		typedef RefCountObjectPtr<Element> ElementPtr;

		struct Param
		{
			GS_ Name paramId;
			GS_ ElementPtr textBlockInstance;

			Param() {}
			Param(Name const & _paramId, Element* _textBlockInstance) : paramId(_paramId), textBlockInstance(_textBlockInstance) {}

			bool serialise(Serialiser & _serialise);
		};

		struct Element
		: public RefCountObject
		, public ICustomStringFormatter
		{
			GS_ TextBlockPtr textBlock;
			GS_ Array<Param> params;

			implement_ ICustomStringFormatter const * get_hard_copy() const { return this; }
			implement_ String format_custom(tchar const * _format, StringFormatterParams & _params) const;

			bool serialise(Serialiser & _serialise);
		};

		GS_ ElementPtr element;

		// this should be only in main, should be copy of something we provided as parameter for buiding instance
		// when we build instance, we format it to gather all params we need from providers and then we dump providers
		// after that we should have a pure instance
		GS_ StringFormatterParams* stringFormatterParams = nullptr;
		GS_ String usingFormat; // should be only in main as well

		static TextBlockInstancePtr build_from_text_block(TextBlock * _textBlock, tchar const * _format, StringFormatterParams & _params);
		static TextBlockInstancePtr build_from_piece(PieceCombiner::PieceInstance<TextBlockGenerator> const * _pieceInstance, tchar const * _format, StringFormatterParams & _params);
		static ElementPtr build_element_from_piece(PieceCombiner::PieceInstance<TextBlockGenerator> const * _pieceInstance);

		friend class TextBlock;
	};

	struct TextBlockInstances
	{
	public:
		bool is_empty() const { return textBlocks.is_empty(); }

		void clear() { textBlocks.clear(); }
		void add_instance_of(TextBlock* _tb, tchar const * _format = nullptr, StringFormatterParams & _params = StringFormatterParams().temp_ref(), Random::Generator & _rg = Random::Generator().temp_ref());
		void add(TextBlockInstance* _tbi) { textBlocks.push_back(TextBlockInstancePtr(_tbi)); }
		void add(TextBlockInstancePtr const & _tbi) { textBlocks.push_back(_tbi); }
		void end_paragraph() { textBlocks.push_back(TextBlockInstancePtr()); }

		String format(tchar const * _newLine = TXT("~"), int _emptyLinesBetween = 1) const;

		bool serialise(Serialiser & _serialise);

	private:
		GS_ Array<TextBlockInstancePtr> textBlocks; // if empty, it is paragraph separator
	};
};
