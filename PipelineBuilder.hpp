#pragma once

#include "Source.hpp"
#include "Stage.hpp"
#include "Sink.hpp"
#include "Emitter.hpp"
#include <functional>
#include <vector>
#include <memory>
#include <stdexcept>

// İleri bildirim
template <typename In, typename Out>
class Pipeline;

/**
 * @brief Akıcı Boru Hattı İnşa Adımı (PipelineBuilderStep).
 * 
 * Derleme zamanında tür güvenliği (compile-time type safety) sağlayarak
 * aşamaların birbirine uyumlu tiplerle zincirlenmesini güvenceye alır.
 */
template <typename CurrentType>
class PipelineBuilderStep {
public:
    std::function<void(Emitter<CurrentType>&)> runner;
    std::vector<std::function<void()>> openHooks;
    std::vector<std::function<void()>> closeHooks;
    std::vector<std::function<void()>> deleters;

    /**
     * @brief Araya yeni bir aşama (Stage<CurrentType, NextType>) ekler.
     * @param stage Eklenecek aşama göstericisi
     * @param own true ise boru hattı sonlandığında nesne bellekten silinir
     */
    template <typename NextType>
    PipelineBuilderStep<NextType> via(Stage<CurrentType, NextType>* stage, bool own = true) {
        if (!stage) {
            throw std::invalid_argument("Stage bos (null) olamaz!");
        }

        PipelineBuilderStep<NextType> next;
        next.openHooks = this->openHooks;
        next.openHooks.push_back([stage]() { stage->open(); });

        next.closeHooks = this->closeHooks;
        next.closeHooks.push_back([stage]() { stage->close(); });

        next.deleters = this->deleters;
        if (own) {
            next.deleters.push_back([stage]() { delete stage; });
        }

        auto prevRunner = this->runner;
        next.runner = [prevRunner, stage](Emitter<NextType>& out) {
            struct MidEmitter : public Emitter<CurrentType> {
                Stage<CurrentType, NextType>* s;
                Emitter<NextType>& o;
                MidEmitter(Stage<CurrentType, NextType>* s, Emitter<NextType>& o) : s(s), o(o) {}
                void emit(const CurrentType& item) override {
                    s->process(item, o);
                }
            } midEmitter(stage, out);
            prevRunner(midEmitter);
        };
        return next;
    }

    /**
     * @brief Boru hattını bir hedef (Sink<CurrentType>) ile sonlandırır ve Pipeline üretir.
     * @param sink Son hedef göstericisi
     * @param own true ise boru hattı sonlandığında nesne bellekten silinir
     */
    Pipeline<void, void> to(Sink<CurrentType>* sink, bool own = true);
};

/**
 * @brief Akıcı (Fluent) Boru Hattı Oluşturucu (PipelineBuilder).
 * 
 * Artım 3 Mimarisi:
 * Farklı sorumluluklara sahip aşamaların herhangi bir sayıda ve sırada
 * bir araya getirilmesini (composability) sağlar.
 * 
 * Örnek:
 * Pipeline p = PipelineBuilder::from(new FileLineSource(in))
 *     .via(new ParserStage())
 *     .via(new PredicateFilterStage(r -> r.status() >= 400))
 *     .via(new ProjectionStage("timestamp","clientIp","status","path"))
 *     .to(new FileSink(out));
 */
class PipelineBuilder {
public:
    template <typename T>
    static PipelineBuilderStep<T> from(Source<T>* src, bool own = true) {
        if (!src) {
            throw std::invalid_argument("Source bos (null) olamaz!");
        }

        PipelineBuilderStep<T> step;
        step.openHooks = {};
        step.closeHooks = {};
        if (own) {
            step.deleters.push_back([src]() { delete src; });
        }
        step.runner = [src](Emitter<T>& out) {
            src->produce(out);
        };
        return step;
    }
};
