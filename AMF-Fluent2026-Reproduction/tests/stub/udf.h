#ifndef UDF_H
#define UDF_H
/* Minimal standalone C-test shim. NOT real Fluent UDF API. */
typedef double real;
typedef struct {real temperature;} mock_cell;
#define C_T(c,t) ((c)->temperature)
#define DEFINE_SPECIFIC_HEAT(name,T,Tref,h,yi) real name(real T,real Tref,real *h,real *yi)
#define DEFINE_PROPERTY(name,c,t) real name(mock_cell *c,void *t)
#endif
