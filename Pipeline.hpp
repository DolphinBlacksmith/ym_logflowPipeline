#pragma once
#include "Source.hpp"
#include "Sink.hpp"
#include <stdexcept>

template <typename I, typename O>
class Pipeline {
private:
    Source<I>* source;
    Sink<O>* sink;

    class InternalEmitter : public Emitter<I> {
        Sink<O>& targetSink;
    public:
        explicit InternalEmitter(Sink<O>& s) : targetSink(s) {}
        void emit(const I& item) /* override silindi */ {
            targetSink.consume(item);
        }
    };

public:
    // HATA DÜZELTİLDİ: source(src), source(src) yerine tek bir source(src) yapıldı
    Pipeline(Source<I>* src, Sink<O>* snk) : source(src), sink(snk) {}

    void run() {
        if (!source || !sink) {
            throw std::runtime_error("Source or Sink cannot be null!");
        }
        InternalEmitter emitter(*sink);
        source->produce(emitter);
    }
};
