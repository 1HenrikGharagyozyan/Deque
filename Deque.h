#pragma once

#include <cstddef>
#include <iterator>
#include <memory>
#include <stdexcept>
#include <cassert>

template <typename _Ty>
class Deque
{
private:
	static constexpr std::size_t BLOCK_SIZE = 64;
	using Block = _Ty*;
	using Map = Block*;

	Map _map;
	std::size_t _size;
	size_t _map_size;
	size_t _start_block;
	std::size_t _start_offset;
	size_t _finish_block;
	std::size_t _finish_offset;

	void allocate_map(std::size_t n_blocks);
	void reallocate_map(bool add_to_front);
	void allocate_block(std::size_t index);
	void deallocate_block(std::size_t index);

	void resize_map(std::size_t new_map_size);
	void destroy_all();

	_Ty* block_pointer(std::size_t block, std::size_t offset);
	const _Ty* block_pointer(std::size_t block, std::size_t offset) const;

public:
	using value_type = _Ty;
	using pointer = _Ty*;
	using const_pointer = const _Ty*;
	using reference = _Ty&;
	using const_reference = const _Ty&;
	using size_type = std::size_t;
	using difference_type = std::ptrdiff_t;

	class Iterator
	{
	public:
		using iterator_category = std::random_access_iterator_tag;
		using value_type = _Ty;
		using difference_type = std::ptrdiff_t;
		using pointer = _Ty*;
		using reference = _Ty&;

		Iterator() noexcept;
		Iterator(Map map, size_type block, size_type offset);
		~Iterator();

		reference operator*() const;
		pointer operator->() const;

		Iterator& operator++();
		Iterator& operator++(int);

		Iterator& operator--();
		Iterator& operator--(int);

		Iterator operator+(difference_type n) const;
		Iterator operator-(difference_type n) const;

		Iterator& operator+=(difference_type n);
		Iterator& operator-=(difference_type n);

		reference operator[](difference_type n) const;

		difference_type operator-(const Iterator& rhs) const;

		bool operator==(const Iterator& rhs) const;
		bool operator!=(const Iterator& rhs) const;
		bool operator<(const Iterator& rhs) const;
		bool operator>(const Iterator& other) const;
		bool operator<=(const Iterator& other) const;
		bool operator>=(const Iterator& other) const;

	private:
		Map _map_ptr = nullptr;
		size_type _block = 0;
		size_type _offset = 0;

		friend class Deque<_Ty>;
		friend class Const_Iterator;
	};

	class Const_Iterator
	{
	public:
		using iterator_category = std::random_access_iterator_tag;
		using value_type = _Ty;
		using difference_type = std::ptrdiff_t;
		using pointer = const _Ty*;
		using reference = const _Ty&;

		Const_Iterator() noexcept;
		Const_Iterator(Map map, size_type block, size_type offset);
		Const_Iterator(const Iterator& it);
		~Const_Iterator();

		reference operator*() const;
		pointer operator->() const;

		Const_Iterator& operator++();
		Const_Iterator operator++(int);

		Const_Iterator& operator--();
		Const_Iterator operator--(int);

		Const_Iterator operator+(difference_type n) const;
		Const_Iterator operator-(difference_type n) const;

		Const_Iterator& operator+=(difference_type n);
		Const_Iterator& operator-=(difference_type n);

		difference_type operator-(const Const_Iterator& rhs) const;

		const_reference operator[](difference_type n) const;

		bool operator==(const Const_Iterator& rhs) const;
		bool operator!=(const Const_Iterator& rhs) const;
		bool operator<(const Const_Iterator& rhs) const;
		bool operator>(const Const_Iterator& rhs) const;
		bool operator<=(const Const_Iterator& rhs) const;
		bool operator>=(const Const_Iterator& rhs) const;

	private:
		const Map _map_ptr;
		size_type _block;
		size_type _offset;
	};


	using iterator = Iterator;
	using const_iterator = Const_Iterator;
	using reverse_iterator = std::reverse_iterator<Iterator>;
	using const_reverse_iterator = std::reverse_iterator<Const_Iterator>;

