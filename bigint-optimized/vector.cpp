#include "vector.h"
#include <cstdlib>
#include <cstdint>
#include <cstring>
#include <algorithm>
#include <cassert>

static vector::buffer_data alloc_data(size_t capacity)
{
    using shared_array = vector::shared_array;
    vector::buffer_data data{};
    data.arr = static_cast<shared_array*>(
            operator new(sizeof(shared_array) + capacity * sizeof(uint32_t)));
    data.arr->ref_counter = 1;
    data.capacity = capacity;
    return data;
}

static void unshare(vector::shared_array* a) {
    if (!--a->ref_counter) {
        operator delete(a);
    }
}

vector::vector()
    : size_(0)
{}

vector::vector(size_t size)
    : size_(size)
{
    if (size <= SMALL_SIZE) {
        // use std::fill instead of memset to activate field static_
        std::fill(buffers_.static_, buffers_.static_ + size, 0);
    } else {
        buffers_.dynamic_ = alloc_data(size);
    }
}

vector::vector(vector const& other)
    : size_(other.size_)
    , buffers_(other.buffers_)
{
    if (size_ > SMALL_SIZE) {
        ++buffers_.dynamic_.arr->ref_counter;
    }
}

vector& vector::operator=(vector const& other)
{
    if (this != &other) {
        vector tmp(other);
        swap(tmp);
    }
    return *this;
}

vector::~vector()
{
    if (size_ > SMALL_SIZE) {
        unshare(buffers_.dynamic_.arr);
    }
    size_ = 0;
}

uint32_t& vector::operator[](size_t i)
{
    assert(i < size_);
    return data()[i];
}

uint32_t const& vector::operator[](size_t i) const
{
    assert(i < size_);
    return data()[i];
}

uint32_t *vector::data() {
    realloc_if_share_();
    return size_ <= SMALL_SIZE ? buffers_.static_ : buffers_.dynamic_.arr->words;
}

uint32_t const *vector::data() const {
    return size_ <= SMALL_SIZE ? buffers_.static_ : buffers_.dynamic_.arr->words;
}

size_t vector::size() const
{
    return size_;
}

void vector::resize(size_t new_size, uint32_t element)
{
    if (new_size < size_) {
        if (size_ <= SMALL_SIZE) {
            std::fill(buffers_.static_ + new_size, buffers_.static_ + size_, 0);
        } else if (new_size <= SMALL_SIZE) {
            shared_array* arr = buffers_.dynamic_.arr;
            // use of std::copy instead of memcpy to activate field static_
            std::copy(arr->words, arr->words + new_size, buffers_.static_);
            unshare(arr);
        } else {
            realloc_if_share_();
            memset(buffers_.dynamic_.arr->words + new_size, 0, (size_ - new_size) * sizeof(uint32_t));
        }
    } else if (new_size > size_) {
        if (size_ > SMALL_SIZE) {
            if (buffers_.dynamic_.arr->ref_counter > 1 || new_size > buffers_.dynamic_.capacity) {
                realloc_data_(new_size > buffers_.dynamic_.capacity ? new_size : buffers_.dynamic_.capacity);
            }
            std::fill(buffers_.dynamic_.arr->words + size_, buffers_.dynamic_.arr->words + new_size, element);
        } else if (new_size > SMALL_SIZE) {
            buffer_data data = alloc_data(new_size);
            memcpy(data.arr->words, buffers_.static_, size_ * sizeof(uint32_t));
            std::fill(data.arr->words + size_, data.arr->words + new_size, element);
            buffers_.dynamic_ = data;
        } else {
            std::fill(buffers_.static_ + size_, buffers_.static_ + new_size, element);
        }
    }
    size_ = new_size;
}

uint32_t& vector::back()
{
    assert(size_ != 0);
    return data()[size_ - 1];
}

uint32_t const& vector::back() const
{
    return data()[size_ - 1];
}

void vector::push_back(uint32_t const& element)
{
    if (size_ < SMALL_SIZE) {
        buffers_.static_[size_++] = element;
    } else if (size_ == SMALL_SIZE) {
        buffer_data data = alloc_data(SMALL_SIZE + 1);
        memcpy(data.arr->words, buffers_.static_, SMALL_SIZE * sizeof(uint32_t));
        data.arr->words[SMALL_SIZE] = element;
        buffers_.dynamic_ = data;
        ++size_;
    } else {
        if (buffers_.dynamic_.arr->ref_counter > 1 || size_ == buffers_.dynamic_.capacity) {
            realloc_data_(size_ == buffers_.dynamic_.capacity ?
                    increased_capacity_() : buffers_.dynamic_.capacity);
        }
        buffers_.dynamic_.arr->words[size_++] = element;
    }
}

void vector::pop_back()
{
    assert(size_ != 0);

    if (size_ <= SMALL_SIZE) {
        --size_;
    } else if (size_ == SMALL_SIZE + 1) {
        shared_array* arr = buffers_.dynamic_.arr;
        std::copy(arr->words, arr->words + SMALL_SIZE, buffers_.static_);
        unshare(arr);
        --size_;
    } else {
        realloc_if_share_();
        buffers_.dynamic_.arr->words[--size_] = 0;
    }
}

bool vector::empty() const
{
    return size_ == 0;
}

void vector::clear()
{
    if (size_ > SMALL_SIZE) {
        unshare(buffers_.dynamic_.arr);
    }
    size_ = 0;
}

void vector::swap(vector& other)
{
    using std::swap;
    swap(buffers_, other.buffers_);
    swap(size_, other.size_);
}

vector::iterator vector::begin()
{
    return data();
}

vector::iterator vector::end()
{
    return data() + size_;
}

vector::const_iterator vector::begin() const
{
    return data();
}

vector::const_iterator vector::end() const
{
    return data() + size_;
}

vector::iterator vector::erase(const_iterator pos)
{
    return erase(pos, pos + 1);
}

vector::iterator vector::erase(const_iterator first, const_iterator last)
{
    if (size_ > SMALL_SIZE) {
        realloc_if_share_();
    }
    auto p1 = const_cast<iterator>(first);
    auto p2 = const_cast<iterator>(last);
    iterator p = p1;
    const_iterator e = const_cast<vector const*>(this)->end();
    while (p2 != e) {
        std::swap(*p1++, *p2++);
    }
    memset(p1, 0, (p2 - p1) * sizeof(uint32_t));
    size_ -= last - first;
    return p;
}

bool operator==(vector const& a, vector const& b)
{
    return a.size_ == b.size_ && memcmp(a.data(), b.data(), a.size_ * sizeof(uint32_t)) == 0;
}

size_t vector::increased_capacity_() const
{
    assert(size_ > SMALL_SIZE);
    return 2 * buffers_.dynamic_.capacity + (buffers_.dynamic_.capacity == 0);
}

void vector::realloc_data_(size_t new_capacity) {
    assert(size_ > SMALL_SIZE);
    assert(new_capacity > SMALL_SIZE);

    buffer_data new_data = alloc_data(new_capacity);
    memcpy(new_data.arr->words, buffers_.dynamic_.arr->words, size_ * sizeof(uint32_t));
    unshare(buffers_.dynamic_.arr);
    buffers_.dynamic_ = new_data;
}

void vector::realloc_if_share_() {
    if (size_ > SMALL_SIZE && buffers_.dynamic_.arr->ref_counter > 1) {
        realloc_data_(buffers_.dynamic_.capacity);
    }
}
