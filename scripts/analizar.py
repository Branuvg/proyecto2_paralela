"""Analisis del punto 6: promedios, speedup, eficiencia y graficas a partir de results/*.csv"""
import pandas as pd
import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt

SPACE = 2 ** 23
d = pd.read_csv("results/mediciones.csv")
p = pd.read_csv("results/procesos.csv")
d["pct"] = (d["llave"] / SPACE * 100).round(1)

g = d.groupby(["llave", "pct", "modo", "N"])["tiempo"].agg(["mean", "std", "min", "max"]).reset_index()
base = g[g.N == 1].set_index("llave")["mean"]          # secuencial (N=1)
g["speedup"] = g.apply(lambda r: base[r.llave] / r["mean"], axis=1)
g["eficiencia"] = g["speedup"] / g["N"]
g["cv_%"] = (g["std"] / g["mean"] * 100).round(1)
# N=1 sirve de baseline para ambos modos
b1 = g[g.N == 1].copy(); b1["modo"] = "cyclic"
g = pd.concat([g, b1]).sort_values(["llave", "modo", "N"]).reset_index(drop=True)
g.round(4).to_csv("results/resumen.csv", index=False)

# llaves probadas por proceso (rep 1) para ver el balance
pp = p[p.rep == 1].groupby(["llave", "modo", "N"])["llaves_probadas"].agg(["min", "max", "sum"]).reset_index()
pp.to_csv("results/balance.csv", index=False)

colors = {"block": "#404040", "cyclic": "#909090"}
styles = {"block": "-o", "cyclic": "--s"}
keys = sorted(d.llave.unique())

# Grafica 1: speedup vs N por llave
fig, axs = plt.subplots(2, 3, figsize=(11, 6.5))
axs = axs.flatten(); axs[-1].axis("off")
for ax, k in zip(axs, keys):
    for m in ("block", "cyclic"):
        s = g[(g.llave == k) & (g.modo == m)]
        ax.plot(s.N, s.speedup, styles[m], color=colors[m], label=m)
    ax.plot([1, 16], [1, 16], ":", color="#bbbbbb", label="ideal")
    ax.set_xscale("log", base=2); ax.set_yscale("log", base=2)
    ax.set_title(f"llave {k}\n({d[d.llave == k].pct.iloc[0]}% del espacio)", fontsize=10)
    ax.set_xlabel("N procesos")
axs[0].set_ylabel("speedup"); axs[3].set_ylabel("speedup"); axs[0].legend(fontsize=8)
plt.tight_layout(); plt.savefig("results/speedup.png", dpi=150); plt.close()

# Grafica 2: tiempo medio vs N por llave
fig, axs = plt.subplots(2, 3, figsize=(11, 6.5))
axs = axs.flatten(); axs[-1].axis("off")
for ax, k in zip(axs, keys):
    for m in ("block", "cyclic"):
        s = g[(g.llave == k) & (g.modo == m)]
        ax.errorbar(s.N, s["mean"], yerr=s["std"].fillna(0), fmt=styles[m], color=colors[m], label=m, capsize=2)
    ax.set_xscale("log", base=2); ax.set_yscale("log")
    ax.set_title(f"llave {k} ({d[d.llave == k].pct.iloc[0]}%)", fontsize=10); ax.set_xlabel("N procesos")
axs[0].set_ylabel("tiempo (s)"); axs[3].set_ylabel("tiempo (s)"); axs[0].legend(fontsize=8)
plt.tight_layout(); plt.savefig("results/tiempos.png", dpi=150); plt.close()

pd.set_option("display.width", 200)
print(g[["llave", "pct", "modo", "N", "mean", "std", "cv_%", "speedup", "eficiencia"]].round(3).to_string(index=False))
