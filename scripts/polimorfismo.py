import pandas as pd
import plotly.express as px
from benchmark import Benchmark
from pathlib import Path

benchmark = Benchmark("polimorfismo")

benchmark.execute()
bench_data = pd.read_csv(benchmark.output)


def getFrameTime(nanoseconds):

    return 1000 / (nanoseconds / 1000000)


data = pd.DataFrame(
    {
        "Cenário": bench_data["name"].map(lambda name: name.split("/")[0]),
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
    title="Tempo de execução dos cenários",
    text="Tempo",
    markers=True,
)
fig.update_traces(textposition="bottom right")
fig.write_html(Path("../resources/polimorfismo_tempo_execucao.html"))

fig = px.line(
    data,
    x="Entidades",
    y="Quadros/s",
    range_y=[0, 1000],
    color="Cenário",
    title="Tempo de execução dos cenários",
    text="Quadros",
    markers=True,
)
fig.update_traces(textposition="bottom right")
fig.write_html(Path("../resources/polimorfismo_fps.html"))
