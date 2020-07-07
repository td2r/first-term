#ifndef BIGINT_VECTOR_H
#define BIGINT_VECTOR_H

#include <cstdlib>
#include <cstdint>

struct vector
{
    using iterator = uint32_t*;
    using const_iterator = uint32_t const*;

    struct shared_buffer {
        size_t capacity;
        size_t ref_counter;
        uint32_t words[];
    };

    vector();
    explicit vector(size_t size);
    vector(vector const& other);
    vector& operator=(vector const& other);

    ~vector();

    uint32_t& operator[](size_t i);
    uint32_t const& operator[](size_t i) const;

    uint32_t* data();
    uint32_t const* data() const;
    size_t size() const;
    void resize(size_t new_size, uint32_t element = 0);

    uint32_t& back();
    uint32_t const& back() const;
    void push_back(uint32_t const& element);
    void pop_back();

    bool empty() const;

    size_t capacity() const;

    void clear();

    void swap(vector& other);

    iterator begin();
    iterator end();

    const_iterator begin() const;
    const_iterator end() const;

    iterator erase(const_iterator pos);
    iterator erase(const_iterator first, const_iterator last);

    friend bool operator==(vector const& a, vector const& b);

private:
    size_t increased_capacity_() const;
    void realloc_data_(size_t new_capacity);
    void realloc_if_share_();
    void realloc_if_share_or_(bool cond, size_t new_capacity);

private:
    static constexpr size_t SMALL_SIZE = sizeof(shared_buffer*) / sizeof(uint32_t);

private:
    bool small_;
    size_t size_;
    union {
        shared_buffer* dynamic_;
        uint32_t static_[SMALL_SIZE];
    } buffers_;
};

#endif //BIGINT_VECTOR_H
