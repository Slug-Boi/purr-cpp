#pragma once
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <sys/_types/_u_int64_t.h>
#include <sys/types.h>

namespace kattio {

struct NoMoreTokensException : std::runtime_error {
    NoMoreTokensException() : std::runtime_error("no more tokens") {}
};

class Tokenizer {
public:
    explicit Tokenizer(std::istream& in = std::cin) : in_(in) {
        if (&in_ == &std::cin) {
            std::ios::sync_with_stdio(false);
            std::cin.tie(nullptr);
        }
    }

    bool hasNext() {
        if (has_peeked_) return true;
        if (in_ >> peeked_) {
            has_peeked_ = true;
            return true;
        }
        return false;
    }

    std::string next() {
        if (!hasNext()) throw NoMoreTokensException();
        has_peeked_ = false;
        return std::move(peeked_);
    }

private:
    std::istream& in_;
    std::string peeked_;
    bool has_peeked_ = false;
};

// Typed reads on top of Tokenizer.
class Scanner : public Tokenizer {
public:
    using Tokenizer::Tokenizer;
    int         nextInt()    { return std::stoi(next());  }
    unsigned long   nextUInt()   { return std::stoul(next());  }
    long long   nextLong()   { return std::stoll(next()); }
    float       nextFloat()  { return std::stof(next());  }
    double      nextDouble() { return std::stod(next());  }
    char        nextChar()   { return next().at(0);       }
};

// Buffered stdout. Use '\n' instead of std::endl (endl forces a flush).
class BufferedStdoutWriter : public std::ostream {
public:
    BufferedStdoutWriter() : std::ostream(std::cout.rdbuf()) {
        std::ios::sync_with_stdio(false);
    }
    ~BufferedStdoutWriter() { flush(); }

};

class BufferedStdoutWriterln : public std::ostream {
    
    BufferedStdoutWriterln() : std::ostream(std::cout.rdbuf()) {
        std::ios::sync_with_stdio(false);
    }
    ~BufferedStdoutWriterln() { flush(); }

    template <typename T>
    BufferedStdoutWriterln& operator<<(const T& value) {
        std::cout << value << '\n';
        return *this;
    }
};

}
