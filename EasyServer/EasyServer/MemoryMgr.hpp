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

// �ڴ��
class MemoryBlock
{
public:
	MemoryBlock():id_(0), ref_count_(0), alloc_(nullptr), next_(nullptr), is_in_pool_(false)
	{

	}

	~MemoryBlock()
	{

	}

	int id_ = 0; // �ڴ��id
	int ref_count_ = 0; // ���ô���
	MemoryAlloc* alloc_ = nullptr; // �����ڴ��
	MemoryBlock* next_ = nullptr; // ��һ��λ��
	bool is_in_pool_ = false; // �Ƿ����ڴ����
	char reserve_[3] = {}; // Ԥ���ֶ�
};

// �ڴ��
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

	// �����ڴ�
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

	// �ͷ��ڴ�
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

			// ���ڴ�Ż��ڴ��
			// Ҫ�ͷŵ��ڴ����ͷ��֮ǰ
			// �����ÿ��ڴ���Ϊ�µ�ͷ��
			block->next_ = header_;
			header_ = block;
		}
		else
		{
			free(block);
		}
	}

	// ��ʼ��
	void InitMemory()
	{
		// ����
		assert(!buffer_);

		// �����ڴ�صĴ�С
		size_t buffer_size = (block_size_ + sizeof(MemoryBlock)) * block_count_;

		// ��ϵͳ������ڴ�
		buffer_ = (char*)malloc(buffer_size);

		if (buffer_)
		{
			// ��ʼ��ÿ���ڴ�鵥Ԫ������Ϣ
			header_ = (MemoryBlock*)buffer_;
			header_->alloc_ = this;
			header_->id_ = 0;
			header_->is_in_pool_ = true;
			header_->next_ = nullptr;
			header_->ref_count_ = 0;

			// �����ڴ����г�ʼ��
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
	char* buffer_ = nullptr; // �ڴ�ص�ַ
	MemoryBlock* header_ = nullptr; // ͷ���ڴ���ַ
	size_t block_size_ = 0; // �ڴ浥Ԫ���С
	size_t block_count_ = 0; // �ڴ浥Ԫ������
	std::mutex alloc_mutex_;
};

template<size_t block_size, size_t block_count>
class MemoryAlloctor :public MemoryAlloc
{
public:
	MemoryAlloctor()
	{
		// �ڴ������ֽ���
		const size_t size = sizeof(void*);
		block_size_ = (block_size / size) * size + ((block_size % size > 0) ? size : 0);

		block_count_ = block_count;
	}

	virtual ~MemoryAlloctor()
	{

	}
};

// �ڴ������
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

	// ��ʼ���ڴ��ӳ������
	void InitAlloc(int begin, int end, MemoryAlloc* mem_alloc)
	{
		for (auto& alloc : alloc_array_)
		{
			alloc = mem_alloc;
		}
	}

public:
	// ɾ���������캯��
	MemoryMgr(const MemoryMgr&) = delete;

	// ɾ�����ƹ��캯��
	MemoryMgr& operator = (const MemoryMgr&) = delete;

	// ��ȡ����
	static MemoryMgr& GetInstance()
	{
		static MemoryMgr instance_;

		return instance_;
	}

	// �����ڴ�
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

	// �ͷ��ڴ�
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

	// �������ü���
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
