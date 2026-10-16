#!/usr/bin/env python3
"""Public repository preflight: original-paper PDF is deliberately not tracked."""
import csv, json, pathlib, math
ROOT=pathlib.Path(__file__).resolve().parents[1]
def rows(path):
    with (ROOT/path).open(encoding="utf-8-sig",newline="") as f:
        return list(csv.DictReader(f))
p=json.loads((ROOT/"config/paper_parameters.json").read_text(encoding="utf-8"))
assert p["thermal"]["solidus"]["value"]==748
assert p["thermal"]["liquidus"]["value"]==911
assert p["thermal"]["cp_solid"]["value"] is None
assert p["thermal"]["latent_heat"]["value"] is None
assert abs(sum(float(x["volume_percent_as_paper"]) for x in rows("data/table1_natural_gas_composition.csv"))-100)<1e-8
t3=rows("data/table3_20_cases.csv")
assert len(t3)==21 and t3[0]["case"]=="N1" and t3[19]["case"]=="N20"
assert float(t3[20]["U_Nm3_per_h_WORKING_UNIT_VERIFY_PAPER_HEADER"])==400
t5=rows("data/table5_baseline_optimized.csv")
assert float(t5[0]["U_Nm3_per_h"])==800
assert math.isclose(float(t5[0]["Y1_h"])*3600,22640,abs_tol=10)
print("PASS: internal transcription checks. Conflict 400 vs 800 remains OPEN.")
print("NOTE: Original PDF not tracked due to public repository; verify against user-owned local copy.")
print("BLOCKED for exact reproduction: 7050 cp, rho, latent heat, detailed CAD, burner-switch schedule.")
