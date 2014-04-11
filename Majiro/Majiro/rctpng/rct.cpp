#include "rct.h"

DWORD RCT::rc8_decompress(BYTE *uncompr, DWORD uncomprLen, BYTE *compr, DWORD comprLen, DWORD width)
{
	DWORD act_uncomprLen = 0;
	DWORD curByte = 0;
	DWORD pos[16];

	pos[0] = -1;
	pos[1] = -2;
	pos[2] = -3;
	pos[3] = -4;
	pos[4] = 3 - width;
	pos[5] = 2 - width;
	pos[6] = 1 - width;
	pos[7] = 0 - width;
	pos[8] = -1 - width;
	pos[9] = -2 - width;
	pos[10] = -3 - width;
	pos[11] = 2 - (width * 2);
	pos[12] = 1 - (width * 2);
	pos[13] = (0 - width) << 1;
	pos[14] = -1 - (width * 2);
	pos[15] = (-1 - width) * 2;

	uncompr[act_uncomprLen++] = compr[curByte++];

	while (1) {
		BYTE flag;
		DWORD copy_bytes, copy_pos;

		if (curByte >= comprLen)
			break;

		flag = compr[curByte++];

		if (!(flag & 0x80))
		{
			if (flag != 0x7f)
				copy_bytes = flag + 1;
			else
			{
				if (curByte + 1 >= comprLen)
					break;

				copy_bytes = compr[curByte++];
				copy_bytes |= compr[curByte++] << 8;
				copy_bytes += 0x80;
			}

			if (curByte + copy_bytes - 1 >= comprLen)
				break;
			if (act_uncomprLen + copy_bytes - 1 >= uncomprLen)
				break;

			memcpy(&uncompr[act_uncomprLen], &compr[curByte], copy_bytes);
			act_uncomprLen += copy_bytes;
			curByte += copy_bytes;
		}
		else
		{
			copy_bytes = flag & 7;
			copy_pos = (flag >> 3) & 0xf;

			if (copy_bytes != 7)
				copy_bytes += 3;
			else {
				if (curByte + 1 >= comprLen)
					break;

				copy_bytes = compr[curByte++];
				copy_bytes |= compr[curByte++] << 8;
				copy_bytes += 0xa;
			}

			for (unsigned int i = 0; i < copy_bytes; i++) {
				if (act_uncomprLen >= uncomprLen)
					break;
				uncompr[act_uncomprLen] = uncompr[act_uncomprLen + pos[copy_pos]];
				act_uncomprLen++;
			}
		}
	}

	//	if (curByte != comprLen)
	//		fprintf(stderr, "compr miss-match %d VS %d\n", curByte, comprLen);

	return act_uncomprLen;
}

DWORD RCT::rct_decompress(BYTE *uncompr, DWORD uncomprLen, BYTE *compr, DWORD comprLen, DWORD width)
{
	DWORD act_uncomprLen = 0;
	DWORD curByte = 0;
	DWORD pos[32];

	pos[0] = -3;
	pos[1] = -6;
	pos[2] = -9;
	pos[3] = -12;
	pos[4] = -15;
	pos[5] = -18;
	pos[6] = (3 - width) * 3;
	pos[7] = (2 - width) * 3;
	pos[8] = (1 - width) * 3;
	pos[9] = (0 - width) * 3;
	pos[10] = (-1 - width) * 3;
	pos[11] = (-2 - width) * 3;
	pos[12] = (-3 - width) * 3;
	pos[13] = 9 - ((width * 3) << 1);
	pos[14] = 6 - ((width * 3) << 1);
	pos[15] = 3 - ((width * 3) << 1);
	pos[16] = 0 - ((width * 3) << 1);
	pos[17] = -3 - ((width * 3) << 1);
	pos[18] = -6 - ((width * 3) << 1);
	pos[19] = -9 - ((width * 3) << 1);
	pos[20] = 9 - width * 9;
	pos[21] = 6 - width * 9;
	pos[22] = 3 - width * 9;
	pos[23] = 0 - width * 9;
	pos[24] = -3 - width * 9;
	pos[25] = -6 - width * 9;
	pos[26] = -9 - width * 9;
	pos[27] = 6 - ((width * 3) << 2);
	pos[28] = 3 - ((width * 3) << 2);
	pos[29] = 0 - ((width * 3) << 2);
	pos[30] = -3 - ((width * 3) << 2);
	pos[31] = -6 - ((width * 3) << 2);

	uncompr[act_uncomprLen++] = compr[curByte++];
	uncompr[act_uncomprLen++] = compr[curByte++];
	uncompr[act_uncomprLen++] = compr[curByte++];

	while (1) {
		BYTE flag;
		DWORD copy_bytes, copy_pos;

		if (curByte >= comprLen)
			break;

		flag = compr[curByte++];

		if (!(flag & 0x80)) {
			if (flag != 0x7f)
				copy_bytes = flag * 3 + 3;
			else {
				if (curByte + 1 >= comprLen)
					break;

				copy_bytes = compr[curByte++];
				copy_bytes |= compr[curByte++] << 8;
				copy_bytes += 0x80;
				copy_bytes *= 3;
			}

			if (curByte + copy_bytes - 1 >= comprLen)
				break;
			if (act_uncomprLen + copy_bytes - 1 >= uncomprLen)
				break;

			memcpy(&uncompr[act_uncomprLen], &compr[curByte], copy_bytes);
			act_uncomprLen += copy_bytes;
			curByte += copy_bytes;
		}
		else {
			copy_bytes = flag & 3;
			copy_pos = (flag >> 2) & 0x1f;

			if (copy_bytes != 3) {
				copy_bytes = copy_bytes * 3 + 3;
			}
			else {
				if (curByte + 1 >= comprLen)
					break;

				copy_bytes = compr[curByte++];
				copy_bytes |= compr[curByte++] << 8;
				copy_bytes += 4;
				copy_bytes *= 3;
			}

			for (unsigned int i = 0; i < copy_bytes; i++) {
				if (act_uncomprLen >= uncomprLen)
					goto out;
				uncompr[act_uncomprLen] = uncompr[act_uncomprLen + pos[copy_pos]];
				act_uncomprLen++;
			}
		}
	}
out:
	//	if (curByte != comprLen)
	//		fprintf(stderr, "compr miss-match %d VS %d\n", curByte, comprLen);

	return act_uncomprLen;
}

