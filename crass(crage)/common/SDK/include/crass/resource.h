#ifndef RESOURCE_H
#define RESOURCE_H

#define RES_FLAG_DIR		0x00000001UL

struct resource {
	struct crass_object cobj;
	struct file_object *rio;
	unsigned long flags;
	const TCHAR *name;
	const TCHAR *path;
	const TCHAR *full_path;
	const TCHAR *extension;
};

#endif	/* RESOURCE_H */
