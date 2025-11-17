#pragma once

#include "iWMPTool.h"

namespace WheresMyPoint
{
	class TransformOp
	: public ITool
	{
		typedef ITool base;
	public: // ITool
		override_ bool load_from_xml(IO::XML::Node const * _node);
		override_ bool update(REF_ Value & _value, Context & _context) const;

	protected:
		virtual void handle_operation(REF_ Value & _value, Value const & _transform) const = 0;
		virtual bool can_handle(Value const & _value) const = 0;

	private:
		ToolSet transformToolSet;
	};

	class TransformOpOnVector
	: public TransformOp
	{
	protected:
		implement_ bool can_handle(Value const & _value) const;
	};

	class TransformOpOnTransform
	: public TransformOp
	{
	protected:
		implement_ bool can_handle(Value const & _value) const;
	};
	
	class TransformOpOnQuat
	: public TransformOp
	{
	protected:
		implement_ bool can_handle(Value const & _value) const;
	};
	
	class VectorToWorldOf
	: public TransformOpOnVector
	{
	protected:
		implement_ void handle_operation(REF_ Value & _vector, Value const & _transform) const;
	};

	class VectorToLocalOf
	: public TransformOpOnVector
	{
	protected:
		implement_ void handle_operation(REF_ Value & _vector, Value const & _transform) const;
	};

	class LocationToWorldOf
	: public TransformOpOnVector
	{
	protected:
		implement_ void handle_operation(REF_ Value & _vector, Value const & _transform) const;
	};

	class LocationToLocalOf
	: public TransformOpOnVector
	{
	protected:
		implement_ void handle_operation(REF_ Value & _vector, Value const & _transform) const;
	};

	class ToWorldOf
	: public TransformOpOnTransform
	{
	protected:
		implement_ void handle_operation(REF_ Value & _value, Value const & _transform) const;
	};

	class ToLocalOf
	: public TransformOpOnTransform
	{
	protected:
		implement_ void handle_operation(REF_ Value & _value, Value const & _transform) const;
	};

	class QuatToWorldOf
	: public TransformOpOnQuat
	{
	protected:
		implement_ void handle_operation(REF_ Value & _value, Value const & _transform) const;
	};

	class QuatToLocalOf
	: public TransformOpOnQuat
	{
	protected:
		implement_ void handle_operation(REF_ Value & _value, Value const & _transform) const;
	};

};
