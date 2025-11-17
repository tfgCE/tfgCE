#pragma once

#include "..\library\libraryStored.h"
#include "..\..\core\memory\refCountObject.h"

namespace Framework
{
	class ColourPalette;
	class VoxelMold;
	struct VoxelMoldContentAccess;
	struct VoxelMoldLayer;
	struct VoxelMoldTransformLayer;
	struct VoxelMoldFullTransformLayer;
	struct IncludeVoxelMoldLayer;

	struct Voxel
	{
		enum Content
		{
			None = -1,
			Invisible = -2, // voxel is invisible but is considered full for creating sides
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

		int voxel = Voxel::None; // NONE - empty, otherwise it is a colour from the palette

		bool operator == (Voxel const& _other) const
		{
			return voxel == _other.voxel;
		}

		static bool can_be_replaced_in_merge(int v) { return v == Voxel::None || v == Voxel::Invisible; }
		static bool is_anything(int v) { return v != Voxel::None; }
		static bool is_solid(int v) { return v >= 0; }
		static bool makes_side(int v) { return is_solid(v) || v == Voxel::Invisible; }
		static bool is_drawable(int v) { return v != Voxel::None; }
	};

	struct VoxelHitInfo
	{
		Optional<VectorInt3> hit;
		Optional<VectorInt3> normal; // will work as neighbouring piece
	};

	struct CombineVoxelsContext
	{

	};

	/**
	 *	Voxel Mold is composed of multiple layers
	 *	Layers are stored in a single list with "depth" parameter that tells the order or belonging
	 */
	struct IVoxelMoldLayer
	: public RefCountObject
	{
		FAST_CAST_DECLARE(IVoxelMoldLayer);
		FAST_CAST_END();

	public:
		static Optional<int> get_child_depth(IVoxelMoldLayer const* _for, Array<RefCountObjectPtr<IVoxelMoldLayer>> const& _layers); // "not set" if no children
		static bool for_every_child_combining(IVoxelMoldLayer const * _of, Array<RefCountObjectPtr<IVoxelMoldLayer>> const& _layers, std::function<bool(IVoxelMoldLayer* layer)> _do);

	public:
		IVoxelMoldLayer();
		virtual ~IVoxelMoldLayer();

	public:
		bool is_temporary() const { return temporary; }
		void be_temporary(bool _temporary = true) { temporary = _temporary; }

		Name const& get_id() const { return id; }
		void set_id(Name const& _id) { id = _id; }
		// changing of any of below requires explicit call of combine_voxels
		int get_depth() const { return depth; }
		void set_depth(int _depth) { depth = max(0, _depth); }
		bool is_visible(VoxelMold* _partOf) const;
		bool get_is_visible() const { return visible; }
		void set_visible(bool _visible) { visible = _visible; }
		bool get_visibility_only() const { return visibilityOnly; }
		void set_visibility_only(bool _only, VoxelMold* _partOf);

		IVoxelMoldLayer* get_parent(VoxelMold* _partOf) const;
		IVoxelMoldLayer* get_top_parent(VoxelMold* _partOf) const;
		bool is_parent_of(VoxelMold* _partOf, IVoxelMoldLayer const* _of) const;
		
	public:
		static IVoxelMoldLayer* create_layer_for(IO::XML::Node const* _node);
		static IO::XML::Node* create_node_for(IO::XML::Node * _parentNode, IVoxelMoldLayer const * _layer);

	public:
		virtual bool load_from_xml(IO::XML::Node const* _node, LibraryLoadingContext& _lc);
		virtual bool prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext);
		virtual bool save_to_xml(IO::XML::Node* _node) const; // this method will save to an existing node

		virtual bool combine(VoxelMold* _for, VoxelMoldLayer * _resultLayer, Array<RefCountObjectPtr<IVoxelMoldLayer>> const& _allLayers, CombineVoxelsContext const & _context) const; // this is used to combine into one layer, will overwrite anything inside
		
