#include <benchmark/benchmark.h>

class GameObject {
public:
  GameObject() = default;
  ~GameObject() = default;

  virtual void Start() {};
  virtual void Update() {};
  virtual void AfterUpdate() {};
};

class DynamicObject : public GameObject {
public:
  void Update() override {
    x += 1.0f;
    y += 1.0f;
    z += 1.0f;
  }

private:
  float x, y, z;
};

struct Position {
  float x;
  float y;
  float z;
};

static void BM_OOP_Update_Fragmented(benchmark::State &state) {
  auto gameObjects = std::vector<GameObject *>();

  for (auto i = 0; i < state.range(); i++) {
    gameObjects.push_back(new DynamicObject());
  }

  for (auto _ : state) {
    for (const auto gameObject : gameObjects) {
      gameObject->Update();
    }
  }

  for (const auto gameObject : gameObjects) {
    delete gameObject;
  }
}

BENCHMARK(BM_OOP_Update_Fragmented)->RangeMultiplier(2)->Range(1, 10'000'000);

static void BM_OOP_Update_Contiguous(benchmark::State &state) {
  auto gameObjects = std::vector<GameObject *>();

  auto dynamicObjects = new DynamicObject[state.range()];
  for (auto i = 0; i < state.range(); i++) {
    gameObjects.push_back(&dynamicObjects[i]);
  }

  for (auto _ : state) {
    for (const auto gameObject : gameObjects) {
      gameObject->Update();
    }
  }
}

BENCHMARK(BM_OOP_Update_Contiguous)->RangeMultiplier(2)->Range(1, 10'000'000);

static void BM_DOD_Update_Fragmented(benchmark::State &state) {
  auto positions = std::vector<Position *>();

  for (auto i = 0; i < state.range(); i++) {
    positions.push_back(new Position());
  }

  for (auto _ : state) {
    for (auto &position : positions) {
      position->x += 1.0f;
      position->y += 1.0f;
      position->z += 1.0f;
    }
  }
}

BENCHMARK(BM_DOD_Update_Fragmented)->RangeMultiplier(2)->Range(1, 10'000'000);

static void BM_DOD_Update_Contiguous(benchmark::State &state) {
  auto positions = std::vector<Position>(state.range());

  for (auto _ : state) {
    for (auto &position : positions) {
      position.x += 1.0f;
      position.y += 1.0f;
      position.z += 1.0f;
    }
  }
}

BENCHMARK(BM_DOD_Update_Contiguous)->RangeMultiplier(2)->Range(1, 10'000'000);

BENCHMARK_MAIN();