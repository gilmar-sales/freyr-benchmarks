# freyr-benchmarks

C++ CMake project benchmarking ECS (Entity Component System) implementations: Freyr, EnTT, Flecs, and Gaia.

## Build

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
```

Benchmarks are fetched via FetchContent (google-benchmark, glm, freyr, entt, tbb, flecs, gaia). No manual dependency install needed.

- Release builds enable LTO/IPO and `-march=native` / `-O3 -ffast-math -funroll-loops`
- Individual benchmarks: `build/conjunto_trabalho`, `build/ecs_iteracao_assincrona`, etc.

## Run

```bash
./cpu_power.sh  # Requires sudo - sets CPU governor to performance
./build/ecs_iteracao_assincrona --benchmark_format=csv > results.csv
```

Python scripts in `scripts/` run benchmarks and collect results. `scripts/run_all.sh` runs all `*.py` scripts except `benchmark.py`.

## Style

- `.clang-format` with Microsoft style, column limit 0, 4-space indent
- C++17 minimum (some targets require C++20: `conjunto_trabalho`)

## Architecture

- `src/*.cpp` — individual benchmark executables using googlebenchmark
- `src/bevy-freyr/` — Rust/Cargo subprojects (Bevy integration tests)
- `src/Components/`, `src/Containers/` — shared C++ components and data structures
- `src/profiling.hpp` — shared profiling utilities

## Key Targets

| Target | Purpose |
|--------|---------|
| `conjunto_trabalho` | Data-oriented design vs OOP comparison |
| `ecs_iteracao_sequencial` | EnTT vs Freyr sequential iteration |
| `ecs_iteracao_assincrona` | EnTT vs Freyr async iteration with TBB |
| `ecs_particionamento_espacial` | Spatial partitioning with Octree |

## bevy-freyr Rust Subprojects

Located in `src/bevy-freyr/freyr/` and `src/bevy-freyr/entt_tbb/`. Build with `cargo build --release`. Profiling requires Tracy (`cargo run --release --features bevy/trace_tracy,bevy/debug`).
