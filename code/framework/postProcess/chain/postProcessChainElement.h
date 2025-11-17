#pragma once

#include "..\..\..\core\fastCast.h"
#include "..\graph\postProcessNode.h"
#include "..\postProcessRenderTargetPointer.h"

namespace Framework
{
	class PostProcessChain;
	class PostProcessChainElement;
	struct PostProcessChainProcessingContext;

	struct PostProcessChainConnector
	{
	public:
		PostProcessChainConnector()
		: index(NONE)
		{
		}

		Name const & get_name() const { return name; }
		int get_index() const { return index; }

		void setup(Name const & _name, int _index)
		{
			an_assert(index == NONE, TXT("do not re-setup!"));
			name = _name;
			index = _index;
		}

	private:
		Name name;
		int index;
	};

	struct PostProcessChainConnection
	{
		PostProcessChainElement* connectedTo;
		int connectedToIndex;

		PostProcessChainConnection()
		: connectedTo(nullptr)
		, connectedToIndex(NONE)
		{
		}

		PostProcessChainConnection(PostProcessChainElement* _connectedTo, int _connectedToIndex)
		: connectedTo(_connectedTo)
		, connectedToIndex(_connectedToIndex)
		{
			an_assert(connectedTo != nullptr);
			an_assert(connectedToIndex != NONE);
		}

		void clear()
		{
			connectedTo = nullptr;
			connectedToIndex = NONE;
		}
	};

	struct PostProcessChainInputConnector
	: public PostProcessChainConnector
	{
		PostProcessChainConnection connection;
		PostProcessRenderTargetPointer inputRenderTarget;

		void clear_connection()
		{
			release_render_target();
			connection.clear();
		}
		
		void release_render_target()
		{
			inputRenderTarget.clear();
		}
	};

	struct PostProcessChainOutputConnector
	: public PostProcessChainConnector
	{
		Array<PostProcessChainConnection> connections;

		void clear_connections()
		{
			connections.clear();
		}
	};

	class PostProcessChainElement
	{
		FAST_CAST_DECLARE(PostProcessChainElement);
		FAST_CAST_END();
	public:
		PostProcessChainElement();
		virtual ~PostProcessChainElement();

		void clear_connections();

		virtual void release_render_targets(); // release all render target pointers - this should be done before post processing starts

		virtual void for_every_shader_program_instance(ForShaderProgramInstance _for_every_shader_program_instance);

		virtual void do_post_process(PostProcessChainProcessingContext const & _processingContext); // do post processing operations based on inputs, propagate to outputs, release inputs

		virtual int get_output_render_target_count() const;
		virtual PostProcessRenderTargetPointer const & get_output_render_target(int _index) const;

		Array<PostProcessChainInputConnector> & all_inputs() { return inputs; }
		Array<PostProcessChainOutputConnector> & all_outputs() { return outputs; }

		virtual bool is_active() const { return true; }

	protected:
		void gather_input(int _index); // update render target pointer for input
		void propagate_outputs(); // set render target pointers in inputs in nodes that outputs lead to

		void add_input(Name const & _name);
		void add_output(Name const & _name);

	protected:
		Array<PostProcessChainInputConnector> inputs;
		Array<PostProcessChainOutputConnector> outputs;
	};
};
