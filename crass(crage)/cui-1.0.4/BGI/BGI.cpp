#include <windows.h>
#include <tchar.h>
#include <crass_types.h>
#include <acui.h>
#include <cui.h>
#include <resource.h>
#include <package.h>
#include <cui_error.h>
#include <utility.h>
#include <stdio.h>

struct acui_information BGI_cui_information = {
	NULL,								/* copyright */
	_T("Ethornell - Buriko General Interpreter"),	/* system */
	_T(".arc .gdb"),					/* package */
	_T("1.0.1"),						/* revision */
	_T("痴漢公賊"),						/* author */
	_T("2008-6-18 8:31"),				/* date */
	NULL,								/* notion */
	ACUI_ATTRIBUTE_LEVEL_UNSTABLE
};	

#pragma pack (1)
typedef struct {
	s8 magic[12];		/* "PackFile    " */
	u32 entries;
} arc_header_t;

typedef struct {
	s8 name[16];
	u32 offset;
	u32 length;
	u32 reserved[2];
} arc_entry_t;

typedef struct {
	s8 magic[16];			/* "DSC FORMAT 1.00" */
	u32 key;
	u32 uncomprlen;
	u32 dec_counts;			/* 解密的次数 */
	u32 reserved;
} dsc_header_t;

typedef struct {
	s8 magic[16];			/* "SDC FORMAT 1.00" */
	u32 key;
	u32 comprlen;
	u32 uncomprlen;
	u16 sum_check;			/* 累加校验值 */
	u16 xor_check;			/* 累异或校验值 */
} gdb_header_t;

typedef struct {
	u32 header_length;
	u32 unknown0;
	u32 length;				/* 实际的数据长度 */
	u32 unknown[13];
} snd_header_t;

typedef struct {
	u16 width;
	u16 height;
	u32 color_depth;
	u32 reserved0[2];
} grp_header_t;

typedef struct {
	s8 magic[16];			/* "CompressedBG___" */
	u16 width;
	u16 height;
	u32 color_depth;
	u32 reserved0[2];
	u32 zero_comprlen;		/* huffman解压后的0值字节压缩的数据长度 */
	u32 key;				/* 解密叶节点权值段用key */
	u32 encode_length;		/* 加密了的叶节点权值段的长度 */
	u8 sum_check;
	u8 xor_check;
	u16 reserved1;
} bg_header_t;
#pragma pack ()


static void *my_malloc(DWORD len)
{
	return malloc(len);
}

/********************* 解密 *********************/

static u8 update_key(u32 *key, u8 *magic)
{
	u32 v0, v1;

	v0 = (*key & 0xffff) * 20021;
	v1 = (magic[1] << 24) | (magic[0] << 16) | (*key >> 16);
	v1 = v1 * 20021 + *key * 346;
	v1 = (v1 + (v0 >> 16)) & 0xffff;
	*key = (v1 << 16) + (v0 & 0xffff) + 1;
	return v1 & 0x7fff;
}

/********************* DSC FORMAT 1.00 *********************/

struct dsc_huffman_code {
	u16 code;
	u16 depth;		/* 该叶节点所在的depth（0开始） */
};

struct dsc_huffman_node {
	unsigned int is_parent;
	unsigned int code;	/* 对于小于256的，code就是字节数据；对于大于256的，高字节的1是标志（lz压缩），低字节是copy_bytes（另外需要+2） */
	unsigned int left_child_index;
	unsigned int right_child_index;
};

/* 按升序排列 */
static int dsc_huffman_code_compare(const void *code1, const void *code2)
{
	struct dsc_huffman_code *codes[2] = { (struct dsc_huffman_code *)code1, (struct dsc_huffman_code *)code2 };
	int compare = (int)codes[0]->depth - (int)codes[1]->depth;

	if (!compare)
		return (int)codes[0]->code - (int)codes[1]->code;

	return compare;
}

