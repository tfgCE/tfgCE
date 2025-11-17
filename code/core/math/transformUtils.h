#pragma once

#include "math.h"

Transform offset_transform_by_forward_angle(Transform const & _placement, REF_ Random::Generator& _rg, float _angle); // precise angle
Transform offset_transform_by_forward_angle(Transform const & _placement, REF_ Random::Generator& _rg, Range const & _angleRange); // it will rotate by random max angle, keeps up dir same
