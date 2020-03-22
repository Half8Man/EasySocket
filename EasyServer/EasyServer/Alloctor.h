#ifndef __ALLOC_H__
#define __ALLOC_H__

// ����new�����
void* operator new(size_t size);

// ����delete�����
void operator delete(void* block);

// ����new[]�����
void* operator new[](size_t size);

// ����delete[]�����
void operator delete[](void* block);

// ��װmalloc����
void* mem_alloc(size_t size);

// ��װfree����
void mem_free(void* block);

#endif // __ALLOC_H__

