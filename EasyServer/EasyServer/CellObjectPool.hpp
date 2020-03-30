#ifndef __CELL_OBJECT_POOL__
#define __CELL_OBJECT_POOL__

#include "HeaderFile.h"

template <class Type, size_t pool_size>
class ObjectPool
{
private:
	struct NodeHeader
	{
		NodeHeader *next;
		int id;
		bool is_in_pool;
		char ref_count;
		char reserve_[2];
	};

public:
	ObjectPool()
	{
		InitPool();
	}

	~ObjectPool()
	{
		if (buffer_)
		{
			delete[] buffer_;
		}
	}

	void *AllocObject(size_t size)
	{
		std::lock_guard<std::mutex> lock(object_mutex_);

		NodeHeader *node = nullptr;

		if (!header_)
		{
			node = (NodeHeader *)new char[size + sizeof(NodeHeader)];
			if (node)
			{
				node->id = -1;
				node->is_in_pool = false;
				node->next = nullptr;
				node->ref_count = 1;
			}
		}
		else
		{
			node = header_;
			header_ = node->next;

			assert(node->ref_count == 0);
			node->ref_count = 1;
		}

		if (node)
		{
			xprintf("AllocObject : %p, id<%d>, size<%d>\n", node, node->id, int(size));
			return (char *)node + sizeof(NodeHeader);
		}

		return nullptr;
	}

	void FreeObject(void *obj)
	{
		NodeHeader *node = (NodeHeader *)((char *)obj - sizeof(NodeHeader));

		assert(node->ref_count == 1);

		if (--node->ref_count != 0)
		{
			return;
		}

		if (node->is_in_pool)
		{
			std::lock_guard<std::mutex> lock(object_mutex_);

			node->next = header_;
			header_ = node;
		}
		else
		{
			delete[] node;
		}
	}

private:
	void InitPool()
	{
		assert(!buffer_);
		size_t size = pool_size * (sizeof(Type) + sizeof(NodeHeader));

		buffer_ = new char[size];

		if (buffer_)
		{
			header_ = (NodeHeader *)buffer_;
			header_->id = 0;
			header_->is_in_pool = true;
			header_->next = nullptr;
			header_->ref_count = 0;

			NodeHeader *temp = header_;
			for (size_t i = 1; i < pool_size; i++)
			{
				NodeHeader *new_node = (NodeHeader *)(buffer_ + (i * (sizeof(Type) + sizeof(NodeHeader))));
				header_->id = i;
				header_->is_in_pool = true;
				header_->next = nullptr;
				header_->ref_count = 0;

				temp->next = new_node;
				temp = new_node;
			}
		}
	}

private:
	NodeHeader *header_ = nullptr;
	char *buffer_ = nullptr;
	std::mutex object_mutex_;
};

template <class Type, size_t pool_size>
class BaseObjectPool
{
public:
	void *operator new(size_t size)
	{
		return ObjectPool().AllocObject(size);
	}

	void operator delete(void *ptr)
	{
		ObjectPool().FreeObject(ptr);
	}

	template <typename... Args>
	static Type *CreateObject(Args... args)
	{
		Type *obj = new Type(args...);
		return obj;
	}

	static void DestroyObject(Type *obj)
	{
		delete obj;
	}

private:
	typedef ObjectPool<Type, pool_size> TypeObjPool;
	static TypeObjPool &ObjectPool()
	{
		static TypeObjPool pool;
		return pool;
	}
};

#endif // !__CELL_OBJECT_POOL__
