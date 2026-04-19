import pandas as pd
import plotly.express as px
from benchmark import Benchmark
from pathlib import Path

benchmark = Benchmark("dispersao_memoria")
benchmark.execute()

bench_data = pd.read_csv(benchmark.output)
bench_data["offset_diff"] = bench_data["deslocamento"].diff()

bench_data["estado_do_cache"] = bench_data["offset_diff"].apply(
    lambda x: "Falha de cache" if pd.isna(x) or abs(x) > 64 else "Acerto de cache"
)

fig = px.pie(
    bench_data,
    names="estado_do_cache",
    color="estado_do_cache",
    color_discrete_map={"Falha de cache": "red", "Acerto de cache": "blue"},
    title="Simulação do uso do cache",
)

fig.write_html(Path("../resources/dispersao_memoria_colorido.html"))
