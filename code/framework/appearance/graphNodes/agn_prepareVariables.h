#pragma once

#include "..\animationGraphNode.h"

//

namespace Framework
{
	namespace AnimationGraphNodesLib
	{
		class PrepareVariables
			: public AnimationGraphNodeWithSingleInput
		{
			FAST_CAST_DECLARE(PrepareVariables);
			FAST_CAST_BASE(AnimationGraphNodeWithSingleInput);
			FAST_CAST_END();

			typedef AnimationGraphNodeWithSingleInput base;

		public:
			static IAnimationGraphNode* create_node();

			PrepareVariables() {}
			virtual ~PrepareVariables() {}

		public: // IAnimationGraphNode
			implement_ bool load_from_xml(IO::XML::Node const* _node, LibraryLoadingContext& _lc);
			implement_ bool prepare_runtime(AnimationGraphRuntime& _runtime, AnimationGraphNodeSet& _inSet);
			implement_ void prepare_instance(AnimationGraphRuntime& _runtime) const;
			implement_ void on_activate(AnimationGraphRuntime& _runtime) const;
			implement_ void on_deactivate(AnimationGraphRuntime& _runtime) const;
			implement_ void execute(AnimationGraphRuntime& _runtime, AnimationGraphExecuteContext& _context, OUT_ AnimationGraphOutput& _output) const;

		private:
			ANIMATION_GRAPH_RUNTIME_VARIABLES_USER(PrepareVariables);

			// we use registers to hold temporary values that we operate on
			// all runtime variables work only because we always process everything in the same order: for each variable we do all operations
			// as they do not change, we may have multiple instances of runtime variables
			struct Registers
			{
				float reg[8];
			};
			struct Operation
			{
				enum Type
				{
					Unknown,
					Animation,
					Controller,
					Limit,
					// math op
						Add,
						Div,
						Mul,
						Sub,
					Presence,
					Value,
					Variable,
					Result
				};

				Type type;

				union Data
				{
					struct AnimationData
					{
						int resultReg;
						Name animation;
						Name what;
					} animation;
					struct ControllerData
					{
						int resultReg;
						Name what;
					} controller;
					struct MathOpData
					{
						int resultReg;
						int reg;
						int byReg;
						float byValue;
					} mathOp;
					struct LimitData
					{
						int resultReg;
						Range range;
						int reg;
					} limit;
					struct PresenceData
					{
						int resultReg;
						Name what;
					} presence;
					struct ValueData
					{
						int resultReg;
						float value;
					} value;
					struct VariableData
					{
						int resultReg;
						Name varID;
					} variable;
					struct ResultData
					{
						int reg;
						bool onlyHigher;
					} result;

					Data() { /* can have invalid data, we will be clearing that on loading */ }
				} data;
			};
			struct Variable
			{
				Name varID;
				float blendTime = 0.0f;
				ArrayStatic<Operation, 20> operations;
			};
			Array<Variable> variables;
		};
	};
};
