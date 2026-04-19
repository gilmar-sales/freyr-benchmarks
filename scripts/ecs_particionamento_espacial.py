import pandas as pd
import plotly.express as px
from benchmark import Benchmark
from pathlib import Path

benchmark = Benchmark("ecs_particionamento_espacial")

benchmark.execute()
bench_data = pd.read_csv(benchmark.output)


def getFrameTime(nanoseconds):
    return 1000 / (nanoseconds / 1000000)


def getName(name):
    split = name.split("/")

    if len(split) <= 2:
        return split[0]

    return f"{split[0]}_{split[2]}Trabalhadores"


data = pd.DataFrame(
    {
        "Cenário": bench_data["name"].map(getName),
        "Entidades": bench_data["name"].map(lambda name: int(name.split("/")[1])),
        "iterations": bench_data["iterations"],
        "Tempo(ms)": bench_data["real_time"].map(lambda time: time / 1000000),
        "Tempo": bench_data["real_time"].map(lambda time: f"{int(time / 1000000)}ms"),
        "Quadros/s": bench_data["real_time"].map(getFrameTime),
        "Quadros": bench_data["real_time"]
        .map(getFrameTime)
        .map(lambda quadros: f"{int(quadros)}fps"),
    }
).sort_values(by=["Cenário", "Entidades"])

fig = px.line(
    data,
    x="Entidades",
    y="Tempo(ms)",
    color="Cenário",
    title="Tempo de execução de particionamento espacial",
    text="Tempo",
    markers=True,
)
fig.update_traces(textposition="bottom right")
fig.write_html(Path("../resources/ecs_particionamento_espacial_tempo_execucao.html"))

fig = px.line(
    data,
    x="Entidades",
    y="Quadros/s",
    range_y=[0, 1000],
    color="Cenário",
    title="Taxa de atualização de particionamento espacial",
    text="Quadros",
    markers=True,
)
fig.update_traces(textposition="bottom right")
fig.write_html(Path("../resources/ecs_particionamento_espacial_fps.html"))
