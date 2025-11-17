#include "base64.h"

#include "..\types\string.h"

tchar const * base64Chars = TXT("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/");

String IO::Base64::encode(void const* _content, int _length)
{
	String encoded;
	encoded.access_data().make_space_for(_length * 4 / 3 + 10);

	String suffix;

	int idx = 0;
	int8 const* _content8bit = plain_cast<int8>(_content);

	int charToEncodeIdx = 0;
	unsigned char charsToEncode[3];
	unsigned char encodedChars[4];

	while (idx < _length || charToEncodeIdx != 0)
	{
		if (idx < _length)
		{
			charsToEncode[charToEncodeIdx] = *_content8bit;
			++charToEncodeIdx;
			++_content8bit;
			++idx;
		}
		else
		{
			// fill up with zeros
			// add info about zeroes, by adding = to the end
			charsToEncode[charToEncodeIdx] = 0;
			++charToEncodeIdx;
			suffix += TXT("=");
		}
		if (charToEncodeIdx == 3)
		{
			encodedChars[0] = (charsToEncode[0] & 0xfc) >> 2;
			encodedChars[1] = ((charsToEncode[0] & 0x03) << 4) + ((charsToEncode[1] & 0xf0) >> 4);
			encodedChars[2] = ((charsToEncode[1] & 0x0f) << 2) + ((charsToEncode[2] & 0xc0) >> 6);
			encodedChars[3] = charsToEncode[2] & 0x3f;

			for_count(int, i, 4)
			{
				encoded += base64Chars[encodedChars[i]];
			}
			charToEncodeIdx = 0;
		}
	}

	encoded += suffix;

	return encoded;
}
