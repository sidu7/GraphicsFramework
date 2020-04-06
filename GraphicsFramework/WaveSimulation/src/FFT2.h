#include "Grid.h"
#include "FFT.h"
#include "glm/glm.hpp"
#include <math.h>
#include <vector>


template <class R>
class FFT2_base
{
    public:
    FFT2_base(int lgW, int lgH, bool inv);
    void operator()(Grid<std::complex<R> > &A);
    private:
    void fft_helper(bool row_first, Grid<std::complex<R> > &A);
    int W, H;
    int lg_W, lg_H;
    bool inverse;
};

template <class R>
FFT2_base<R>::FFT2_base(int lgW, int lgH, bool inv) : 
    W(1 << lgW), H(1 << lgH), lg_W(lgW), lg_H(lgH), inverse(inv)
{}

template <class R>
void FFT2_base<R>::fft_helper(bool row_first, Grid<std::complex<R> > &A)
{
    int first, second, lg_f;
    if(row_first)
    {
        first = W;
        second = H;
        lg_f = lg_H;
    }
    else
    {
        first = H;
        second = W;
        lg_f = lg_W;
    }
    

    for(int q = 0; q < first; ++q)
    {
        std::vector<std::complex<R> > C;
        for(int p = 0; p < second; ++p)
        {
            C.push_back(row_first? A(q,p) : A(p,q));
        }

        FFT_base<R> fft(lg_f,inverse);
        std::complex<R>* ft = fft(&C[0]);;
       
        for(int p = 0; p < second; ++p)
        {
            if(row_first) { A(q,p) = ft[p]; }
            else { A(p,q) = ft[p]; }
        }
    }
}

template <class R>
void FFT2_base<R>::operator()(Grid<std::complex<R> > &A)
{
    if(!inverse) fft_helper(false,A);
    fft_helper(true,A);
    if(inverse) fft_helper(false,A);
}

template <class R>
class FFT2 : public FFT2_base<R>
{
    public:
    FFT2(int lgW, int lgH);
};

template <class R>
FFT2<R>::FFT2(int lgW, int lgH) : FFT2_base<R>(lgW, lgH, false)
{}

template <class R>
class IFFT2 : public FFT2_base<R>
{
    public:
    IFFT2(int lgW, int lgH);
};

template <class R>
IFFT2<R>::IFFT2(int lgW, int lgH) : FFT2_base<R>(lgW, lgH, true)
{}