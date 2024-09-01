import os

import pandas as pd
import plotly.express as px

executables = []


def find_exe(folder):
    if "." not in folder:
        for fname in os.listdir(folder):
            if "memory_allocation.exe" in fname:
                executables.append(folder + '/' + fname)
            else:
                find_exe(folder + '/' + fname)


if not os.path.exists('memory_fragmentation.csv'):
    find_exe('build')

    if len(executables) == 0:
        ValueError("O arquivo executavel nao foi encontrado")

    executable = executables[0]

    if len(executables) > 1:
        for executableOption in executables:
            if "Release" in executableOption:
                executable = executableOption
                break

    os.system(os.getcwd() + "/" + executable + " --benchmark_out=memory_fragmentation.csv --benchmark_out_format=csv")

bench_data = pd.read_csv("memory_fragmentation.csv")

print(bench_data)

fig = px.scatter(bench_data, x="n", y="offset", title='Dispersão alocações dinâmicas')
fig.show()
