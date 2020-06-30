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
    size_t increased_capacity() const;
    void new_buffer(size_t new_capacity);

    template<bool Condition, typename S = void>
    using enable_if_t = typename std::enable_if<Condition, S>::type;

//    C++14 only =(
//    template<typename S>
//    static constexpr bool is_trivially_destructible_v = std::is_trivially_destructible<S>::value;

//    template<typename S>
//    static constexpr bool is_nothrow_copy_constructible_v = std::is_nothrow_copy_constructible<S>::value;

    template<typename T_ = T>
    static enable_if_t<std::is_trivially_destructible<T_>::value>
    destroy_all(T* dest, size_t size);

    template<typename T_ = T>
    static enable_if_t<!std::is_trivially_destructible<T_>::value>
    destroy_all(T* dest, size_t size);

    template<typename T_ = T>
    static enable_if_t<std::is_nothrow_copy_constructible<T_>::value>
    copy_construct_all(T* dest, T const* src, size_t size);

    template<typename T_ = T>
    static enable_if_t<!std::is_nothrow_copy_constructible<T_>::value>
    copy_construct_all(T* dest, T const* src, size_t size);

private:
    T* data_;
    size_t size_;
    size_t capacity_;
};

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
        auto deleter = [](T* ptr) {
            operator delete(ptr);
        };
        std::unique_ptr<T, decltype(deleter)> p(static_cast<T*>(operator new(other.size_ * sizeof(T))), deleter);
        copy_construct_all(p.get(), other.data_, other.size_);
        data_ = p.release();
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
        tmp.reserve(increased_capacity());

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

    ptrdiff_t dist = end() - pos;
    push_back(element);
    iterator it = end() - 1;
    while (dist-- != 0) {
        std::swap(*(it - 1), *it);
        --it;
    }
    return it;
}

template <typename T>
typename vector<T>::iterator vector<T>::erase(const_iterator pos)
{
    return erase(pos, pos + 1);
}

template <typename T>
typename vector<T>::iterator vector<T>::erase(const_iterator first, const_iterator last)
{
    assert(begin() <= first && first <= last && last <= end());

    auto p1 = const_cast<iterator>(first);
    auto p2 = const_cast<iterator>(last);
    iterator p = p1;
    while (p2 != end()) {
        std::swap(*p1, *p2);
        ++p1, ++p2;
    }
    destroy_all(p1, p2 - p1);
    size_ = p1 - data_;
    return p;
}

template <typename T>
size_t vector<T>::increased_capacity() const
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

template<typename T>
template<typename T_>
typename std::enable_if<std::is_trivially_destructible<T_>::value>::type
vector<T>::destroy_all(T* dest, size_t size)
{}

template<typename T>
template<typename T_>
typename std::enable_if<!std::is_trivially_destructible<T_>::value>::type
vector<T>::destroy_all(T* dest, size_t size)
{
    while (size != 0) {
        dest[--size].~T();
    }
}

template<typename T>
template<typename T_>
typename std::enable_if<std::is_nothrow_copy_constructible<T_>::value>::type
vector<T>::copy_construct_all(T *dest, const T *src, size_t size)
{
    for (size_t i = 0; i != size; ++i) {
        new (dest + i) T(src[i]);
    }
}

template<typename T>
template<typename T_>
typename std::enable_if<!std::is_nothrow_copy_constructible<T_>::value>::type
vector<T>::copy_construct_all(T *dest, const T *src, size_t size)
{
    size_t i = 0;
    try {
        for (; i != size; ++i) {
            new(dest + i) T(src[i]);
        }
    } catch(...) {
        destroy_all(dest, i);
        throw;
    }
}
