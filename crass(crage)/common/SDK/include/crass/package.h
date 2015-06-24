#ifndef PACKAGE_H
#define PACKAGE_H
	
	
struct package_directory_entry_operation {
	
	
};

	
struct package_directory_operation {
	add_on_entry(struct __package_directory *, );
	
};


struct package_directory_entry {
	struct package_directory_entry_operation *op;
	struct list_head entry;							/* 目录项链表节点 */
	unsigned int ref_count;							/* 引用计数 */
	struct __package_directory *pd;					/* 所属的目录项 */
	void *data;										/* 目录项数据 */
	unsigned int data_length;						/* 目录项数据长度 */
};

struct __package_directory {
	struct package_directory_operation *op;
	unsigned int entries;					/* 索引节点数 */
	struct list_head link;					/* 索引节点链表 */
	unsigned int ref_count;					/* 引用计数 */
	struct package *pkg;					/* 所属的封包 */
	//lock_t lock;
};
	
#endif	/* PACKAGE_H */
