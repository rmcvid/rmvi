#include "dj_fft.h"
#include <vector>
#include <complex>
#include <cstdlib>
#include "fft_wrapper.h" // header C public

extern "C" {
    //constexpr auto Pi = 3.141592653589793238462643383279502884;

/* wrapper existant pour computeFFT (déjà présent) */
    void computeFFT(const double *in_re, const double *in_im,
                    FourierCoeff *coeffs,
                    int N) {
        // pas très efficace car on doit creer la std::vector à chaque appel
        using namespace dj;
        std::vector<std::complex<double>> input;
        input.reserve(16);
        for (int i = 0; i < 16; ++i) input.emplace_back(in_re[i], in_im[i]);
        // ici on utilise la FFT 1D de dj, le mieux serait set up pour utiliser le gpu
        auto spectrum = fft1d<double>(input, fft_dir::DIR_FWD); 
        for (int i = 0; i < N; ++i) {
            coeffs[i].c_re = spectrum[i].real();
            coeffs[i].c_im = spectrum[i].imag();
            coeffs[i].amp  = std::abs(spectrum[i]);
            coeffs[i].phase= std::arg(spectrum[i]);
            // fréquence angulaire centrée (k - N/2) si on souhaite spectre centré
            coeffs[i].freq = 2.0 * Pi * (i - N/2) / (double)N;
        }
    }
} // extern "C"
