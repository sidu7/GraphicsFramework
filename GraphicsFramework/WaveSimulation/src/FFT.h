// FFT.h
// -- Fast Discrete Fourier Transform
// jsh 10/09

#ifndef CS245FFT
#define CS245FFT

#include <complex>
#include <cmath>


template<class REAL>
class FFT_base {
  public:
    FFT_base(unsigned lg_dim, bool inverse);
    ~FFT_base(void);
    template <class OTHER>
      std::complex<REAL> *operator()(OTHER *array);
  private:
    const unsigned lg_dimension, dimension;
    REAL sign2pi, norm;
    unsigned *bit_reverse;
    std::complex<REAL> *transform;
    FFT_base& operator=(const FFT_base&);
};


template<class REAL>
class FFT : public FFT_base<REAL> {
  public:
    FFT(unsigned lg_dim);
};


template<class REAL>
class IFFT : public FFT_base<REAL> {
  public:
    IFFT(unsigned lg_dim);
};


/////////////////////////////////////////////////////////////////

template<class R>
FFT_base<R>::FFT_base(unsigned lg_dim, bool inv)
    : lg_dimension(lg_dim),
      dimension(1<<lg_dim),
      sign2pi((inv?R(8):R(-8)) * std::atan(R(1))),
      norm(inv ? R(1)/R(dimension) : R(1)) {
  bit_reverse = new unsigned[dimension];
  for (unsigned i=0; i < dimension; ++i) {
    unsigned r = 0;
    for (unsigned j=0, n=i; j < lg_dimension; ++j,n >>= 1)
      r = (r << 1) | (n & 0x01);
    bit_reverse[i] = r;
  }
  transform = new std::complex<R>[dimension];
}


template<class R>
FFT_base<R>::~FFT_base(void) {
  delete[] transform;
  delete[] bit_reverse;
}


template<class R>
template<class OTHER>
std::complex<R> *FFT_base<R>::operator()(OTHER *in) {
  for (unsigned p=0; p < dimension; ++p)
    transform[p] = std::complex<R>(in[bit_reverse[p]]);
  for (unsigned h=0; h < lg_dimension; ++h) {
    unsigned m = (1 << h),
             n = (m << 1);
    std::complex<R> twidle = std::exp(std::complex<R>(0,sign2pi/n));
    for (unsigned k=0; k < dimension; k+=n) {
      std::complex<R> twidle_j(1);
      for (unsigned j=0; j < m; ++j) {
        std::complex<R> w = twidle_j*transform[k+j+m],
                        temp = transform[k+j] - w;
        transform[k+j] = transform[k+j] + w;
        transform[k+j+m] = temp;
        twidle_j *= twidle;
      }
    }
  }
  for (unsigned p=0; p < dimension; ++p)
    transform[p] *= norm;
  return transform;
}


template<class R>
FFT<R>::FFT(unsigned lg_dim)
    : FFT_base<R>(lg_dim,false) {
}


template<class R>
IFFT<R>::IFFT(unsigned lg_dim)
    : FFT_base<R>(lg_dim,true) {
}


#endif

