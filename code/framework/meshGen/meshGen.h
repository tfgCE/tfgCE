#pragma once

/**
 *			M E S H   G E N E R A T I O N
 *			-----------------------------
 *
 *
 *
 *	Building block of mesh generation is mesh generation element.
 *	It can be anything:
 *		- shape
 *			- basic: shape: cube, sphere, torus
 *			- complex/custom shape: servomotor, control panel, ventilation
 *			  this would be complex shape that is single mesh
 *		- set of elements: leg composed of leg elements + servomotors etc
 *		- modifiers: applying placement, applying skinning etc
 *
 *	All shapes have to be hard-coded. They may or may not require additional parameters.
 *	Parameters can be:
 *		Float
 *		Int
 *		Vector3
 *		Vector4
 *		Quat
 *		Matrix44
 *		(add more if required)
 *	Set of elements may instantiate shapes inside calculating parameters for them.
 *
 *	Parameters (by name) used when generating - those might be applied after ending each element!
 *		placement			Transform			placement of mesh
 *		scale				float/Vector3		scale of mesh (before placement is applied)
 *		materialIndex		int					material that should be applied to this mesh (will be stored in appropriate array to be combined into material slot)
 *		colour				Colour				vertices' colour
 *		u					float				vertices' u
 *		uv					Vector2				vertices' uv
 *		uvw					Vector3				vertices' uvW
 *		skinningBone		int					if vertices skinned to single bone
 *							Name				as above but will be solved through generation context - it's easier to give name than index
 *		skinningBones		VectorInt4			skinning vertices to more than one bone
 *		skinningWeights		Vector4				skinning vertices to more than one bone
 *
 *	(more parameters per shape/element)
 *
 *	Minimal plan for now
 *		(1) Parameters set by user 
 *		(1) Shapes using those parameters
 *		(2) Auto calculated parameters
 *		(3) Auto set parameters depending on some arbitrary quality (number of sides etc) - it might be using some global values
 *
 *
 */