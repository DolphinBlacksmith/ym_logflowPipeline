#pragma once

template <typename I>
class Sink {
public:
    virtual ~Sink() {}
    virtual void consume(const I& item) = 0;
};
