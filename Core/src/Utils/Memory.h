#pragma once
#include <utility>

#ifdef DEBUG_CONFIG
	inline uint32_t AllocatedRefCount = 0;
	inline uint32_t AllocatedScopeRefCount = 0;

	#define ALLOCATED_REF_COUNT AllocatedRefCount
	#define ALLOCATED_SCOPEREF_COUNT AllocatedScopeRefCount

	#define UNDEFINED_PTR 3722304989
#else
	#define ALLOCATED_REF_COUNT
	#define ALLOCATED_SCOPEREF_COUNT

	#define UNDEFINED_PTR 0
#endif

template<typename T>
class ScopeRef
{
public:
	ScopeRef() = default;
	ScopeRef(T* _ptr) : ptr(_ptr)
	{
#ifdef DEBUG_CONFIG
		AllocatedScopeRefCount++;
#endif
	}

	~ScopeRef()
	{
		delete ptr;
#ifdef DEBUG_CONFIG
		AllocatedScopeRefCount--;
#endif
	}

	ScopeRef(const ScopeRef&) = delete;

	operator bool() const { return ptr; }
	operator T* () const { return ptr; }
	T* operator->() const { return ptr; }
	T& operator*() const { return *ptr; }

	inline T* GetPtr() const { return ptr; }
	inline T& Get() const { return *ptr; }

	ScopeRef& operator=(const ScopeRef&) = delete;
private:
	T* ptr = nullptr;
};

template<typename T>
class Ref
{
public:
	Ref(T* _ptr = nullptr) : ptr(_ptr), refs(new uint32_t(1))
	{
#ifdef DEBUG_CONFIG
		AllocatedRefCount++;
#endif
	}

	Ref(T* _ptr, uint32_t* _refs) : ptr(_ptr), refs(_refs) { ++(*refs); }
	Ref(const Ref& other) : ptr(other.ptr), refs(other.refs) { ++(*refs); }

	Ref(Ref&& other) : ptr(other.ptr), refs(other.refs)
	{
		other.ptr = nullptr;
		other.refs = nullptr;
	}

	~Ref() { release(); }

	operator bool() const { return ptr; }
	operator T* () const { return ptr; }
	T* operator->() const { return ptr; }
	T& operator*() const { return *ptr; }

	inline T* GetPtr() const { return ptr; }
	inline T& Get() const { return *ptr; }

	template<typename T2>
	inline Ref<T2> As() const { return Ref<T2>(ptr, refs); }

	inline const uint32_t& GetRefCount() const { return *refs; }

	Ref& operator=(const Ref& other) noexcept
	{
		if (this != &other)
		{
			release();
			ptr = other.ptr;
			refs = other.refs;
			++(*refs);
		}

		return *this;
	}

	Ref& operator=(Ref&& other) noexcept
	{
		if (this != &other)
		{
			release();
			ptr = other.ptr;
			refs = other.refs;
			other.ptr = nullptr;
			other.refs = nullptr;
		}

		return *this;
	}

	bool operator==(const Ref& other) const { return ptr == other.ptr; }
private:
	void release()
	{
		if (refs && --(*refs) == 0)
		{
			delete ptr;
			delete refs;

#ifdef DEBUG_CONFIG
			AllocatedRefCount--;
#endif
		}
	}

	T* ptr = nullptr;
	uint32_t* refs;
};

template<typename T>
class WeakRef
{
public:
	WeakRef() = default;
	WeakRef(const Ref<T>& ref) : ptr(ref.GetPtr()), refs((uint32_t*)&ref.GetRefCount()) {}
	~WeakRef() = default;

	operator bool() const { return IsValid(); }
	operator T* () const { return ptr; }
	T* operator->() const { return ptr; }
	T& operator*() const { return *ptr; }

	inline T* GetPtr() const { return ptr; }
	inline T& Get() const { return *ptr; }

	inline const bool IsValid() const { return refs && *refs != UNDEFINED_PTR; }
	Ref<T> Lock() const { return IsValid() ? Ref<T>(ptr, refs) : Ref<T>(nullptr); }
private:
	T* ptr = nullptr;
	uint32_t* refs = 0;
};

template<typename T, typename... Args>
[[nodiscard]] ScopeRef<T> CreateScopeRef(Args&&... args)
{
	return ScopeRef<T>(new T(std::forward<Args>(args)...));
}

template<typename T, typename... Args>
[[nodiscard]] Ref<T> CreateRef(Args&&... args)
{
	return Ref<T>(new T(std::forward<Args>(args)...));
}