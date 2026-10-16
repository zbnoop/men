#ifndef ALUMINUM_PARAMS_H
#define ALUMINUM_PARAMS_H

/* Qiu et al. 2018, paper-confirmed values */
#define AL_T_SOLIDUS 748.0
#define AL_T_LIQUIDUS 911.0
#define AL_K_SOLID 235.0
#define AL_K_LIQUID 62.8

/* DEMO only; these cp and latent heat values are NOT given by the paper.
 * Fail production compilation until verified 7050 alloy data are obtained.
 * Tests may use -DALLOW_DEMO_MATERIAL. */
#ifndef ALLOW_DEMO_MATERIAL
#error "STOP: Replace DEMO 7050 cp and latent heat with verified values before Fluent build."
#endif
#define AL_CP_SOLID 900.0
#define AL_CP_LIQUID 1100.0
#define AL_LATENT_HEAT 390000.0
#endif
