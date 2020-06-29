#pragma once

#include <algorithm>
#include <cassert>
#include <cstring>
#include <cstdio>
#include <memory>

template <typename T>
struct vector
{
    typedef T* iterator;
    typedef T const* const_iterator;

    vector();                               // O(1) nothrow
    vector(vector const&);                  // O(N) strong
    vector& operator=(vector const& other); // O(N) strong

    ~vector();                              // O(N) nothrow

    T& operator[](size_t i);                // O(1) nothrow
    T const& operator[](size_t i) const;    // O(1) nothrow

    T* data();                              // O(1) nothrow
    T const* data() const;                  // O(1) nothrow
    size_t size() const;                    // O(1) nothrow

    T& front();                             // O(1) nothrow
    T const& front() const;                 // O(1) nothrow

    T& back();                              // O(1) nothrow
    T const& back() const;                  // O(1) nothrow
    void push_back(T const&);               // O(1)* strong
    void pop_back();                        // O(1) nothrow

    bool empty() const;                     // O(1) nothrow

    size_t capacity() const;                // O(1) nothrow
    void reserve(size_t);                   // O(N) strong
    void shrink_to_fit();                   // O(N) strong

    void clear();                           // O(N) nothrow

    void swap(vector&);                     // O(1) nothrow

    iterator begin();                       // O(1) nothrow
    iterator end();                         // O(1) nothrow

    const_iterator begin() const;           // O(1) nothrow
    const_iterator end() const;             // O(1) nothrow

    // iterator insert(iterator pos, T const&); // O(N) weak
    iterator insert(const_iterator pos, T const&); // O(N) weak

    // iterator erase(iterator pos);           // O(N) weak
    iterator erase(const_iterator pos);     // O(N) weak

    // iterator erase(iterator first, iterator last); // O(N) weak
    iterator erase(const_iterator first, const_iterator last); // O(N) weak

private:
    size_t increase_capacity() const;
    void new_buffer(size_t new_capacity);

private:
    T* data_;
    size_t size_;
    size_t capacity_;
};

namespace {
    template <typename T>
    typename std::enable_if<std::is_trivially_destructible<T>::value>::type
    destroy_all(T* dest, size_t size) {}

    template <typename T>
    typename std::enable_if<!std::is_trivially_destructible<T>::value>::type
    destroy_all(T* dest, size_t size)
    {
        while (size != 0) {
            dest[--size].~T();
        }
    }

    template <typename T>
    typename std::enable_if<std::is_nothrow_copy_constructible<T>::value>::type
    copy_construct_all(T* dest, T const* src, size_t size)
    {
        size_t i = 0;
        while (i != size) {
            new (dest + i) T(src[i]);
            ++i;
        }
    }

    template <typename T>
    typename std::enable_if<!std::is_nothrow_copy_constructible<T>::value>::type
    copy_construct_all(T* dest, T const* src, size_t size)
    {
        size_t i = 0;
        try {
            while (i != size) {
                new (dest + i) T(src[i]);
                ++i;
            }
        } catch (...) {
            destroy_all(dest, i);
            throw;
        }
    }
}

template <typename T>
vector<T>::vector()
    : data_(nullptr)
    , size_(0)
    , capacity_(0)
    {}

template <typename T>
vector<T>::vector(vector<T> const& other)
    : data_(nullptr)
    , size_(0)
    , capacity_(0)
{
    if (other.size_ != 0) {
        data_ = static_cast<T*>(operator new(other.size_ * sizeof(T)));
        try {
            copy_construct_all(data_, other.data_, other.size_);
        } catch (...) {
            operator delete(data_);
            throw;
        }
        size_ = other.size_;
        capacity_ = other.size_;
    }
}

template <typename T>
vector<T>& vector<T>::operator=(vector<T> const& other)
{
    if (this != &other) {
        vector<T> copy(other);
        swap(copy);
    }
    return *this;
}

template <typename T>
vector<T>::~vector()
{
    destroy_all(data_, size_);
    operator delete(data_);
    capacity_ = size_ = 0;
}

template <typename T>
T& vector<T>::operator[](size_t i)
{
    assert(i < size_);
    return data_[i];
}

template <typename T>
T const& vector<T>::operator[](size_t i) const
{
    assert(i < size_);
    return data_[i];
}

template <typename T>
T* vector<T>::data()
{
    return data_;
}

