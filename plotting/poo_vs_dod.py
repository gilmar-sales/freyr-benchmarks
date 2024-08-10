import plotly.express as px
import pandas as pd

bench_data = pd.read_csv("C:/Dev/freyr-benchmarks/build/MSVC/Release/Release/bench.csv")

data = pd.DataFrame({
    "name": bench_data["name"].map(lambda name: name.split('/')[0]),
    "entities": bench_data["name"].map(lambda name: int(name.split('/')[1])),
    "iterations": bench_data["iterations"],
    "real_time": bench_data["real_time"],
    "cpu_time": bench_data["cpu_time"]
}).sort_values(by=["name", "entities"])

print(data)

fig = px.line(data, x="entities", y="cpu_time", color="name", title='Tempo de execução POO vs DOD')
fig.show()
