#include <R.h>
#include <Rinternals.h>
#include <R_ext/Rdynload.h>
#include <stdlib.h> // for NULL

/* .Call calls */
extern SEXP C_opus_decode(SEXP in_file, SEXP out_file,
                           SEXP rate_sexp, SEXP stereo_sexp,
                           SEXP gain_sexp, SEXP float_sexp);

static const R_CallMethodDef CallEntries[] = {
    {"C_opus_decode", (DL_FUNC) &C_opus_decode, 6},
    {NULL, NULL, 0}
};

void R_init_audio_opus(DllInfo *dll)
{
    R_registerRoutines(dll, NULL, CallEntries, NULL, NULL);
    R_useDynamicSymbols(dll, FALSE);
}