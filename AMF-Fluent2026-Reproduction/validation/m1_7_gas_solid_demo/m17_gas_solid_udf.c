/*
 * M1.7 isolated DEMO UDF for Fluent 2026 R1.
 *
 * This library intentionally uses the M1.5/M1.6 demonstration values only:
 * rho=2700 kg/m3, cp_s=900, cp_l=1100 J/(kg K), L=390000 J/kg,
 * Tsolidus=748 K and Tliquidus=911 K.  It is not a 7050 alloy model and
 * must not be used by the production UDF or by the formal paper case.
 *
 * The M1.7 boundary condition is gas -> solid CHT.  There is deliberately
 * no bottom heat-flux profile in this library.  Interface heat rates are
 * obtained independently from Fluent's native heat-transfer report; the
 * execute-at-end callback below audits volume energies and inlet/outlet
 * enthalpy only.
 */
#include "udf.h"
#include <math.h>
#include <stdio.h>
#include <string.h>

#define M17_T_SOLIDUS 748.0
#define M17_T_LIQUIDUS 911.0
#define M17_CP_SOLID 900.0
#define M17_CP_LIQUID 1100.0
#define M17_LATENT_HEAT 390000.0
#define M17_K_SOLID 235.0
#define M17_K_LIQUID 62.8
#define M17_RHO 2700.0
#define M17_T_REF 300.0
#define M17_T_INLET 1200.0
#define M17_GAS_CP 1006.43

static real m17_mushy_cp(void)
{
    return 0.5 * (M17_CP_SOLID + M17_CP_LIQUID)
         + M17_LATENT_HEAT / (M17_T_LIQUIDUS - M17_T_SOLIDUS);
}

static real m17_fraction(real T)
{
    if (T <= M17_T_SOLIDUS) return 0.0;
    if (T >= M17_T_LIQUIDUS) return 1.0;
    return (T - M17_T_SOLIDUS) / (M17_T_LIQUIDUS - M17_T_SOLIDUS);
}

static real m17_sensible_h(real T)
{
    const real cp_m = 0.5 * (M17_CP_SOLID + M17_CP_LIQUID);
    if (T <= M17_T_SOLIDUS)
        return M17_CP_SOLID * (T - M17_T_REF);
    if (T < M17_T_LIQUIDUS)
        return M17_CP_SOLID * (M17_T_SOLIDUS - M17_T_REF)
             + cp_m * (T - M17_T_SOLIDUS);
    return M17_CP_SOLID * (M17_T_SOLIDUS - M17_T_REF)
         + cp_m * (M17_T_LIQUIDUS - M17_T_SOLIDUS)
         + M17_CP_LIQUID * (T - M17_T_LIQUIDUS);
}

static real m17_integrated_h(real T)
{
    return m17_sensible_h(T) + M17_LATENT_HEAT * m17_fraction(T);
}

static real m17_cp_value(real T)
{
    if (T <= M17_T_SOLIDUS) return M17_CP_SOLID;
    if (T >= M17_T_LIQUIDUS) return M17_CP_LIQUID;
    return m17_mushy_cp();
}

static real m17_k_value(real T)
{
    if (T <= M17_T_SOLIDUS) return M17_K_SOLID;
    if (T >= M17_T_LIQUIDUS) return M17_K_LIQUID;
    return M17_K_SOLID
         + (M17_K_LIQUID - M17_K_SOLID)
         * (T - M17_T_SOLIDUS) / (M17_T_LIQUIDUS - M17_T_SOLIDUS);
}

DEFINE_SPECIFIC_HEAT(m17_equivalent_cp, T, Tref, h, yi)
{
    *h = m17_integrated_h(T) - m17_integrated_h(Tref);
    return m17_cp_value(T);
}

DEFINE_PROPERTY(m17_thermal_conductivity, c, t)
{
    return m17_k_value(C_T(c, t));
}

/*
 * Audit convention:
 *   inlet_enthalpy_W  = positive enthalpy entering through fluid_inlet;
 *   outlet_enthalpy_W = positive enthalpy leaving through fluid_outlet;
 *   net_boundary_W    = inlet_enthalpy_W - outlet_enthalpy_W.
 *
 * F_FLUX is the signed outward mass flux used by Fluent.  The interface
 * fluxes are intentionally not inferred from BOUNDARY_HEAT_FLUX here: the
 * independent native /report/fluxes/heat-transfer output is authoritative
 * for the M1.7 interface-continuity check.
 */