		virtual void rescale(Optional<int> const& _moreVoxels, Optional<int> const& _lessVoxels); // only this layer!

		virtual VectorInt3 to_local(VoxelMold* _partOf, VectorInt3 const& _world, bool _ignoreMirrors = false) const;
		virtual VectorInt3 to_world(VoxelMold* _partOf, VectorInt3 const& _local, bool _ignoreMirrors = false) const;

	public:
		IVoxelMoldLayer* create_copy() const;
		void fill_copy_basics(IVoxelMoldLayer* _copy) const; // id, depth, visibility

	protected:
		virtual IVoxelMoldLayer* create_for_copy() const = 0;
		virtual void fill_copy(IVoxelMoldLayer* _copy) const;

	protected:
		bool temporary = false;
		Name id;
		int depth = 0;
		bool visible = true;
		bool visibilityOnly = false;
	};

	struct VoxelMoldLayer
	: public IVoxelMoldLayer
	{
		FAST_CAST_DECLARE(VoxelMoldLayer);
		FAST_CAST_BASE(IVoxelMoldLayer);
		FAST_CAST_END();

		typedef IVoxelMoldLayer base;
	public:
		VoxelMoldLayer();
		virtual ~VoxelMoldLayer();

	public:
		RangeInt3 const& get_range() const { return range; }

		void reset();
		void clear();
		void prune();
		Voxel& access_at(VectorInt3 const& _at); // will make it larger if required
		Voxel const * get_at(VectorInt3 const& _at) const; // will make it larger if required

		void merge(VoxelMoldLayer const* _layer); // merges _layer into this one

		void offset_placement(VectorInt3 const& _by); // just range change

		void fill_at(VoxelMold* _for, VoxelMoldLayer const * _combinedResultLayer, VectorInt3 const& _worldAt, std::function<bool(Voxel& _voxel)> _doForVoxel); // _doForVoxel returns true if filled

	public:
		override_ bool load_from_xml(IO::XML::Node const* _node, LibraryLoadingContext& _lc);
		override_ bool save_to_xml(IO::XML::Node* _node) const; // this method will save to an existing node

		override_ bool combine(VoxelMold* _for, VoxelMoldLayer* _resultLayer, Array<RefCountObjectPtr<IVoxelMoldLayer>> const& _allLayers, CombineVoxelsContext const& _context) const; // this is used to combine into one layer, will overwrite anything inside

		override_ void rescale(Optional<int> const& _moreVoxels, Optional<int> const& _lessVoxels);

	protected:
		override_ IVoxelMoldLayer* create_for_copy() const;
		override_ void fill_copy(IVoxelMoldLayer* _copy) const;

	protected:
		RangeInt3 range = RangeInt3::empty;
		Array<Voxel> voxels;

		void copy_data_from(RangeInt3 const& _otherRange, Array<Voxel> const& _otherVoxels); // copies only intersecting part, if you want to include whole thing, make space first
		void combine_data_from(RangeInt3 const& _otherRange, Array<Voxel> const& _otherVoxels); // copies only non-none elements, only intersecting part as above
		void merge_data_from(RangeInt3 const& _otherRange, Array<Voxel> const& _otherVoxels); // copies only non-none elements and if there is empty space, only intersecting part as above

		friend class VoxelMold;
		friend struct VoxelMoldContentAccess;
		friend struct VoxelMoldTransformLayer;
		friend struct VoxelMoldFullTransformLayer;
		friend struct IncludeVoxelMoldLayer;
	};

