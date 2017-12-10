#ifndef TWOVARSPOLYNOMIAL_H
#define TWOVARSPOLYNOMIAL_H

#include <polynomial.h>

/* Полином от двух переменных
 * Рассматривается как обычный полином одной переменной (y),
 * коэффициентами которого являются полиномы другой переменной (x).
 * Реализованы только необходимые функции.
 */

class TwoVarsPolynomial
{
private:
    std::size_t deg;                    // Степень
    std::vector<Polynomial> coeffs;     // Коэффициенты - полиномы
public:
    TwoVarsPolynomial(std::size_t degree);
    virtual ~TwoVarsPolynomial();
    // Дифференцирует полином x и y раз по соответствующим переменным
    TwoVarsPolynomial &differentiate(std::size_t x, std::size_t y);
    // Вычисляет полином при заданных значениях х и у
    double operator()(const double &x, const double &y) const;
    // Возвращает требуемый коэффициент-полином
    Polynomial &operator[](std::size_t index);
    const Polynomial &operator[](std::size_t index) const;
};

#endif // TWOVARSPOLYNOMIAL_H
