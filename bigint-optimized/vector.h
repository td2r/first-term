#ifndef BIGINT_VECTOR_H
#define BIGINT_VECTOR_H

#include <cstdlib>
#include <cstdint>

struct vector
{
    typedef uint32_t* iterator;
    typedef uint32_t const* const_iterator;

    struct shared_array {
        size_t ref_counter;
        uint32_t words[];
    };

    struct buffer_data {
        size_t capacity;
        shared_array* arr;
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

private:
    static const size_t SMALL_SIZE = sizeof(buffer_data) / sizeof(uint32_t);

private:
    size_t size_;
    union {
        buffer_data dynamic_;
        uint32_t static_[SMALL_SIZE];
    } buffers_;
};

#endif //BIGINT_VECTOR_H
