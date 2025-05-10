#include <cstring>
#include <iostream>

class String {
private:
  char* arr_ = nullptr;
  size_t size_ = 0;
  size_t capacity_ = 1;

  String(size_t count) : arr_(new char[count + 1]), size_(count), capacity_(count + 1) {}

public:
  String() {}

  String(const char* str) : String(strlen(str)) {
    memcpy(arr_, str, size_);
    arr_[size_] = '\0';
  }

  String(size_t count, char c) : String(count) {
    memset(arr_, c, count);
    arr_[count] = '\0';
  }

  String(char letter) : String(1, letter) {}

  String(const String& other) : arr_(new char[other.capacity_]), size_(other.size_), capacity_(other.capacity_) {
    memcpy(arr_, other.arr_, size_);
    arr_[size_] = '\0';
  }

  String& operator=(String other) {
    swap(other);
    return *this;
  }

  size_t size() const {
    return size_;
  }

  size_t length() const {
    return size_;
  }

  size_t capacity() const {
    return capacity_ - 1;
  }

  const char* data() const {
    return arr_;
  }

  char* data() {
    return arr_;
  }

  void swap(String& other) {
    std::swap(arr_, other.arr_);
    std::swap(size_, other.size_);
    std::swap(capacity_, other.capacity_);
  }

  const char& operator[](size_t index) const {
    return arr_[index];
  }

  char& operator[](size_t index) {
    return arr_[index];
  }

  void pop_back() {
    --size_;
    arr_[size_] = '\0';
  }

  void push_back(char letter) {
    if (size_ == capacity_ - 1) {
      capacity_ *= 2;
      arr_ = new char[sizeof(char) * capacity_];
    }
    arr_[size_] = letter;
    size_++;
    arr_[size_] = '\0';
  }

  char& front() {
    return *arr_;
  }

  const char& front() const {
    return *arr_;
  }

  char& back() {
    return *(arr_ + size_ - 1);
  }

  const char& back() const {
    return *(arr_ + size_ - 1);
  }

  String& operator+=(const char& letter) {
    push_back(letter);
    return *this;
  }

  String& operator+=(const String& strAdd) {
    size_t lnOfStrAdd = strAdd.size();
    for (size_t i = 0; i < lnOfStrAdd; ++i) {
      push_back(strAdd[i]);
    }
    return *this;
  }

  size_t find(const String& substring) const {
    size_t lnOfSubstr = substring.size();
    for (size_t i = 0; i < size_ - lnOfSubstr; ++i) {
      const int rc = strncmp(arr_ + i, substring.data(), lnOfSubstr);
      if (rc == 0) {
        return i;
      }
    }
    return size_;
  }

  size_t rfind(const String& substring) const {
    size_t lnOfSubstr = substring.size();
    for (size_t i = size_ - lnOfSubstr + 1; i > 0; --i) {
      const int rc = strncmp(arr_ + i - 1, substring.data(), lnOfSubstr);
      if (rc == 0) {
        return i - 1;
      }
    }
    return size_;
  }

  String substr(size_t start, size_t count) const {
    String result;
    for (size_t i = start; i < start + count; ++i) {
      result += arr_[i];
    }
    return result;
  }

  bool empty() const {
    return (size_ == 0);
  }

  void clear() {
    size_ = 0;
    capacity_ = 1;
    arr_[0] = '\0';
  }

  void shrink_to_fit() {
    arr_ = (char*) realloc(arr_, sizeof(char) * (size_ + 1));
    capacity_ = size_ + 1;
  }

  ~String() {
    delete[] arr_;
  }
};

bool operator==(const String& str1, const String& str2) {
  if (str1.size() != str2.size()) {
    return false;
  }
  if (strcmp(str1.data(), str2.data()) != 0) {
    return false;
  }
  return true;
}

bool operator!=(const String& str1, const String& str2) {
  return !(str1 == str2);
}

bool operator<(const String& str1, const String& str2) {
  if (strcmp(str1.data(), str2.data()) < 0) {
    return true;
  }
  return false;
}

bool operator>(const String& str1, const String& str2) {
  return (str2 < str1);
}

bool operator<=(const String& str1, const String& str2) {
  return !(str1 > str2);
}

bool operator>=(const String& str1, const String& str2) {
  return !(str1 < str2);
}

String operator+(const String& str1, const String& str2) {
  String result = str1;
  result += str2;
  return result;
}

std::ostream& operator<<(std::ostream& output_stream, const String& str) {
  output_stream << str.data();
  return output_stream;
}

std::istream& operator>>(std::istream& input_stream, String& str) {
  char current_char;

  while (input_stream.get(current_char)) {
    if (isspace(current_char)) {
      break;
    }
    str.push_back(current_char);
  }
  return input_stream;
}
