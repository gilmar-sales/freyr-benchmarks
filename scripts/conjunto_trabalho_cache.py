import plotly.graph_objects as go
from benchmark import PerfBenchmark
from plotly.subplots import make_subplots
from pathlib import Path

por_sistema = PerfBenchmark("conjunto_trabalho", "ConjuntoTrabalho_PorSistema")
por_objeto = PerfBenchmark("conjunto_trabalho", "ConjuntoTrabalho_PorObjeto")
por_bloco = PerfBenchmark("conjunto_trabalho", "ConjuntoTrabalho_PorBloco")

por_sistema.execute()
por_objeto.execute()
por_bloco.execute()

benchmarks = [por_sistema, por_objeto, por_bloco]
labels = ["PorSistema", "PorObjeto", "PorBloco"]
counters_list = [b.parse_perf_results() for b in benchmarks]


def get(counters, key):
    return counters.get(key, 0) or 0


def miss_rate(counters, misses_key, refs_key):
    m = counters.get(misses_key, 0) or 0
    r = counters.get(refs_key, 0) or 0
    return (m / r * 100) if r > 0 else 0


fig = make_subplots(
    rows=2,
    cols=2,
    subplot_titles=(
        "Falhas de Cache Absolutas (LLC)",
        "Taxa de Falha — LLC e L1 (%)",
        "Falhas de Carregamento L1 Absolutas",
        "Falhas de Carregamento LLC Absolutas",
    ),
    vertical_spacing=0.18,
    horizontal_spacing=0.12,
)

# ── 1. Falhas de cache absolutas (LLC) ───────────────────────────────────────
fig.add_trace(
    go.Bar(
        name="Falhas de Cache",
        x=labels,
        y=[get(c, "cache-misses") for c in counters_list],
        text=[f"{get(c, 'cache-misses'):,}" for c in counters_list],
        textposition="outside",
        showlegend=False,
    ),
    row=1,
    col=1,
)

# ── 2. Taxas de falha agrupadas ───────────────────────────────────────────────
llc_rates = [miss_rate(c, "cache-misses", "cache-references") for c in counters_list]
l1_rates = [
    miss_rate(c, "L1-dcache-load-misses", "L1-dcache-loads") for c in counters_list
]

fig.add_trace(
    go.Bar(
        name="Taxa de Falha LLC",
        x=labels,
        y=llc_rates,
        text=[f"{v:.2f}%" for v in llc_rates],
        textposition="outside",
    ),
    row=1,
    col=2,
)

fig.add_trace(
    go.Bar(
        name="Taxa de Falha L1",
        x=labels,
        y=l1_rates,
        text=[f"{v:.2f}%" for v in l1_rates],
        textposition="outside",
    ),
    row=1,
    col=2,
)

# ── 3. Falhas de carregamento L1 absolutas ────────────────────────────────────
fig.add_trace(
    go.Bar(
        name="Falhas de Carregamento L1",
        x=labels,
        y=[get(c, "L1-dcache-load-misses") for c in counters_list],
        text=[f"{get(c, 'L1-dcache-load-misses'):,}" for c in counters_list],
        textposition="outside",
        showlegend=False,
    ),
    row=2,
    col=1,
)

# ── 4. Falhas de carregamento LLC absolutas ───────────────────────────────────
fig.add_trace(
    go.Bar(
        name="Falhas de Carregamento LLC",
        x=labels,
        y=[get(c, "LLC-load-misses") for c in counters_list],
        text=[f"{get(c, 'LLC-load-misses'):,}" for c in counters_list],
        textposition="outside",
        showlegend=False,
    ),
    row=2,
    col=2,
)

fig.update_layout(
    title=dict(
        text="Comparação de Falhas de Cache — Conjunto de Trabalho",
        font=dict(size=20),
        x=0.5,
    ),
    barmode="group",
    font=dict(family="monospace", size=12),
    legend=dict(
        orientation="h",
        yanchor="bottom",
        y=1.02,
        xanchor="right",
        x=1,
    ),
    height=700,
    margin=dict(t=100, b=60),
)

fig.update_yaxes(tickformat=",", row=1, col=1)
fig.update_yaxes(tickformat=",", row=2, col=1)
fig.update_yaxes(tickformat=",", row=2, col=2)
fig.update_yaxes(ticksuffix="%", row=1, col=2)

fig.write_html(Path("../resources/conjunto_trabalho_cache.html"))
