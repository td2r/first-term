#include "vector.h"
#include <cstdint>
#include <cstring>
#include <algorithm>
#include <cassert>

static vector::shared_buffer* alloc_data(size_t capacity)
{
    using shared_buffer = vector::shared_buffer;
    shared_buffer* buffer;
    buffer = static_cast<shared_buffer*>(
            operator new(sizeof(shared_buffer) + capacity * sizeof(uint32_t)));
    buffer->ref_counter = 1;
    buffer->capacity = capacity;
    return buffer;
}

static void unshare(vector::shared_buffer* a) {
    if (--a->ref_counter == 0) {
        operator delete(a);
    }
}

vector::vector()
    : small_(true)
    , size_(0)
{}

vector::vector(size_t size)
    : small_(size <= SMALL_SIZE)
    , size_(size)
{
    uint32_t* words = small_ ? buffers_.static_ : (buffers_.dynamic_ = alloc_data(size))->words;
    std::fill(words, words + size, 0);
}

vector::vector(vector const& other)
    : small_(other.small_)
    , size_(other.size_)
    , buffers_(other.buffers_)
{
    if (!small_) {
        ++buffers_.dynamic_->ref_counter;
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
    if (!small_) {
        unshare(buffers_.dynamic_);
    }
    small_ = false;
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
    return small_ ? buffers_.static_ : buffers_.dynamic_->words;
}

uint32_t const *vector::data() const {
    return small_ ? buffers_.static_ : buffers_.dynamic_->words;
}

size_t vector::size() const
{
    return size_;
}

void vector::resize(size_t new_size, uint32_t element)
{
    realloc_if_share_or_(new_size > capacity(), new_size);
    uint32_t* words = data();
    if (new_size < size_) {
        std::fill(words + new_size, words + size_, 0);
    } else if (new_size > size_) {
        std::fill(words + size_, words + new_size, element);
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
    realloc_if_share_or_(size_ == capacity(), increased_capacity_());
    data()[size_++] = element;
}

void vector::pop_back()
{
    assert(size_ != 0);
    data()[--size_] = 0;
}

bool vector::empty() const
{
    return size_ == 0;
}

size_t vector::capacity() const {
    return small_ ? SMALL_SIZE : buffers_.dynamic_->capacity;
}

void vector::clear()
{
    uint32_t* words = data();
    std::fill(words, words + size_, 0);
    size_ = 0;
}

void vector::swap(vector& other)
{
    using std::swap;
    swap(small_, other.small_);
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
    realloc_if_share_();
    auto p1 = const_cast<iterator>(first);
    auto p2 = const_cast<iterator>(last);
    iterator p = p1;
    const_iterator e = const_cast<vector const*>(this)->end();
    while (p2 != e) {
        std::swap(*p1++, *p2++);
    }
    std::fill(p1, p2, 0);
    size_ -= last - first;
    return p;
}

bool operator==(vector const& a, vector const& b)
{
    return a.size_ == b.size_ && std::memcmp(a.data(), b.data(), a.size_ * sizeof(uint32_t)) == 0;
}

size_t vector::increased_capacity_() const
{
    return 2 * capacity();
}

void vector::realloc_data_(size_t new_capacity) {
    shared_buffer* new_buffer = alloc_data(new_capacity);
    uint32_t const* words = const_cast<vector const*>(this)->data();
    std::copy(words, words + size_, new_buffer->words);
    if (small_) {
        small_ = false;
    } else {
        unshare(buffers_.dynamic_);
    }
    buffers_.dynamic_ = new_buffer;
}

void vector::realloc_if_share_() {
    realloc_if_share_or_(false, 0);
}

void vector::realloc_if_share_or_(bool cond, size_t new_capacity) {
    if ((!small_ && buffers_.dynamic_->ref_counter > 1) || cond) {
        realloc_data_(cond ? new_capacity : buffers_.dynamic_->capacity);
    }
}