	Deque();
	Deque(size_type count, const_reference value = _Ty());
	Deque(std::initializer_list<value_type> init);
	Deque(const Deque& other);
	Deque(Deque&& other) noexcept;
	~Deque();

	Deque& operator=(const Deque& other);
	Deque& operator=(Deque&& other) noexcept;

	void assign(std::initializer_list<value_type> init);
	void assign(size_type count, const_reference value);

	void push_back(const_reference value);
	void push_front(const_reference value);

	void pop_back();
	void pop_front();

	template <typename... Args>
	reference emplace_back(Args&&... args);

	template <typename... Args>
	reference emplace_front(Args&&... args);

	template <typename... Args>
	iterator emplace(iterator pos, Args&&... args);

	reference front();
	const_reference front() const;
	reference back();
	const_reference back() const;

	void clear();

	reference operator[](size_type index);
	const_reference operator[](size_type index) const;
	reference at(size_type index);
	const_reference at(size_type index) const;

	size_type capacity() const noexcept;
	size_type size() const;
	bool empty() const;

	void resize(size_type new_size, const_reference value = _Ty());

	void swap(Deque& other);

	iterator insert(iterator pos, const_reference value);
	iterator erase(iterator pos);

	// ITERATOR 

	iterator begin();
	iterator end();
	const_iterator begin() const;
	const_iterator end() const;
	const_iterator cbegin() const;
	const_iterator cend() const;

	reverse_iterator rbegin();
	reverse_iterator rend();
	const_reverse_iterator rbegin() const;
	const_reverse_iterator rend() const;
	const_reverse_iterator crbegin() const;
	const_reverse_iterator crend() const;

	bool operator==(const Deque& other) const;
	bool operator!=(const Deque& other) const;

};

// IMPLEMENTATION

template<typename _Ty>
void Deque<_Ty>::allocate_map(std::size_t n_blocks)
{
	if (n_blocks < 8) // минимальный размер карты
		n_blocks = 8; 

	_map = static_cast<Map>(::operator new(n_blocks * sizeof(Block)));
	for (std::size_t i = 0; i < n_blocks; ++i)
		_map[i] = nullptr;

	_map_size = n_blocks;
}

template<typename _Ty>
void Deque<_Ty>::reallocate_map(bool add_to_front)
{
	size_type old_block_count = _finish_block - _start_block + 1;
	size_type new_map_size = std::max(static_cast<size_type>(8), _map_size * 2);

	Map new_map = static_cast<Map>(::operator new(new_map_size * sizeof(Block)));
	for (size_type i = 0; i < new_map_size; ++i)
		new_map[i] = nullptr;

	size_type new_start_block = add_to_front ? new_map_size / 4 : (new_map_size / 2 - old_block_count / 2);

	for (size_type i = 0; i < old_block_count; ++i)
		new_map[new_start_block + i] = _map[_start_block + i];

	::operator delete(_map);

	_map = new_map;
	_start_block = new_start_block;
	_finish_block = _start_block + old_block_count - 1;
	_map_size = new_map_size;
}

template<typename _Ty>
void Deque<_Ty>::allocate_block(std::size_t index)
{
	_map[index] = static_cast<Block>(::operator new(BLOCK_SIZE * sizeof(value_type)));
}

template<typename _Ty>
void Deque<_Ty>::deallocate_block(std::size_t index)
{
	::operator delete(static_cast<void*>(_map[index]));
	_map[index] = nullptr;
}

template<typename _Ty>
void Deque<_Ty>::resize_map(std::size_t new_map_size)
{
	Map new_map = static_cast<Map>(::operator new(new_map_size * sizeof(Block)));
	for (std::size_t i = 0; i < new_map_size; ++i)
		new_map[i] = nullptr;

	::operator delete(_map);
	_map = new_map;
	_map_size = new_map_size;
}