	struct VoxelMoldTransformLayer
	: public IVoxelMoldLayer
	{
		FAST_CAST_DECLARE(VoxelMoldTransformLayer);
		FAST_CAST_BASE(IVoxelMoldLayer);
		FAST_CAST_END();

		typedef IVoxelMoldLayer base;
	public:
		struct Rotate
		{
			int pitch = 0;
			int yaw = 0;
			int roll = 0;
			int& access_element(int i) { an_assert(i >= 0 && i <= 2); return i == 0 ? pitch : (i == 1 ? yaw : roll); }

			bool load_from_xml_child_node(IO::XML::Node const* _node, tchar const* _childName);
			bool save_to_xml_child_node(IO::XML::Node* _node, tchar const* _childName) const;

			void prepare_axes(OUT_ VectorInt3& xAxis, OUT_ VectorInt3& yAxis, OUT_ VectorInt3& zAxis) const;
		};

		struct Mirror
		{
			int mirrorAxis = NONE; // 0, 1, 2
			int at = 0;
			bool axis = false; // if axis, "at" will be kept as it is
			int dir = 0; // if 0 will just mirror, if 1 will mirror -1 to 1 (1 replaced with content of -1), if -1 will mirror 1 to -1
			bool load_from_xml_child_node(IO::XML::Node const* _node, tchar const* _childName);
			bool save_to_xml_child_node(IO::XML::Node* _node, tchar const* _childName) const;
		};

	public:
		VoxelMoldTransformLayer();
		virtual ~VoxelMoldTransformLayer();

		Rotate& access_rotate() { return rotate; }
		Mirror& access_mirror() { return mirror; }
		VectorInt3& access_offset() { return offset; }

	public:
		override_ bool load_from_xml(IO::XML::Node const* _node, LibraryLoadingContext& _lc);
		override_ bool save_to_xml(IO::XML::Node* _node) const; // this method will save to an existing node

		override_ bool combine(VoxelMold* _for, VoxelMoldLayer* _resultLayer, Array<RefCountObjectPtr<IVoxelMoldLayer>> const& _allLayers, CombineVoxelsContext const& _context) const; // this is used to combine into one layer, will overwrite anything inside

		override_ void rescale(Optional<int> const& _moreVoxels, Optional<int> const& _lessVoxels);

		override_ VectorInt3 to_local(VoxelMold* _partOf, VectorInt3 const& _world, bool _ignoreMirrors = false) const;
		override_ VectorInt3 to_world(VoxelMold* _partOf, VectorInt3 const& _local, bool _ignoreMirrors = false) const;

	protected:
		override_ IVoxelMoldLayer* create_for_copy() const;
		override_ void fill_copy(IVoxelMoldLayer* _copy) const;

	protected:
		// two layers that are used to get data from
		RefCountObjectPtr<VoxelMoldLayer> tempALayer;
		RefCountObjectPtr<VoxelMoldLayer> tempBLayer;
		
		// first rotate
		Rotate rotate;

		// then mirror
		Mirror mirror;

		// then offset
		VectorInt3 offset = VectorInt3::zero;

	};
	
	struct VoxelMoldFullTransformLayer
	: public IVoxelMoldLayer
	{
		FAST_CAST_DECLARE(VoxelMoldFullTransformLayer);
		FAST_CAST_BASE(IVoxelMoldLayer);
		FAST_CAST_END();

		typedef IVoxelMoldLayer base;
	public:
		VoxelMoldFullTransformLayer();
		virtual ~VoxelMoldFullTransformLayer();

		Rotator3 & access_rotate() { return rotate; }
		Vector3& access_offset() { return offset; }

	public:
		override_ bool load_from_xml(IO::XML::Node const* _node, LibraryLoadingContext& _lc);
		override_ bool save_to_xml(IO::XML::Node* _node) const; // this method will save to an existing node

		override_ bool combine(VoxelMold* _for, VoxelMoldLayer* _resultLayer, Array<RefCountObjectPtr<IVoxelMoldLayer>> const& _allLayers, CombineVoxelsContext const& _context) const; // this is used to combine into one layer, will overwrite anything inside

		override_ void rescale(Optional<int> const& _moreVoxels, Optional<int> const& _lessVoxels);

		override_ VectorInt3 to_local(VoxelMold* _partOf, VectorInt3 const& _world, bool _ignoreMirrors = false) const;
		override_ VectorInt3 to_world(VoxelMold* _partOf, VectorInt3 const& _local, bool _ignoreMirrors = false) const;

	protected:
		override_ IVoxelMoldLayer* create_for_copy() const;
		override_ void fill_copy(IVoxelMoldLayer* _copy) const;

	protected:
		// two layers that are used to get data from
		RefCountObjectPtr<VoxelMoldLayer> tempALayer;
		RefCountObjectPtr<VoxelMoldLayer> tempBLayer;
		
		// first rotate
		Rotator3 rotate = Rotator3::zero;

		// then offset
		Vector3 offset = Vector3::zero;

	};

