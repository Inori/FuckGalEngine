#ifndef CRASS_OBJECT
#define CRASS_OBJECT

struct crass_object {
	const TCHAR *name;
	unsigned int name_length;
	unsigned long ref_count;
	void (*release)(void *);
	struct list_head entry;
	struct crass_object *parent;
};

extern struct crass_object *crass_object_create(const TCHAR *name);
extern struct crass_object *crass_object_init(struct crass_object *cobj, const TCHAR *name, void (*release)(void *));
extern struct crass_object *crass_object_get_ref(struct crass_object *cobj);
extern void crass_object_put_ref(struct crass_object *cobj);
extern const TCHAR *crass_object_set_name(struct crass_object *cobj, const TCHAR *name);

#endif	/* CRASS_OBJECT */