template<typename _Ty>
void Deque<_Ty>::destroy_all()
{
	if (_map)
	{
		for (size_type i = 0; i < _map_size; ++i)
		{
			if (_map[i])
			{
				for (size_type j = 0; j < BLOCK_SIZE; ++j)
					std::destroy_at(&_map[i][j]);
			}
			deallocate_block(i);
		}

		::operator delete(static_cast<void*>(_map));
		_map = nullptr;
	}
	_map_size = 0;
	_start_block = _start_offset = _finish_block = _finish_offset = 0;
}

template<typename _Ty>
inline _Ty* Deque<_Ty>::block_pointer(std::size_t block, std::size_t offset)
{
	return _map[block] + offset;
}

template<typename _Ty>
inline const _Ty* Deque<_Ty>::block_pointer(std::size_t block, std::size_t offset) const
{
	return _map[block] + offset;
}

template<typename _Ty>
inline Deque<_Ty>::Deque()
{
	allocate_map(8);
	_start_block = _map_size / 2;
	_start_offset = 0;
	_finish_block = _start_block;
	_finish_offset = 0;
	_size = 0;
	allocate_block(_start_block);
}

template<typename _Ty>
Deque<_Ty>::Deque(size_type count, const_reference value)
	: Deque()
{
	for (size_type i = 0; i < count; ++i)
		push_back(value);
}

template<typename _Ty>
Deque<_Ty>::Deque(std::initializer_list<value_type> init)
	: Deque()
{
	for (const auto& elem : init)
		push_back(elem);
}

template<typename _Ty>
inline Deque<_Ty>::Deque(const Deque& other)
	: Deque()
{
	for (const auto& elem : other)
		push_back(elem);
}

template<typename _Ty>
inline Deque<_Ty>::Deque(Deque&& other) noexcept
	: _map(other._map)
	, _map_size(other._map_size)
	, _start_block(other._start_block), _start_offset(other._start_offset)
	, _finish_block(other._finish_block), _finish_offset(other._finish_offset)
{
	other._map = nullptr;
	other._map_size = 0;
	other._start_block = 0;
	other._start_offset = 0;
	other._finish_block = 0;
	other._finish_offset = 0;
}

template<typename _Ty>
inline Deque<_Ty>::~Deque()
{
	clear();
	destroy_all();
}

template<typename _Ty>
Deque<_Ty>& Deque<_Ty>::operator=(const Deque& other)
{
	if (this == &other)
		return *this;

	destroy_all();
	allocate_map(other._map_size);

	for (size_type i = 0; i < other._size; ++i)
		push_back(other[i]);

	return *this;
}

template<typename _Ty>
Deque<_Ty>& Deque<_Ty>::operator=(Deque&& other) noexcept
{
	if (this != &other) 
	{
		// Освободить текущие ресурсы
		clear();
		destroy_all();

		// Переместить указатели
		_map = other._map;
		_map_size = other._map_size;
		_start_block = other._start_block;
		_start_offset = other._start_offset;
		_finish_block = other._finish_block;
		_finish_offset = other._finish_offset;
		_size = other._size;

		// Обнулить источник
		other._map = nullptr;
		other._map_size = 0;
		other._start_block = other._start_offset = 0;
		other._finish_block = other._finish_offset = 0;
		other._size = 0;
	}
	return *this;
}

template<typename _Ty>
void Deque<_Ty>::assign(std::initializer_list<value_type> init)
{
	clear();
	for (const auto& elem : init)
		push_back(elem);
}

template<typename _Ty>
void Deque<_Ty>::assign(size_type count, const_reference value)
{
	clear();
	for (size_type i = 0; i < count; ++i)
		push_back(value);
}

template<typename _Ty>
void Deque<_Ty>::push_back(const_reference value)
{
	if (_finish_offset == BLOCK_SIZE)
	{
		if (_finish_block + 1 >= _map_size)
			reallocate_map(false);

		if (_map[_finish_block + 1] == nullptr)
			allocate_block(_finish_block + 1);

		++_finish_block;
		_finish_offset = 0;
	}

	::new (static_cast<void*>(_map[_finish_block] + _finish_offset)) value_type(value);
	++_finish_offset;
	++_size;
}

