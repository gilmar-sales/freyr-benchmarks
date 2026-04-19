#include <atomic>
#include <benchmark/benchmark.h>
#include <fstream>
#include <mutex>
#include <string>
#include <thread>

#include "profiling.hpp"

bool is_prime(int n)
{
    if (n <= 1)
        return false;

    for (int i = 2; i * i <= n; ++i)
    {
        if (n % i == 0)
            return false;
    }

    return true;
}

const thread_local int MAX_NUMBER = 10'000'000;

std::vector<std::string> labels = {};
std::mutex               labelsMutex;
std::atomic<long>        trackSequence = 1;

const std::string& NovaLabel(const std::string& label)
{
    std::lock_guard<std::mutex> lock(labelsMutex);
    return labels.emplace_back(label);
}

const auto TrackPrincipal = perfetto::ProcessTrack::Current();

void DividirPorBlocos(int threadCount)
{
    const auto& trackDividirPorBlocos = perfetto::Track::ThreadScoped((void*) trackSequence.fetch_add(1), TrackPrincipal);

    USER_TRACE_BEGIN("DividirPorBlocos", trackDividirPorBlocos);

    std::vector<std::thread> threads;
    int                      blockSize = MAX_NUMBER / threadCount;

    for (int i = 0; i < threadCount; ++i)
    {
        threads.emplace_back(std::thread([i, blockSize, &trackDividirPorBlocos]() {
            const auto& track = perfetto::Track::ThreadScoped(nullptr, trackDividirPorBlocos);
            const auto& label = NovaLabel("DividirPorBlocos Thread: " + std::to_string(i));

            int start = i * blockSize + 1;
            int end   = (i + 1) * blockSize;
            USER_TRACE_BEGIN(label.c_str(), track, "start", start, "end", end);

            for (int j = start; j <= end && j <= MAX_NUMBER; ++j)
            {
                benchmark::DoNotOptimize(is_prime(j));
            }

            USER_TRACE_END(track);
        }));
    }

    for (auto& thread : threads)
    {
        if (thread.joinable())
            thread.join();
    }

    USER_TRACE_END(trackDividirPorBlocos);
}

void DividirAlternandoNumeros(int threadCount)
{
    const auto& trackAlternando = perfetto::Track::ThreadScoped((void*) trackSequence.fetch_add(1), TrackPrincipal);

    USER_TRACE_BEGIN("DividirAlternandoNumeros", trackAlternando);

    std::vector<std::thread> threads;
    for (int i = 0; i < threadCount; ++i)
    {
        threads.emplace_back(std::thread([i, threadCount, &trackAlternando]() {
            const auto& track = perfetto::Track::ThreadScoped(nullptr, trackAlternando);
            const auto& label = NovaLabel("DividirAlternandoNumeros Thread: " + std::to_string(i));

            USER_TRACE_BEGIN(label.c_str(), track, "start", i + 1, "threadCount", threadCount);
            for (int j = i + 1; j <= MAX_NUMBER; j += threadCount)
            {
                benchmark::DoNotOptimize(is_prime(j));
            }
            USER_TRACE_END(track);
        }));
    }

    for (auto& thread : threads)
    {
        if (thread.joinable())
            thread.join();
    }
    USER_TRACE_END(trackAlternando);
}

void DividirDinamicamente(int threadCount, int high_low_size)
{
    const auto& trackDinamicamente = perfetto::Track::ThreadScoped((void*) trackSequence.fetch_add(1), TrackPrincipal);
    const auto& labelDinamicamente = NovaLabel("DividirDinamicamente x " + std::to_string(high_low_size));
    USER_TRACE_BEGIN(labelDinamicamente.c_str(), trackDinamicamente);

    std::vector<std::thread> threads;
    std::atomic<int>         high_low = 1;

    for (int i = 0; i < threadCount; ++i)
    {
        threads.emplace_back(std::thread([i, &high_low, high_low_size, &trackDinamicamente, &labelDinamicamente]() {
            const auto& track = perfetto::Track::ThreadScoped(nullptr, trackDinamicamente);
            const auto& label = NovaLabel(labelDinamicamente + " Thread: " + std::to_string(i));

            USER_TRACE_BEGIN(label.c_str(), track);

            auto high = 0;
            do
            {
                high = high_low.fetch_add(high_low_size);

                for (int j = high - high_low_size; j <= high && j <= MAX_NUMBER; ++j)
                {
                    benchmark::DoNotOptimize(is_prime(j));
                }
            } while (high - high_low_size < MAX_NUMBER);

            USER_TRACE_END(track);
        }));
    }

    for (auto& thread : threads)
    {
        if (thread.joinable())
            thread.join();
    }
    USER_TRACE_END(trackDinamicamente);
}

int main()
{
    labels.reserve(100);

    std::unique_ptr<perfetto::TracingSession> mTracingSession;

    auto args = perfetto::TracingInitArgs();
    args.backends |= perfetto::kInProcessBackend;

    perfetto::Tracing::Initialize(args);
    perfetto::TrackEvent::Register();

    perfetto::protos::gen::TrackEventConfig track_event_cfg;

    perfetto::TraceConfig cfg;
    cfg.add_buffers()->set_size_kb(1024 * 1024); // Record up to 1 GiB.

    auto* ds_cfg = cfg.add_data_sources()->mutable_config();
    ds_cfg->set_name("track_event");
    ds_cfg->set_track_event_config_raw(track_event_cfg.SerializeAsString());

    mTracingSession = perfetto::Tracing::NewTrace();
    mTracingSession->Setup(cfg);

    mTracingSession->StartBlocking();

    USER_TRACE_BEGIN("Thread Principal", TrackPrincipal);
    auto threads = std::vector<std::thread>();

    threads.push_back(std::thread([]() {
        DividirPorBlocos(4);
    }));

    threads.push_back(std::thread([]() {
        DividirAlternandoNumeros(4);
    }));

    threads.push_back(std::thread([]() {
        DividirDinamicamente(4, 1000);
    }));

    for (auto& thread : threads)
    {
        if (thread.joinable())
            thread.join();
    }

    USER_TRACE_END(TrackPrincipal);

    mTracingSession->StopBlocking();

    const auto trace_data = mTracingSession->ReadTraceBlocking();

    std::ofstream output;
    output.open("divisao_carga_trabalho.pftrace", std::ios::out | std::ios::binary);
    output.write(&trace_data[0], trace_data.size());
    output.close();
}
