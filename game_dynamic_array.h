/* date = October 5th 2023 8:34 pm */

#ifndef GAME_DYNAMIC_ARRAY_H

#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <initializer_list>
#include <utility>

template<class T>
struct Array
{
	typedef int32_t bool32;
	typedef T DataType;
	typedef DataType* Iterator;
	typedef const DataType* ConstIterator;
    
	Array() {}
	Array(std::initializer_list<DataType> list) { push_range(list.begin(), list.end()); }
	~Array() { if (data) free(data); } 
    
	Array(const Array<DataType>& source) 
		: size(source.size), cap(source.cap)
	{
		data = alloc_and_zero(cap * sizeof(DataType));
		size_t copy_size = size * sizeof(DataType);
		memcpy(data, source.data, copy_size);
	}
    
	Array<DataType>& operator=(const Array<DataType>& source)
	{
		if (data) free(data);
		size = source.size;
		cap = source.cap;
		data = alloc_and_zero(cap * sizeof(DataType));
		size_t copy_size = size * sizeof(DataType);
		memcpy(data, source.data, copy_size);
		return *this;
	}
    
	Array(Array<DataType>&& source)
		: size(source.size), cap(source.cap), data(source.data)
	{
		source.size = 0;
		source.cap  = 0;
		source.data = nullptr;
	}
    
	Array<DataType>& operator=(Array<DataType>&& source) 
	{
		size = source.size;
		cap = source.cap;
		if (data) free(data);
		data = source.data;
		source.size = 0;
		source.cap = 0;
		source.data = nullptr;
		return *this;
	}
    
	DataType* alloc_and_zero(size_t alloc_size)
	{
		DataType* result = static_cast<DataType*>(malloc(alloc_size));
		assert(result);
		memset(result, 0, alloc_size);
		return result;
	}
    
	void grow()
	{
		size_t new_cap = (size * 2 + 1);
		DataType* new_data = alloc_and_zero(new_cap * sizeof(DataType));
		size_t copy_size = size * sizeof(DataType);
		memcpy(new_data, data, copy_size);
		if (data) free(data);
		data = new_data;
		cap = new_cap;
	}
    
	void fit(size_t count) { if (count > cap) grow(); }
	void erase_range(Iterator range_start, Iterator range_end)
	{
		assert(range_start >= begin() && range_start < end());
		assert(range_end >= begin() && range_end < end());
		if (range_start < range_end)
		{
			uintptr_t erase_count = (range_end - range_start);
			for (Iterator at = range_end; at != end(); ++at)
				*range_start++ = *at;
			size -= erase_count;
		}
	}
    
	void erase(Iterator itr)
	{
		assert(itr >= begin() && itr < end());
		for (Iterator at = itr; at != end(); ++at, ++itr)
			if(at != (end() - 1))
				*at = *(itr + 1);
		--size;
	}
    
	void push_back(const DataType& value) { fit(size + 1); data[size] = value; ++size; } 
    // NOTE(alexey): Don't care about moving???
	void push_back(DataType&& value) { fit(size + 1); data[size] = std::move(value); ++size; }
    
	void push_range(ConstIterator begin, ConstIterator end)
	{
		for (ConstIterator at = begin;
             at != end; 
             ++at)
		{
			push_back(*at);
		}
	}
    
	void pop_back() { --size; } 
    
    // TODO(alexey): Think more how to handle the situation when the size equal to 0.
    // So we don't get a negative index.
	const DataType& back() { assert(size); return data[size - 1]; } 
	size_t length() const { return size; }
	size_t capacity() const {  return cap; } 
	void clear() { size = 0; }
	bool32 empty() const { return !length(); }
	DataType& operator[](int32_t index) { assert((index >= 0) && (index < size)); return data[index]; } 
    
	const DataType& operator[](int32_t index) const { assert((index >= 0) && (index < size)); return data[index]; } 
    
	Iterator begin() { return &data[0]; } 
	Iterator end() { return &data[size]; } 
	ConstIterator begin() const { return &data[0]; } 
	ConstIterator end() const { return &data[size]; } 
    
	size_t size{ 0 };
	size_t cap{ 0 };
	DataType* data{ nullptr };
};

#define GAME_DYNAMIC_ARRAY_H
#endif //GAME_DYNAMIC_ARRAY_H