template <typename T>
T const* vector<T>::data() const
{
    return data_;
}

template <typename T>
size_t vector<T>::size() const
{
    return size_;
}

template <typename T>
T& vector<T>::front()
{
    assert(size_ != 0);
    return *data_;
}

template <typename T>
T const& vector<T>::front() const
{
    assert(size_ != 0);
    return *data_;
}

template <typename T>
T& vector<T>::back()
{
    assert(size_ != 0);
    return data_[size_ - 1];
}

template <typename T>
T const& vector<T>::back() const
{
    assert(size_ != 0);
    return data_[size_ - 1];
}

template <typename T>
void vector<T>::push_back(T const& element)
{
    if (size_ == capacity_) {
        vector<T> tmp;
        tmp.capacity_ = increase_capacity();
        tmp.data_ = static_cast<T*>(operator new(tmp.capacity_ * sizeof(T)));
        copy_construct_all(tmp.data_, data_, size_);
        tmp.size_ = size_;
        tmp.push_back(element);
        swap(tmp);
    } else {
        new (data_ + size_) T(element);
        ++size_;
    }
}

template <typename T>
void vector<T>::pop_back()
{
    assert(size_ != 0);
    data_[--size_].~T();
}

template <typename T>
bool vector<T>::empty() const
{
    return size_ == 0;
}

template <typename T>
size_t vector<T>::capacity() const
{
    return capacity_;
}

template <typename T>
void vector<T>::reserve(size_t new_capacity)
{
    if (capacity_ < new_capacity) {
        new_buffer(new_capacity);
    }
}

template <typename T>
void vector<T>::shrink_to_fit()
{
    if (capacity_ > size_) {
        new_buffer(size_);
    }
}

template <typename T>
void vector<T>::clear()
{
    destroy_all(data_, size_);
    size_ = 0;
}

template <typename T>
void vector<T>::swap(vector& rhs)
{
    using std::swap;
    swap(data_, rhs.data_);
    swap(size_, rhs.size_);
    swap(capacity_, rhs.capacity_);
}

template <typename T>
typename vector<T>::iterator vector<T>::begin()
{
    return data_;
}

template <typename T>
typename vector<T>::iterator vector<T>::end()
{
    return data_ + size_;
}

template <typename T>
typename vector<T>::const_iterator vector<T>::begin() const
{
    return data_;
}

template <typename T>
typename vector<T>::const_iterator vector<T>::end() const
{
    return data_ + size_;
}

template <typename T>
typename vector<T>::iterator vector<T>::insert(const_iterator pos, T const& element)
{
    assert(begin() <= pos && pos <= end());
    auto p = const_cast<iterator>(pos);
    vector<T> tmp;
    tmp.new_buffer(size_ == capacity_ ? increase_capacity() : capacity_);
    copy_construct_all(tmp.data_, data_, p - begin());
    tmp.size_ = p - begin();
    iterator r = tmp.end();
    tmp.push_back(element);
    copy_construct_all(tmp.end(), p, end() - p);
    tmp.size_ += end() - p;
    assert(tmp.size_ == size_ + 1);
    swap(tmp);
    return r;
}

template <typename T>
typename vector<T>::iterator vector<T>::erase(const_iterator pos)
{
    return erase(pos, pos + 1);
}

template <typename T>
typename vector<T>::iterator vector<T>::erase(const_iterator first, const_iterator last)
{
    assert(begin() <= first && last <= end());
    auto p = const_cast<iterator>(first);
    iterator p1 = p;
    auto p2 = const_cast<iterator>(last);
    iterator e = end();
    while (p2 != e) {
        std::swap(*p1, *p2);
        ++p1, ++p2;
    }
    destroy_all(p1, p2 - p1);
    size_ = p1 - data_;
    return p;
}

template <typename T>
size_t vector<T>::increase_capacity() const
{
    return 2 * capacity_ + (capacity_ == 0);
}

template <typename T>
void vector<T>::new_buffer(size_t new_capacity)
{
    assert(new_capacity >= size_);

    vector<T> tmp;
    if (new_capacity != 0) {
        tmp.data_ = static_cast<T*>(operator new(new_capacity * sizeof(T)));
        tmp.capacity_ = new_capacity;
        copy_construct_all(tmp.data_, data_, size_);
        tmp.size_ = size_;
    }

    swap(tmp);
}