template<typename _Ty>
void Deque<_Ty>::push_front(const_reference value)
{
	if (_start_offset == 0)
	{
		if (_start_block == 0)
			reallocate_map(true);

		if (_map[_start_block - 1] == nullptr)
			allocate_block(_start_block - 1);

		--_start_block;
		_start_offset = BLOCK_SIZE;
	}

	--_start_offset;
	::new (static_cast<void*>(_map[_start_block] + _start_offset)) value_type(value);
	++_size;
}

template<typename _Ty>
void Deque<_Ty>::pop_back()
{
	if (empty()) 
		throw std::out_of_range("Deque is empty!");

	if (_finish_offset == 0) 
	{
		// Переход на предыдущий блок
		_finish_block--;
		_finish_offset = BLOCK_SIZE - 1;
	}
	else
		--_finish_offset;

	std::destroy_at(block_pointer(_finish_block, _finish_offset));
	--_size;
}

template<typename _Ty>
void Deque<_Ty>::pop_front()
{
	if (empty()) 
		throw std::out_of_range("Deque is empty!");

	std::destroy_at(block_pointer(_start_block, _start_offset));

	if (++_start_offset == BLOCK_SIZE) 
	{
		deallocate_block(_start_block);
		++_start_block;
		_start_offset = 0;
	}

	--_size;
}

template<typename _Ty>
template<typename ...Args>
typename Deque<_Ty>::reference Deque<_Ty>::emplace_back(Args && ...args)
{
	if (_finish_offset == BLOCK_SIZE)
	{
		if (_finish_block + 1 >= _map_size)
			reallocate_map(false);

		if (_map[_finish_block + 1] == nullptr)
			allocate_block(_finish_block + 1);

		++_finish_block;
		_finish_offset = 0;
	}

	::new (static_cast<void*>(_map[_finish_block] + _finish_offset)) value_type(std::forward<Args>(args)...);
	++_size;
	return _map[_finish_block][_finish_offset++];
}

template<typename _Ty>
template<typename ...Args>
typename Deque<_Ty>::reference Deque<_Ty>::emplace_front(Args && ...args)
{
	if (_start_offset == 0)
	{
		if (_start_block == 0)
			reallocate_map(true);

		if (_map[_start_block - 1] == nullptr)
			allocate_block(_start_block - 1);

		--_start_block;
		_start_offset = BLOCK_SIZE;
	}

	--_start_offset;
	::new (static_cast<void*>(_map[_start_block] + _start_offset)) value_type(std::forward<Args>(args)...);
	++_size;
	return _map[_start_block][_start_offset];
}

template<typename _Ty>
template<typename ...Args>
typename Deque<_Ty>::iterator Deque<_Ty>::emplace(iterator pos, Args && ...args)
{
	size_type index = static_cast<size_type>(pos - begin());

	if (_size == capacity())
		resize_map(_map_size * 2);

	for (size_type i = _size; i > index; --i)
		(*this)[i] = std::move((*this)[i - 1]);

	size_type block = index / BLOCK_SIZE;
	size_type offset = index % BLOCK_SIZE;

	_Ty* ptr = &_map[block][offset];

	ptr->~_Ty();
	new (ptr) value_type(std::forward<Args>(args)...);

	++_size;

	return iterator(_map, block, offset);
}

template<typename _Ty>
typename Deque<_Ty>::reference Deque<_Ty>::front()
{
	return _map[_start_block][_start_offset];
}

template<typename _Ty>
typename Deque<_Ty>::const_reference Deque<_Ty>::front() const
{
	return _map[_start_block][_start_offset];
}

template<typename _Ty>
typename Deque<_Ty>::reference Deque<_Ty>::back()
{
	size_type ob = _finish_offset == 0 ? _finish_block - 1 : _finish_block;
	size_type oo = _finish_offset == 0 ? BLOCK_SIZE - 1 : _finish_offset - 1;
	return _map[ob][oo];
}

