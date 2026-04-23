#include <chrono>
#include <fcntl.h>

#include <entt/entt.hpp>
#include <perfetto.h>
#include <tbb/blocked_range.h>
#include <tbb/parallel_for.h>
#include <tbb/tbb.h>

namespace perfetto
{
    class TracingSession;
}

PERFETTO_DEFINE_CATEGORIES(
    perfetto::Category("entt_tbb").SetDescription("Application"));

PERFETTO_TRACK_EVENT_STATIC_STORAGE();

struct Position
{
    float x;
    float y;
    float z;
};

struct Velocity
{
    float x;
    float y;
    float z;
};

struct Acceleration
{
    float x;
    float y;
    float z;
};

constexpr auto BlockSize = 8192;

int main()
{
    auto args = perfetto::TracingInitArgs();
    args.backends |= perfetto::kInProcessBackend;
    args.shmem_size_hint_kb = 32 * 1024;
    perfetto::Tracing::Initialize(args);
    perfetto::TrackEvent::Register();

    perfetto::TraceConfig cfg;
    cfg.set_write_into_file(true);     // Stream to disk, not RAM
    cfg.set_file_write_period_ms(100); // Drain buffer every 500ms
    cfg.set_flush_period_ms(1000);     // Flush SMB every 10s to avoid out-of-order drops
    cfg.mutable_incremental_state_config()->set_clear_period_ms(500);

    auto* buf = cfg.add_buffers();
    buf->set_size_kb(512 * 1024);
    buf->set_fill_policy(
        perfetto::TraceConfig::BufferConfig::RING_BUFFER);

    auto* ds_cfg = cfg.add_data_sources()->mutable_config();
    ds_cfg->set_name("track_event");

    perfetto::protos::gen::TrackEventConfig te_cfg;
    te_cfg.add_enabled_categories("entt_tbb");
    te_cfg.set_disable_incremental_timestamps(false);
    ds_cfg->set_track_event_config_raw(te_cfg.SerializeAsString()); // ← fix the bug here too

    auto       session = perfetto::Tracing::NewTrace();
    const auto output  = std::format("entt_tbb_trace_{}.pftrace", std::chrono::system_clock::now().time_since_epoch().count());
    int        fd      = open(output.c_str(), O_RDWR | O_CREAT | O_TRUNC, 0600);
    session->Setup(cfg, fd);
    session->StartBlocking();

    entt::registry registry;

    for (int i = 0; i < 1'000'000; ++i)
    {
        auto entity = registry.create();
        registry.emplace<Position>(entity, 0.0f, 0.0f, 0.0f);
        registry.emplace<Velocity>(entity, 0.0f, 0.0f, 0.0f);
        registry.emplace<Acceleration>(entity, 1.0f, 1.0f, 1.0f);
    }

    auto group = registry.group<Position, Velocity, Acceleration>();
    auto count = group.size();

    for (int i = 0; i < 1000; ++i)
    {
        TRACE_EVENT("entt_tbb", "Update");
        thread_local float deltaTime = 0.016f;
        tbb::parallel_for(tbb::blocked_range<size_t>(0, count, BlockSize), [&group](const tbb::blocked_range<size_t>& range) {
            TRACE_EVENT("entt_tbb", "ApplyGravitySystem");
            for (auto i = range.begin(); i != range.end(); ++i)
            {
                auto  entity       = group[i];
                auto& acceleration = group.get<Acceleration>(entity);
                acceleration.y -= 9.84f * deltaTime;
            }
        });

        auto count_vel_accell = group.size();
        tbb::parallel_for(tbb::blocked_range<size_t>(0, count, BlockSize), [&group](const tbb::blocked_range<size_t>& range) {
            TRACE_EVENT("entt_tbb", "ApplyAccelerationSystem");
            for (auto i = range.begin(); i != range.end(); ++i)
            {
                auto entity                   = group[i];
                auto [velocity, acceleration] = group.get<Velocity, Acceleration>(entity);
                velocity.x += acceleration.x * deltaTime;
                velocity.y += acceleration.y * deltaTime;
                velocity.z += acceleration.z * deltaTime;
            }
        });

        tbb::parallel_for(tbb::blocked_range<size_t>(0, count, BlockSize), [&group](const tbb::blocked_range<size_t>& range) {
            TRACE_EVENT("entt_tbb", "ApplyVelocitySystem");
            for (auto i = range.begin(); i != range.end(); ++i)
            {
                auto entity               = group[i];
                auto [position, velocity] = group.get<Position, Velocity>(entity);
                position.x += velocity.x * deltaTime;
                position.y += velocity.y * deltaTime;
                position.z += velocity.z * deltaTime;
            }
        });
    }

    session->StopBlocking();
    close(fd);

    return 0;
}
