/* Equivalent Cp demo for fixed-solid-zone aluminum melting (no liquid motion).
 * NOT verified in Fluent 2026. Requires validated material properties. */
#include "udf.h"
#include "aluminum_params.h"

static real aluminum_cp_value(real T)
{
    if (T <= AL_T_SOLIDUS) return AL_CP_SOLID;
    if (T >= AL_T_LIQUIDUS) return AL_CP_LIQUID;
    return 0.5*(AL_CP_SOLID+AL_CP_LIQUID)
          +AL_LATENT_HEAT/(AL_T_LIQUIDUS-AL_T_SOLIDUS);
}

static real aluminum_integrated_h(real T)
{
    const real cm=0.5*(AL_CP_SOLID+AL_CP_LIQUID)+AL_LATENT_HEAT/(AL_T_LIQUIDUS-AL_T_SOLIDUS);
    if (T<=AL_T_SOLIDUS) return AL_CP_SOLID*(T-AL_T_SOLIDUS);
    if (T<AL_T_LIQUIDUS) return cm*(T-AL_T_SOLIDUS);
    return cm*(AL_T_LIQUIDUS-AL_T_SOLIDUS)+AL_CP_LIQUID*(T-AL_T_LIQUIDUS);
}

static real aluminum_k_value(real T)
{
    if (T<=AL_T_SOLIDUS) return AL_K_SOLID;
    if (T>=AL_T_LIQUIDUS) return AL_K_LIQUID;
    return AL_K_SOLID+(AL_K_LIQUID-AL_K_SOLID)*(T-AL_T_SOLIDUS)/(AL_T_LIQUIDUS-AL_T_SOLIDUS);
}

static real aluminum_fraction_value(real T)
{
    if (T<=AL_T_SOLIDUS) return 0.0;
    if (T>=AL_T_LIQUIDUS) return 1.0;
    return (T-AL_T_SOLIDUS)/(AL_T_LIQUIDUS-AL_T_SOLIDUS);
}

DEFINE_SPECIFIC_HEAT(aluminum_equivalent_cp,T,Tref,h,yi)
{
    *h=aluminum_integrated_h(T)-aluminum_integrated_h(Tref);
    return aluminum_cp_value(T);
}

DEFINE_PROPERTY(aluminum_thermal_conductivity,c,t)
{
    return aluminum_k_value(C_T(c,t));
}