DEFINE_EXECUTE_AT_END(m17_gas_solid_energy_audit)
{
    Domain *domain = NULL;
    Thread *t;
    face_t f;
    real solid_volume = 0.0;
    real fluid_volume = 0.0;
    real solid_sensible = 0.0;
    real solid_latent = 0.0;
    real solid_total = 0.0;
    real fluid_sensible = 0.0;
    real liquid_volume = 0.0;
    real solid_tmin = 1.0e30;
    real solid_tmax = -1.0e30;
    real fluid_tmin = 1.0e30;
    real fluid_tmax = -1.0e30;
    real inlet_enthalpy = 0.0;
    real outlet_enthalpy = 0.0;
    real inlet_boundary_heat = 0.0;
    real outlet_boundary_heat = 0.0;
    static real cumulative_boundary_energy = 0.0;
    static real cumulative_native_boundary_energy = 0.0;
    static real previous_time = 0.0;

    if (CURRENT_TIME <= 0.0) return;

#if !RP_HOST
    domain = Get_Domain(1);
    thread_loop_c(t, domain)
    {
        const char *name = THREAD_NAME(t);
        const int is_solid = (name != NULL && strstr(name, "solid_zone") != NULL);
        const int is_fluid = (name != NULL && strstr(name, "fluid_zone") != NULL);
        cell_t c;

        if (!is_solid && !is_fluid) continue;
        begin_c_loop(c, t)
        {
            const real temp = C_T(c, t);
            real rho = C_R(c, t);
            const real cell_volume = C_VOLUME(c, t);
            if (rho <= 0.0) rho = is_solid ? M17_RHO : 1.225;

            if (is_solid)
            {
                const real fraction = m17_fraction(temp);
                solid_volume += cell_volume;
                solid_sensible += rho * cell_volume
                                * (m17_sensible_h(temp) - m17_sensible_h(M17_T_REF));
                solid_latent += rho * cell_volume * M17_LATENT_HEAT * fraction;
                solid_total += rho * cell_volume
                             * (m17_integrated_h(temp) - m17_integrated_h(M17_T_REF));
                liquid_volume += cell_volume * fraction;
                if (temp < solid_tmin) solid_tmin = temp;
                if (temp > solid_tmax) solid_tmax = temp;
            }
            else
            {
                fluid_volume += cell_volume;
                fluid_sensible += rho * cell_volume * M17_GAS_CP
                                * (temp - M17_T_REF);
                if (temp < fluid_tmin) fluid_tmin = temp;
                if (temp > fluid_tmax) fluid_tmax = temp;
            }
        }
        end_c_loop(c, t)
    }

    thread_loop_f(t, domain)
    {
        const char *name = THREAD_NAME(t);
        const int is_inlet = (name != NULL && strstr(name, "fluid_inlet") != NULL);
        const int is_outlet = (name != NULL && strstr(name, "fluid_outlet") != NULL);
        if (!BOUNDARY_FACE_THREAD_P(t) || (!is_inlet && !is_outlet)) continue;

        begin_f_loop(f, t)
        {
            const cell_t c0 = F_C0(f, t);
            Thread *t0 = THREAD_T0(t);
            real mass_flux;
            real temperature;
            real enthalpy_flux;
            real area[ND_ND];
            if (NULL_CELL_P(c0) || t0 == NULL) continue;
            mass_flux = F_FLUX(f, t);
            temperature = C_T(c0, t0);
            enthalpy_flux = mass_flux * M17_GAS_CP * (temperature - M17_T_REF);
            F_AREA(area, f, t);

            if (is_inlet)
            {
                if (enthalpy_flux < 0.0) inlet_enthalpy -= enthalpy_flux;
                else outlet_enthalpy += enthalpy_flux;
                inlet_boundary_heat += BOUNDARY_HEAT_FLUX(f, t) * NV_MAG(area);
            }
            else
            {
                if (enthalpy_flux > 0.0) outlet_enthalpy += enthalpy_flux;
                else inlet_enthalpy -= enthalpy_flux;
                outlet_boundary_heat += BOUNDARY_HEAT_FLUX(f, t) * NV_MAG(area);
            }
        }
        end_f_loop(f, t)
    }
#endif

#if !RP_HOST
    solid_volume = PRF_GRSUM1(solid_volume);
    fluid_volume = PRF_GRSUM1(fluid_volume);
    solid_sensible = PRF_GRSUM1(solid_sensible);
    solid_latent = PRF_GRSUM1(solid_latent);
    solid_total = PRF_GRSUM1(solid_total);
    fluid_sensible = PRF_GRSUM1(fluid_sensible);
    liquid_volume = PRF_GRSUM1(liquid_volume);
    inlet_enthalpy = PRF_GRSUM1(inlet_enthalpy);
    outlet_enthalpy = PRF_GRSUM1(outlet_enthalpy);
    inlet_boundary_heat = PRF_GRSUM1(inlet_boundary_heat);
    outlet_boundary_heat = PRF_GRSUM1(outlet_boundary_heat);
    solid_tmin = PRF_GRLOW1(solid_tmin);
    solid_tmax = PRF_GRHIGH1(solid_tmax);
    fluid_tmin = PRF_GRLOW1(fluid_tmin);
    fluid_tmax = PRF_GRHIGH1(fluid_tmax);
#endif

    node_to_host_real_7(solid_volume, fluid_volume, solid_sensible, solid_latent,
                        solid_total, fluid_sensible, liquid_volume);
    node_to_host_real_7(inlet_enthalpy, outlet_enthalpy, inlet_boundary_heat,
                        outlet_boundary_heat, solid_tmin, solid_tmax, fluid_tmin);
    node_to_host_real_1(fluid_tmax);

#if RP_HOST
    {
        const real net_boundary_power = inlet_enthalpy - outlet_enthalpy;
        const real native_net_boundary_power = inlet_boundary_heat + outlet_boundary_heat;
        const real dt = (CURRENT_TIME > previous_time) ? CURRENT_TIME - previous_time : 0.0;
        const real total_delta_energy = solid_total + fluid_sensible;
        if (dt > 0.0) cumulative_boundary_energy += net_boundary_power * dt;
        if (dt > 0.0) cumulative_native_boundary_energy += native_net_boundary_power * dt;
        previous_time = CURRENT_TIME;
        {
            const real residual = total_delta_energy - cumulative_boundary_energy;
            const real native_residual = total_delta_energy - cumulative_native_boundary_energy;

            FILE *fp = fopen("m17_energy.csv", (CURRENT_TIME <= CURRENT_TIMESTEP) ? "w" : "a");
            if (fp != NULL)
            {
                if (CURRENT_TIME <= CURRENT_TIMESTEP)
                    fprintf(fp, "time_s,solid_volume_m3,fluid_volume_m3,solid_sensible_energy_J,solid_latent_energy_J,solid_total_enthalpy_J,fluid_sensible_energy_J,total_delta_energy_J,inlet_enthalpy_W,outlet_enthalpy_W,net_boundary_energy_W,cumulative_boundary_energy_J,convective_energy_residual_J,inlet_boundary_heat_transfer_W,outlet_boundary_heat_transfer_W,native_net_boundary_power_W,cumulative_native_boundary_energy_J,native_energy_residual_J,liquid_volume_fraction,solid_Tmin_K,solid_Tmax_K,fluid_Tmin_K,fluid_Tmax_K\n");
                fprintf(fp, "%.12g,%.12g,%.12g,%.12g,%.12g,%.12g,%.12g,%.12g,%.12g,%.12g,%.12g,%.12g,%.12g,%.12g,%.12g,%.12g,%.12g,%.12g,%.12g,%.12g,%.12g,%.12g,%.12g\n",
                        CURRENT_TIME, solid_volume, fluid_volume, solid_sensible,
                        solid_latent, solid_total, fluid_sensible, total_delta_energy,
                        inlet_enthalpy, outlet_enthalpy, net_boundary_power,
                        cumulative_boundary_energy, residual, inlet_boundary_heat,
                        outlet_boundary_heat, native_net_boundary_power,
                        cumulative_native_boundary_energy, native_residual,
                        (solid_volume > 0.0) ? liquid_volume / solid_volume : 0.0,
                        solid_tmin, solid_tmax, fluid_tmin, fluid_tmax);
                fclose(fp);
            }
            Message("M17 audit: t=%g s, solid sensible=%g J, latent=%g J, fluid=%g J, inletH=%g W, outletH=%g W, nativeIn=%g W, nativeOut=%g W, nativeResidual=%g J, liquid_Vf=%g, solid Tmax=%g K\n",
                    CURRENT_TIME, solid_sensible, solid_latent, fluid_sensible,
                    inlet_enthalpy, outlet_enthalpy, inlet_boundary_heat,
                    outlet_boundary_heat, native_residual,
                    (solid_volume > 0.0) ? liquid_volume / solid_volume : 0.0,
                    solid_tmax);
        }
    }
#endif
}
