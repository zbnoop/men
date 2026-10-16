/*
 * M1.5 isolated DEMO UDF for Fluent 2026 R1.
 *
 * This file intentionally uses the repository's demo-only material values:
 * rho=2700 kg/m3, cp_s=900, cp_l=1100 J/(kg K), L=390000 J/kg.
 * It is not a paper-reproduction material model and must not be promoted to
 * the production UDF without verified 7050 alloy data.
 */
#include "udf.h"
#include <math.h>
#include <stdio.h>
#include <string.h>

#define M15_T_SOLIDUS 748.0
#define M15_T_LIQUIDUS 911.0
#define M15_CP_SOLID 900.0
#define M15_CP_LIQUID 1100.0
#define M15_LATENT_HEAT 390000.0
#define M15_K_SOLID 235.0
#define M15_K_LIQUID 62.8
#define M15_RHO 2700.0
#define M15_INITIAL_T 300.0
#define M15_DEMO_HEAT_FLUX 5.0e6
#define M15_HOT_X_TOL 1.0e-8

static real m15_cp_value(real T)
{
    if (T <= M15_T_SOLIDUS) return M15_CP_SOLID;
    if (T >= M15_T_LIQUIDUS) return M15_CP_LIQUID;
    return 0.5 * (M15_CP_SOLID + M15_CP_LIQUID)
         + M15_LATENT_HEAT / (M15_T_LIQUIDUS - M15_T_SOLIDUS);
}
static real m15_integrated_h(real T)
{
    const real cm = 0.5 * (M15_CP_SOLID + M15_CP_LIQUID)
                  + M15_LATENT_HEAT / (M15_T_LIQUIDUS - M15_T_SOLIDUS);
    if (T <= M15_T_SOLIDUS) return M15_CP_SOLID * (T - M15_T_SOLIDUS);
    if (T < M15_T_LIQUIDUS) return cm * (T - M15_T_SOLIDUS);
    return cm * (M15_T_LIQUIDUS - M15_T_SOLIDUS)
         + M15_CP_LIQUID * (T - M15_T_LIQUIDUS);
}

static real m15_k_value(real T)
{
    if (T <= M15_T_SOLIDUS) return M15_K_SOLID;
    if (T >= M15_T_LIQUIDUS) return M15_K_LIQUID;
    return M15_K_SOLID
         + (M15_K_LIQUID - M15_K_SOLID)
         * (T - M15_T_SOLIDUS) / (M15_T_LIQUIDUS - M15_T_SOLIDUS);
}

static real m15_fraction(real T)
{
    if (T <= M15_T_SOLIDUS) return 0.0;
    if (T >= M15_T_LIQUIDUS) return 1.0;
    return (T - M15_T_SOLIDUS) / (M15_T_LIQUIDUS - M15_T_SOLIDUS);
}

DEFINE_SPECIFIC_HEAT(m15_equivalent_cp, T, Tref, h, yi)
{
    *h = m15_integrated_h(T) - m15_integrated_h(Tref);
    return m15_cp_value(T);
}

DEFINE_PROPERTY(m15_thermal_conductivity, c, t)
{
    return m15_k_value(C_T(c, t));
}

/*
 * The Workbench retry mesh exports the rectangle as one wall zone.  This
 * DEMO-only profile therefore supplies heat only on the x=0 face and zero
 * heat flux on the other three faces, preserving a one-dimensional slab
 * problem without changing the production geometry or UDF.
 */
DEFINE_PROFILE(m15_demo_wall_heat_flux, t, i)
{
    face_t f;
    begin_f_loop(f, t)
    {
        real xyz[ND_ND];
        F_CENTROID(xyz, f, t);
        F_PROFILE(f, t, i) = (xyz[0] <= M15_HOT_X_TOL)
                           ? M15_DEMO_HEAT_FLUX : 0.0;
    }
    end_f_loop(f, t)
}

/*
 * Host-side CSV audit. The cell and face sums are formed on compute nodes and
 * reduced with Fluent's parallel-safe reductions. The hot-wall sign is
 * defined here as positive heat entering the solid domain.
 */