template<typename _Ty>
typename Deque<_Ty>::const_reference Deque<_Ty>::back() const
{
	size_type ob = _finish_offset == 0 ? _finish_block - 1 : _finish_block;
	size_type oo = _finish_offset == 0 ? BLOCK_SIZE - 1 : _finish_offset - 1;
	return _map[ob][oo];
}

template<typename _Ty>
inline void Deque<_Ty>::clear()
{
	if (empty())
		return;

	size_type total = _size;
	size_type block = _start_block;
	size_type offset = _start_offset;

	for (size_type i = 0; i < total; ++i)
	{
		_map[block][offset].~_Ty();
		if (++offset == BLOCK_SIZE)
		{
			offset = 0;
			++block;
		}
	}

	for (size_type i = 0; i < _map_size; ++i)
	{
		if (_map[i])
			deallocate_block(i);
	}

	size_type center = _map_size / 2;
	allocate_map(center);

	_start_block = _finish_block = center;
	_start_offset = _finish_offset = 0;
	_size = 0;
}

template<typename _Ty>
typename Deque<_Ty>::reference Deque<_Ty>::operator[](size_type index)
{
	size_type offset = _start_offset + index;
	size_type block = _start_block + offset / BLOCK_SIZE;
	size_type block_offset = offset % BLOCK_SIZE;
	return _map[block][block_offset];
}

template<typename _Ty>
typename Deque<_Ty>::const_reference Deque<_Ty>::operator[](size_type index) const
{
	size_type offset = _start_offset + index;
	size_type block = _start_block + offset / BLOCK_SIZE;
	size_type block_offset = offset % BLOCK_SIZE;
	return _map[block][block_offset];
}

template<typename _Ty>
typename Deque<_Ty>::reference Deque<_Ty>::at(size_type index)
{
	if (index >= _size)
		throw std::out_of_range("at() out of range");
	return (*this)[index];
}

template<typename _Ty>
typename Deque<_Ty>::const_reference Deque<_Ty>::at(size_type index) const
{
	if (index >= _size)
		throw std::out_of_range("at() out of range");
	return (*this)[index];
}

template<typename _Ty>
typename Deque<_Ty>::size_type Deque<_Ty>::capacity() const noexcept
{
	return _map_size * BLOCK_SIZE;
}

template<typename _Ty>
typename Deque<_Ty>::size_type Deque<_Ty>::size() const
{
	return _size;
}

template<typename _Ty>
inline bool Deque<_Ty>::empty() const
{
	return _size == 0;
}

template<typename _Ty>
void Deque<_Ty>::resize(size_type new_size, const_reference value)
{
	if (new_size < _size)
	{
		size_type to_destroy = _size - new_size;
		size_type block = _finish_block;
		size_type offset = _finish_offset;

		while (to_destroy > 0)
		{
			if (offset == 0)
			{
				--block;
				offset = BLOCK_SIZE;
			}
			--offset;
			std::destroy_at(&_map[block][offset]);
			--to_destroy;
		}

		_finish_block = block;
		_finish_offset = offset;
		_size = new_size;
	}
	else if (new_size > _size)
	{
		while (_size < new_size)
			push_back(value);
	}
}

template<typename _Ty>
inline void Deque<_Ty>::swap(Deque& other)
{
	std::swap(_map, other._map);
	std::swap(_map_size, other._map_size);
	std::swap(_start_block, other._start_block);
	std::swap(_start_offset, other._start_offset);
	std::swap(_finish_block, other._finish_block);
	std::swap(_finish_offset, other._finish_offset);
	std::swap(_size, other._size);
}

template<typename _Ty>
typename Deque<_Ty>::iterator Deque<_Ty>::insert(iterator pos, const_reference value)
{
	size_type index = pos - begin();
	if (index == size()) 
	{
		push_back(value);
		return end() - 1;
	}

	push_back(back());
	for (size_type i = size() - 1; i > index; --i)
		(*this)[i] = (*this)[i - 1];

	(*this)[index] = value;
	return begin() + index;
}

