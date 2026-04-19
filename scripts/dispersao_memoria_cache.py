import plotly.graph_objects as go
from benchmark import PerfBenchmark
from plotly.subplots import make_subplots
from pathlib import Path

alocacao_dinamica = PerfBenchmark(
    "dispersao_memoria_execucao", "Alocacao_Dinamica_Execucao"
)
alocacao_contigua = PerfBenchmark(
    "dispersao_memoria_execucao", "Alocacao_Contigua_Execucao"
)

alocacao_dinamica.execute()
alocacao_contigua.execute()

benchmarks = [alocacao_dinamica, alocacao_contigua]
labels = ["Alocação Dinâmica", "Alocação Contígua"]
counters_list = [b.parse_perf_results() for b in benchmarks]


def get(counters, key):
    return counters.get(key, 0) or 0


fig = make_subplots(
    rows=2,
    cols=2,
    subplot_titles=(
        "Falhas de ramo",
        "Taxa de Falha L1 (%)",
        "Falhas de cache",
        "Instruções por Ciclo (IPC)",
    ),
    vertical_spacing=0.18,
    horizontal_spacing=0.15,
)

cache_refs = [get(c, "cache-references") for c in counters_list]
cache_misses = [get(c, "cache-misses") for c in counters_list]

branches = [get(c, "branches") for c in counters_list]
branch_misses = [get(c, "branch-misses") for c in counters_list]
branch_miss_ratio = [
    misses / br * 100 if br > 0 else 0 for misses, br in zip(branch_misses, branches)
]

l1_dc_loads = [get(c, "L1-dcache-loads") for c in counters_list]
l1_dc_load_misses = [get(c, "L1-dcache-load-misses") for c in counters_list]
l1_data_miss_ratio = [
    misses / loads * 100 if loads > 0 else 0
    for misses, loads in zip(l1_dc_load_misses, l1_dc_loads)
]

instructions = [get(c, "instructions") for c in counters_list]
cycles = [get(c, "cycles") for c in counters_list]
ipc = [ins / cyc if cyc > 0 else 0 for ins, cyc in zip(instructions, cycles)]

colors = ["#27ae60", "#e74c3c"]

fig.add_trace(
    go.Bar(
        x=labels,
        y=branch_misses,
        text=[f"{v:,}" for v in branch_misses],
        textposition="outside",
        marker_color=colors,
        showlegend=False,
    ),
    row=1,
    col=1,
)

fig.add_trace(
    go.Bar(
        x=labels,
        y=l1_data_miss_ratio,
        text=[f"{v:.2f}%" for v in l1_data_miss_ratio],
        textposition="outside",
        marker_color=colors,
        showlegend=False,
    ),
    row=1,
    col=2,
)

fig.add_trace(
    go.Bar(
        x=labels,
        y=cache_misses,
        text=[f"{v:,}" for v in cache_misses],
        textposition="outside",
        marker_color=colors,
        showlegend=False,
    ),
    row=2,
    col=1,
)

fig.add_trace(
    go.Bar(
        x=labels,
        y=ipc,
        text=[f"{v:.2f}" for v in ipc],
        textposition="outside",
        marker_color=colors,
        showlegend=False,
    ),
    row=2,
    col=2,
)

fig.update_layout(
    title=dict(
        text="Eficiência — Cache e Ramos vs IPC",
        font=dict(size=20),
        x=0.5,
    ),
    barmode="group",
    font=dict(family="Arial", size=12),
    height=550,
    margin=dict(t=80, b=60),
)

fig.update_yaxes(tickformat=",", row=1, col=1)
fig.update_yaxes(ticksuffix="%", row=1, col=2)
fig.update_yaxes(tickformat=",", row=2, col=1)

fig.write_html(Path("../resources/dispersao_memoria_cache.html"))
