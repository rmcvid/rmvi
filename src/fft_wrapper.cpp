// Remplace ta computeFFT par ça (C++ wrapper file)
#include "dj_fft.h"
#include "fft_wrapper.h"
#include <vector>
#include <complex>
#include <cmath>
#include <algorithm>
#include <cstdlib>

// gives the next power of two >= v
static int next_pow2(int v) {
    int p = 1;
    while (p < v) p <<= 1;
    return p;
}

// Build ordering: 0, +1, -1, +2, -2, ...
static void build_order(std::vector<int>& order, int Nfft, int M) {
    order.clear();
    order.reserve(M);
    order.push_back(0);
    for (int k = 1; (int)order.size() < M; ++k) {
        if ((int)order.size() < M) order.push_back( k % Nfft );
        if ((int)order.size() < M) order.push_back( (Nfft - k) % Nfft );
    }
}

extern "C" {

void computeFFT(const double *in_re, const double *in_im,
                FourierCoeff *coeffs, int Nfourier, int Npoint)
{
    using namespace dj;
    if (Npoint <= 0 || Nfourier <= 0) return;

    // 1) compute mean and center the points (reduces DC)
    double meanx = 0.0, meany = 0.0;
    for (int i = 0; i < Npoint; ++i) { meanx += in_re[i]; meany += in_im[i]; }
    meanx /= Npoint; meany /= Npoint;

    // 2) choose FFT size (power of two) and zero-pad
    int Nfft = next_pow2(Npoint);
    std::vector<std::complex<double>> input(Nfft, {0.0, 0.0});
    for (int i = 0; i < Npoint; ++i) input[i] = std::complex<double>(in_re[i] - meanx, in_im[i] - meany);

    // optional: scale to [-1,1] by max radius (not mandatory)
    double maxabs = 0.0;
    for (int i = 0; i < Npoint; ++i) {
        double r = std::hypot(input[i].real(), input[i].imag());
        if (r > maxabs) maxabs = r;
    }
    if (maxabs > 0.0) {
        for (int i = 0; i < Npoint; ++i) input[i] /= maxabs;
        // keep zeros padded as zero
    }

    // 3) call fft
    auto spectrum = fft1d<double>(input, fft_dir::DIR_FWD);
    int Ns = (int)spectrum.size(); // == Nfft

    // 4) build selection order (0, +1, -1, +2, -2...)
    std::vector<int> order;
    build_order(order, Ns, Nfourier);

    // 5) fill coeffs according to order. Frequency computed from kprime in [-Nfft/2..Nfft/2)
    for (int i = 0; i < Nfourier; ++i) {
        int idx = order[i];
        std::complex<double> c = spectrum[idx];
        // dj::fft1d uses 1/sqrt(N) normalization; if you want amplitude in usual 1/N scale:
        double amp = std::abs(c);        // currently scaled by 1/sqrt(N)
        // you may want to uniformize: amp /= sqrt((double)Ns); or amp /= (double)Ns;
        // pick convention consistent with reconstruction below
        coeffs[i].c_re  = c.real();
        coeffs[i].c_im  = c.imag();
        coeffs[i].amp   = amp; // adjust normalization here if needed
        coeffs[i].phase = std::arg(c);

        // compute k' centered
        int kprime = (idx <= Ns/2) ? idx : idx - Ns;   // negative for upper half
        // If you animate with t in [0,1): omega = 2*pi*kprime
        coeffs[i].freq  = 2.0 * Pi * (double)kprime / (double)Ns; // rad per normalized period (t in [0,1))
    }
}
} // extern "C"
