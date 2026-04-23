#include <Freyr/Freyr.hpp>

struct Position : fr::Component
{
    float x;
    float y;
    float z;
};

struct Velocity : fr::Component
{
    float x;
    float y;
    float z;
};

struct Acceleration : fr::Component
{
    float x;
    float y;
    float z;
};

class ApplyGravitySystem : fr::System
{

  public:
    explicit ApplyGravitySystem(const Ref<fr::Scene>& scene) : System(scene) {}

    void Update(float deltaTime) override
    {
        mScene->CreateQuery()->EachAsync<Acceleration>([deltaTime](auto entity, Acceleration& acceleration) {
            acceleration.y -= 9.84f * deltaTime;
        });
    }
};

class ApplyAccelerationSystem : fr::System
{

  public:
    explicit ApplyAccelerationSystem(const Ref<fr::Scene>& scene) : System(scene) {}

    void Update(float deltaTime) override
    {
        mScene->CreateQuery()->EachAsync<Velocity, Acceleration>([deltaTime](auto entity, Velocity& velocity, const Acceleration& acceleration) {
            velocity.x += acceleration.x * deltaTime;
            velocity.y += acceleration.y * deltaTime;
            velocity.z += acceleration.z * deltaTime;
        });
    }
};

class ApplyVelocitySystem : fr::System
{

  public:
    explicit ApplyVelocitySystem(const Ref<fr::Scene>& scene) : System(scene) {}

    void Update(float deltaTime) override
    {
        mScene->CreateQuery()->EachAsync<Position, Velocity>([deltaTime](auto entity, Position& position, const Velocity& velocity) {
            position.x += velocity.x * deltaTime;
            position.y += velocity.y * deltaTime;
            position.z += velocity.z * deltaTime;
        });
    }
};

class MyApp final : public skr::IApplication
{
  public:
    explicit MyApp(const Ref<skr::ServiceProvider>& rootServiceProvider) :
        IApplication(rootServiceProvider), mScene(rootServiceProvider->GetService<fr::Scene>()) {}

    void Run() override
    {
        mScene->BeginProfiling();

        mScene->CreateArchetypeBuilder()
            .WithComponent(Position {})
            .WithComponent(Acceleration { .x = 1.0f, .y = 1.0f, .z = 1.0f })
            .WithComponent(Velocity {})
            .WithEntities(1'000'000)
            .Build();

        for (auto i = 0; i < 1000; i++)
        {
            mScene->Update(0.016f);
        }

        mScene->EndProfiling();
    }

  private:
    Ref<fr::Scene> mScene;
};

int main()
{
    auto app = skr::ApplicationBuilder()
                   .WithExtension<fr::FreyrExtension>([&](fr::FreyrExtension& freyr) {
                       freyr.WithOptions([&](fr::FreyrOptionsBuilder& freyrOptions) {
                                freyrOptions
                                    .WithThreadCount(std::thread::hardware_concurrency() - 1)
                                    .WithArchetypeChunkCapacity(4096);
                            })
                           .WithComponent<Position>()
                           .WithComponent<Velocity>()
                           .WithComponent<Acceleration>()
                           .WithPipeline([](fr::PipelineBuilder& pipeline) {
                               pipeline
                                   .WithSystem<ApplyGravitySystem>()
                                   .WithSystem<ApplyAccelerationSystem>()
                                   .WithSystem<ApplyVelocitySystem>();
                           });
                   })
                   .Build<MyApp>();

    app->Run();

    return 0;
}
