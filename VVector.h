#ifndef VECTOR_H
#define VECTOR_H
#define VECTOR_MEMORY_IMPLEMENTED

#include <iostream>
#include <cstddef>
#include <iterator>
#include <algorithm>
#include <initializer_list>
#include <stdexcept>
#include <utility>
#include <memory>

template <class T>
class Vector {
 private:
  T* vector_;
  std::size_t size_;
  std::size_t capacity_;
  std::allocator<T> allocator_;

  void AllocateMemory(std::size_t capacity) {
    vector_ = allocator_.allocate(capacity);
  }

  void DeallocateMemory() {
    if (vector_) {
      allocator_.deallocate(vector_, capacity_);
      vector_ = nullptr;
    }
  }

  void DestroyElements(std::size_t size) {
    std::destroy(vector_, vector_ + size);
  }

  void Reallocate(std::size_t new_capacity) {
    T* new_vector = allocator_.allocate(new_capacity);

    try {
      std::uninitialized_move(vector_, vector_ + size_, new_vector);
    } catch (...) {
      std::destroy(new_vector, new_vector + size_);
      allocator_.deallocate(new_vector, new_capacity);
      throw;
    }

    DestroyElements(size_);
    DeallocateMemory();

    vector_ = new_vector;
    capacity_ = new_capacity;
  }

 public:
  using ValueType = T;                                                // NOLINT
  using SizeType = std::size_t;                                       // NOLINT
  using Iterator = T*;                                                // NOLINT
  using ConstIterator = const T*;                                     // NOLINT
  using ReverseIterator = std::reverse_iterator<Iterator>;            // NOLINT
  using ConstReverseIterator = std::reverse_iterator<ConstIterator>;  // NOLINT
  using Pointer = T*;                                                 // NOLINT
  using ConstPointer = const T*;                                      // NOLINT
  using Reference = T&;                                               // NOLINT
  using ConstReference = const T&;                                    // NOLINT

  Vector() : vector_(nullptr), size_(0), capacity_(0) {
  }

  ~Vector() {
    DestroyElements(size_);
    DeallocateMemory();
  }

  explicit Vector(std::size_t capacity) : size_(capacity), capacity_(capacity) {
    if (size_ > 0) {
      try {
        AllocateMemory(capacity_);
        std::uninitialized_default_construct_n(vector_, capacity_);
      } catch (...) {
        DeallocateMemory();
        throw;
      }
    } else {
      vector_ = nullptr;
    }
  }

  Vector(std::size_t size, const T& value) : size_(size), capacity_(size) {
    std::size_t i = 0;
    try {
      AllocateMemory(size);
      for (; i < size_; i++) {
        new (vector_ + i) T(value);
      }
    } catch (...) {
      std::destroy(vector_, vector_ + i);
      DeallocateMemory();
      throw;
    }
  }

  template <class Iterator, class = std::enable_if_t<std::is_base_of_v<
      std::forward_iterator_tag, typename std::iterator_traits<Iterator>::iterator_category>>>

  Vector(Iterator first, Iterator last) : size_(std::distance(first, last)), capacity_(size_) {
    std::size_t i = 0;
    try {
      AllocateMemory(size_);
      for (; i < size_; i++) {
        new (vector_ + i) T(first[i]);
      }
    } catch (...) {
      std::destroy(vector_, vector_ + i);
      DeallocateMemory();
      throw;
    }
  }

  Vector(std::initializer_list<ValueType> list) : Vector(list.begin(), list.end()) {
  }

  Vector(const Vector& other) : size_(0), capacity_(0) {
    try {
      AllocateMemory(other.capacity_);
      for (; size_ < other.size_; ++size_) {
        new(vector_ + size_) T(other.vector_[size_]);
      }
    } catch (...) {
      std::destroy(vector_, vector_ + size_);
      allocator_.deallocate(vector_, other.capacity_);
      size_ = 0;
      throw;
    }
    size_ = other.size_;
    capacity_ = other.capacity_;
  }

  Vector(Vector&& other) noexcept : vector_(other.vector_), size_(other.size_), capacity_(other.capacity_) {
    other.vector_ = nullptr;
    other.size_ = 0;
    other.capacity_ = 0;
  }

  Vector& operator=(const Vector& other) {
    if (this != &other) {
      Vector temp(other);
      Swap(temp);
    }
    return *this;
  }

