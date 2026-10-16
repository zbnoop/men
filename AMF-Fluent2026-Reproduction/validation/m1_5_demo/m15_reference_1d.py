"""Offline 1-D enthalpy reference for the isolated M1.5 DEMO.

This is deliberately not a Fluent result.  It mirrors the constants and
piecewise equivalent-cp/enthalpy law in m15_demo_udf.c so that the latent heat
integral and the finite-volume energy ledger can be checked without inventing
a solver transcript when the local mesh-to-Fluent route is unavailable.
"""

from __future__ import annotations

import csv
import math
from pathlib import Path

TS = 748.0
TL = 911.0
CP_S = 900.0
CP_L = 1100.0
LATENT = 390_000.0
K_S = 235.0
K_L = 62.8
RHO = 2700.0
T_INITIAL = 300.0
T_HOT = 1000.0

LENGTH = 0.1
N_CELLS = 100
DX = LENGTH / N_CELLS
DT = 0.002
MAX_STEPS = 100_000

CM = 0.5 * (CP_S + CP_L) + LATENT / (TL - TS)
H_MUSHY = CM * (TL - TS)


def cp_value(temp: float) -> float:
    if temp <= TS:
        return CP_S
    if temp >= TL:
        return CP_L
    return CM


def h_value(temp: float) -> float:
    """The same arbitrary-reference enthalpy as DEFINE_SPECIFIC_HEAT."""
    if temp <= TS:
        return CP_S * (temp - TS)
    if temp < TL:
        return CM * (temp - TS)
    return H_MUSHY + CP_L * (temp - TL)


def temperature_from_h(enthalpy: float) -> float:
    if enthalpy <= 0.0:
        return TS + enthalpy / CP_S
    if enthalpy < H_MUSHY:
        return TS + enthalpy / CM
    return TL + (enthalpy - H_MUSHY) / CP_L


def conductivity(temp: float) -> float:
    if temp <= TS:
        return K_S
    if temp >= TL:
        return K_L
    return K_S + (K_L - K_S) * (temp - TS) / (TL - TS)


def liquid_fraction(temp: float) -> float:
    if temp <= TS:
        return 0.0
    if temp >= TL:
        return 1.0
    return (temp - TS) / (TL - TS)


def run(output_csv: Path) -> dict[str, float | int]:
    # Enthalpy-per-mass is the state variable; temperature is recovered with
    # the exact inverse of the UDF's piecewise integrated enthalpy.
    temperatures = [T_INITIAL for _ in range(N_CELLS)]
    enthalpies = [h_value(T_INITIAL) for _ in range(N_CELLS)]
    cumulative_input = 0.0
    max_abs_residual = 0.0
    rows: list[dict[str, float | int]] = []

    # The initial state is written explicitly for auditability.
    def record(step: int, q_hot: float, residual: float) -> None:
        rows.append(
            {
                "step": step,
                "time_s": step * DT,
                "hot_q_in_W": q_hot,
                "cumulative_hot_input_J": cumulative_input,
                "delta_enthalpy_J": RHO * DX * sum(
                    h_value(temp) - h_value(T_INITIAL) for temp in temperatures
                ),
                "energy_residual_J": residual,
                "liquid_volume_fraction": sum(liquid_fraction(temp) for temp in temperatures)
                / N_CELLS,
                "Tmin_K": min(temperatures),
                "Tmax_K": max(temperatures),
            }
        )

    record(0, 0.0, 0.0)
    for step in range(1, MAX_STEPS + 1):
        # Positive q is heat directed from the hot boundary toward +x.
        q_faces = [0.0 for _ in range(N_CELLS + 1)]
        q_faces[0] = conductivity(temperatures[0]) * (T_HOT - temperatures[0]) / (0.5 * DX)
        for face in range(1, N_CELLS):
            k_face = 2.0 * conductivity(temperatures[face - 1]) * conductivity(temperatures[face])
            k_face /= conductivity(temperatures[face - 1]) + conductivity(temperatures[face])
            q_faces[face] = k_face * (temperatures[face - 1] - temperatures[face]) / DX
        q_faces[N_CELLS] = 0.0  # insulated cold-side boundary

        for cell in range(N_CELLS):
            enthalpies[cell] += DT * (q_faces[cell] - q_faces[cell + 1]) / (RHO * DX)
        temperatures = [temperature_from_h(h) for h in enthalpies]

        q_hot = q_faces[0]
        cumulative_input += q_hot * DT
        delta_h = RHO * DX * sum(h_value(temp) - h_value(T_INITIAL) for temp in temperatures)
        residual = delta_h - cumulative_input
        max_abs_residual = max(max_abs_residual, abs(residual))

        if step % 5000 == 0 or step == 1 or step == MAX_STEPS:
            record(step, q_hot, residual)

    with output_csv.open("w", newline="", encoding="utf-8") as stream:
        writer = csv.DictWriter(stream, fieldnames=list(rows[0].keys()))
        writer.writeheader()
        writer.writerows(rows)

    return {
        "steps": MAX_STEPS,
        "final_time_s": MAX_STEPS * DT,
        "final_Tmin_K": min(temperatures),
        "final_Tmax_K": max(temperatures),
        "final_liquid_fraction": sum(liquid_fraction(temp) for temp in temperatures) / N_CELLS,
        "cumulative_input_J": cumulative_input,
        "final_delta_enthalpy_J": RHO * DX * sum(
            h_value(temp) - h_value(T_INITIAL) for temp in temperatures
        ),
        "max_abs_energy_residual_J": max_abs_residual,
    }


def main() -> None:
    output_csv = Path(__file__).with_name("m15_reference_1d.csv")
    latent_integral = (CM - 0.5 * (CP_S + CP_L)) * (TL - TS)
    assert math.isclose(latent_integral, LATENT, rel_tol=0.0, abs_tol=1e-9)
    assert math.isclose(h_value(TL) - h_value(TS), LATENT + 0.5 * (CP_S + CP_L) * (TL - TS), abs_tol=1e-9)
    result = run(output_csv)
    # The absolute ledger is ~2e8 J; a few microjoules are floating-point
    # summation noise, not a physical imbalance.
    assert result["max_abs_energy_residual_J"] < 1e-5
    print("M15 reference checks: PASS")
    print(f"latent_integral_J_per_kg={latent_integral:.12g}")
    for key, value in result.items():
        print(f"{key}={value}")
    print(f"csv={output_csv}")


if __name__ == "__main__":
    main()
