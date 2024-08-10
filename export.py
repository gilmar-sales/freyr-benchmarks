import os
import plotly.express as px
import pandas as pd

executables = []

def find_exe(folder):
    if "." not in folder: 
        for fname in os.listdir(folder):
            if "freyr-benchmarks.exe" in fname:
                executables.append(folder + '/'+ fname)
            else: 
                find_exe(folder+ '/'+ fname)
            

if not os.path.exists('bench.csv'):
    find_exe('build')

    if len(executables) == 0:
        ValueError("O arquivo executavel nao foi encontrado")

    executable = executables[0]

    if len(executables) > 1:
        for executableOption in executables:
            if "Release" in executableOption:
                executable = executableOption
                break
            
    os.system(os.getcwd()+"/" + executable + " --benchmark_out=bench.csv --benchmark_out_format=csv")

bench_data = pd.read_csv("bench.csv", header=8)

data = pd.DataFrame({
    "Cenário": bench_data["name"].map(lambda name: name.split('/')[0]),
    "Entidades": bench_data["name"].map(lambda name: int(name.split('/')[1])),
    "iterations": bench_data["iterations"],
    "real_time": bench_data["real_time"],
    "Tempo(ms)": bench_data["cpu_time"].map(lambda time: time / 1000000)
}).sort_values(by=["Cenário", "Entidades"])

fig = px.line(data, x="Entidades", y="Tempo(ms)", color="Cenário", title='Tempo de execução dos cenários')
fig.show()
