#pragma once

inline bool does_need_endian_adjust(uint const _numBytes)
{
#ifdef AN_LITTLE_ENDIAN
	{
		return false;
	}
#elif AN_BIG_ENDIAN
	{
		return _numBytes > 1 ? true : false;
	}
#else
#error Choose between little endian and big endian
#endif
}

inline void adjust_endian(void * const _data, uint const _numBytes)
{
#ifdef AN_LITTLE_ENDIAN
	{
		// it's ok
	}
#elif AN_BIG_ENDIAN
	{
		byte * fwd = reinterpret_cast<byte*>(_data);
		byte * bck = &(reinterpret_cast<byte*>(_data))[_numBytes - 1];
		for (int i = ((_numBytes + 1) >> 1); i > 0; --i, ++fwd, --bck)
		{
			byte temp = *fwd;
			*fwd = *bck;
			*bck = temp;
		}
	}
#else
#error Choose between little endian and big endian
#endif
}

inline void adjust_endian_series(void * const _data, uint const _numBytesSingle, uint const _numBytes)
{
#ifdef AN_LITTLE_ENDIAN
	{
		// it's ok
	}
#elif AN_BIG_ENDIAN
	{
		byte * start = reinterpret_cast<byte*>(_data);
		uint numBytesLeft = _numBytes;
		while (numBytesLeft > 0)
		{
			uint thisSize = min(_numBytesSingle, numBytesLeft);
			adjust_endian(start, thisSize);
			start +=_numBytesSingle;
			numBytesLeft -= thisSize;
		}
	}
#else
#error Choose between little endian and big endian
#endif
}

inline void adjust_endian_stride(void * const _data, uint const _numBytesSingle, uint const _strideCount, uint const _strideSize)
{
#ifdef AN_LITTLE_ENDIAN
	{
		// it's ok
	}
#elif AN_BIG_ENDIAN
	{
		byte * start = reinterpret_cast<byte*>(_data);
		uint stridesLeft = _strideCount;
		while (stridesLeft)
		{
			adjust_endian(start, _numBytesSingle);
			start += _strideSize;
			-- stridesLeft;
		}
	}
#else
#error Choose between little endian and big endian
#endif
}
