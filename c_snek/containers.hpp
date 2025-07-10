#pragma once

#include <stdlib.h>
#include <string.h>

#ifndef MIN(x,y)
#define MIN(x,y) ((x > y)?(y):(x))
#endif
#define MUL_KB 1024
namespace dimanari
{
	constexpr size_t Align(size_t s)
	{
		constexpr size_t alignment = size_t(-8);
		return (s + 7) & alignment;
	}

	class Stack
	{
		size_t size;
		size_t cap;
		char* data;
	public:
		Stack() : size(0), cap(0), data(nullptr) {}

		void Init(size_t _cap)
		{
			size = 0;
			cap = Align(_cap);
			data = new char[cap];
		}

		void Copy(const Stack& other)
		{
			cap = other.cap;
			size = other.size;
			data = new char[cap];
			memcpy(data, other.data, cap);
		}

		void Release()
		{
			delete data;
			data = nullptr;
			cap = 0;
		}

		void Resize(size_t _cap)
		{
			_cap = Align(_cap);
			if (_cap != cap)
				return;
			void* temp = new char[_cap];
			memcpy(temp, data, cap);
			delete data;
			data = (char*)temp;
			cap = _cap;
		}

		void* RAW() const
		{
			return (void*)data;
		}

		inline size_t Size() const
		{
			return size;
		}

		void* Allocate(size_t amount)
		{
			size_t offset = size;
			amount = Align(amount);
			if (size + amount > cap)
				return nullptr;
			size += amount;
			return data + offset;
		}

		void* Release(size_t amount)
		{
			size_t offset = size;
			amount = Align(amount);
			if (size < amount)
				amount = size;
			size -= amount;
			return data + size;
		}
	};


	template<size_t cap = 1024>
	class TStack
	{
		size_t size;
		char data[cap];
	public:
		void* RAW() const
		{
			return (void*)data;
		}

		TStack() : size(0), data("") {}

		inline size_t Size() const
		{
			return size;
		}

		void* Allocate(size_t amount)
		{
			size_t offset = size;
			amount = Align(amount);
			if (size + amount > cap)
				return nullptr;
			size += amount;
			return data + offset;
		}

		void* Release(size_t amount)
		{
			size_t offset = size;
			amount = Align(amount);
			if (size < amount)
				amount = size;
			size -= amount;
			return data + size;
		}
	};

	template<class T> class Vector
	{
	protected:
		size_t cap;
		size_t size;
		T* data;
		int SetSize(size_t new_size)
		{
			if (new_size == cap)
				return 0;
			T* temp = (T*)malloc(sizeof(T) * new_size);
			if (nullptr == temp)
				return -1;
			size_t copy_size = MIN(size, new_size) * sizeof(T);
			size_t zero_size = new_size * sizeof(T) - copy_size;
			memcpy(temp, data, copy_size);
			memset((char*)temp + copy_size, 0, zero_size);
			free(data);
			data = temp;
			cap = new_size;

			return 0;
		}
		int IncreaseCap()
		{
			return SetSize(cap * 2);
		}
	public:
		inline size_t Size() const
		{
			return size;
		}

		inline size_t Capacity() const
		{
			return cap;
		}

		void Clear()
		{
			size = 0;
		}

		void ClearCplx()
		{
			while (size)
			{
				data[--size].~T();
			}
		}

		Vector(size_t _cap = 2) : cap(_cap), size(0), data(nullptr)
		{
			data = (T*)malloc(sizeof(T) * cap);
			memset(data, 0, sizeof(T) * cap);
		}
		Vector(const Vector<T>& other) : cap(other.cap), size(other.size), data(nullptr)
		{
			data = (T*)malloc(sizeof(T) * cap);
			memcpy(data, other.data, sizeof(T) * size);
		}

		const Vector<T>& operator=(const Vector<T>& other)
		{
			SetSize(other.cap);
			size = other.size;
			memcpy(data, other.data, sizeof(T) * size);

			return *this;
		}

		~Vector()
		{
			free(data);
			data = nullptr;
		}

		T& At(size_t index)
		{
			if (size <= index)
				return *((T*)nullptr);
			return data[index];
		}
		const T& At(size_t index) const
		{
			if (size <= index)
				return *((T*)nullptr);
			return data[index];
		}

		T& Peek()
		{
			return At(size - 1);
		}
		const T& Peek() const
		{
			return At(size - 1);
		}

		T& operator[](size_t index)
		{
			return At(index);
		}
		const T& operator[](size_t index) const
		{
			return At(index);
		}

		int PushBack(const T& value)
		{
			if (cap == size)
			{
				if (IncreaseCap())
					return -1;
			}
			data[size] = value;
			return size++;
		}

		const const T& PopBack()
		{
			if (0 == size)
			{
				return *((T*)nullptr);
			}
			return data[--size];
		}

		const void PopBackCplx()
		{
			if (0 == size)
			{
				return;// *((T*)nullptr);
			}
			data[--size].~T();
		}

		int ShrinkToFit()
		{
			SetSize(size);
		}
	};


	class noncopyable
	{
	public:
		noncopyable() = default;
		noncopyable(const noncopyable&) = delete;
		noncopyable& operator=(const noncopyable&) = delete;
	};

	template<class T>
	class Singleton : private noncopyable
	{
	public:
		static T* GetInstance()
		{
			static T* temp = new T;
			return temp;
		}
	private:
		Singleton() = delete;
	};

}