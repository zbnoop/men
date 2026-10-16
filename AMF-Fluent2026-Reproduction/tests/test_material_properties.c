#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "../udf/aluminum_equivalent_cp.c"

static void ck(const char *s,double a,double b,double tol)
{
    if (fabs(a-b)>tol) {fprintf(stderr,"FAIL %s: %g != %g\n",s,a,b);exit(1);}
}
int main(void)
{
    double t,h,eps=0.0005;
    const double w=AL_T_LIQUIDUS-AL_T_SOLIDUS;
    ck("cp-s",aluminum_cp_value(300),AL_CP_SOLID,1e-10);
    ck("cp-at-solidus",aluminum_cp_value(AL_T_SOLIDUS),AL_CP_SOLID,1e-10);
    ck("cp-l",aluminum_cp_value(1200),AL_CP_LIQUID,1e-10);
    ck("cp-at-liquidus",aluminum_cp_value(AL_T_LIQUIDUS),AL_CP_LIQUID,1e-10);
    ck("k-s",aluminum_k_value(300),AL_K_SOLID,1e-10);
    ck("k-at-solidus",aluminum_k_value(AL_T_SOLIDUS),AL_K_SOLID,1e-10);
    ck("k-l",aluminum_k_value(1200),AL_K_LIQUID,1e-10);
    ck("k-at-liquidus",aluminum_k_value(AL_T_LIQUIDUS),AL_K_LIQUID,1e-10);
    ck("k-mid",aluminum_k_value(829.5),(AL_K_SOLID+AL_K_LIQUID)/2,1e-10);
    ck("fraction-at-solidus",aluminum_fraction_value(AL_T_SOLIDUS),0.0,1e-10);
    ck("fraction-at-liquidus",aluminum_fraction_value(AL_T_LIQUIDUS),1.0,1e-10);
    ck("fraction",aluminum_fraction_value(829.5),0.5,1e-10);
    ck("h-at-solidus",aluminum_integrated_h(AL_T_SOLIDUS),0.0,1e-10);
    ck("h-at-liquidus",aluminum_integrated_h(AL_T_LIQUIDUS),
        0.5*(AL_CP_SOLID+AL_CP_LIQUID)*w+AL_LATENT_HEAT,1e-7);
    ck("latent integral",aluminum_integrated_h(AL_T_LIQUIDUS)-aluminum_integrated_h(AL_T_SOLIDUS),
        (AL_CP_SOLID+AL_CP_LIQUID)*w/2+AL_LATENT_HEAT,1e-7);
    for(t=300;t<=1120;t+=9.43) {
        if(fabs(t-AL_T_SOLIDUS)<1 || fabs(t-AL_T_LIQUIDUS)<1) continue;
        ck("cp-derivative",(aluminum_integrated_h(t+eps)-aluminum_integrated_h(t-eps))/(2*eps),
            aluminum_cp_value(t),1e-5);
        h=0;
        ck("DEFINE-cp",aluminum_equivalent_cp(t,300,&h,NULL),aluminum_cp_value(t),1e-10);
        ck("DEFINE-h",h,aluminum_integrated_h(t)-aluminum_integrated_h(300),1e-7);
    }
    mock_cell c={829.5};
    ck("DEFINE-k",aluminum_thermal_conductivity(&c,NULL),(AL_K_SOLID+AL_K_LIQUID)/2,1e-10);
    puts("PASS demo math tests; Fluent solver not tested");
    return 0;
}
