
#include <complex>
#include <fstream>
#include <vector>
#include <cstdint>

class GameObject {
public:
    GameObject() = default;

    virtual ~GameObject() = default;

    virtual void Start() {
    };

    virtual void Update() {
    };

    virtual void AfterUpdate() {
    };
};

class DynamicObject final : public GameObject {
public:
    DynamicObject()
        : x(0),
          y(0),
          z(0) {
    }

    void Update() override {
        x += 1.0f;
        y += 1.0f;
        z += 1.0f;
    }

private:
    float x, y, z;
};

constexpr auto GameObjectCount = 10'000;

int main() {
    auto file = std::fstream("../memory_fragmentation.csv", std::fstream::out);
    file.clear();

    file << "n,offset,x,y" << std::endl;

    auto gameObjects = std::vector<GameObject *>();

    for (int i = 0; i < GameObjectCount; i++) {
        gameObjects.push_back(new DynamicObject());
    }

    const auto minAddress = *std::min(gameObjects.begin(), gameObjects.end());

    const auto half = static_cast<std::uint64_t>(std::sqrt(GameObjectCount));

    auto i = 0;
    for (auto gameObject: gameObjects) {
        const auto offset = (gameObject - minAddress);
        const auto x = (gameObject - minAddress) / half;
        const auto y = (gameObject - minAddress) % half;

        file << i++ << "," << offset << "," << x << "," << y << std::endl;
    }


    return 0;
}