DEFINE_EXECUTE_AT_END(m15_energy_audit)
{
    /* Fluent 2026 can call execute-at-end once at t=0 before cell data are
     * initialized.  Do not touch cell/face fields during that callback. */
    if (CURRENT_TIME <= 0.0) return;

    Domain *domain = NULL;
    Thread *t;
    face_t f;
    real volume = 0.0;
    real delta_h = 0.0;
    real latent_energy = 0.0;
    real liquid_volume = 0.0;
    real t_min = 1.0e30;
    real t_max = -1.0e30;
    real hot_q_in = 0.0;
    Thread *probe_t;
    cell_t probe_c;
    static real cumulative_q_in = 0.0;
    static real previous_time = 0.0;

#if !RP_HOST
    domain = Get_Domain(1);
    thread_loop_c(probe_t, domain)
    {
        begin_c_loop(probe_c, probe_t)
        {
            const real temp = C_T(probe_c, probe_t);
            const real cell_volume = C_VOLUME(probe_c, probe_t);
            real rho = C_R(probe_c, probe_t);
            if (rho <= 0.0) rho = M15_RHO;
            volume += cell_volume;
            delta_h += rho * cell_volume
                     * (m15_integrated_h(temp) - m15_integrated_h(M15_INITIAL_T));
            latent_energy += rho * cell_volume * M15_LATENT_HEAT
                           * m15_fraction(temp);
            liquid_volume += cell_volume * m15_fraction(temp);
            if (temp < t_min) t_min = temp;
            if (temp > t_max) t_max = temp;
        }
        end_c_loop(probe_c, probe_t)
    }

    thread_loop_f(t, domain)
    {
        if (BOUNDARY_FACE_THREAD_P(t) && THREAD_TYPE(t) == THREAD_F_WALL)
        {
            begin_f_loop(f, t)
            {
                real area[ND_ND];
                F_AREA(area, f, t);
                real xyz[ND_ND];
                F_CENTROID(xyz, f, t);
                if (xyz[0] <= M15_HOT_X_TOL)
                    hot_q_in += M15_DEMO_HEAT_FLUX * NV_MAG(area);
            }
            end_f_loop(f, t)
        }
    }

#endif

#if !RP_HOST
    volume = PRF_GRSUM1(volume);
    delta_h = PRF_GRSUM1(delta_h);
    latent_energy = PRF_GRSUM1(latent_energy);
    liquid_volume = PRF_GRSUM1(liquid_volume);
    hot_q_in = PRF_GRSUM1(hot_q_in);
    t_min = PRF_GRLOW1(t_min);
    t_max = PRF_GRHIGH1(t_max);
#endif

    /* Global reductions complete on compute nodes; explicitly send the
     * audit ledger to the host before writing the CSV and Message output. */
    node_to_host_real_4(volume, delta_h, liquid_volume, hot_q_in);
    node_to_host_real_1(latent_energy);
    node_to_host_real_2(t_min, t_max);

#if RP_HOST
    if (CURRENT_TIME > previous_time)
        cumulative_q_in += hot_q_in * (CURRENT_TIME - previous_time);
    previous_time = CURRENT_TIME;

    {
        FILE *fp = fopen("m15_energy.csv", (CURRENT_TIME <= CURRENT_TIMESTEP) ? "w" : "a");
        if (fp != NULL)
        {
            if (CURRENT_TIME <= CURRENT_TIMESTEP)
                fprintf(fp, "time_s,volume_m3,delta_enthalpy_J,latent_energy_J,hot_q_in_W,cumulative_hot_input_J,energy_residual_J,liquid_volume_fraction,Tmin_K,Tmax_K\n");
            fprintf(fp, "%.12g,%.12g,%.12g,%.12g,%.12g,%.12g,%.12g,%.12g,%.12g,%.12g\n",
                    CURRENT_TIME, volume, delta_h, latent_energy, hot_q_in,
                    cumulative_q_in, delta_h - cumulative_q_in,
                    (volume > 0.0) ? liquid_volume / volume : 0.0,
                    t_min, t_max);
            fclose(fp);
        }
    }
    Message("M15 audit: t=%g s, dH=%g J, Qin=%g J, residual=%g J, liquid_Vf=%g, Tmax=%g K\n",
            CURRENT_TIME, delta_h, cumulative_q_in, delta_h - cumulative_q_in,
            (volume > 0.0) ? liquid_volume / volume : 0.0, t_max);
#endif
}
