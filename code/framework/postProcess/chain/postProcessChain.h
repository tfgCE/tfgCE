#pragma once

#include "..\graph\postProcessNode.h"

#include "..\..\..\core\containers\array.h"
#include "..\..\..\core\system\video\fragmentShaderOutputSetup.h"
#include "..\..\..\core\system\video\camera.h"

struct Tags;

namespace System
{
	class RenderTarget;
	class Video3D;
	class Camera;
};

namespace Framework
{
	interface_class PostProcessChainElement;
	class PostProcessChainInputElement;
	class PostProcessChainOutputElement;
	class PostProcessChainProcessElement;
	class PostProcessRenderTargetManager;
	struct PostProcessRenderTargetPointer;
	class PostProcess;

	/** 
	 *	First element (or elements) in chain should be output from rendering.
	 *	After adding element, chain should be linked.
	 *	During linking it is made sure that at the end there is "output" element.
	 *	Each element has inputs and outputs. And it knows how many users of each output there are.
	 *	Processing elements have PostProcessInstance inside (that is also aware of being PostProcessChainElement).
	 *	Such elements know what are they're inputs and outputs.
	 *
	 *	All elements should have same size! (?!)
	 *
	 *
	 *	Anyway, because all I wrote above doesn't make much sense, here's another view of things:
	 *	- Input and output elements are done automatically.
	 *	- There can be multiple input nodes for multiple render targets - it is possible to override_ names, for example to have "current frame", "previous frame" render targets available.
	 *	- PostProcessInstances from PostProcessChainElement find inputs they need in previous elements in chain by name.
	 *	- Main output is defined with define_output
	 *	- How to use?
	 *			1. define output to know what do we actually need from post process chain
	 *			2. add post process instances to do something - add them in order they will be processed - from first to last
	 *			3. every frame setup output render target usage - so we can inform render target that we need particular textures
	 *			4. inform about camera we use for post process - this is useful for view rays
	 *			5. do rendering
	 *			6. do post process - providing render target(s) as input (remember to copy default values)
	 *			7. release render targets - they should be released, so they can be used by something else
	 */
	class PostProcessChain
	{
	public:
		PostProcessChain(PostProcessRenderTargetManager* _renderTargetManager);
		~PostProcessChain();

		PostProcessRenderTargetManager* get_render_target_manager() { return renderTargetManager; }

	public: // creating / setting up
		void define_output(::System::FragmentShaderOutputSetup const & _outputDefinition);
		::System::FragmentShaderOutputSetup const & get_output_definition() const { return outputDefinition; }
		
		PostProcessChainProcessElement* add(PostProcess* _postProcess, Tags const & _requires, Tags const & _disallows);
		void clear();

		void mark_linking_required() { requiresLinking = true; }

		inline bool is_valid() const { an_assert(!requiresLinking, TXT("it should be linked before validation")); return isValid; }

	public: // running
		// mark which parts of render target will be used (when everything is linked we know which ones we will use and which won't)
		void setup_render_target_output_usage(::System::RenderTarget* _inputRenderTarget);
		void setup_render_target_output_usage(Array<::System::RenderTarget*> const & _inputRenderTargets, Array<Name> const * _nameOverrides);

		// it should be camera that was used in actual rendering - it has to have projection matrix calculated
		void set_camera_that_was_used_to_render(::System::Camera const & _camera);

		void for_every_shader_program_instance(ForShaderProgramInstance _for_every_shader_program_instance);

		void do_post_process(::System::RenderTarget* _inputRenderTarget, ::System::Video3D* _v3d);
		void do_post_process(Array<::System::RenderTarget*> const & _inputRenderTargets, Array<Name> const * _nameOverrides, ::System::Video3D* _v3d);

		void render_output(::System::Video3D* _v3d, LOCATION_ Vector2 const & _leftBottom, SIZE_ Vector2 const & _size);

		void release_render_targets();

		int get_output_render_target_count() const;
		PostProcessRenderTargetPointer const & get_output_render_target(int _index = 0) const;

	private:
		PostProcessRenderTargetManager* renderTargetManager;

		Array<PostProcessChainElement*> elements;
		Array<PostProcessChainElement*> activeElements;
		bool isValid; // are all elements valid
		bool requiresLinking;

		::System::FragmentShaderOutputSetup outputDefinition;

		::System::Camera cameraUsedToRender;
#ifdef AN_ASSERT
		bool cameraUsedToRenderProvided;
#endif

#ifdef AN_DEVELOPMENT
		bool calledForEveryShaderProgramInstance = false;
#endif

		Array<PostProcessChainInputElement*> inputElements;
		PostProcessChainOutputElement* outputElement;
		Array<RefCountObjectPtr<::System::RenderTarget>> inputRenderTargets;

		void add(PostProcessChainElement* _element);

		void disconnect_elements();
		void arrange_input_and_output(int _inputCount = NONE); // create if missing, make sure they are at the end
		void link(); // link all elements by name

		void internal_setup_render_target_output_usage(::System::RenderTarget*const* _pInputRenderTargets, Name const* _nameOverrides, int _count);
		void internal_do_post_process(::System::RenderTarget*const* _pInputRenderTargets, Name const* _nameOverrides, int _count, ::System::Video3D* _v3d);

		void use_as_input(::System::RenderTarget*const* _pInputRenderTargets, Name const* _nameOverrides, int _count);
	};

};
