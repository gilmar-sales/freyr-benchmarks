from pathlib import Path

import pandas as pd
import plotly.express as px
from benchmark import Benchmark

benchmark = Benchmark("update_method_dod")

benchmark.execute()

bench_data = pd.read_csv(benchmark.output)

columns = [
    "name",
    "iterations",
    "real_time",
    "cpu_time",
    "time_unit",
    "bytes_per_second",
    "items_per_second",
    "label",
    "error_occurred",
    "error_message",
]

data = bench_data[columns].copy()
if data.empty:
    data = pd.DataFrame(columns=columns)

data["entities"] = pd.to_numeric(data["name"].str.split("/").str[1])
data["benchmark"] = data["name"].str.split("/").str[0]
data["name_base"] = data["benchmark"]
data["real_time_ms"] = (data["real_time"] / 1_000_000).round()
data["cpu_time_ms"] = data["cpu_time"] / 1_000_000
data["fps"] = (1000 / data["real_time_ms"]).round()
data["text"] = data["real_time_ms"]

data = data.sort_values(by=["name_base", "entities"])

fig = px.line(
    data,
    x="entities",
    y="real_time_ms",
    color="name_base",
    markers=True,
    text="text",
    title="Tempo de Execução — MetodoUpdate",
    labels={
        "entities": "Entidades",
        "real_time_ms": "Tempo (ms)",
        "name_base": "Cenario",
    },
)
fig.update_traces(textposition="top center", texttemplate="%{text} ms")
fig.write_html(Path("../resources/update_method_dod_tempo_execucao.html"))

fig = px.line(
    data,
    x="entities",
    y="fps",
    color="name_base",
    markers=True,
    text="fps",
    title="FPS — MetodoUpdate",
    labels={
        "entities": "Entidades",
        "fps": "FPS",
        "name_base": "Cenario",
    },
)
fig.update_traces(textposition="top center", texttemplate="%{text} FPS")
fig.write_html(Path("../resources/update_method_dod_fps.html"))