static void dsc_create_huffman_tree(struct dsc_huffman_node *huffman_nodes,
									struct dsc_huffman_code *huffman_code, 
									unsigned int leaf_node_counts)
{
	unsigned int nodes_index[2][512];	/* 构造当前depth下节点索引的辅助用数组，
										 * 其长度是当前depth下可能的最大节点数 */
	unsigned int next_node_index = 1;	/* 0已经默认分配给根节点了 */
	unsigned int depth_nodes = 1;		/* 第0深度的节点数(2^N)为1 */
	unsigned int depth = 0;				/* 当前的深度 */	
	unsigned int switch_flag = 0;

	nodes_index[0][0] = 0;
	for (unsigned int n = 0; n < leaf_node_counts; ) {
		unsigned int depth_existed_nodes;/* 当前depth下创建的叶节点计数 */
		unsigned int depth_nodes_to_create;	/* 当前depth下需要创建的节点数（对于下一层来说相当于父节点） */
		unsigned int *huffman_nodes_index;
		unsigned int *child_index;

		huffman_nodes_index = nodes_index[switch_flag];
		switch_flag ^= 1;
		child_index = nodes_index[switch_flag];

		depth_existed_nodes = 0;
		/* 初始化隶属于同一depth的所有节点 */
		while (huffman_code[n].depth == depth) {
			struct dsc_huffman_node *node = &huffman_nodes[huffman_nodes_index[depth_existed_nodes++]];
			/* 叶节点集中在左侧 */
			node->is_parent = 0;
			node->code = huffman_code[n++].code;
		}
		depth_nodes_to_create = depth_nodes - depth_existed_nodes;
		for (unsigned int i = 0; i < depth_nodes_to_create; i++) {
			struct dsc_huffman_node *node = &huffman_nodes[huffman_nodes_index[depth_existed_nodes + i]];

			node->is_parent = 1;
			child_index[i * 2 + 0] = node->left_child_index = next_node_index++;
			child_index[i * 2 + 1] = node->right_child_index = next_node_index++;
		}
		depth++;
		depth_nodes = depth_nodes_to_create * 2;	/* 下一depth可能的节点数 */
	}
}

static unsigned int dsc_huffman_decompress(struct dsc_huffman_node *huffman_nodes, 
								  unsigned int dec_counts, unsigned char *uncompr, 
								  unsigned int uncomprlen, unsigned char *compr, 
								  unsigned int comprlen)
{
	struct bits bits;
	unsigned int act_uncomprlen = 0;
	int err = 0;
	
	bits_init(&bits, compr, comprlen);	
	for (unsigned int k = 0; k < dec_counts; k++) {
		unsigned char child;
		unsigned int node_index;
		unsigned int code;
		
		node_index = 0;
		do {
			if (bit_get_high(&bits, &child))
				goto out;

			if (!child)
				node_index = huffman_nodes[node_index].left_child_index;
			else
				node_index = huffman_nodes[node_index].right_child_index;
		} while (huffman_nodes[node_index].is_parent);

		code = huffman_nodes[node_index].code;

		if (code >= 256) {	// 12位编码的win_pos, lz压缩
			unsigned int copy_bytes, win_pos;
			
			copy_bytes = (code & 0xff) + 2;
			if (bits_get_high(&bits, 12, &win_pos)) 
				break;
	
			win_pos += 2;			
			for (unsigned int i = 0; i < copy_bytes; i++) {
				uncompr[act_uncomprlen] = uncompr[act_uncomprlen - win_pos];
				act_uncomprlen++;
			}
		} else
			uncompr[act_uncomprlen++] = code;
	}
out:
	return act_uncomprlen;
}

