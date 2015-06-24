
static DWORD cxdec_decode_first_stage(DWORD *pseed, DWORD hash)
{
	DWORD ret;

	switch (seed_rand(pseed) % 3) {
	case 0:
		ret = *(u32 *)&EncryptionControlBlock[(seed_rand(pseed) & 0x3ff) << 2];
		break;
	case 1:
		ret = seed_rand(pseed);
		break;
	case 2:
		ret = hash;
		break;
	}

	return ret;
}

static DWORD seed_rand(DWORD *pseed)
{
	DWORD seed = *pseed;
	*pseed = 1103515245 * seed + 12345;
	return *pseed ^ (seed << 16) ^ (seed >> 16);
}

static DWORD cxdec_decode_stage0(DWORD *pseed, int stage, DWORD hash);
static DWORD cxdec_decode_stage1(DWORD *pseed, int stage, DWORD hash);

static DWORD cxdec_decode_stage0(DWORD *pseed, int stage, DWORD hash)
{
	DWORD ret;

	if (stage == 1)
		return cxdec_decode_first_stage(pseed, hash);

	if (seed_rand(pseed) & 1)
		ret = cxdec_decode_stage1(pseed, stage - 1, hash);
	else
		ret = cxdec_decode_stage0(pseed, stage - 1, hash);

//	DWORD seed = *pseed;
//	*pseed = 1103515245 * seed + 12345;
//	switch (((109 * seed + 57) ^ (seed >> 16)) & 7) {
	switch (seed_rand(pseed) & 7) {
	case 0:
		ret = ~ret;
		break;
	case 1:
		ret--;
		break;
	case 2:
		ret ^= seed_rand(pseed);
		break;
	case 3:
		if (seed_rand(pseed) & 1)
			ret += seed_rand(pseed);
		else
	        ret -= seed_rand(pseed);
		break;
	case 4:
		ret++;
		break;
	case 5:
		ret = 0 - ret;
		break;
	case 6:
		ret = ((ret & 0x55555555) << 1) | ((ret & 0xaaaaaaaa) >> 1);
		break;
	case 7:
		ret = *(u32 *)&EncryptionControlBlock[(ret & 0x3ff) << 2];
		break;
	}
	return ret;
}

static DWORD cxdec_decode_stage1(DWORD *pseed, int stage, DWORD hash)
{
	DWORD ret, tmp;

	if (stage == 1)
		return cxdec_decode_first_stage(pseed, hash);

	if (seed_rand(pseed) & 1)
		ret = cxdec_decode_stage1(pseed, stage - 1, hash);
	else
		ret = cxdec_decode_stage0(pseed, stage - 1, hash);

	tmp = ret;

	if (seed_rand(pseed) & 1)
		ret = cxdec_decode_stage1(pseed, stage - 1, hash);
	else
		ret = cxdec_decode_stage0(pseed, stage - 1, hash);

	switch (seed_rand(pseed) % 6) {
	case 0:
		ret += tmp;
		break;
   	case 1:
		ret -= tmp;
		break;
   	case 2:
		ret >>= (tmp & 0x0f);
		break;
   	case 3:
		ret *= tmp;
		break;
    case 4:
		ret = tmp - ret;
		break;
    case 5:
		ret <<= (tmp & 0x0f);
	}
	return ret;
}

static void cxdec_execute_decode(struct cxdec_callback *callback, DWORD hash, DWORD *ret1, DWORD *ret2)
{
	DWORD seed = hash & 0x7f;
	hash >>= 7;
	*ret1 = cxdec_decode_stage1(&seed, 5, hash);
	*ret2 = cxdec_decode_stage1(&seed, 5, hash);
}
