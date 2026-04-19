#include <benchmark/benchmark.h>

class GameObject
{
  public:
    GameObject()          = default;
    virtual ~GameObject() = default;

    virtual void Start() {};
    virtual void BeforeUpdate() {};
    virtual void Update() {};
    virtual void AfterUpdate() {};
};

class DynamicObject final : public GameObject
{
  public:
    void Update() override
    {
        x += 1.0f;
        y += 1.0f;
        z += 1.0f;
    }

  private:
    float x {}, y {}, z {};
};

struct Position
{
    float x;
    float y;
    float z;
};

static void MetodoUpdate_ChamadaDireta(benchmark::State& state)
{
    auto gameObjects = std::vector<DynamicObject>(state.range());

    for (auto _ : state)
    {
        for (auto& gameObject : gameObjects)
        {
            gameObject.Update();
        }
    }
}

static void MetodoUpdate_Polimorfismo(benchmark::State& state)
{
    auto gameObjects = std::vector<GameObject*>();

    auto dynamicObjects = new DynamicObject[state.range()];

    for (auto i = 0; i < state.range(); i++)
    {
        gameObjects.push_back(&dynamicObjects[i]);
    }

    for (auto _ : state)
    {
        for (const auto gameObject : gameObjects)
        {
            gameObject->Update();
        }
    }

    delete[] dynamicObjects;
}

BENCHMARK(MetodoUpdate_Polimorfismo)
    ->Iterations(1000)
    ->DenseRange(4'000'000, 10'000'000, 1'000'000)
    ->ReportAggregatesOnly(true);

BENCHMARK(MetodoUpdate_ChamadaDireta)
    ->Iterations(1000)
    ->DenseRange(4'000'000, 10'000'000, 1'000'000)
    ->ReportAggregatesOnly(true);

BENCHMARK_MAIN();