static unsigned int dsc_decompress(dsc_header_t *dsc_header, unsigned int dsc_len, 
								   unsigned char *uncompr, unsigned int uncomprlen)
{
	/* 叶节点编码：前256是普通的单字节数据code；后256是lz压缩的code（每种code对应一种不同的copy_bytes */
	struct dsc_huffman_code huffman_code[512];
	/* 潜在的，相当于双字节编码（单字节ascii＋双字节lz） */
	struct dsc_huffman_node huffman_nodes[1023];	
	u8 magic[2];
	unsigned int key;
	unsigned char *enc_buf;	
	
	enc_buf = (unsigned char *)(dsc_header + 1);
	magic[0] = dsc_header->magic[0];
	magic[1] = dsc_header->magic[1];
	key = dsc_header->key;

	/* 解密叶节点深度信息 */
	memset(huffman_code, 0, sizeof(huffman_code));
	unsigned int leaf_node_counts = 0;
	for (unsigned int i = 0; i < 512; i++) {
		u8 depth;

		depth = enc_buf[i] - update_key(&key, magic);
		if (depth) {
			huffman_code[leaf_node_counts].depth = depth;
			huffman_code[leaf_node_counts].code = i;
			leaf_node_counts++;
		}
	}	

	qsort(huffman_code, leaf_node_counts, sizeof(struct dsc_huffman_code), dsc_huffman_code_compare);

	dsc_create_huffman_tree(huffman_nodes, huffman_code, leaf_node_counts);

	unsigned char *compr = enc_buf + 512;
	unsigned int act_uncomprlen;
	act_uncomprlen = dsc_huffman_decompress(huffman_nodes, dsc_header->dec_counts, 
		uncompr, dsc_header->uncomprlen, compr, dsc_len - sizeof(dsc_header_t) - 512);
	
	return act_uncomprlen;
}

/********************* CompressedBG___ *********************/

typedef struct {
	unsigned int valid;				/* 是否有效的标记 */
	unsigned int weight;			/* 权值 */
	unsigned int is_parent;			/* 是否是父节点 */
	unsigned int parent_index;		/* 父节点索引 */
	unsigned int left_child_index;	/* 左子节点索引 */
	unsigned int right_child_index;	/* 右子节点索引 */		
} bg_huffman_node;

static void	decode_bg(unsigned char *enc_buf, unsigned int enc_buf_len, 
					  unsigned int key, unsigned char *ret_sum, unsigned char *ret_xor)
{	
	unsigned char sum = 0;
	unsigned char xor = 0;	
	u8 magic[2] = { 0, 0 };
	
	for (unsigned int i = 0; i < enc_buf_len; i++) {
		enc_buf[i] -= update_key(&key, magic);
		sum += enc_buf[i];
		xor ^= enc_buf[i];
	}
	*ret_sum = sum;
	*ret_xor = xor;
}

static unsigned int	bg_create_huffman_tree(bg_huffman_node *nodes, u32 *leaf_nodes_weight)
{
	unsigned int parent_node_index = 256;	/* 父节点从nodes[]的256处开始 */
	bg_huffman_node *parent_node = &nodes[parent_node_index];
	unsigned int root_node_weight = 0;	/* 根节点权值 */
	unsigned int i;

	/* 初始化叶节点 */
	for (i = 0; i < 256; i++) {
		nodes[i].valid = !!leaf_nodes_weight[i];
		nodes[i].weight = leaf_nodes_weight[i];
		nodes[i].is_parent = 0;
		root_node_weight += nodes[i].weight;
	}

	while (1) {		
		unsigned int child_node_index[2];
		
		/* 创建左右子节点 */
		for (i = 0; i < 2; i++) {
			unsigned int min_weight;
			
			min_weight = -1;
			child_node_index[i] = -1;
			/* 遍历nodes[], 找到weight最小的2个节点作为子节点 */
			for (unsigned int n = 0; n < parent_node_index; n++) {
				if (nodes[n].valid) {
					if (nodes[n].weight < min_weight) {
						min_weight = nodes[n].weight;
						child_node_index[i] = n;
					}
				}
			}
			/* 被找到的子节点标记为无效，以便不参与接下来的查找 */			
			nodes[child_node_index[i]].valid = 0;
			nodes[child_node_index[i]].parent_index = parent_node_index;
		}
		/* 创建父节点 */		
		parent_node->valid = 1;
		parent_node->is_parent = 1;
		parent_node->left_child_index = child_node_index[0];
		parent_node->right_child_index = child_node_index[1];
		parent_node->weight = nodes[parent_node->left_child_index].weight
			+ nodes[parent_node->right_child_index].weight;	
		if (parent_node->weight == root_node_weight)
			break;
		parent_node = &nodes[++parent_node_index];
	}

	return parent_node_index;
}
		
