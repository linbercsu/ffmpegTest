//
// Created by linlin zhao on 2024/5/23.
//

#pragma once

#include <exception>

namespace next {

class DecoderException : public std::exception {
public:
    DecoderException(int error);

    const char * what() const noexcept override {
        return "DecoderException";
    }

private:
    int mError;
};

    class Exception {

    };
}


