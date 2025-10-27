#ifndef FFT_WRAPPER_H
#define FFT_WRAPPER_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    double freq;   /* fréquence angulaire */
    double amp;    /* amplitude (rayon) */
    double phase;  /* phase initiale */
    double c_re;   /* partie réelle du coefficient */
    double c_im;   /* partie imaginaire du coefficient */
} FourierCoeff;

/* computeFFT :
   in_re/in_im : tableaux d'entrée (taille N)
   out_re/out_im : tableaux en sortie (taille N) - coefficients complexes
*/
void computeFFT(const double *in_re, const double *in_im,
                    FourierCoeff *coeffs,
                    int Nfourier, int Npoint);

#ifdef __cplusplus
}
#endif

#endif // FFT_WRAPPER_H

