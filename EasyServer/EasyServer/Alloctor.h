#ifndef __ALLOC_H__
#define __ALLOC_H__

// 重载new运算符
void* operator new(size_t size);

// 重载delete运算符
void operator delete(void* block);

// 重载new[]运算符
void* operator new[](size_t size);

// 重载delete[]运算符
void operator delete[](void* block);

// 封装malloc函数
void* mem_alloc(size_t size);

// 封装free函数
void mem_free(void* block);

#endif // __ALLOC_H__

