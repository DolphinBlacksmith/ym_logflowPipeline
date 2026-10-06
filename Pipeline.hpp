#pragma once

#include "Source.hpp"
#include "Stage.hpp"
#include "Sink.hpp"
#include <stdexcept>

/**
 * @brief Boru Hattı Yöneticisi (Pipeline).
 * 
 * Artım 2 Mimari Gelişimi:
 * Artım 1'de boru hattı yalnızca `Source -> Sink` şeklinde doğrudan dizgi aktarımı yapıyordu.
 * Artım 2'de ise mimari geliştirilerek boru hattının ortasına ara bir filtre/işlemci
 * (`Stage<In, Out>`) eklendi.
 * 
 * Veri Akış Modeli:
 * 1. `Source<In>` veri üretir ve `SourceInputEmitter`'a iletir (`produce`).
 * 2. `SourceInputEmitter`, girdiyi `Stage<In, Out>::process(...)` metoduna aktarır.
 * 3. `Stage`, geçerli veriyi dönüştürerek `StageOutputEmitter`'a iletir (hatalıları atlar).
 * 4. `StageOutputEmitter`, çıktıyı son durak olan `Sink<Out>::consume(...)` metoduna aktarır.
 * 
 * Bu yapı sayesinde boru hattındaki adımlar birbirlerinden tamamen bağımsız ve gevşek bağlıdır (loosely coupled).
 */
template <typename In, typename Out>
class Pipeline {
private:
    Source<In>* source;
    Stage<In, Out>* stage;
    Sink<Out>* sink;

    // Stage çıktısını Sink hedefine ileten dahili Emitter
    class StageOutputEmitter : public Emitter<Out> {
    private:
        Sink<Out>& targetSink;
    public:
        explicit StageOutputEmitter(Sink<Out>& s) : targetSink(s) {}
        void emit(const Out& item) override {
            targetSink.consume(item);
        }
    };

    // Source çıktısını Stage işlemcisine ileten dahili Emitter
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
    /**
     * @brief Source -> Stage -> Sink hattını oluşturan kurucu (Artım 2 Standart Yapısı).
     */
    Pipeline(Source<In>* src, Stage<In, Out>* stg, Sink<Out>* snk)
        : source(src), stage(stg), sink(snk) {}

    /**
     * @brief Boru hattını çalıştırır, yaşam döngüsünü (open/close) yönetir ve veri akışını koordine eder.
     */
    void run() {
        if (!source || !stage || !sink) {
            throw std::runtime_error("Source, Stage ve Sink bos (null) olamaz!");
        }

        stage->open();
        StageOutputEmitter stageOutEmitter(*sink);
        SourceInputEmitter sourceInEmitter(*stage, stageOutEmitter);
        source->produce(sourceInEmitter);
        stage->close();
    }
};
