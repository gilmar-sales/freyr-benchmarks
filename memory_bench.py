import pandas as pd
import plotly.graph_objects as go
from plotly.subplots import make_subplots

bench_data = pd.read_csv("memory_bench.csv", header=0)

print(bench_data)

fig = make_subplots(rows=1, cols=4)

fig.add_trace(
    go.Bar(x=bench_data["Tipo"], y=bench_data["Leitura"], name="Leitura(GB/s)"), 1, 1
)

fig.add_trace(
    go.Bar(x=bench_data["Tipo"], y=bench_data["Escrita"], name="Escrita(GB/s)"), 1, 2
)

fig.add_trace(
    go.Bar(x=bench_data["Tipo"], y=bench_data["Cópia"], name="Cópia(GB/s)"), 1, 3
)

fig.add_trace(
    go.Bar(x=bench_data["Tipo"], y=bench_data["Latência"], name="Latência(ns)"), 1, 4
)

fig.show()