	struct IncludeVoxelMoldLayer
	: public IVoxelMoldLayer
	{
		FAST_CAST_DECLARE(IncludeVoxelMoldLayer);
		FAST_CAST_BASE(IVoxelMoldLayer);
		FAST_CAST_END();

		typedef IVoxelMoldLayer base;
	public:
		IncludeVoxelMoldLayer();
		virtual ~IncludeVoxelMoldLayer();

		VoxelMold* get_included() const { return included.get(); }
		void set_included(VoxelMold* vm) { included = vm; }

		VoxelMoldLayer* get_included_layer() const { return includedLayer.get(); }

	public:
		override_ bool load_from_xml(IO::XML::Node const* _node, LibraryLoadingContext& _lc);
		override_ bool prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext);
		override_ bool save_to_xml(IO::XML::Node* _node) const; // this method will save to an existing node

		override_ bool combine(VoxelMold* _for, VoxelMoldLayer* _resultLayer, Array<RefCountObjectPtr<IVoxelMoldLayer>> const& _allLayers, CombineVoxelsContext const& _context) const; // this is used to combine into one layer, will overwrite anything inside

		// can't rescale!

	protected:
		override_ IVoxelMoldLayer* create_for_copy() const;
		override_ void fill_copy(IVoxelMoldLayer* _copy) const;

	protected:
		UsedLibraryStored<VoxelMold> included;

