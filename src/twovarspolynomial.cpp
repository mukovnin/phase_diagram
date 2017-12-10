#include "twovarspolynomial.h"

using std::size_t;

TwoVarsPolynomial::TwoVarsPolynomial(size_t degree)
    : deg(degree), coeffs(deg + 1)
{

}


TwoVarsPolynomial::~TwoVarsPolynomial()
{

}


TwoVarsPolynomial &TwoVarsPolynomial::differentiate(size_t x, size_t y)
{
    while (x--)
        for (auto &item : coeffs)
            item.differentiate();
    if (y)
    {
        if (deg)
            while (y--)
            {
                for (decltype(coeffs.size()) i = 1; i < coeffs.size(); ++i)
                    coeffs[i - 1] = i * coeffs[i];
                coeffs.pop_back();
                --deg;
            }
        else
            coeffs[0] = {0.0};
    }
    return *this;
}


double TwoVarsPolynomial::operator()(const double &x, const double &y) const
{
    Polynomial p(coeffs.size() - 1);
    for (decltype(coeffs.size()) i = 0; i < coeffs.size(); ++i)
        p[i] = coeffs[i](x);
    return p(y);
}


Polynomial &TwoVarsPolynomial::operator[](size_t index)
{
    return coeffs[index];
}


const Polynomial &TwoVarsPolynomial::operator[](size_t index) const
{
    return coeffs[index];
}
