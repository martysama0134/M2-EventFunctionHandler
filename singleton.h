#ifndef __INC_SINGLETON_H__
#define __INC_SINGLETON_H__
#pragma once

#include <cassert>

#if !defined(WIN32) && !defined(__forceinline)
#define __forceinline __attribute__((always_inline))
#endif

template <typename T> class CSingleton
{
	static inline T * ms_singleton = nullptr;

public:
	CSingleton()
	{
		assert(!ms_singleton);
		ms_singleton = static_cast<T*>(this);
	}

	virtual ~CSingleton()
	{
		assert(ms_singleton);
		ms_singleton = nullptr;
	}

	__forceinline static T & Instance()
	{
		assert(ms_singleton);
		return (*ms_singleton);
	}

	__forceinline static T * InstancePtr()
	{
		return (ms_singleton);
	}

	__forceinline static T & instance()
	{
		assert(ms_singleton);
		return (*ms_singleton);
	}

	__forceinline static T * instance_ptr()
	{
		return (ms_singleton);
	}

	// prevent manager 0x0 by deleting copy/assignment operators
	CSingleton(const CSingleton&) = delete;
	CSingleton& operator=(const CSingleton&) = delete;
	CSingleton(CSingleton&&) = delete;
	CSingleton& operator=(CSingleton&&) = delete;
};

template <typename T>
using singleton = CSingleton<T>;

#endif
