#ifndef __MEMORY_MGR_HPP__
#define __MEMORY_MGR_HPP__

#include <mutex>

#include <string.h>
#include <stdlib.h>
#include <assert.h>

#ifdef _DEBUG
	#include <stdio.h>
	#define xprintf(...) printf(__VA_ARGS__)
	//#define xprintf(...)
#else
	#define xprintf(...)
#endif

const int kMaxMemorySize = 1024;

class MemoryAlloc;

// 内存块
class MemoryBlock
{
public:
	MemoryBlock():id_(0), ref_count_(0), alloc_(nullptr), next_(nullptr), is_in_pool_(false)
	{

	}

	~MemoryBlock()
	{

	}

	int id_ = 0; // 内存块id
	int ref_count_ = 0; // 引用次数
	MemoryAlloc* alloc_ = nullptr; // 所属内存池
	MemoryBlock* next_ = nullptr; // 下一块位置
	bool is_in_pool_ = false; // 是否在内存池中
	char reserve_[3] = {}; // 预留字段
};

// 内存池
class MemoryAlloc
{
public:
	MemoryAlloc()
		: buffer_(nullptr), header_(nullptr), block_size_(0), block_count_(0)
	{

	}

	virtual ~MemoryAlloc()
	{
		if (buffer_)
		{
			free(buffer_);
		}
	}

	// 申请内存
	void* AllocMemory(size_t size)
	{
		std::lock_guard<std::mutex> lock(alloc_mutex_);
		if (!buffer_)
		{
			InitMemory();
		}

		MemoryBlock* block = nullptr;

		if (!header_)
		{
			block = (MemoryBlock*)malloc(size + sizeof(MemoryBlock));
			if (block)
			{
				block->alloc_ = nullptr;
				block->id_ = -1;
				block->is_in_pool_ = false;
				block->next_ = nullptr;
				block->ref_count_ = 1;
			}
		}
		else
		{
			block = header_;
			header_ = block->next_;

			assert(block->ref_count_ == 0);
			block->ref_count_ = 1;
		}

		if (block)
		{
			xprintf("AllocMemory : %p, id<%d>, size<%d>\n", block, block->id_, int(size));
			return (char*)block + sizeof(MemoryBlock);
		}

		return nullptr;
	}

	// 释放内存
	void FreeMemory(void* data)
	{
		MemoryBlock* block = (MemoryBlock*)((char*)data - sizeof(MemoryBlock));

		assert(block->ref_count_ == 1);

		if (--block->ref_count_ != 0)
		{
			return;
		}

		if (block->is_in_pool_)
		{
			std::lock_guard<std::mutex> lock(alloc_mutex_);

			// 将内存放回内存池
			// 要释放的内存放在头部之前
			// 并将该块内存作为新的头部
			block->next_ = header_;
			header_ = block;
		}
		else
		{
			free(block);
		}
	}

	// 初始化
	void InitMemory()
	{
		// 断言
		assert(!buffer_);

		// 计算内存池的大小
		size_t buffer_size = (block_size_ + sizeof(MemoryBlock)) * block_count_;

		// 向系统申请池内存
		buffer_ = (char*)malloc(buffer_size);

		if (buffer_)
		{
			// 初始化每个内存块单元描述信息
			header_ = (MemoryBlock*)buffer_;
			header_->alloc_ = this;
			header_->id_ = 0;
			header_->is_in_pool_ = true;
			header_->next_ = nullptr;
			header_->ref_count_ = 0;

			// 遍历内存块进行初始化
			MemoryBlock* temp = header_;
			for (size_t i = 1; i < block_count_; i++)
			{
				MemoryBlock* new_block = (MemoryBlock*)(buffer_ + (i * (block_size_ + sizeof(MemoryBlock))));
				new_block->alloc_ = this;
				new_block->id_ = i;
				new_block->is_in_pool_ = true;
				new_block->next_ = nullptr;
				new_block->ref_count_ = 0;

				temp->next_ = new_block;
				temp = new_block;
			}
		}
	}

protected:
	char* buffer_ = nullptr; // 内存池地址
	MemoryBlock* header_ = nullptr; // 头部内存块地址
	size_t block_size_ = 0; // 内存单元块大小
	size_t block_count_ = 0; // 内存单元块数量
	std::mutex alloc_mutex_;
};

template<size_t block_size, size_t block_count>
class MemoryAlloctor :public MemoryAlloc
{
public:
	MemoryAlloctor()
	{
		// 内存对齐的字节数
		const size_t size = sizeof(void*);
		block_size_ = (block_size / size) * size + ((block_size % size > 0) ? size : 0);

		block_count_ = block_count;
	}

	virtual ~MemoryAlloctor()
	{

	}
};

// 内存管理工具
class MemoryMgr
{
private:
	MemoryMgr()
	{
		InitAlloc(0, 64, &mem_alloctor_64_);
		InitAlloc(65, 128, &mem_alloctor_128_);
		InitAlloc(129, 256, &mem_alloctor_256_);
		InitAlloc(257, 512, &mem_alloctor_512_);
		InitAlloc(513, 1024, &mem_alloctor_1024_);
	}

	~MemoryMgr() = default;

	// 初始化内存池映射数组
	void InitAlloc(int begin, int end, MemoryAlloc* mem_alloc)
	{
		for (auto& alloc : alloc_array_)
		{
			alloc = mem_alloc;
		}
	}

public:
	// 删除拷贝构造函数
	MemoryMgr(const MemoryMgr&) = delete;

	// 删除复制构造函数
	MemoryMgr& operator = (const MemoryMgr&) = delete;

	// 获取单例
	static MemoryMgr& GetInstance()
	{
		static MemoryMgr instance_;

		return instance_;
	}

	// 申请内存
	void* Alloc(size_t size)
	{
		if (size <= kMaxMemorySize)
		{
			return alloc_array_[size]->AllocMemory(size);
		}

		MemoryBlock* block = (MemoryBlock*)malloc(size + sizeof(MemoryBlock));
		if (block)
		{
			block->alloc_ = nullptr;
			block->id_ = -1;
			block->is_in_pool_ = false;
			block->next_ = nullptr;
			block->ref_count_ = 1;

			xprintf("AllocMemory : %p, id<%d>, size<%d>\n", block, block->id_, int(size));
			return (char*)block + sizeof(MemoryBlock);
		}

		return nullptr;
	}

	// 释放内存
	void Free(void* data)
	{
		MemoryBlock* block = (MemoryBlock*)((char*)data - sizeof(MemoryBlock));
		xprintf("FreeMemory : %p, id<%d>\n", block, block->id_);

		if (block->is_in_pool_)
		{
			block->alloc_->FreeMemory(data);
		}
		else
		{
			if (--block->ref_count_ == 0)
			{
				free(block);
			}
		}
	}

	// 增加引用计数
	void AddRefCount(void* data)
	{
		MemoryBlock* block = (MemoryBlock*)((char*)data - sizeof(MemoryBlock));
		block->ref_count_++;
	}

private:
	MemoryAlloctor<64, 10000> mem_alloctor_64_;
	MemoryAlloctor<128, 10000> mem_alloctor_128_;
	MemoryAlloctor<256, 10000> mem_alloctor_256_;
	MemoryAlloctor<512, 10000> mem_alloctor_512_;
	MemoryAlloctor<1024, 10000> mem_alloctor_1024_;
	MemoryAlloc* alloc_array_[kMaxMemorySize + 1];
};

#endif // !__MEMORY_MGR_HPP__
