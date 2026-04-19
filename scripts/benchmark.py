import os
from sys import platform

dataDir = "../data"


def find_build_directory():
    buildDirectories = [
        "../cmake-build-release",
        "../build/GCC/Release",
        "../build/Clang/Release",
        "../out",
        "../build",
    ]

    for buildDir in buildDirectories:
        if os.path.exists(buildDir):
            return buildDir

    raise ValueError("O diretorio de build nao foi encontrado")


def find_program(folder, name):
    try:
        next_directories = []
        for file_name in os.listdir(folder):
            if file_name.endswith(name):
                return folder + "/" + file_name
            elif os.path.isdir(folder + "/" + file_name):
                next_directories.append(folder + "/" + file_name)

        for next_directory in next_directories:
            next_try = find_program(next_directory, name)
            if next_try.endswith(name):
                return next_try
    except Exception:
        return ""


class Benchmark:
    def __init__(self, name, suffix=""):
        self.build_directory = find_build_directory()

        if platform == "win32":
            self.executable = find_program(self.build_directory, f"{name}.exe")
        else:
            self.executable = find_program(self.build_directory, f"{name}")

        if not self.executable:
            print(self.executable)
            raise ValueError("O arquivo executavel nao foi encontrado")

        if len(suffix) > 0:
            suffix = f"_{suffix}"

        self.output = f"{dataDir}/{name}{suffix}_{platform}"

    def execute(self):
        if not os.path.exists(f"{self.output}.csv"):
            print("Iniciando benchmark...")
            os.system(
                f"{os.getcwd()}/{self.executable} --benchmark_format=csv --benchmark_out={self.output}.json > {self.output}.csv"
            )
        self.output = f"{self.output}.csv"


PERF_EVENTS = [
    "cycles",
    "instructions",
    "branches",
    "branch-misses",
    "cache-references",
    "cache-misses",
    "L1-dcache-loads",
    "L1-dcache-load-misses",
    "LLC-loads",
    "LLC-load-misses",
]


class PerfBenchmark(Benchmark):
    def __init__(self, name, benchmark_filter: str = ""):
        super().__init__(name, benchmark_filter)
        self.benchmark_filter = benchmark_filter
        self.perf_output = f"{self.output}_perf.txt"

    def execute(self):
        if platform == "win32":
            print("perf não é suportado no Windows. Executando benchmark padrão.")
            super().execute()
            return

        if not os.path.exists(f"{self.output}.csv"):
            print("Iniciando benchmark...")
            filter_arg = (
                f'--benchmark_filter="{self.benchmark_filter}"'
                if self.benchmark_filter
                else ""
            )

            perf_events = ",".join(PERF_EVENTS)

            cmd = (
                f"perf stat "
                f"-e {perf_events} "
                f"{os.getcwd()}/{self.executable} "
                f"--benchmark_report_aggregates_only=true "
                f"--benchmark_format=csv "
                f"--benchmark_out={self.output}.json "
                f"{filter_arg} "
                f"> {self.output}.csv "
                f"2> {self.perf_output}"
            )

            os.system(cmd)

        self.output = f"{self.output}.csv"

    def parse_perf_results(self) -> dict:
        """
        Lê o arquivo de saída do perf stat e retorna os contadores coletados.
        Deve ser chamado após execute().
        """
        import re

        if not os.path.exists(self.perf_output):
            raise FileNotFoundError(
                f"Arquivo de saída do perf não encontrado: {self.perf_output}\n"
                "Certifique-se de chamar execute() antes de parse_perf_results()."
            )

        counters = {}
        with open(self.perf_output, "r") as f:
            for line in f:
                line = line.strip()
                for event in PERF_EVENTS:
                    match = re.search(rf"([\d.,]+)\s+{re.escape(event)}", line)
                    if match:
                        raw = match.group(1).replace(".", "").replace(",", "")
                        try:
                            counters[event] = int(raw)
                        except ValueError:
                            pass

        return counters
