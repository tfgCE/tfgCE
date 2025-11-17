#pragma once

#include "..\library\libraryStored.h"
#include "..\..\core\memory\refCountObject.h"



namespace System
{
	class Video3D;
	struct ShaderProgramInstance;
	struct VertexFormat;
};

namespace Framework
{
	class ColourPalette;
	class Sprite;
	class SpriteTextureProcessor;
	class Texture;
	struct SpriteLayer;
	struct SpriteTransformLayer;
	struct SpriteFullTransformLayer;
	struct IncludeSpriteLayer;
	struct SpriteContentAccess;

	struct SpritePixel
	{
		enum Content
		{
			None = -1,
			Invisible = -2, // sprite pixel is invisible but is considered full for creating sides
			ForceEmpty = -3, // will clear if combined/merged

			MinActualContent = ForceEmpty,
			MaxActualContent = Invisible
		};
		static tchar const* content_to_char(Content _c)
		{
			if (_c == Invisible) return TXT("invisible");
			if (_c == ForceEmpty) return TXT("force empty");
			return TXT("?");
		}

		int pixel = SpritePixel::None; // NONE - empty, otherwise it is a colour from the palette

		bool operator == (SpritePixel const& _other) const
		{
			return pixel == _other.pixel;
		}

		static bool can_be_replaced_in_merge(int v) { return v == SpritePixel::None || v == SpritePixel::Invisible; }
		static bool is_anything(int v) { return v != SpritePixel::None; }
		static bool is_solid(int v) { return v >= 0; }
		static bool makes_side(int v) { return is_solid(v) || v == SpritePixel::Invisible; }
		static bool is_drawable(int v) { return v != SpritePixel::None; }
	};

	struct SpritePixelHitInfo
	{
		Optional<VectorInt2> hit;
	};

	struct CombineSpritePixelsContext
	{

	};

	namespace Editor
	{
		class Sprite;
		class SpriteLayout;
	};

	/**
	 *	Sprite is composed of multiple layers
	 *	Layers are stored in a single list with "depth" parameter that tells the order or belonging
	 */
	struct ISpriteLayer
	: public RefCountObject
	{
		FAST_CAST_DECLARE(ISpriteLayer);
		FAST_CAST_END();

	public:
		static Optional<int> get_child_depth(ISpriteLayer const* _for, Array<RefCountObjectPtr<ISpriteLayer>> const& _layers); // "not set" if no children
		static bool for_every_child_combining(ISpriteLayer const * _of, Array<RefCountObjectPtr<ISpriteLayer>> const& _layers, std::function<bool(ISpriteLayer* layer)> _do);

	public:
		ISpriteLayer();
		virtual ~ISpriteLayer();

	public:
		bool is_temporary() const { return temporary; }
		void be_temporary(bool _temporary = true) { temporary = _temporary; }

		Name const& get_id() const { return id; }
		void set_id(Name const& _id) { id = _id; }
		// changing of any of below requires explicit call of combine_pixels
		int get_depth() const { return depth; }
		void set_depth(int _depth) { depth = max(0, _depth); }
		bool is_visible(Sprite* _partOf) const;
		bool get_is_visible() const { return visible; }
		void set_visible(bool _visible) { visible = _visible; }
		bool get_visibility_only() const { return visibilityOnly; }
		void set_visibility_only(bool _only, Sprite* _partOf);

		ISpriteLayer* get_parent(Sprite* _partOf) const;
		ISpriteLayer* get_top_parent(Sprite* _partOf) const;
		bool is_parent_of(Sprite* _partOf, ISpriteLayer const* _of) const;
		
	public:
		static ISpriteLayer* create_layer_for(IO::XML::Node const* _node);
		static IO::XML::Node* create_node_for(IO::XML::Node * _parentNode, ISpriteLayer const * _layer);

	public:
		virtual bool load_from_xml(IO::XML::Node const* _node, LibraryLoadingContext& _lc);
		virtual bool prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext);
		virtual bool save_to_xml(IO::XML::Node* _node) const; // this method will save to an existing node

		virtual bool combine(Sprite* _for, SpriteLayer * _resultLayer, Array<RefCountObjectPtr<ISpriteLayer>> const& _allLayers, CombineSpritePixelsContext const & _context) const; // this is used to combine into one layer, will overwrite anything inside
		
		virtual void rescale(Optional<int> const& _moreSpritePixels, Optional<int> const& _lessSpritePixels); // only this layer!