static unsigned int bg_huffman_decompress(bg_huffman_node *huffman_nodes, 
										  unsigned int root_node_index,
										  unsigned char *uncompr, unsigned int uncomprlen, 
										  unsigned char *compr, unsigned int comprlen)
{
	struct bits bits;
		
	bits_init(&bits, compr, comprlen);
	for (unsigned int act_uncomprlen = 0; act_uncomprlen < uncomprlen; act_uncomprlen++) {
		unsigned char child;
		unsigned int node_index;
		
		node_index = root_node_index;
		do {
			if (bit_get_high(&bits, &child))
				goto out;

			if (!child)
				node_index = huffman_nodes[node_index].left_child_index;
			else
				node_index = huffman_nodes[node_index].right_child_index;
		} while (huffman_nodes[node_index].is_parent);

		uncompr[act_uncomprlen] = node_index;
	}
out:
	return act_uncomprlen;
}

static unsigned int zero_decompress(unsigned char *uncompr, unsigned int uncomprlen,
		unsigned char *compr, unsigned int comprlen)
{
	unsigned int act_uncomprlen = 0;
	int dec_zero = 0;
	unsigned int curbyte = 0;

	while (1) {
		unsigned int bits = 0;
		unsigned int copy_bytes = 0;
		unsigned char code;
		
		do {
			if (curbyte >= comprlen)
				goto out;

			code = compr[curbyte++];
			copy_bytes |= (code & 0x7f) << bits;
			bits += 7;		
		} while (code & 0x80);
		
		if (act_uncomprlen + copy_bytes > uncomprlen)
			break;
		if (!dec_zero && (curbyte + copy_bytes > comprlen))	
			break;
		
		if (!dec_zero) {
			memcpy(&uncompr[act_uncomprlen], &compr[curbyte], copy_bytes);
			curbyte += copy_bytes;
			dec_zero = 1;
		} else {
			memset(&uncompr[act_uncomprlen], 0, copy_bytes);
			dec_zero = 0;
		}
		act_uncomprlen += copy_bytes;
	}
out:	
	return act_uncomprlen;
}

static void bg_average_defilting(unsigned char *dib_buf, unsigned int width, 
								 unsigned int height, unsigned int bpp)
{
	unsigned int line_len = width * bpp;
	
	for (unsigned int y = 0; y < height; y++) {
		for (unsigned int x = 0; x < width; x++) {
			for (unsigned int p = 0; p < bpp; p++) {
				unsigned int a, b;
				unsigned int avg;

				b = y ? dib_buf[(y - 1) * line_len + x * bpp + p] : -1;
				a = x ? dib_buf[y * line_len + (x - 1) * bpp + p] : -1;
				avg = 0;
				
				if (a != -1)
					avg += a;
				if (b != -1)
					avg += b;
				if (a != -1 && b != -1)
					avg /= 2;
					
				dib_buf[y * line_len + x * bpp + p] += avg;
			}
		}
	}
}

static unsigned int bg_decompress(bg_header_t *bg_header, unsigned int bg_len, 
								  unsigned char *image_buf, unsigned int image_size)
{
	unsigned int act_uncomprlen = 0;
	unsigned int i;		
	unsigned char *enc_buf = (unsigned char *)(bg_header + 1);
	
	/* 解密叶节点权值 */
	unsigned char sum;
	unsigned char xor;
	decode_bg(enc_buf, bg_header->encode_length, bg_header->key, &sum, &xor);	
	if (sum != bg_header->sum_check || xor != bg_header->xor_check)
		return -CUI_EMATCH;

	/* 初始化叶节点权值 */
	u32 leaf_nodes_weight[256];
	unsigned int curbyte = 0;
	for (i = 0; i < 256; i++) {
		unsigned int bits = 0;		
		u32 weight = 0;
		unsigned char code;
		
		do {
			if (curbyte >= bg_header->encode_length)
				return 0;
			code = enc_buf[curbyte++];
			weight |= (code & 0x7f) << bits;
			bits += 7;		
		} while (code & 0x80);	
		leaf_nodes_weight[i] = weight;
	}

	bg_huffman_node nodes[511];
	unsigned int root_node_index = bg_create_huffman_tree(nodes, leaf_nodes_weight);
	unsigned char *zero_compr = (unsigned char *)malloc(bg_header->zero_comprlen);
	if (!zero_compr)
		return -CUI_EMEM;

	unsigned char *compr = enc_buf + bg_header->encode_length;
	unsigned int comprlen = bg_len - sizeof(bg_header_t) - bg_header->encode_length;
	act_uncomprlen = bg_huffman_decompress(nodes, root_node_index, 
		zero_compr, bg_header->zero_comprlen, compr, comprlen);
	if (act_uncomprlen != bg_header->zero_comprlen) {
		free(zero_compr);
		return -CUI_EUNCOMPR;
	}

	act_uncomprlen = zero_decompress(image_buf, image_size, zero_compr, bg_header->zero_comprlen);
	free(zero_compr);
	
	bg_average_defilting(image_buf, bg_header->width, bg_header->height, bg_header->color_depth / 8);

	return act_uncomprlen;
}

