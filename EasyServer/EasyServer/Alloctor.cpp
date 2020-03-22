#include "Alloctor.h"
#include "MemoryMgr.hpp"

#include <stdlib.h>

void* operator new(size_t size)
{
	return MemoryMgr::GetInstance().Alloc(size);
}

void operator delete(void* block)
{
	MemoryMgr::GetInstance().Free(block);
}

void* operator new[](size_t size)
{
	return MemoryMgr::GetInstance().Alloc(size);
}

void operator delete[](void* block)
{
	MemoryMgr::GetInstance().Free(block);
}

void* mem_alloc(size_t size)
{
	return ::malloc(size);
}

void mem_free(void* block)
{
	::free(block);
}