		virtual VectorInt2 to_local(Sprite* _partOf, VectorInt2 const& _world, bool _ignoreMirrors = false) const;
		virtual VectorInt2 to_world(Sprite* _partOf, VectorInt2 const& _local, bool _ignoreMirrors = false) const;

	public:
		ISpriteLayer* create_copy() const;
		void fill_copy_basics(ISpriteLayer* _copy) const; // id, depth, visibility

	protected:
		virtual ISpriteLayer* create_for_copy() const = 0;
		virtual void fill_copy(ISpriteLayer* _copy) const;

	protected:
		bool temporary = false;
		Name id;
		int depth = 0;
		bool visible = true;
		bool visibilityOnly = false;
	};

	struct SpriteLayer
	: public ISpriteLayer
	{
		FAST_CAST_DECLARE(SpriteLayer);
		FAST_CAST_BASE(ISpriteLayer);
		FAST_CAST_END();

		typedef ISpriteLayer base;
	public:
		SpriteLayer();
		virtual ~SpriteLayer();

	public:
		RangeInt2 const& get_range() const { return range; }

		void reset();
		void clear();
		void prune();
		SpritePixel& access_at(VectorInt2 const& _at); // will make it larger if required
		SpritePixel const * get_at(VectorInt2 const& _at) const; // will make it larger if required

		void merge(SpriteLayer const* _layer); // merges _layer into this one

		void offset_placement(VectorInt2 const& _by); // just range change

		void fill_at(Sprite* _for, SpriteLayer const * _combinedResultLayer, VectorInt2 const& _worldAt, std::function<bool(SpritePixel& _pixel)> _doForSpritePixel); // _doForSpritePixel returns true if filled

	public:
		override_ bool load_from_xml(IO::XML::Node const* _node, LibraryLoadingContext& _lc);
		override_ bool save_to_xml(IO::XML::Node* _node) const; // this method will save to an existing node

		override_ bool combine(Sprite* _for, SpriteLayer* _resultLayer, Array<RefCountObjectPtr<ISpriteLayer>> const& _allLayers, CombineSpritePixelsContext const& _context) const; // this is used to combine into one layer, will overwrite anything inside

		override_ void rescale(Optional<int> const& _moreSpritePixels, Optional<int> const& _lessSpritePixels);

	protected:
		override_ ISpriteLayer* create_for_copy() const;
		override_ void fill_copy(ISpriteLayer* _copy) const;

	protected:
		RangeInt2 range = RangeInt2::empty;
		Array<SpritePixel> pixels; // from bottom left corner

		void copy_data_from(RangeInt2 const& _otherRange, Array<SpritePixel> const& _otherSpritePixels); // copies only intersecting part, if you want to include whole thing, make space first
		void combine_data_from(RangeInt2 const& _otherRange, Array<SpritePixel> const& _otherSpritePixels); // copies only non-none elements, only intersecting part as above
		void merge_data_from(RangeInt2 const& _otherRange, Array<SpritePixel> const& _otherSpritePixels); // copies only non-none elements and if there is empty space, only intersecting part as above

		friend class Sprite; 
		friend struct SpriteContentAccess;
		friend struct SpriteTransformLayer;
		friend struct SpriteFullTransformLayer;
		friend struct IncludeSpriteLayer;
	};

	struct SpriteTransformLayer
	: public ISpriteLayer
	{
		FAST_CAST_DECLARE(SpriteTransformLayer);
		FAST_CAST_BASE(ISpriteLayer);
		FAST_CAST_END();

		typedef ISpriteLayer base;
	public:
		struct Rotate
		{
			int yaw = 0;
			int& access_element(int i) { an_assert(i >= 1 && i <= 1); return yaw; }

			bool load_from_xml_child_node(IO::XML::Node const* _node, tchar const* _childName);
			bool save_to_xml_child_node(IO::XML::Node* _node, tchar const* _childName) const;

			void prepare_axes(OUT_ VectorInt2& xAxis, OUT_ VectorInt2& yAxis) const;
		};

		struct Mirror
		{
			int mirrorAxis = NONE; // 0, 1
			int at = 0;
			bool axis = false; // if axis, "at" will be kept as it is
			int dir = 0; // if 0 will just mirror, if 1 will mirror -1 to 1 (1 replaced with content of -1), if -1 will mirror 1 to -1
			bool load_from_xml_child_node(IO::XML::Node const* _node, tchar const* _childName);
			bool save_to_xml_child_node(IO::XML::Node* _node, tchar const* _childName) const;
		};

	public:
		SpriteTransformLayer();
		virtual ~SpriteTransformLayer();

		Rotate& access_rotate() { return rotate; }
		Mirror& access_mirror() { return mirror; }
		VectorInt2& access_offset() { return offset; }

	public:
		override_ bool load_from_xml(IO::XML::Node const* _node, LibraryLoadingContext& _lc);
		override_ bool save_to_xml(IO::XML::Node* _node) const; // this method will save to an existing node

		override_ bool combine(Sprite* _for, SpriteLayer* _resultLayer, Array<RefCountObjectPtr<ISpriteLayer>> const& _allLayers, CombineSpritePixelsContext const& _context) const; // this is used to combine into one layer, will overwrite anything inside

		override_ void rescale(Optional<int> const& _moreSpritePixels, Optional<int> const& _lessSpritePixels);

		override_ VectorInt2 to_local(Sprite* _partOf, VectorInt2 const& _world, bool _ignoreMirrors = false) const;
		override_ VectorInt2 to_world(Sprite* _partOf, VectorInt2 const& _local, bool _ignoreMirrors = false) const;

	protected:
		override_ ISpriteLayer* create_for_copy() const;
		override_ void fill_copy(ISpriteLayer* _copy) const;

	protected:
		// two layers that are used to get data from
		RefCountObjectPtr<SpriteLayer> tempALayer;
		RefCountObjectPtr<SpriteLayer> tempBLayer;
		
		// first rotate
		Rotate rotate;

		// then mirror
		Mirror mirror;

		// then offset
		VectorInt2 offset = VectorInt2::zero;

	};
	
	struct SpriteFullTransformLayer
	: public ISpriteLayer
	{
		FAST_CAST_DECLARE(SpriteFullTransformLayer);
		FAST_CAST_BASE(ISpriteLayer);
		FAST_CAST_END();

		typedef ISpriteLayer base;
	public:
		SpriteFullTransformLayer();
		virtual ~SpriteFullTransformLayer();

		Rotator3 & access_rotate() { return rotate; }
		Vector2& access_offset() { return offset; }

	public:
		override_ bool load_from_xml(IO::XML::Node const* _node, LibraryLoadingContext& _lc);
		override_ bool save_to_xml(IO::XML::Node* _node) const; // this method will save to an existing node

		override_ bool combine(Sprite* _for, SpriteLayer* _resultLayer, Array<RefCountObjectPtr<ISpriteLayer>> const& _allLayers, CombineSpritePixelsContext const& _context) const; // this is used to combine into one layer, will overwrite anything inside

		override_ void rescale(Optional<int> const& _moreSpritePixels, Optional<int> const& _lessSpritePixels);

		override_ VectorInt2 to_local(Sprite* _partOf, VectorInt2 const& _world, bool _ignoreMirrors = false) const;
		override_ VectorInt2 to_world(Sprite* _partOf, VectorInt2 const& _local, bool _ignoreMirrors = false) const;

	protected:
		override_ ISpriteLayer* create_for_copy() const;
		override_ void fill_copy(ISpriteLayer* _copy) const;

	protected:
		// two layers that are used to get data from
		RefCountObjectPtr<SpriteLayer> tempALayer;
		RefCountObjectPtr<SpriteLayer> tempBLayer;
		
		// first rotate
		Rotator3 rotate = Rotator3::zero;

		// then offset
		Vector2 offset = Vector2::zero;

	};

	struct IncludeSpriteLayer
	: public ISpriteLayer
	{
		FAST_CAST_DECLARE(IncludeSpriteLayer);
		FAST_CAST_BASE(ISpriteLayer);
		FAST_CAST_END();

		typedef ISpriteLayer base;
	public:
		IncludeSpriteLayer();
		explicit IncludeSpriteLayer(Sprite* vm);
		virtual ~IncludeSpriteLayer();

		Sprite* get_included() const { return included.get(); }
		void set_included(Sprite* vm) { included = vm; }

		SpriteLayer* get_included_layer() const { return includedLayer.get(); }

	public:
		override_ bool load_from_xml(IO::XML::Node const* _node, LibraryLoadingContext& _lc);
		override_ bool prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext);
		override_ bool save_to_xml(IO::XML::Node* _node) const; // this method will save to an existing node

		override_ bool combine(Sprite* _for, SpriteLayer* _resultLayer, Array<RefCountObjectPtr<ISpriteLayer>> const& _allLayers, CombineSpritePixelsContext const& _context) const; // this is used to combine into one layer, will overwrite anything inside

		// can't rescale!

	protected:
		override_ ISpriteLayer* create_for_copy() const;
		override_ void fill_copy(ISpriteLayer* _copy) const;

	protected:
		UsedLibraryStored<Sprite> included;

		RefCountObjectPtr<SpriteLayer> includedLayer; // copy of included
	};

	struct SpriteDrawingContext
	{
		VectorInt2 scale = VectorInt2::one; // can be mirrored
		Optional<VectorInt2> stretchTo; // overrides scale, if zero, takes original size
	};

	struct SpriteVertex
	{
		Vector3 location;
		Vector2 uv;

		SpriteVertex() {}
		SpriteVertex(Vector2 const& _uv, Vector3 const& _loc) : location(_loc), uv(_uv) {}
	};

	struct SpriteDrawingData
	{
		Texture* texture = nullptr;
		ArrayStatic<SpriteVertex, 4> vertices;
	};

	/**
	 *	Sprite is not an actual texture part
	 *	It is used to texture and texture parts
	 */
	class Sprite
	: public LibraryStored
	{
		FAST_CAST_DECLARE(Sprite);
		FAST_CAST_BASE(LibraryStored);
		FAST_CAST_END();
		LIBRARY_STORED_DECLARE_TYPE();

		typedef LibraryStored base;
	public:
		static void initialise_static();
		static void close_static();

		static ::System::VertexFormat* get_vertex_format() { return s_vertexFormat; }

		Sprite(Library * _library, LibraryName const & _name);
		virtual ~Sprite();

	public:
		void draw_at(::System::Video3D* _v3d, VectorInt2 const& _at, SpriteDrawingContext const& _context = SpriteDrawingContext()) const;
		void prepare_to_draw_at(OUT_ SpriteDrawingData& _data, VectorInt2 const& _at, SpriteDrawingContext const& _context = SpriteDrawingContext()) const;

		void draw_stretch_at(::System::Video3D* _v3d, Range2 const & _at, bool _maintainAspectRatio = false) const; // will stretch

		void draw(::System::Video3D* _v3d, SpriteDrawingData const& _data) const;

	public:
		Vector2 const& get_pixel_size() const { return pixelSize; }
		Vector2 const& get_zero_offset() const { return zeroOffset; }

		int get_sprite_grid_draw_priority() const { return spriteGridDrawPriority; }

		ColourPalette const* get_colour_palette() const { return colourPalette.get(); }

		RangeInt2 get_range() const;

		VectorInt2 to_pixel_coord(Vector2 _coord) const;
		Vector2 to_full_coord(VectorInt2 _coord, Optional<Vector2> const & _insideSpritePixel = NP) const; // half/centre
		RangeInt2 to_pixel_range(Range2 const& _range, bool _fullyInlcuded) const;
		Range2 to_full_range(RangeInt2 const& _range) const;

	public: // LibraryStored
#ifdef AN_DEVELOPMENT
		override_ void ready_for_reload();
#endif
		override_ void prepare_to_unload();

		override_ bool load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc);
		override_ bool prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext);

		override_ bool save_to_xml(IO::XML::Node* _node) const; // this method will save to an existing node

		override_ void debug_draw() const;

		override_ bool editor_render() const;

		override_ Editor::Base* create_editor(Editor::Base* _current) const;

	private:
		friend struct SpriteContentAccess;

	protected:
		static ::System::VertexFormat* s_vertexFormat;

		::System::ShaderProgramInstance* shaderProgramInstance;

		UsedLibraryStored<ColourPalette> colourPalette;
		
		struct TextureUsage
		{
			Name groupId; // if possible to group together
			UsedLibraryStored<Texture> texture; // generated by sprite-texture processor
			VectorInt2 size;
			VectorInt2 leftBottomOffset; // does not include zero offset
			Range2 uvRange; // bottom left -> top right
		} textureUsage;

		Vector2 pixelSize = Vector2::one;
		Vector2 zeroOffset = Vector2::zero; // per axis if 0.0, it is at the edge between pixel:-1 and pixel:0, if 0.5 it is at the centre of pixel:0

		int spriteGridDrawPriority = 0;

		RefCountObjectPtr<SpriteLayer> resultLayer; // if we combine all the layers, this is the result, the result is what we see

		int editedLayerIndex = NONE;

		Array<RefCountObjectPtr<ISpriteLayer>> layers; // top to bottom (children go after the parent)

		friend struct IncludeSpriteLayer;
		friend class SpriteTextureProcessor;
	};

	// accessor is a class to manipulate contents of sprite (and to make the sprite itself a much lighter thing
	struct SpriteContentAccess
	{
		static void set_pixel_size(Sprite* _this, Vector2 const& _pixelSize) { _this->pixelSize = _pixelSize; }
		static void set_zero_offset(Sprite* _this, Vector2 const& _zeroOffset) { _this->zeroOffset = _zeroOffset; }
		static void set_sprite_grid_draw_priority(Sprite* _this, int _spriteGridDrawPriority) { _this->spriteGridDrawPriority = _spriteGridDrawPriority; }
		static void set_colour_palette(Sprite* _this, ColourPalette const* _colourPalette, bool _convertToNewOne = false);
		static void remap_colours(Sprite* _this, Array<int> const& _remapColours);

		//

		static void rescale_more_pixels(Sprite* _this, int _scale); // for each existing pixel (one dimension) will scale by _scale
		static void rescale_less_pixels(Sprite* _this, int _scale); // will reduce number of pixels

		//

		static Array<RefCountObjectPtr<ISpriteLayer>> const& get_layers(Sprite const* _this) { return _this->layers; }
		static ISpriteLayer* access_layer(Sprite const* _this, int _idx) { return _this->layers.is_index_valid(_idx) ? _this->layers[_idx].get() : nullptr; }
		static ISpriteLayer* access_edited_layer(Sprite* _this);
		static int get_edited_layer_index(Sprite const* _this);
		static void set_edited_layer_index(Sprite * _this, int _index); // will change visibility only if pixel layer

		//

		static void move_edited_layer(Sprite* _this, int _by);
		static void change_depth_of_edited_layer(Sprite* _this, int _by);
		static void change_depth_of_edited_layer_and_its_children(Sprite* _this, int _by);
		static void change_depth_of_edited_layers_children(Sprite* _this, int _by);
		static void remove_edited_layer(Sprite* _this);
		static void add_layer(Sprite* _this, ISpriteLayer* _layer, int _offset = 1);
		static void duplicate_edited_layer(Sprite* _this, int _offset = 1);

		//

		static bool merge_temporary_layers(Sprite* _this, ISpriteLayer* _exceptLayer = nullptr);

		static void fill_at(Sprite* _this, SpriteLayer* _inLayer, VectorInt2 const& _worldAt, std::function<bool(SpritePixel& _pixel)> _doForSpritePixel); // _doForSpritePixel returns true if filled

		static void clear(Sprite* _this);

		static void prune(Sprite* _this);
		static void combine_pixels(Sprite* _this);
		static void combine_pixels(Sprite* _this, CombineSpritePixelsContext const& _context);

		static RangeInt2 get_combined_range(Sprite* _this);

		static bool merge_with_next(Sprite* _this, OUT_ String& _resultInfo);
		static bool merge_with_children(Sprite* _this, OUT_ String& _resultInfo);

		static void merge_all(Sprite* _this); // basically combines all and then replaces all layers with the result

		//

		static bool can_include(Sprite const* _this, Sprite const* _other);

		//

		static bool ray_cast(Sprite const* _this, Vector2 const& _start, OUT_ SpritePixelHitInfo& _hitInfo, bool _onlyActualContent, bool _ignoreZeroOffset = false);
		static bool ray_cast_xy(Sprite const* _this, Vector2 const& _start, OUT_ SpritePixelHitInfo& _hitInfo); // will return location at xy plane, even if won't hit anything

		//

		static void debug_draw_pixel(Sprite* _this, VectorInt2 const& _at, Colour const& _colour, bool _front = false);
		static void debug_draw_box_region(Sprite* _this, Vector2 const& _a, Vector2 const& _b, Colour const& _colour, bool _front = false, float _fillAlpha = 0.0f);

		//

		struct RenderContext
		{
			VectorInt2 pixelOffset = VectorInt2::zero;
			Optional<Plane> highlightPlane;
			Optional<Colour> wholeColour;
			Optional<Colour> editedLayerColour;
			Optional<Colour> pixelEdgeColour;
			bool onlySolidPixels = false;
			bool ignoreZeroOffset = false;
		};
		static void render(Sprite const * _this, RenderContext const& _context);

	};
};
