#pragma once

#include "..\..\framework\library\libraryStored.h"

namespace TeaForGodEmperor
{
	class LineModel
	: public Framework::LibraryStored
	{
		FAST_CAST_DECLARE(LineModel);
		FAST_CAST_BASE(Framework::LibraryStored);
		FAST_CAST_END();
		LIBRARY_STORED_DECLARE_TYPE();

		typedef Framework::LibraryStored base;
	public:
		struct Line
		{
			Vector3 a = Vector3::zero;
			Vector3 b = Vector3::zero;
			Optional<Colour> colour;

			Line() {}
			Line(Vector3 const& _a, Vector3 const& _b, Optional<Colour> const& _colour) : a(_a), b(_b), colour(_colour) {}
			bool load_from_xml(IO::XML::Node const* _node);
		};

		LineModel(Framework::Library * _library, Framework::LibraryName const & _name);

		Array<Line> const& get_lines() const { return lines; }

	public:	// use only when can't be accessed
		void clear();
		void add_line(Line const& _l);
		void mirror(Plane const& _p);
		void fit_xy(Range2 const& _r);

	public: // LibraryStored
#ifdef AN_DEVELOPMENT
		override_ void ready_for_reload();
#endif
		override_ bool load_from_xml(IO::XML::Node const * _node, Framework::LibraryLoadingContext & _lc);
		override_ bool prepare_for_game(Framework::Library* _library, Framework::LibraryPrepareContext& _pfgContext);

		override_ void debug_draw() const;

	protected:
		virtual ~LineModel();

	private:
		Array<Line> lines;
		static bool load_lines_into(IO::XML::Node const* _node, Array<Line>& _lines, LineModel const* _forLineModel);

	private:
		// during load we just store xml node and we will be actually processing everything only when preparing
		// this way we may have any order of loading line models
		Concurrency::SpinLock prepareLock;
		RefCountObjectPtr<IO::XML::Node> loadFromNode;
		bool load_lines_on_demand(); // will load during
	};
};