  Vector& operator=(Vector&& other) noexcept {
    if (this != &other) {
      DestroyElements(size_);
      DeallocateMemory();
      vector_ = other.vector_;
      size_ = other.size_;
      capacity_ = other.capacity_;
      other.vector_ = nullptr;
      other.size_ = 0;
      other.capacity_ = 0;
    }
    return *this;
  }

  SizeType Size() const {
    return size_;
  }

  SizeType Capacity() const {
    return capacity_;
  }

  bool Empty() const {
    return size_ == 0;
  }

  Reference operator[](SizeType index) {
    return vector_[index];
  }

  ConstReference operator[](SizeType index) const {
    return vector_[index];
  }

  Reference At(SizeType index) {
    if (index >= size_) {
      throw std::out_of_range("The index is out of range");
    }
    return vector_[index];
  }

  ConstReference At(SizeType index) const {
    if (index >= size_) {
      throw std::out_of_range("The index is out of range");
    }
    return vector_[index];
  }

  Reference Front() {
    return vector_[0];
  }

  ConstReference Front() const {
    return vector_[0];
  }

  Reference Back() {
    return vector_[size_ - 1];
  }

  ConstReference Back() const {
    return vector_[size_ - 1];
  }

  Pointer Data() {
    return capacity_ == 0 ? nullptr : vector_;
  }

  ConstPointer Data() const {
    return capacity_ == 0 ? nullptr : vector_;
  }

  void Swap(Vector& other) {
    std::swap(size_, other.size_);
    std::swap(capacity_, other.capacity_);
    std::swap(vector_, other.vector_);
  }

  void Resize(SizeType new_size, const ValueType& value) {
    if (new_size > capacity_) {
      T* new_vector = nullptr;
      T* new_vector_end = nullptr;

      try {
        new_vector = allocator_.allocate(new_size);
      } catch (...) {
        allocator_.deallocate(new_vector, new_size);
        new_vector = nullptr;
        throw;
      }

      try {
        new_vector_end = std::uninitialized_move(vector_, vector_ + size_, new_vector);
      } catch (...) {
        allocator_.deallocate(new_vector, new_size);
        throw;
      }
      try {
        std::uninitialized_fill(new_vector_end, new_vector + new_size, value);
        new_vector_end = new_vector + new_size;
      } catch (...) {
        std::destroy(new_vector, new_vector_end);
        allocator_.deallocate(new_vector, new_size);
        throw;
      }
      DestroyElements(size_);
      DeallocateMemory();

      vector_ = new_vector;
      size_ = new_size;
      capacity_ = new_size;
    } else if (new_size > size_) {
      try {
        std::uninitialized_fill(vector_ + size_, vector_ + new_size, value);
      } catch (...) {
        std::destroy(vector_ + size_, vector_ + new_size);
        throw;
      }
      size_ = new_size;
    } else if (new_size < size_) {
      std::destroy(vector_ + new_size, vector_ + size_);
      size_ = new_size;
    }
  }

  void Resize(SizeType new_size) {
    if (new_size > capacity_) {
      T* new_vector = nullptr;
      try {
        new_vector = allocator_.allocate(new_size);
      } catch (...) {
        allocator_.deallocate(new_vector, new_size);
        throw;
      }

      T* new_vector_end = new_vector;

      try {
        new_vector_end = std::uninitialized_move(vector_, vector_ + size_, new_vector);
        std::uninitialized_value_construct_n(new_vector_end, new_size - size_);
      } catch (...) {
        std::destroy(new_vector, new_vector_end);
        allocator_.deallocate(new_vector, new_size);
        throw;
      }
      DestroyElements(size_);
      DeallocateMemory();

      vector_ = new_vector;
      size_ = new_size;
      capacity_ = new_size;
    } else if (new_size > size_) {
      try {
        std::uninitialized_value_construct_n(vector_ + size_, new_size - size_);
      } catch (...) {
        std::destroy(vector_ + size_, vector_ + new_size);
        throw;
      }
      size_ = new_size;
    } else if (new_size < size_) {
      std::destroy(vector_ + new_size, vector_ + size_);
      size_ = new_size;
    }
  }

  void Reserve(std::size_t new_capacity) {
    if (new_capacity > capacity_) {
      try {
        Reallocate(new_capacity);
      } catch (...) {
        throw;
      }
    }
  }

