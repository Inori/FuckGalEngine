#ifndef CRASS_H
#define CRASS_H

#ifdef CRASS_EXPORTS
#define CRASS_API __declspec(dllexport)
#else
#define CRASS_API __declspec(dllimport)
#endif 

/* TCHAR缓冲区的长度 */
#define SOT(x)	(sizeof(x) / sizeof(TCHAR))

/*********** 控制台信息显示 **************/

#if 0
/* 显示信息定义 */
#define CRASS_VERBOSE_LEVEL_SLIENT		0
#define CRASS_VERBOSE_LEVEL_DEBUG		1
#define CRASS_VERBOSE_LEVEL_WARNING		2
#define CRASS_VERBOSE_LEVEL_ERROR		3
#define CRASS_VERBOSE_LEVEL_FAULT		4
#define CRASS_VERBOSE_LEVEL_NORMAL		255

CRASS_API extern unsigned int crass_verbose_level;
CRASS_API void crass_printf(unsigned int level, const TCHAR *fmt, ...);

/* 普通信息 */
#define crass_message(fmt, arg...)	\
	crass_printf(CRASS_VERBOSE_LEVEL_NORMAL, fmt, ##arg)
/* 调试信息 */
#define crass_debug(fmt, arg...)	\
	crass_printf(CRASS_VERBOSE_LEVEL_DEBUG, fmt, ##arg)
/* 发出警告信息 */
#define crass_warning(fmt, arg...)	\
	crass_printf(CRASS_VERBOSE_LEVEL_WARNING, fmt, ##arg)
/* 发生了错误 */
#define crass_error(fmt, arg...)	\
	crass_printf(CRASS_VERBOSE_LEVEL_ERROR, fmt, ##arg)	
/* 发生了通常不可能发生的严重错误 */
#define crass_fault(fmt, arg...)	\
	crass_printf(CRASS_VERBOSE_LEVEL_FAULT, fmt, ##arg)
#endif
/* 通过容器成员获得容器指针。
 * @pointer - 成员指针。
 * @type - 容器的数据类型。
 * @member - 成员变量的名称。
 */
#define container_of(pointer, type, member)	\
	((type *)((char *)(pointer) - (char *)&(((type *)0)->member)))

#define SOT(x)	(sizeof(x) / sizeof(TCHAR))
	
/*********** 通用双向链表 **************/

/* 链表节点 */
struct list_head {
	struct list_head *prev;	/* 前项指针 */
	struct list_head *next;	/* 后项指针 */
};

/* 添加链表节点的内部实现。*/
static inline void __list_add(struct list_head *prev, struct list_head *_new, struct list_head *next)
{
	_new->prev = prev;
	_new->next = next;	
	prev->next = _new;
	next->prev = _new;
}

/* 删除链表节点的内部实现。*/
static inline void __list_del(struct list_head *prev, struct list_head *next)
{
	prev->next = next;
	next->prev = prev;
}

/* 初始化链表节点。
 * @head - 被初始化的节点。
 */
static inline void list_init(struct list_head *head)
{
	head->prev = head->next = head;
}

/* 添加链表节点。
 * @_new - 待加入的节点。
 * @head - 被加入的头节点。
 */
static inline void list_add(struct list_head *_new, struct list_head *head)
{
	__list_add(head, _new, head->next);
}

/* 在链表结尾添加节点。
 * @_new - 待加入的节点。
 * @head - 被加入的头节点。
 */
static inline void list_add_tail(struct list_head *_new, struct list_head *head)
{
	__list_add(head->prev, _new, head);
}

/* 删除节点。
 * @head - 待删除的节点。
 */
static inline void list_del(struct list_head *head)
{
	__list_del(head->prev, head->next);
}

/* 删除并重新初始化节点。
 * @head - 待删除的节点。
 */
static inline void list_del_init(struct list_head *head)
{
	__list_del(head->prev, head->next);
	list_init(head);
}

/* 判断链表是否为空。
 * @head - 待测试的的链表头节点。
 */
static inline int list_empty(struct list_head *head)
{
	return head->next == head->prev ? (head->next == head ? 1 : 0) : 0;
}

/* 遍历链表节点。
 * @pos - 当前节点。
 * @head - 链表头节点。
 */
#define list_for_each(pos, head) \
	for (pos = (head)->next; pos != (head); \
        	pos = pos->next)
  
/* 反向遍历链表节点。
 * @pos - 当前节点。
 * @head - 链表头节点。
 */
#define list_for_each_reverse(pos, head) \
	for (pos = (head)->prev; pos != (head); \
        	pos = pos->prev)   
        	
/* 安全遍历链表节点。
 * @pos - 当前节点。
 * @safe - 预保存下一个节点。
 * @head - 链表头节点。
 */        	
#define list_for_each_safe(pos, safe, head) \
	for (pos = (head)->next, safe = pos->next; pos != (head); \
        	pos = safe, safe = safe->next)   
    
/* 安全反向遍历链表节点。
 * @pos - 当前节点。
 * @safe - 预保存下一个节点。
 * @head - 链表头节点。
 */         	       	
#define list_for_each_safe_reverse(pos, safe, head) \
	for (pos = (head)->prev, safe = pos->prev; pos != (head); \
        	pos = safe, safe = safe->prev)  
   
/* 获得链表项指针。
 */ 
#define list_entry(p, type, member)	container_of(p, type, member)

/*********** 同步操作 **************/


/*********** 文件对象操作 **************/

struct file_object;
struct io_request_operation {
	struct io_request *(*create)(const TCHAR *path, unsigned long flags);
	struct io_request *(*open)(const TCHAR *path, unsigned long flags);
	int (*close)(struct io_request *fobj);
	long (*read)(struct io_request *fobj, void *buf, unsigned long buf_len);
	void *(*read_only)(struct io_request *fobj, unsigned long buf_len);
	long (*readvec)(struct io_request *fobj, void *buf, unsigned long buf_len, long offset, unsigned long method);
	void *(*readvec_only)(struct io_request *fobj, unsigned long buf_len, long offset, unsigned long method);
	long (*write)(struct io_request *fobj, void *buf, unsigned long buf_len);
	long (*writevec)(struct io_request *fobj, void *buf, unsigned long buf_len, long offset, unsigned long method);
	int (*length_of)(struct io_request *fobj, unsigned long *len);	
};

struct io_request {
	struct io_request_operation *op;
	

	

};

struct file_object {
	struct file_operation *fio;
	

	

};


#endif	/* CRASS_H */
