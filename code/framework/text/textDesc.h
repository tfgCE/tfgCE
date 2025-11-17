#pragma once

#include "stringFormatter.h"
#include "textBlockRef.h"

#include "..\..\core\memory\refCountObject.h"
#include "..\..\core\random\random.h"

namespace Framework
{
	class TextDescBuilder;
	struct TextBlockInstances;
	struct TextDescElement;

	typedef RefCountObjectPtr<TextDescElement> TextDescElementPtr;

	/**
	 *	Text description is built with elements
	 *
	 *	TextDesc itself is broken into descriptions identified by id or number:
	 *
	 *		descId	descNo	descIO	text
	 *		Intro	0		0		Hello world!							<- first paragraph, one TextDescElement
	 *
	 *		Info	10		0		Let's start with the beggining.			<- openning of second paragraph
	 *		Info	10		10		There was nothing that interesting.		<- second sentence of second paragraph
	 *
	 *		...
	 *
	 *	This will end up with (if paragraphs are used):
	 *
	 *		Hello World!
	 *
	 *		Let's start with the beggining. There was nothing that interesting.
	 *
	 *		...
	 *
	 *	It is possible that some descriptions share same descNo - they will be in same place in description although order may be random
	 *	If they share same descId - only one per descId will be chosen
	 *	If you use descIO it is advised to use contentId to tell them apart
	 *
	 */
	struct TextDescElement
	: public RefCountObject
	{
	public:
		static bool load_from_xml_into(REF_ Array<TextDescElementPtr> & _ptrs, IO::XML::Node const * _node, LibraryLoadingContext & _lc, tchar const * _nodeNameAndID = TXT("descriptionElement"), tchar const * _groupNode = TXT("descriptionElements"));

		bool load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc, tchar const * _id);
		bool prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext);

	private:
		float chance = 1.0f;
		float priority = 0.0f;
		Name descId; // describes part of description, it is possible that multiple descriptions share same descNo but have different descId
		int descNo; // always has to be defined, two elements of same descNo can't exist unless descId is different
		int descInternalOffset = 0; // to block longer desc pieces
		Name contendId; // mutliple descriptions may share same descNo and descId but we have to tell them apart
		Tags descTags;
		Tags blockDescTags; // blocks any containing mentioned tags (does_match_any)
		Name paragraphId; // to break into paragraphs
		TextBlockRef text;

		friend class TextDescBuilder;
	};

	class TextDescBuilder
	{
	public:
		void build(REF_ TextBlockInstances & _textBlockInstances, tchar const * _format, StringFormatterParams & _params, Random::Generator _usingGenerator = Random::Generator()) const;

	public: // setup
		void add(TextDescElement * _element, ProvideStringFormatterParams _provideFunc = nullptr);
		void define_desc_id(Name const & _descId, int _no) { descIdNo[_descId] = _no; }

	private:
		struct TextDescElementRef
		{
			TextDescElementPtr element;
			ProvideStringFormatterParams provideParamsFunc = nullptr;
		};
		Array<TextDescElementRef> elements; // elements used to build description

		Map<Name, int> descIdNo; // number assigned to desc id
	};
};