  void ShrinkToFit() {
    if (size_ < capacity_) {
      try {
        Reallocate(size_);
      } catch (...) {
        throw;
      }
    }
  }

  void Clear() {
    DestroyElements(size_);
    size_ = 0;
  }

  void PushBack(ConstReference value) {
    if (size_ == capacity_) {
      std::size_t new_capacity = capacity_ == 0 ? 1 : 2 * capacity_;
      T* new_vector = nullptr;

      try {
        new_vector = allocator_.allocate(new_capacity);
      } catch (...) {
        allocator_.deallocate(new_vector, new_capacity);
        throw;
      }
      T* new_vector_end = new_vector;

      try {
        new_vector_end = std::uninitialized_move(vector_, vector_ + size_, new_vector);
        ::new (static_cast<void*>(new_vector_end)) T(value);
        ++new_vector_end;
      } catch (...) {
        std::destroy(new_vector, new_vector_end + 1);
        allocator_.deallocate(new_vector, new_capacity);
        throw;
      }

      DestroyElements(size_);
      DeallocateMemory();

      vector_ = new_vector;
      size_ = size_ + 1;
      capacity_ = new_capacity;
    } else {
      try {
        ::new (static_cast<void*>(vector_ + size_)) T(value);
      } catch (...) {
        std::destroy_at(vector_ + size_);
        throw;
      }
      ++size_;
    }
  }

  void PushBack(ValueType&& value) {
    if (size_ == capacity_) {
      std::size_t new_capacity = capacity_ == 0 ? 1 : 2 * capacity_;
      T* new_vector = nullptr;

      try {
        new_vector = allocator_.allocate(new_capacity);
      } catch (...) {
        allocator_.deallocate(new_vector, new_capacity);
        throw;
      }
      T* new_vector_end = new_vector;

      try {
        new_vector_end = std::uninitialized_move(vector_, vector_ + size_, new_vector);
        ::new (static_cast<void*>(new_vector_end)) T(std::move(value));
        ++new_vector_end;
      } catch (...) {
        std::destroy(new_vector, new_vector_end + 1);
        allocator_.deallocate(new_vector, new_capacity);
        throw;
      }

      DestroyElements(size_);
      DeallocateMemory();

      vector_ = new_vector;
      size_ = size_ + 1;
      capacity_ = new_capacity;
    } else {
      try {
        ::new (static_cast<void*>(vector_ + size_)) T(std::move(value));
      } catch (...) {
        std::destroy_at(vector_ + size_);
        throw;
      }
      ++size_;
    }
  }

  void PopBack() {
    if (size_ > 0) {
      std::destroy_at(vector_ + --size_);
    }
  }

  Iterator begin() {  // NOLINT
    return vector_;
  }

  Iterator end() {  // NOLINT
    return vector_ + size_;
  }

  ConstIterator begin() const {  // NOLINT
    return vector_;
  }

  ConstIterator end() const {  // NOLINT
    return vector_ + size_;
  }

  ReverseIterator rbegin() {  // NOLINT
    return std::reverse_iterator(end());
  }

  ReverseIterator rend() {  // NOLINT
    return std::reverse_iterator(begin());
  }

  ConstReverseIterator rbegin() const {  // NOLINT
    return std::reverse_iterator(end());
  }

  ConstReverseIterator rend() const {  // NOLINT
    return std::reverse_iterator(begin());
  }

  ConstIterator cbegin() const {  // NOLINT
    return begin();
  }

  ConstIterator cend() const {  // NOLINT
    return end();
  }

  ConstReverseIterator crbegin() const {  // NOLINT
    return rbegin();
  }

  ConstReverseIterator crend() const {  // NOLINT
    return rend();
  }

  bool operator==(const Vector& other) const {
    return size_ == other.size_ && std::equal(begin(), end(), other.begin());
  }

  bool operator!=(const Vector& other) const {
    return !(*this == other);
  }

  bool operator<(const Vector& other) const {
    return std::lexicographical_compare(begin(), end(), other.begin(), other.end());
  }

  bool operator>=(const Vector& other) const {
    return !(*this < other);
  }

  bool operator>(const Vector& other) const {
    return other < *this;
  }

  bool operator<=(const Vector& other) const {
    return !(*this > other);
  }
};

#endif
