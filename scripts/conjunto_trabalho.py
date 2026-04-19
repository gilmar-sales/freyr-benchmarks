import pandas as pd
import plotly.graph_objects as go
from benchmark import Benchmark
from pathlib import Path

AGGREGATE_SUFFIXES = ["_mean", "_median", "_stddev", "_cv"]

LAYOUT_BASE = dict(
    font=dict(family="'Courier New', monospace", size=12),
    margin=dict(l=70, r=40, t=70, b=60),
    legend=dict(borderwidth=1),
    xaxis=dict(),
    yaxis=dict(),
)

benchmark = Benchmark("conjunto_trabalho")

benchmark.execute()


def load_data() -> pd.DataFrame:
    df = pd.read_csv(benchmark.output)
    df.columns = df.columns.str.strip()
    df["name"] = df["name"].str.strip()

    def classify(name: str):
        for suf in AGGREGATE_SUFFIXES:
            if name.endswith(suf):
                return suf.lstrip("_"), name[: -len(suf)]
        return "run", name

    df[["row_type", "base_name"]] = pd.DataFrame(
        df["name"].apply(classify).tolist(), index=df.index
    )
    df["short_name"] = df["base_name"].str.split("/").str[0]

    for col in ["real_time", "cpu_time", "iterations"]:
        df[col] = pd.to_numeric(df[col], errors="coerce")

    return df


def get_runs(df: pd.DataFrame) -> pd.DataFrame:
    return df[df["row_type"] == "run"].copy()


def get_aggregates(df: pd.DataFrame) -> pd.DataFrame:
    return df[df["row_type"] != "run"].copy()


def print_summary(runs: pd.DataFrame) -> None:
    print("\n" + "=" * 65)
    print("  RESUMO — ESTATÍSTICAS DOS RUNS INDIVIDUAIS (pandas)")
    print("=" * 65)
    summary = runs.groupby("short_name")["real_time"].agg(
        repetições="count",
        média="mean",
        mediana="median",
        desvio_pad="std",
    )
    summary["cv_%"] = (summary["desvio_pad"] / summary["média"] * 100).round(2).astype(
        str
    ) + "%"
    print(summary.to_string())
    print()


def fig_bar_aggregates(df: pd.DataFrame) -> go.Figure:
    agg = get_aggregates(df)
    benchmarks = agg["short_name"].unique()
    metrics = [("mean", "Média"), ("median", "Mediana"), ("stddev", "Desvio Padrão")]

    fig = go.Figure()
    for metric, label in metrics:
        subset = agg[agg["row_type"] == metric]
        vals = [
            float(subset.loc[subset["short_name"] == b, "real_time"].iloc[0])
            if b in subset["short_name"].values
            else 0.0
            for b in benchmarks
        ]
        fig.add_trace(
            go.Bar(
                name=label.capitalize(),
                x=list(benchmarks),
                y=vals,
                text=[f"{v:,.1f}" for v in vals],
                textposition="outside",
                hovertemplate="<b>%{x}</b><br>"
                + label
                + ": %{y:,.2f} ns<extra></extra>",
            )
        )
    fig.update_layout(
        **LAYOUT_BASE,
        title="Agregados por Benchmark — Tempo real (ns)",
        barmode="group",
        yaxis_title="Tempo real (ns)",
        xaxis_title="Benchmark",
    )
    return fig


def main() -> None:
    df = load_data()
    runs = get_runs(df)

    print_summary(runs)

    figures = [
        fig_bar_aggregates(df),
    ]

    for i, fig in enumerate(figures):
        fig.write_html(Path(f"../resources/conjunto_trabalho_{i}.html"))


if __name__ == "__main__":
    main()
