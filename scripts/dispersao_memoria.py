import pandas as pd
import plotly.express as px
from benchmark import Benchmark
from pathlib import Path

benchmark = Benchmark("dispersao_memoria")

benchmark.execute()
bench_data = pd.read_csv(benchmark.output)

fig = px.scatter(
    bench_data, x="n", y="deslocamento", title="Dispersão alocações dinâmicas"
)
fig.write_html(Path("../resources/dispersao_memoria.html"))