/********************* arc *********************/

static int BGI_arc_match(struct package *pkg)
{
	arc_header_t *arc_header;

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	arc_header = (arc_header_t *)malloc(sizeof(*arc_header));
	if (!arc_header) {
		pkg->pio->close(pkg);
		return -CUI_EMEM;
	}

	if (pkg->pio->read(pkg, arc_header, sizeof(*arc_header))) {
		free(arc_header);
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	if (memcmp(arc_header->magic, "PackFile    ", 12) || !arc_header->entries) {
		free(arc_header);
		pkg->pio->close(pkg);
		return -CUI_EMATCH;
	}
	package_set_private(pkg, arc_header);

	return 0;
}

static int BGI_arc_extract_directory(struct package *pkg,
									 struct package_directory *pkg_dir)
{	
	arc_header_t *arc_header;

	arc_header = (arc_header_t *)package_get_private(pkg);
	pkg_dir->index_entries = arc_header->entries;
	pkg_dir->directory_length = pkg_dir->index_entries * sizeof(arc_entry_t);
	pkg_dir->directory = malloc(pkg_dir->directory_length);
	if (!pkg_dir->directory)
		return -CUI_EMEM;

	if (pkg->pio->read(pkg, pkg_dir->directory, pkg_dir->directory_length))
		return -CUI_EREAD;

	pkg_dir->index_entry_length = sizeof(arc_entry_t);

	return 0;
}

static int BGI_arc_parse_resource_info(struct package *pkg,
									   struct package_resource *pkg_res)
{
	arc_header_t *arc_header;
	arc_entry_t *arc_entry;

	arc_header = (arc_header_t *)package_get_private(pkg);
	arc_entry = (arc_entry_t *)pkg_res->actual_index_entry;
	strcpy(pkg_res->name, arc_entry->name);
	pkg_res->name_length = -1;			/* -1表示名称以NULL结尾 */
	pkg_res->raw_data_length = arc_entry->length;
	pkg_res->actual_data_length = 0;	/* 数据都是明文 */
	pkg_res->offset = arc_entry->offset 
		+ sizeof(arc_header_t) + arc_header->entries * sizeof(arc_entry_t);

	return 0;
}

static int BGI_arc_extract_resource(struct package *pkg,
									struct package_resource *pkg_res)
{
	dsc_header_t *dsc_header;
	grp_header_t *grp_header;
	snd_header_t *snd_header;
	unsigned char *compr, *uncompr;
	unsigned int act_uncomprlen;

	compr = (unsigned char *)malloc((unsigned int)pkg_res->raw_data_length);
	if (!compr)
		return -CUI_EMEM;

	if (pkg->pio->readvec(pkg, compr, pkg_res->raw_data_length, 
			pkg_res->offset, IO_SEEK_SET)) {
		free(compr);
		return -CUI_EREADVEC;
	}

	grp_header = (grp_header_t *)compr;
	snd_header = (snd_header_t *)compr;
	dsc_header = (dsc_header_t *)compr;

	if (!memcmp(dsc_header->magic, "DSC FORMAT 1.00", 16)) {
		uncompr = (unsigned char *)malloc(dsc_header->uncomprlen);
		if (!uncompr) {
			free(compr);
			return -CUI_EMEM;
		}

		act_uncomprlen = dsc_decompress(dsc_header, (DWORD)pkg_res->raw_data_length, 
			uncompr, dsc_header->uncomprlen);		
		if (act_uncomprlen != dsc_header->uncomprlen) {
			free(uncompr);
			free(compr);
			return -CUI_EUNCOMPR;
		}
		free(compr);
	} else if (!memcmp(dsc_header->magic, "CompressedBG___", 16)) {
		bg_header_t *bg_header;
		unsigned int image_size;
		unsigned char *image_buffer;

		bg_header = (bg_header_t *)compr;
		image_size = bg_header->width * bg_header->height * bg_header->color_depth / 8;
		image_buffer = (unsigned char *)malloc(image_size);
		if (!image_buffer) {
			free(compr);
			return -CUI_EMEM;
		}
	
		act_uncomprlen = bg_decompress(bg_header, (DWORD)pkg_res->raw_data_length, 
			image_buffer, image_size);		
		if (act_uncomprlen != image_size) {
			free(image_buffer);
			free(compr);
			return -CUI_EUNCOMPR;			
		}

		if (MyBuildBMPFile(image_buffer, image_size, NULL, 0, 
				bg_header->width, -bg_header->height, bg_header->color_depth,
					(unsigned char **)&pkg_res->actual_data, (DWORD *)&pkg_res->actual_data_length,
						my_malloc)) {
			free(image_buffer);
			free(compr);
			return -CUI_EMEM;		
		}
		free(image_buffer);
		free(compr);
		pkg_res->replace_extension = _T(".bmp");
		pkg_res->flags = PKG_RES_FLAG_REEXT;
		return 0;
	} else /* 早期的BGI系统通常不做dsc封装 */ 
		if (snd_header->header_length == sizeof(snd_header_t) 
			&& !memcmp((char *)(snd_header + 1), "OggS", 4)) {
		char *ogg_magic = (char *)(snd_header + 1);
		unsigned char *ogg;
		unsigned int ogg_len;

		ogg_len = (DWORD)pkg_res->raw_data_length - sizeof(snd_header_t);
		ogg = (unsigned char *)malloc(ogg_len);
		if (!ogg) {
			free(compr);
			return -CUI_EMEM;
		}
		memcpy(ogg, ogg_magic, ogg_len);
		free(compr);
		pkg_res->actual_data = ogg;
		pkg_res->actual_data_length = ogg_len;
		pkg_res->replace_extension = _T(".ogg");
		pkg_res->flags = PKG_RES_FLAG_REEXT;
		return 0;
	} else if (pkg_res->raw_data_length == 
			(grp_header->width * grp_header->height * grp_header->color_depth / 8
				+ sizeof(grp_header_t))) {
		BYTE *image = (BYTE *)(grp_header + 1);
		unsigned int image_size = (unsigned int)pkg_res->raw_data_length - sizeof(grp_header_t);

		if (MyBuildBMPFile(image, image_size, NULL, 0, 
				grp_header->width, -grp_header->height, grp_header->color_depth,
				(unsigned char **)&pkg_res->actual_data, (DWORD *)&pkg_res->actual_data_length,
					my_malloc)) {
			free(compr);
			return -CUI_EMEM;		
		}
		free(compr);
		pkg_res->replace_extension = _T(".grp.bmp");
		pkg_res->flags = PKG_RES_FLAG_REEXT;
		return 0;
	} else {	/* output raw */
		pkg_res->raw_data = compr;
		return 0;
	}

	grp_header = (grp_header_t *)uncompr;
	snd_header = (snd_header_t *)uncompr;
	char *ogg_magic = (char *)(snd_header + 1);
	if (snd_header->header_length == sizeof(snd_header_t) 
			&& !memcmp(ogg_magic, "OggS", 4)) {
		unsigned char *ogg;
		unsigned int ogg_len;

		ogg_len = act_uncomprlen - sizeof(snd_header_t);
		ogg = (unsigned char *)malloc(ogg_len);
		if (!ogg) {
			free(uncompr);
			return -CUI_EMEM;
		}
		memcpy(ogg, ogg_magic, ogg_len);
		free(uncompr);
		pkg_res->actual_data = ogg;
		pkg_res->actual_data_length = ogg_len;
		pkg_res->replace_extension = _T(".ogg");
		pkg_res->flags = PKG_RES_FLAG_REEXT;
		return 0;
	} else if (act_uncomprlen == (grp_header->width * grp_header->height 
		* grp_header->color_depth / 8 + sizeof(grp_header_t))) {
		BYTE *image = (BYTE *)(grp_header + 1);
		unsigned int image_size = act_uncomprlen - sizeof(grp_header_t);

		if (MyBuildBMPFile(image, image_size, NULL, 0, 
				grp_header->width, -grp_header->height, grp_header->color_depth,
					(unsigned char **)&pkg_res->actual_data, (DWORD *)&pkg_res->actual_data_length,
						my_malloc)) {
			free(uncompr);
			return -CUI_EMEM;		
		}
		free(uncompr);
		pkg_res->replace_extension = _T(".grp.bmp");
		pkg_res->flags = PKG_RES_FLAG_REEXT;
		return 0;
	}
		
	pkg_res->actual_data = uncompr;
	pkg_res->actual_data_length = act_uncomprlen;

	return 0;
}

static int BGI_arc_save_resource(struct resource *res,
								 struct package_resource *pkg_res)
{
	if (res->rio->create(res))
		return -CUI_ECREATE;

	if (pkg_res->actual_data && pkg_res->actual_data_length) {;
		if (res->rio->write(res, pkg_res->actual_data, pkg_res->actual_data_length)) {
			res->rio->close(res);
			return -CUI_EWRITE;
		}
	} else if (pkg_res->raw_data && pkg_res->raw_data_length) {
		if (res->rio->write(res, pkg_res->raw_data, pkg_res->raw_data_length)) {
			res->rio->close(res);
			return -CUI_EWRITE;
		}
	}
	res->rio->close(res);
	
	return 0;
}

static void BGI_arc_release_resource(struct package *pkg,
									 struct package_resource *pkg_res)
{
	if (!pkg || !pkg_res)
		return;

	if (pkg_res->actual_data) {
		free(pkg_res->actual_data);
		pkg_res->actual_data = NULL;
	}

	if (pkg_res->raw_data) {
		free(pkg_res->raw_data);
		pkg_res->raw_data = NULL;
	}
}

static void BGI_arc_release(struct package *pkg,
							struct package_directory *pkg_dir)
{
	arc_header_t *arc_header;
	
	if (!pkg || !pkg_dir)
		return;

	if (pkg_dir->directory) {
		free(pkg_dir->directory);
		pkg_dir->directory = NULL;
	}

	arc_header = (arc_header_t *)package_get_private(pkg);
	if (arc_header) {
		free(arc_header);
		package_set_private(pkg, NULL);
	}

	pkg->pio->close(pkg);
}

static cui_ext_operation BGI_arc_operation = {
	BGI_arc_match,				/* match */
	BGI_arc_extract_directory,	/* extract_directory */
	BGI_arc_parse_resource_info,/* parse_resource_info */
	BGI_arc_extract_resource,	/* extract_resource */
	BGI_arc_save_resource,		/* save_resource */
	BGI_arc_release_resource,	/* release_resource */
	BGI_arc_release				/* release */
};
	
/********************* gdb *********************/

static int BGI_gdb_match(struct package *pkg)
{
	gdb_header_t *gdb_header;

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	gdb_header = (gdb_header_t *)malloc(sizeof(*gdb_header));
	if (!gdb_header) {
		pkg->pio->close(pkg);
		return -CUI_EMEM;
	}

	if (pkg->pio->read(pkg, gdb_header, sizeof(*gdb_header))) {
		free(gdb_header);
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	if (strncmp((char *)gdb_header->magic, "SDC FORMAT 1.00", 16)) {
		free(gdb_header);
		pkg->pio->close(pkg);
		return -CUI_EMATCH;
	}
	package_set_private(pkg, gdb_header);

	return 0;
}

static int BGI_gdb_extract_resource(struct package *pkg,
									struct package_resource *pkg_res)
{
	gdb_header_t *gdb_header;
	unsigned char *enc_buf, *dec_buf;
	u8 magic[2];
	u32 key;
	
	gdb_header = (gdb_header_t *)package_get_private(pkg);
	enc_buf = (unsigned char *)malloc(gdb_header->comprlen * 2);
	if (!enc_buf)
		return -CUI_EMEM;
	
	if (pkg->pio->read(pkg, enc_buf, gdb_header->comprlen)) {
		free(enc_buf);
		return -CUI_EREAD;
	}
	
	/* sdc decode() */
	dec_buf = enc_buf + gdb_header->comprlen;	
	magic[0] = gdb_header->magic[0];
	magic[1] = gdb_header->magic[1];
	key = gdb_header->key;	
	
	unsigned int sum = 0;
	unsigned int xor = 0;
	for (unsigned int i = 0; i < gdb_header->comprlen; i++) {
		dec_buf[i] = enc_buf[i] - update_key(&key, magic);
		sum += enc_buf[i];
		xor ^= enc_buf[i];
	}
	if ((sum & 0xffff) != gdb_header->sum_check 
			|| (xor & 0xffff) != gdb_header->xor_check) {
		free(enc_buf);	
		return -CUI_EMATCH;
	}
	
	/* sdc decompress() */	
	unsigned char *uncompr = (unsigned char *)malloc(gdb_header->uncomprlen);
	if (!uncompr) {
		free(enc_buf);	
		return -CUI_EMEM;		
	}
	memset(uncompr, 0, gdb_header->uncomprlen);
	unsigned int act_uncomprlen = 0;
	for (i = 0; i < gdb_header->comprlen; ) {
		unsigned int copy_bytes, win_pos;
		
		if (dec_buf[i] & 0x80) {
			u8 code = dec_buf[i] & 0x7f;
			
			copy_bytes = (code >> 3) + 2;
			win_pos = ((code & 7) << 8) | dec_buf[i + 1] + 2;
			memcpy(&uncompr[act_uncomprlen], &uncompr[act_uncomprlen - win_pos], copy_bytes);
			act_uncomprlen += copy_bytes;
			i += 2;
		} else {
			copy_bytes = dec_buf[i++] + 1;
			memcpy(&uncompr[act_uncomprlen], &dec_buf[i], copy_bytes);
			act_uncomprlen += copy_bytes;
			i += copy_bytes;
		}
	}
	free(enc_buf);
	if (act_uncomprlen != gdb_header->uncomprlen) {
		free(uncompr);
		return -CUI_EUNCOMPR;
	}
	pkg_res->raw_data_length = gdb_header->comprlen;
	pkg_res->actual_data = uncompr;
	pkg_res->actual_data_length = act_uncomprlen;

	return 0;
}

static int BGI_gdb_save_resource(struct resource *res,
								 struct package_resource *pkg_res)
{
	if (res->rio->create(res))
		return -CUI_ECREATE;

	if (pkg_res->actual_data && pkg_res->actual_data_length) {
		if (res->rio->write(res, pkg_res->actual_data, pkg_res->actual_data_length)) {
			res->rio->close(res);
			return -CUI_EWRITE;
		}
	}
	res->rio->close(res);
	
	return 0;
}

static void BGI_gdb_release_resource(struct package *pkg,
									 struct package_resource *pkg_res)
{
	if (!pkg || !pkg_res)
		return;

	if (pkg_res->actual_data) {
		free(pkg_res->actual_data);
		pkg_res->actual_data = NULL;
	}
}

static void BGI_gdb_release(struct package *pkg,
							struct package_directory *pkg_dir)
{
	gdb_header_t *gdb_header;
	
	if (!pkg || !pkg_dir)
		return;

	gdb_header = (gdb_header_t *)package_get_private(pkg);
	if (gdb_header) {
		free(gdb_header);
		package_set_private(pkg, NULL);
	}

	pkg->pio->close(pkg);
}

static cui_ext_operation BGI_gdb_operation = {
	BGI_gdb_match,				/* match */
	NULL,						/* extract_directory */
	NULL,						/* parse_resource_info */
	BGI_gdb_extract_resource,	/* extract_resource */
	BGI_gdb_save_resource,		/* save_resource */
	BGI_gdb_release_resource,	/* release_resource */
	BGI_gdb_release				/* release */
};

int CALLBACK BGI_register_cui(struct cui_register_callback *callback)
{
	if (callback->add_extension(callback->cui, _T(".arc"), NULL, 
		NULL, &BGI_arc_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_DIR))
			return -1;
	
	if (callback->add_extension(callback->cui, _T(".gdb"), _T(".gdb.dump"), 
		NULL, &BGI_gdb_operation, CUI_EXT_FLAG_PKG))
			return -1;

	return 0;
}
