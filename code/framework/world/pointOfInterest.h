#pragma once

#include "..\..\core\math\math.h"
#include "..\..\core\memory\refCountObject.h"
#include "..\..\core\other\customParameters.h"
#include "..\..\core\other\simpleVariableStorage.h"
#include "..\..\core\tags\tag.h"

namespace Framework
{
	struct LibraryLoadingContext;

	/**
	 *	Points of interest may have different automatic functions, check pointOfInterestInstance.cpp
	 */
	struct PointOfInterest
	: public RefCountObject
	{
		Name name;
		Tags tags;
		Transform offset;
		Name socketName;
		Array<ConvexHull> volumes; // note that volumes are WITHOUT offset
		SimpleVariableStorage parameters; // if copying, will use parameters that are available at mesh generation
		SimpleVariableStorage parametersFromOwner; // if possible will try to get those parameters from owner and replace existing ones, this will make sure they are always taken from the owner
		// special cases for parameters:
		//	delayObjectCreation is loaded as a parameter into parameters

#ifdef AN_DEVELOPMENT
		bool debugThis = false;
#endif

		PointOfInterest();

		bool load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc);
		bool load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc, tchar const * _parametersNodeName, tchar const * _parametersFromOwnerNodeName);

		void apply(Transform const & _transform);
		void apply(Matrix44 const & _transform);
		PointOfInterest* create_copy() const;
	};

	typedef RefCountObjectPtr<PointOfInterest> PointOfInterestPtr;
};
