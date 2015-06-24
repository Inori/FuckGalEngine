class node {
	unsigned int key0;
	unsigned int key1;
	unsigned int key2;
	unsigned int key3;
	unsigned int key;
};

class node_queue {
public:
	unsigned int count() const
	{
		return (tail - begin) / sizeof(node_queue);
	}
	unsigned int size() const
	{
		return (end - begin) / sizeof(node_queue);
	}
	void insert(node *insert_point, node &new_node);
	void copy(node *dst, node *src, unsigned int count);
	void copy_reverse(node *dst, node *src, unsigned int count);

private:
	node *begin;
	node *tail;
	node *end;
};