template<typename _Ty>
typename Deque<_Ty>::iterator Deque<_Ty>::erase(iterator pos)
{
	size_type index = pos - begin();
	for (size_type i = index; i < size() - 1; ++i)
		(*this)[i] = (*this)[i + 1];

	pop_back();
	return begin() + index;
}

template<typename _Ty>
typename Deque<_Ty>::iterator Deque<_Ty>::begin()
{
	return iterator(_map, _start_block, _start_offset);
}

template<typename _Ty>
typename Deque<_Ty>::iterator Deque<_Ty>::end()
{
	return iterator(_map, _finish_block, _finish_offset);
}

template<typename _Ty>
typename Deque<_Ty>::const_iterator Deque<_Ty>::begin() const
{
	return const_iterator(_map, _start_block, _start_offset);
}

template<typename _Ty>
typename Deque<_Ty>::const_iterator Deque<_Ty>::end() const
{
	return const_iterator(_map, _finish_block, _finish_offset);
}

template<typename _Ty>
typename Deque<_Ty>::const_iterator Deque<_Ty>::cbegin() const
{
	return begin();
}

template<typename _Ty>
typename Deque<_Ty>::const_iterator Deque<_Ty>::cend() const
{
	return end();
}

template<typename _Ty>
typename Deque<_Ty>::reverse_iterator Deque<_Ty>::rbegin()
{
	return reverse_iterator(end());
}

template<typename _Ty>
typename Deque<_Ty>::reverse_iterator Deque<_Ty>::rend()
{
	return reverse_iterator(begin());
}

template<typename _Ty>
typename Deque<_Ty>::const_reverse_iterator Deque<_Ty>::rbegin() const
{
	return const_reverse_iterator(end());
}

template<typename _Ty>
typename Deque<_Ty>::const_reverse_iterator Deque<_Ty>::rend() const
{
	return const_reverse_iterator(begin());
}

template<typename _Ty>
typename Deque<_Ty>::const_reverse_iterator Deque<_Ty>::crbegin() const
{
	return rbegin();
}

template<typename _Ty>
typename Deque<_Ty>::const_reverse_iterator Deque<_Ty>::crend() const
{
	return rend();
}

template<typename _Ty>
bool Deque<_Ty>::operator==(const Deque& other) const
{
	if (_size != other._size)
		return false;
	return std::equal(begin(), end(), other.begin());
}

template<typename _Ty>
bool Deque<_Ty>::operator!=(const Deque& other) const
{
	return !(*this == other);
}

template<typename _Ty>
Deque<_Ty>::Iterator::Iterator() noexcept = default;

template<typename _Ty>
Deque<_Ty>::Iterator::Iterator(Map map, size_type block, size_type offset)
	: _map_ptr(map)
	, _block(block)
	, _offset(offset)
{
}

template<typename _Ty>
Deque<_Ty>::Iterator::~Iterator() = default;

template<typename _Ty>
typename Deque<_Ty>::Iterator::reference Deque<_Ty>::Iterator::operator*() const
{
	return _map_ptr[_block][_offset];
}

template<typename _Ty>
typename Deque<_Ty>::Iterator::pointer Deque<_Ty>::Iterator::operator->() const
{
	return &((*_map_ptr)[_block][_offset]);
}

template<typename _Ty>
typename Deque<_Ty>::Iterator& Deque<_Ty>::Iterator::operator++()
{
	if (++_offset == BLOCK_SIZE)
	{
		++_block;
		_offset = 0;
	}
	return *this;
}

template<typename _Ty>
typename Deque<_Ty>::Iterator& Deque<_Ty>::Iterator::operator++(int)
{
	Iterator temp = *this;
	++(*this);
	return temp;
}

template<typename _Ty>
typename Deque<_Ty>::Iterator& Deque<_Ty>::Iterator::operator--()
{
	if (_offset == 0)
	{
		--_block;
		_offset = BLOCK_SIZE - 1;
	}
	else
		--_offset;
	return *this;
}

