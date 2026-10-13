#pragma once

#include "Source.hpp"
#include "Stage.hpp"
#include "Sink.hpp"
#include "PipelineBuilder.hpp"
#include <stdexcept>
#include <functional>
#include <vector>

template <typename In = void, typename Out = void>
class Pipeline;

/**
 * @brief Genel ve Dinamik Boru Hattı (Artım 3 - PipelineBuilder Çıktısı).
 */
template <>
class Pipeline<void, void> {
private:
    std::function<void()> runFn_;
    std::vector<std::function<void()>> deleters_;

public:
    Pipeline() = default;

    Pipeline(std::function<void()> runFn, std::vector<std::function<void()>> deleters)
        : runFn_(std::move(runFn)), deleters_(std::move(deleters)) {}

    ~Pipeline() {
        for (auto& del : deleters_) {
            if (del) del();
        }
    }

    Pipeline(Pipeline&& other) noexcept
        : runFn_(std::move(other.runFn_)), deleters_(std::move(other.deleters_)) {
        other.deleters_.clear();
    }

    Pipeline& operator=(Pipeline&& other) noexcept {
        if (this != &other) {
            for (auto& del : deleters_) {
                if (del) del();
            }
            runFn_ = std::move(other.runFn_);
            deleters_ = std::move(other.deleters_);
            other.deleters_.clear();
        }
        return *this;
    }

    Pipeline(const Pipeline&) = delete;
    Pipeline& operator=(const Pipeline&) = delete;

    void run() {
        if (runFn_) {
            runFn_();
        }
    }
};

/**
 * @brief Tipli Boru Hattı (Artım 2 Geriye Dönük Uyumluluk).
 */
template <typename In, typename Out>
class Pipeline {
private:
    Source<In>* source = nullptr;
    Stage<In, Out>* stage = nullptr;
    Sink<Out>* sink = nullptr;

    class StageOutputEmitter : public Emitter<Out> {
    private:
        Sink<Out>& targetSink;
    public:
        explicit StageOutputEmitter(Sink<Out>& s) : targetSink(s) {}
        void emit(const Out& item) override {
            targetSink.consume(item);
        }
    };

    class SourceInputEmitter : public Emitter<In> {
    private:
        Stage<In, Out>& targetStage;
        StageOutputEmitter& outputEmitter;
    public:
        SourceInputEmitter(Stage<In, Out>& stg, StageOutputEmitter& outEmit)
            : targetStage(stg), outputEmitter(outEmit) {}

        void emit(const In& item) override {
            targetStage.process(item, outputEmitter);
        }
    };

public:
    Pipeline(Source<In>* src, Stage<In, Out>* stg, Sink<Out>* snk)
        : source(src), stage(stg), sink(snk) {}

    void run() {
        if (!source || !stage || !sink) {
            throw std::runtime_error("Source, Stage ve Sink bos (null) olamaz!");
        }

        sink->open();
        stage->open();
        try {
            StageOutputEmitter stageOutEmitter(*sink);
            SourceInputEmitter sourceInEmitter(*stage, stageOutEmitter);
            source->produce(sourceInEmitter);
        } catch (...) {
            stage->close();
            sink->close();
            throw;
        }
        stage->close();
        sink->close();
    }
};

template <typename CurrentType>
Pipeline<void, void> PipelineBuilderStep<CurrentType>::to(Sink<CurrentType>* sink, bool own) {
    if (!sink) {
        throw std::invalid_argument("Sink bos (null) olamaz!");
    }

    auto opens = this->openHooks;
    opens.push_back([sink]() { sink->open(); });

    auto closes = this->closeHooks;
    closes.push_back([sink]() { sink->close(); });

    auto dels = this->deleters;
    if (own) {
        dels.push_back([sink]() { delete sink; });
    }

    auto theRunner = this->runner;
    auto runAll = [theRunner, sink, opens, closes]() {
        for (auto& o : opens) {
            o();
        }

        std::exception_ptr caughtException = nullptr;
        try {
            struct FinalEmitter : public Emitter<CurrentType> {
                Sink<CurrentType>* s;
                FinalEmitter(Sink<CurrentType>* s) : s(s) {}
                void emit(const CurrentType& item) override {
                    s->consume(item);
                }
            } finalEmitter(sink);

            theRunner(finalEmitter);
        } catch (...) {
            caughtException = std::current_exception();
        }

        for (auto it = closes.rbegin(); it != closes.rend(); ++it) {
            try {
                (*it)();
            } catch (...) {
                if (!caughtException) caughtException = std::current_exception();
            }
        }

        if (caughtException) {
            std::rethrow_exception(caughtException);
        }
    };

    return Pipeline<void, void>(runAll, dels);
}