		RefCountObjectPtr<VoxelMoldLayer> includedLayer; // copy of included
	};

	/**
	 *	Voxel Mold is not an actual mesh
	 *	It is used to create meshes and textures
	 */
	class VoxelMold
	: public LibraryStored
	{
		FAST_CAST_DECLARE(VoxelMold);
		FAST_CAST_BASE(LibraryStored);
		FAST_CAST_END();
		LIBRARY_STORED_DECLARE_TYPE();

		typedef LibraryStored base;
	public:
		VoxelMold(Library * _library, LibraryName const & _name);
		virtual ~VoxelMold();

	public:
		float get_voxel_size() const { return voxelSize; }
		Vector3 const& get_zero_offset() const { return zeroOffset; }

		ColourPalette const* get_colour_palette() const { return colourPalette.get(); }

		RangeInt3 get_range() const;

		VectorInt3 to_voxel_coord(Vector3 _coord) const;
		Vector3 to_full_coord(VectorInt3 _coord, Optional<Vector3> const & _insideVoxel = NP) const; // half/centre
		RangeInt3 to_voxel_range(Range3 const& _range, bool _fullyInlcuded) const;
		Range3 to_full_range(RangeInt3 const& _range) const;

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

	public:
		void debug_draw_voxel(VectorInt3 const& _at, Colour const& _colour, bool _front = false);
		void debug_draw_box_region(Vector3 const& _a, Vector3 const & _b, Colour const& _colour, bool _front = false, float _fillAlpha = 0.0f);

		struct RenderContext
		{
			Vector3 lightDir = Vector3(-0.2f, -0.7f, 2.0f).normal();
			float useLight = 0.2f;
			Optional<Plane> highlightPlane;
			Optional<Colour> editedLayerColour;
			Optional<Colour> voxelEdgeColour;
			bool onlySolidVoxels = false;
		};
		void render(RenderContext const & _context) const;

	protected:
		UsedLibraryStored<ColourPalette> colourPalette;

		float voxelSize = 0.01f;
		Vector3 zeroOffset = Vector3::zero; // per axis if 0.0, it is at the edge between voxel:-1 and voxel:0, if 0.5 it is at the centre of voxel:0

		RefCountObjectPtr<VoxelMoldLayer> resultLayer; // if we combine all the layers, this is the result, the result is what we see

		int editedLayerIndex = NONE;

		Array<RefCountObjectPtr<IVoxelMoldLayer>> layers; // top to bottom (children go after the parent)

		friend struct IncludeVoxelMoldLayer;

		friend struct VoxelMoldContentAccess;
	};

	struct VoxelMoldContentAccess
	{
		static void set_voxel_size(VoxelMold * _this, float _voxelSize) { _this->voxelSize = _voxelSize; }
		static void set_zero_offset(VoxelMold* _this, Vector3 const& _zeroOffset) { _this->zeroOffset = _zeroOffset; }
		static void set_colour_palette(VoxelMold* _this, ColourPalette const* _colourPalette, bool _convertToNewOne = false);

		//

		static void rescale_more_voxels(VoxelMold * _this, int _scale); // for each existing voxel (one dimension) will scale by _scale
		static void rescale_less_voxels(VoxelMold * _this, int _scale); // will reduce number of voxels

		//

		static Array<RefCountObjectPtr<IVoxelMoldLayer>> const& get_layers(VoxelMold const * _this) { return _this->layers; }
		static IVoxelMoldLayer* access_layer(VoxelMold const* _this, int _idx) { return _this->layers.is_index_valid(_idx) ? _this->layers[_idx].get() : nullptr; }
		static IVoxelMoldLayer* access_edited_layer(VoxelMold* _this);
		static int get_edited_layer_index(VoxelMold const * _this);
		static void set_edited_layer_index(VoxelMold* _this, int _index); // will change visibility only if voxel layer

		//

		static void move_edited_layer(VoxelMold* _this, int _by);
		static void change_depth_of_edited_layer(VoxelMold* _this, int _by);
		static void change_depth_of_edited_layer_and_its_children(VoxelMold* _this, int _by);
		static void change_depth_of_edited_layers_children(VoxelMold* _this, int _by);
		static void remove_edited_layer(VoxelMold* _this);
		static void add_layer(VoxelMold* _this, IVoxelMoldLayer* _layer, int _offset = 1);
		static void duplicate_edited_layer(VoxelMold* _this, int _offset = 1);

		static bool merge_temporary_layers(VoxelMold* _this, IVoxelMoldLayer* _exceptLayer = nullptr);

		//

		static void fill_at(VoxelMold* _this, VoxelMoldLayer* _inLayer, VectorInt3 const& _worldAt, std::function<bool(Voxel& _voxel)> _doForVoxel); // _doForVoxel returns true if filled

		//

		static void prune(VoxelMold* _this);
		static void combine_voxels(VoxelMold* _this);
		static void combine_voxels(VoxelMold* _this, CombineVoxelsContext const& _context);

		static RangeInt3 get_combined_range(VoxelMold* _this);

		static bool merge_with_next(VoxelMold* _this, OUT_ String& _resultInfo);
		static bool merge_with_children(VoxelMold* _this, OUT_ String& _resultInfo);

		//

		static bool can_include(VoxelMold const * _this, VoxelMold const* _other);

		//

		static bool ray_cast(VoxelMold const * _this, Vector3 const& _start, Vector3 const& _dir, OUT_ VoxelHitInfo& _hitInfo, bool _onlyActualContent);
		static bool ray_cast_xy(VoxelMold const* _this, Vector3 const& _start, Vector3 const& _dir, OUT_ VoxelHitInfo& _hitInfo); // will return location at xy plane, even if won't hit anything
		static bool ray_cast_plane(VoxelMold const* _this, Vector3 const& _start, Vector3 const& _dir, OUT_ VoxelHitInfo& _hitInfo, int _axisPlane, float _planeT); // as above but for any plane
	};
};