template<typename _Ty>
typename Deque<_Ty>::Iterator& Deque<_Ty>::Iterator::operator--(int)
{
	Iterator temp = *this;
	--(*this);
	return temp;
}

template<typename _Ty>
typename Deque<_Ty>::Iterator Deque<_Ty>::Iterator::operator+(difference_type n) const
{
	difference_type offset = static_cast<difference_type>(_offset) + n;
	size_type block = _block + offset / BLOCK_SIZE;
	size_type off = offset % BLOCK_SIZE;

	if (off < 0) // обработка отрицательного остатка
	{ 
		block--;
		off += BLOCK_SIZE;
	}
	return Iterator(_map_ptr, block, off);
}

template<typename _Ty>
typename Deque<_Ty>::Iterator Deque<_Ty>::Iterator::operator-(difference_type n) const
{
	return *this + (-n);
}

template<typename _Ty>
typename Deque<_Ty>::Iterator& Deque<_Ty>::Iterator::operator+=(difference_type n)
{
	*this = *this + n;
	return *this;
}

template<typename _Ty>
typename Deque<_Ty>::Iterator& Deque<_Ty>::Iterator::operator-=(difference_type n)
{
	*this = *this - n;
	return *this;
}

template<typename _Ty>
typename Deque<_Ty>::Iterator::reference Deque<_Ty>::Iterator::operator[](difference_type n) const
{
	return *(*this + n);
}

template <typename _Ty>  
typename Deque<_Ty>::Iterator::difference_type Deque<_Ty>::Iterator::operator-(const Iterator& rhs) const  
{  
	difference_type block_diff = static_cast<difference_type>(_block) - rhs._block;
	difference_type offset_diff = static_cast<difference_type>(_offset) - rhs._offset;
	return block_diff * BLOCK_SIZE + offset_diff;
}

template<typename _Ty>
bool Deque<_Ty>::Iterator::operator==(const Iterator& rhs) const
{
	return _map_ptr == rhs._map_ptr && _block == rhs._block && _offset == rhs._offset;
}

template<typename _Ty>
bool Deque<_Ty>::Iterator::operator!=(const Iterator& rhs) const
{
	return !(*this == rhs);
}

template<typename _Ty>
bool Deque<_Ty>::Iterator::operator<(const Iterator& rhs) const
{
	return (_map_ptr == rhs._map_ptr) && ((_block < rhs._block) || (_block == rhs._block && _offset < rhs._offset));
}

template<typename _Ty>
bool Deque<_Ty>::Iterator::operator>(const Iterator& other) const
{
	return other < *this;
}

template <typename _Ty>
bool Deque<_Ty>::Iterator::operator<=(const Iterator& other) const 
{
	return !(other < *this);
}

template <typename _Ty>
bool Deque<_Ty>::Iterator::operator>=(const Iterator& other) const 
{
	return !(*this < other);
}

template<typename _Ty>
Deque<_Ty>::Const_Iterator::Const_Iterator() noexcept = default;

template<typename _Ty>
Deque<_Ty>::Const_Iterator::Const_Iterator(Map map, size_type block, size_type offset)
	: _map_ptr(map)
	, _block(block)
	, _offset(offset)
{
}

template<typename _Ty>
Deque<_Ty>::Const_Iterator::Const_Iterator(const Iterator& it)
	: _map_ptr(it._map_ptr)
	, _block(it._block)
	, _offset(it._offset)
{
}

template<typename _Ty>
inline Deque<_Ty>::Const_Iterator::~Const_Iterator() = default;

template<typename _Ty>
typename Deque<_Ty>::Const_Iterator::reference Deque<_Ty>::Const_Iterator::operator*() const
{
	return (*_map_ptr)[_block][_offset];
}

template<typename _Ty>
typename Deque<_Ty>::Const_Iterator::pointer Deque<_Ty>::Const_Iterator::operator->() const
{
	return &((*_map_ptr)[_block][_offset]);
}

