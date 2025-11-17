#ifdef AN_FMOD
inline FMOD_VECTOR System::SoundUtils::convert(Vector3 const& _vector)
{
	FMOD_VECTOR ret;
	ret.x = _vector.x; ret.y = _vector.y; ret.z = _vector.z; return ret; } // *plain_cast<FMOD_VECTOR>(&_vector);
#endif
