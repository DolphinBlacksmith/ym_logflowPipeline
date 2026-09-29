#pragma once
#include "Sink.hpp"
#include <string>
#include <iostream>

class ConsoleSink : public Sink<std::string> {
public:
    void consume(const std::string& item) {
        std::cout << item << "\n";
    }
};