template<typename _Ty>
typename Deque<_Ty>::Const_Iterator& Deque<_Ty>::Const_Iterator::operator++()
{
	if (++_offset >= BLOCK_SIZE)
	{
		++_block;
		_offset = 0;
	}
	return *this;
}

template<typename _Ty>
typename Deque<_Ty>::Const_Iterator Deque<_Ty>::Const_Iterator::operator++(int)
{
	Const_Iterator temp = *this;
	++(*this);
	return temp;
}

template<typename _Ty>
typename Deque<_Ty>::Const_Iterator& Deque<_Ty>::Const_Iterator::operator--()
{
	if (_offset == 0)
	{
		--_block;
		_offset = BLOCK_SIZE - 1;
	}
	else
		--_offset;
	return *this;
}

template<typename _Ty>
typename Deque<_Ty>::Const_Iterator Deque<_Ty>::Const_Iterator::operator--(int)
{
	Const_Iterator temp = *this;
	--(*this);
	return temp;
}

template<typename _Ty>
typename Deque<_Ty>::Const_Iterator Deque<_Ty>::Const_Iterator::operator+(difference_type n) const
{
	difference_type offset = static_cast<difference_type>(_offset) + n;
	size_type block = _block + offset / BLOCK_SIZE;
	size_type off = offset % BLOCK_SIZE;

	if (off < 0) 
	{ // обработка отрицательного остатка
		block--;
		off += BLOCK_SIZE;
	}
	return Const_Iterator(_map_ptr, block, off);
}

template<typename _Ty>
typename Deque<_Ty>::Const_Iterator Deque<_Ty>::Const_Iterator::operator-(difference_type n) const
{
	return *this + (-n);
}

template<typename _Ty>
typename Deque<_Ty>::Const_Iterator& Deque<_Ty>::Const_Iterator::operator+=(difference_type n)
{
	*this = *this + n;
	return *this;
}

template<typename _Ty>
typename Deque<_Ty>::Const_Iterator& Deque<_Ty>::Const_Iterator::operator-=(difference_type n)
{
	*this = *this - n;
	return *this;
}

template<typename _Ty>
typename Deque<_Ty>::Const_Iterator::difference_type Deque<_Ty>::Const_Iterator::operator-(const Const_Iterator& rhs) const
{
	difference_type block_diff = static_cast<difference_type>(_block) - rhs._block;
	difference_type offset_diff = static_cast<difference_type>(_offset) - rhs._offset;
	return block_diff * BLOCK_SIZE + offset_diff;
}

template<typename _Ty>  
typename Deque<_Ty>::const_reference Deque<_Ty>::Const_Iterator::operator[](difference_type n) const  
{  
	return *(*this + n);
}

template <typename _Ty>
inline bool Deque<_Ty>::Const_Iterator::operator==(const Const_Iterator& rhs) const
{
	return _map_ptr == rhs._map_ptr && _block == rhs._block && _offset == rhs._offset;
}

template <typename _Ty>
inline bool Deque<_Ty>::Const_Iterator::operator!=(const Const_Iterator& rhs) const
{
	return !(*this == rhs);
}

template<typename _Ty>
inline bool Deque<_Ty>::Const_Iterator::operator<(const Const_Iterator& rhs) const
{
	return (_map_ptr == rhs._map_ptr) && ((_block < rhs._block) || (_block == rhs._block && _offset < rhs._offset));
}

template<typename _Ty>
inline bool Deque<_Ty>::Const_Iterator::operator>(const Const_Iterator& rhs) const
{
	return rhs < *this;
}

template<typename _Ty>
inline bool Deque<_Ty>::Const_Iterator::operator<=(const Const_Iterator& rhs) const
{
	return !(*this > rhs);
}

template<typename _Ty>
inline bool Deque<_Ty>::Const_Iterator::operator>=(const Const_Iterator& rhs) const
{
	return !(*this < rhs);
}


