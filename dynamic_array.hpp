#ifndef DYNAMIC_ARRAY_HPP
#define DYNAMIC_ARRAY_HPP

#include <Windows.h>
#include <new>
#include <utility>
#include <stdexcept>
#include <initializer_list>

using __size = unsigned __int64;

template <typename array_type>
class dynamic_array
{
private:
	using __value      = array_type;
	using __address    = array_type*;
	using __lvalue_ref = array_type&;
	using __rvalue_ref = array_type&&;

	using __exit_val   = signed int;

public:
	using value_type      = __value;
	using pointer 	      = __address;
	using const_reference = const __lvalue_ref;
	using reference       = __lvalue_ref;

	using iterator = __address;

private:
	__address _head     = nullptr;
	__address _tail     = nullptr;
	__address _inserter = nullptr;

public:
	dynamic_array() noexcept = default;

	dynamic_array(std::initializer_list<__value> initializer_array)
	{
		__exit_val _result = _allocate_(initializer_array.size());

		if (_result != 0)
			throw std::runtime_error("dynamic_array(std::initializer_list<array_type>)->_allocate_(_new_capacity): FAILED TO ALLOCATE NEW MEMORY CHUNK ON HEAP!");

		for (const __value* _iterator = initializer_array.begin(); _iterator != initializer_array.end(); ++_iterator)
		{
			new(_inserter) __value(*_iterator);
			++_inserter;
		}
	}

	dynamic_array(const dynamic_array& initializer_array)
	{
		__exit_val _result = _allocate_(initializer_array.capacity());

		if (_result != 0)
			throw std::runtime_error("dynamic_array(const dynamic_array&)->_allocate_(_new_capacity): FAILED TO ALLOCATE NEW MEMORY CHUNK ON HEAP!");

		for (__size i = 0; i < initializer_array.size(); ++i)
		{
			new(_inserter) __value(initializer_array[i]);
			++_inserter;
		}
	}

	dynamic_array(dynamic_array&& initializer_array) noexcept(false)
	{
		__exit_val _result = _allocate_(initializer_array.capacity());

		if (_result != 0)
			throw std::runtime_error("dynamic_array(dynamic_array&&)->_allocate_(_new_capacity): FAILED TO ALLOCATE NEW MEMORY CHUNK ON HEAP!");

		for (__size i = 0; i < initializer_array.size(); ++i)
		{
			new(_inserter) __value(std::move(initializer_array[i]));
			++_inserter;
		}
	}

	dynamic_array(const __size new_capacity)
	{
		__exit_val _result = _allocate_(new_capacity);

		if (_result != 0)
			throw std::runtime_error("dynamic_array(new_capacity)->_allocate_(_new_capacity): FAILED TO ALLOCATE NEW MEMORY CHUNK ON HEAP!");
	}

	template <typename... _rvalues>
	dynamic_array(__rvalue_ref initializer_value, _rvalues&&... initializer_arguments)
	{
		this->pack_back(std::move(initializer_value), std::move(initializer_arguments)...);
	}

	dynamic_array(__address from, __address to, bool from_rbegin)
	{
		if (from_rbegin != true)
		{
			__size _capacity = static_cast<__size>(to - from);

			__exit_val _result = _allocate_(_capacity);

			if (_result != 0)
				throw std::runtime_error("dynamic_array(from, to, from_rbegin)->_allocate_(_new_capacity): FAILED TO ALLOCATE NEW MEMORY CHUNK ON HEAP!");

			while (from != to)
			{
				new(_inserter) __value(*from);
				++_inserter;
				++from;
			}
		}

		else
		{
			__size _capacity = static_cast<__size>(from - to);

			__exit_val _result = _allocate_(_capacity);

			if (_result != 0)
				throw std::runtime_error("dynamic_array(from, to, from_rbegin)->_allocate_(_new_capacity): FAILED TO ALLOCATE NEW MEMORY CHUNK ON HEAP!");

			while (from != to)
			{
				new(_inserter) __value(*from);
				++_inserter;
				--from;
			}
		}
	}

	~dynamic_array() noexcept(false)
	{
		__exit_val _result = _destroy_();

		if (_result != 0)
			throw std::runtime_error("~dynamic_array->_destroy_(): FAILED TO FREE OLD MEMORY!");
	}

private:
	[[nodiscard]] __exit_val _allocate_(const __size _new_capacity)
	{
		if (_head != nullptr)
		{
			__exit_val _result = _destroy_();

			if (_result != 0)
				return _result;
		}

		_head = reinterpret_cast<__address>(HeapAlloc(GetProcessHeap(), 0, (_new_capacity * sizeof(__value))));

		if (_head == nullptr)
			return -1;

		_tail     = _head + _new_capacity;
		_inserter = _head;

		return 0;
	}

