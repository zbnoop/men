"""Compare user CSV vs paper Table 5. Columns: case,Y1_h,Y2_Nm3_per_t_al,Y3_percent.
Usage: python scripts/compare_benchmarks.py simulations.csv
"""
import csv,sys,pathlib
ROOT=pathlib.Path(__file__).resolve().parents[1]
if len(sys.argv)!=2:
    print(__doc__);sys.exit(2)
def read(path):
    with open(path,encoding="utf-8-sig",newline="") as f:return list(csv.DictReader(f))
ref={r["case"]:r for r in read(ROOT/"data/table5_baseline_optimized.csv")}
for r in read(sys.argv[1]):
    n=r["case"]
    if n not in ref:
        print("SKIP unknown case:",n);continue
    for k in ("Y1_h","Y2_Nm3_per_t_al","Y3_percent"):
        a=float(r[k]);b=float(ref[n][k])
        print(f"{n:15s} {k:20s} sim={a:.6f} paper={b:.6f} error={100*(a-b)/b:+.3f}%")
