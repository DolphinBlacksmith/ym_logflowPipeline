#pragma once
#include "Emitter.hpp"

template <typename I, typename O>
class Stage {
public:
    virtual ~Stage() {}
    virtual void process(const I& input, Emitter<O>& out) = 0;
    virtual void open() {}
    virtual void close() {}
};