	[[nodiscard]] __exit_val _reallocate_(const __size _new_capacity)
	{
		__address _new_head = reinterpret_cast<__address>(HeapAlloc(GetProcessHeap(), 0, (_new_capacity * sizeof(__value))));

		if (_new_head == nullptr)
			return -1;

		__address _temp_tail     = _new_head + _new_capacity;
		__address _temp_inserter = _new_head;

		if ((static_cast<__size>(_tail - _head) > _new_capacity) && (static_cast<__size>(_inserter - _head) >= _new_capacity))
		{
			for (__size i = 0; i < _new_capacity; ++i)
			{
				new(_temp_inserter) __value(std::move(*(_head + i)));
				++_temp_inserter;
			}

			--_inserter;

			while (_inserter != _head - 1)
			{
				_inserter->~__value();
				--_inserter;
			}

			_tail     = _temp_tail;
			_inserter = _temp_inserter;
		}

		else
		{
			for (__size i = 0; i < static_cast<__size>(_inserter - _head); ++i)
			{
				new(_temp_inserter) __value(std::move(*(_head + i)));
				++_temp_inserter;
				(_head + i)->~__value();
			}

			_tail     = _temp_tail;
			_inserter = _temp_inserter;
		}

		BOOL _result = HeapFree(GetProcessHeap(), 0, _head);

		if (_result != TRUE)
			return -2;

		_head = _new_head;

		return 0;
	}

	[[nodiscard]] __exit_val _destroy_()
	{
		if (_inserter != _head)
		{
			--_inserter;

			while (_inserter != _head - 1)
			{
				_inserter->~__value();
				--_inserter;
			}
		}

		_inserter = nullptr;
		_tail = nullptr;

		BOOL _result = HeapFree(GetProcessHeap(), 0, _head);

		if (_result != TRUE)
			return -2;

		_head = nullptr;

		return 0;
	}

public:
	void push_back(__rvalue_ref x_value) noexcept(false)
	{
		if (_head == nullptr)
		{
			__exit_val _result = _allocate_(4UI64);

			if (_result != 0)
			{
				throw std::runtime_error("push_back(array_type&&)->_allocate_(_new_capacity): FAILED TO ALLOCATE NEW MEMORY CHUNK ON HEAP!");
				return;
			}

			new(_inserter) __value(std::move(x_value));
			++_inserter;
		}

		else if (_inserter != _tail)
		{
			new(_inserter) __value(std::move(x_value));
			++_inserter;
		}

		else
		{
			__exit_val _result = _reallocate_((2UI64 * static_cast<__size>(_tail - _head)));

			if (_result == -1)
			{
				throw std::runtime_error("push_back(array_type&&)->_reallocate_(_new_capacity): FAILED TO ALLOCATE NEW MEMORY CHUNK ON HEAP!");
				return;
			}

			else if (_result == -2)
			{
				throw std::runtime_error("push_back(array_type&&)->_reallocate_(_new_capacity): FAILED TO FREE OLD MEMORY!");
				return;
			}

			else
			{
				new(_inserter) __value(std::move(x_value));
				++_inserter;
			}
		}
	}

	void push_back(const __lvalue_ref x_value)
	{
		if (_head == nullptr)
		{
			__exit_val _result = _allocate_(4UI64);

			if (_result != 0)
			{
				throw std::runtime_error("push_back(const array_type&)->_allocate_(_new_capacity): FAILED TO ALLOCATE NEW MEMORY CHUNK ON HEAP!");
				return;
			}

			new(_inserter) __value(x_value);
			++_inserter;
		}

		else if (_inserter != _tail)
		{
			new(_inserter) __value(x_value);
			++_inserter;
		}

		else
		{
			__exit_val _result = _reallocate_((2UI64 * static_cast<__size>(_tail - _head)));

			if (_result == -1)
			{
				throw std::runtime_error("push_back(const array_type&)->_reallocate_(_new_capacity): FAILED TO ALLOCATE NEW MEMORY CHUNK ON HEAP!");
				return;
			}

			else if (_result == -2)
			{
				throw std::runtime_error("push_back(const array_type&)->_reallocate_(_new_capacity): FAILED TO FREE OLD MEMORY!");
				return;
			}

			else
			{
				new(_inserter) __value(x_value);
				++_inserter;
			}
		}
	}

	template <typename... __arguments>
	void construct_back(__arguments&&... constructor_args)
	{
		if (_head == nullptr)
		{
			__exit_val _result = _allocate_(4UI64);

			if (_result != 0)
			{
				throw std::runtime_error("construct_back(__arguments&&...)->_allocate_(_new_capacity): FAILED TO ALLOCATE NEW MEMORY CHUNK ON HEAP!");
				return;
			}

			new(_inserter) __value(std::forward<__arguments>(constructor_args)...);
			++_inserter;
		}

		else if (_inserter != _tail)
		{
			new(_inserter) __value(std::forward<__arguments>(constructor_args)...);
			++_inserter;
		}

		else
		{
			__exit_val _result = _reallocate_((2UI64 * static_cast<__size>(_tail - _head)));

			if (_result == -1)
			{
				throw std::runtime_error("construct_back(__arguments&&...)->_reallocate_(_new_capacity): FAILED TO ALLOCATE NEW MEMORY CHUNK ON HEAP!");
				return;
			}

			else if (_result == -2)
			{
				throw std::runtime_error("construct_back(__arguments&&...)->_reallocate_(_new_capacity): FAILED TO FREE OLD MEMORY!");
				return;
			}

			else
			{
				new(_inserter) __value(std::forward<__arguments>(constructor_args)...);
				++_inserter;
			}
		}
	}

	void pack_back(__rvalue_ref x_value)
	{
		this->push_back(std::move(x_value));
	}

	template <typename... __rvalues>
	void pack_back(__rvalue_ref x_value, __rvalues&&... x_arguments)
	{
		this->push_back(std::move(x_value));

		this->pack_back(std::move(x_arguments)...);
	}

	[[nodiscard]] __address data() const
	{
		return _head;
	}

	[[nodiscard]] __address const* address() const
	{
		return &_head;
	}

	[[nodiscard]] __size size() const
	{
		return static_cast<__size>(_inserter - _head);
	}

	[[nodiscard]] __size capacity() const
	{
		return static_cast<__size>(_tail - _head);
	}

	[[nodiscard]] __address begin() const
	{
		return _head;
	}

	[[nodiscard]] __address end() const
	{
		return _inserter;
	}

	[[nodiscard]] __address rbegin() const
	{
		return (_inserter - 1);
	}

	[[nodiscard]] __address rend() const
	{
		return (_head - 1);
	}

	[[nodiscard]] __value first() const
	{
		return *_head;
	}

	[[nodiscard]] __value first()
	{
		return *_head;
	}

	[[nodiscard]] __value last() const
	{
		return *(_inserter - 1);
	}

	[[nodiscard]] __value last()
	{
		return *(_inserter - 1);
	}

	[[nodiscard]] const __lvalue_ref operator[] (const __size position) const
	{
		if (position >= static_cast<__size>(_inserter - _head))
			throw std::runtime_error("dynamic_array->operator[position]: ACCESSING OUT OF RANGE!");

		else
			return *(_head + position);
	}

	[[nodiscard]] __lvalue_ref operator[] (const __size position)
	{
		if (position >= static_cast<__size>(_inserter - _head))
			throw std::runtime_error("dynamic_array->operator[position]: ACCESSING OUT OF RANGE!");

		else
			return *(_head + position);
	}

	const dynamic_array& operator= (const dynamic_array& x_array)
	{
		if (&_head == x_array.address())
			return *this;

		__exit_val _result = _allocate_(x_array.capacity());

		if (_result == -2)
		{
			throw std::runtime_error("dynamic_array->operator= (const dynamic_array&)->_allocate_(_new_capacity): FAILED TO FREE OLD MEMORY!");
			return *this;
		}

		if (_result == -1)
		{
			throw std::runtime_error("dynamic_array->operator= (const dynamic_array&)->_allocate_(_new_capacity): FAILED TO ALLOCATE NEW MEMORY CHUNK ON HEAP!");
			return *this;
		}

		for (__size i = 0; i < x_array.size(); ++i)
		{
			new(_inserter) __value(x_array[i]);
			++_inserter;
		}

		return *this;
	}

	const dynamic_array& operator= (dynamic_array&& x_array) noexcept(false)
	{
		if (&_head == x_array.address())
			return *this;

		__exit_val _result = _allocate_(x_array.capacity());

		if (_result == -2)
		{
			throw std::runtime_error("dynamic_array->operator= (dynamic_array&&)->_allocate_(_new_capacity): FAILED TO FREE OLD MEMORY!");
			return *this;
		}

		if (_result == -1)
		{
			throw std::runtime_error("dynamic_array->operator= (dynamic_array&&)->_allocate_(_new_capacity): FAILED TO ALLOCATE NEW MEMORY CHUNK ON HEAP!");
			return *this;
		}

		for (__size i = 0; i < x_array.size(); ++i)
		{
			new(_inserter) __value(std::move(x_array[i]));
			++_inserter;
		}

		return *this;
	}
};

#endif /* DYNAMIC_ARRAY_HPP */
