#include "polynomial.h"
#include <algorithm>
#include <functional>
#include <cmath>

#define _USE_MATH_DEFINES

using std::abs;
using std::vector;
using std::size_t;


Polynomial::Polynomial(const std::initializer_list<double> &coefficients)
    : coeffs(coefficients)
{
    if (coeffs.empty())
        coeffs.resize(1);
    deg = coeffs.size() - 1;
}


Polynomial::Polynomial(const size_t degree)
    : coeffs(degree + 1), deg(degree)
{

}


Polynomial::Polynomial()
    : coeffs(1), deg(0)
{

}


Polynomial::~Polynomial()
{

}


size_t Polynomial::degree() const
{
    return deg;
}


void Polynomial::correctDegree()
{
    // Исключение слишком малых по модулю старших коэффициентов многочлена
    while (coeffs.size() > 1 && abs(coeffs.back()) < zeroEps)
    {
        coeffs.pop_back();
        --deg;
    }
}


Polynomial &Polynomial::differentiate()
{
    // Дифференцирование
    if (deg)
    {
        for (decltype(coeffs.size()) i = 1; i < coeffs.size(); ++i)
            coeffs[i - 1] = i * coeffs[i];
        coeffs.pop_back();
        --deg;
        correctDegree();
    }
    else
        coeffs[0] = 0.0;
    return *this;
}


vector<double> Polynomial::roots()
{
    correctDegree();
    switch (deg)
    {
        // Если степень меньше 5, решение аналитическое
        case 0:
            return vector<double>({0.0});
            break;
        case 1:
            return getLinearEquationSolution();
            break;
        case 2:
            return getQuadraticEquationSolution();
            break;
        case 3:
            return getCubicEquationSolution();
            break;
        case 4:
            return getQuarticEquationSolution();
            break;
        // Иначе решение численное
        default:
            double minX = getLowRootsLimit();
            double maxX = getHighRootsLimit();
            createSturmSystem();
            vector<double> res;
            searchRoots(minX, maxX, res);
            return res;
            break;
    }
}


// Численно ищет корни на отрезке l..r
void Polynomial::searchRoots(double l, double r, vector<double> &vec)
{
    double m = (l + r) / 2;
    // Тривиальный случай
    if ((r - l) < rootEps)
    {
        vec.push_back(m);
        return;
    }
    // Число перемен знака в стандартной системе Штурма при x = l и x = r
    int leftChangesCount = 0, rightChangesCount = 0;
    for (decltype(sturmSystem.size()) i = 1; i < sturmSystem.size(); ++i)
    {
        if (sturmSystem[i - 1](l) * sturmSystem[i](l) < 0)
            ++leftChangesCount;
        if (sturmSystem[i - 1](r) * sturmSystem[i](r) < 0)
            ++rightChangesCount;
    }
    // Количество корней
    int rootsCount = leftChangesCount - rightChangesCount;

    if (rootsCount > 1)
    {
        // Рекурсивные вызовы
        searchRoots(l, m, vec);
        searchRoots(m, r, vec);
    }
    else if (rootsCount == 1)
    {
        // Уточнение корня
        double L = (*this)(l);
        double x;
        // Вторая производная
        Polynomial secondDerivative = (*this);
        secondDerivative.differentiate().differentiate();
        // Выбор начального приближения для метода Ньтютона
        if (secondDerivative(m) > 0)
            x = L > 0 ? l : r;
        else
            x = L < 0 ? l : r;
        if (findRootNewton(l, r, x, 20, sturmSystem[1]))
            vec.push_back(x);
        else
            // Методом Ньютона найти не удалось, используем деление пополам
            vec.push_back(findRootBisection(l, r));
    }
}


double Polynomial::findRootBisection(double lX, double rX) const
{
    // Поиск корня делением отрезка пополам
    double lY = (*this)(lX), rY = (*this)(rX);
    while (abs(rX - lX) > rootEps)
    {
        if (abs(lY) < zeroEps)
            return lX;
        else if (abs(rY) < zeroEps)
            return rX;
        else
        {
            double mX = (lX + rX) / 2;
            double mY = (*this)(mX);
            if (lY * mY <= 0.0)
            {
                rX = mX;
                rY = mY;
            }
            else
            {
                lX = mX;
                lY = mY;
            }
        }
    }
    return lX;
}


bool Polynomial::findRootNewton(double l, double r, double &x, unsigned maxN, const Polynomial &dP) const
{
    // Поиск корня методом Ньютона
    double val, f;
    do
    {
        val = dP(x);
        if (abs(val) < zeroEps || x < l || x > r || !(maxN--))
            // Вышли за границы отрезка (метод не сходится) или превысили максимально допустимое число итераций
            return false;
        f = (*this)(x) / val;
        x -= f;
    }
    while (abs(f) > rootEps);
    return true;
}


double Polynomial::getLowRootsLimit() const
{
    /* Нижняя граница корней полинома.
     * Для многочлена P(x) определяется как верхняя граница многочлена P(-x).
     */
    Polynomial p = *this;
    for (size_t i = 0; i <= p.degree(); ++i)
        if (i % 2)
            p.coeffs[i] *= -1;
    return -1 * (p.getHighRootsLimit());
}


double Polynomial::getHighRootsLimit() const
{
    /* Верхняя граница корней полинома.
     * Для многочлена степени N с a[N] > 0 она равна 1 + (B/A[N])^(1/k),
     * где B - наибольшая из абсолютных величин отрицательных коэффициентов,
     * k определяется из условия: a[k] - старший из отрицательных коэффициентов.
     */
    Polynomial p = *this;
    if (p[p.degree()] < 0)
        p = -p;
    auto negativeRightPos = std::find_if(p.coeffs.crbegin(), p.coeffs.crend(), [](const double &val){return val < 0;});
    if (negativeRightPos == p.coeffs.crend())
        return 0;
    else
    {
        auto index = p.coeffs.cend() - negativeRightPos.base();
        auto maxAbsNegPos = std::min_element(p.coeffs.cbegin(), p.coeffs.cend());
        return 1 + pow(-(*maxAbsNegPos) / p[p.degree()], 1.0 / index);
    }
}


void Polynomial::createSturmSystem()
{
    /* Построение стандартной системы Штурма
     * Первый член - исходный полином,
     * второй член - его производная,
     * третий и последующий члены - взятые с обратным знаком остатки от деления двух предыдущих друг на друга,
     * последний член - полином нулевой степени (константа).
     */
    sturmSystem.clear();
    Polynomial p = *this;
    sturmSystem.push_back(p);
    sturmSystem.push_back(p.differentiate());
    Polynomial r;
    do
    {
        r = sturmSystem[sturmSystem.size() - 2] % sturmSystem.back();
        sturmSystem.push_back(-r);
    }
    while (r.degree());
}


vector<double> Polynomial::getLinearEquationSolution() const
{
    // Линейное уравнение (хотя корень один, результат объявлен как вектор, т.к. функция roots() возвращает вектор)
    vector<double> solution;
    solution.push_back(-coeffs[0] / coeffs[1]);
    return solution;
}


vector<double> Polynomial::getQuadraticEquationSolution() const
{
    // Квадратное уравнение
    vector<double> solution;
    const double &a = coeffs[2];
    const double &b = coeffs[1];
    const double &c = coeffs[0];
    double d = pow(b, 2) - 4 * a * c;
    if (abs(d) < zeroEps)
        solution.push_back(-0.5 * b / a);
    else if (d > 0)
    {
        solution.push_back(0.5 * (sqrt(d) - b) / a);
        solution.push_back(-0.5 * (sqrt(d) + b) / a);
    }
    return solution;
}


vector<double> Polynomial::getCubicEquationSolution() const
{
    // Кубическое уравнение (по формулам Кардано)
    vector<double> solution;
    const double &a = coeffs[3];
    const double &b = coeffs[2];
    const double &c = coeffs[1];
    const double &d = coeffs[0];
    double dd, q, p, t[2];
    t[0] = b / (3 * a);
    p = pow(t[0], 2) - c / (3 * a);
    t[1] = pow(p, 3);
    q = pow(t[0], 3) - (b * c / (3 * a) - d) / (2 * a);
    dd = t[1] - pow(q, 2);
    if (dd > 0)
    {
        double f = acos(-q / sqrt(t[1]));
        for (size_t i = 0; i < 3; ++i)
            solution.push_back(2 * sqrt(p) * cos((f + 2 * M_PI * i) / 3) - t[0]);
    }
    else
        solution.push_back(cbrt(sqrt(-dd) - q) - cbrt(q + sqrt(-dd)) - t[0]);
    return solution;
}


vector<double> Polynomial::getQuarticEquationSolution() const
{
    // Уравнение четвёртой степени (метод Феррари)
    vector<double> solution;
    double b = coeffs[3] / coeffs[4];
    double c = coeffs[2] / coeffs[4];
    double d = coeffs[1] / coeffs[4];
    double e = coeffs[0] / coeffs[4];
    Polynomial resolvent({e * (4 * c - pow(b, 2)) - pow(d, 2),
                          b * d - 4 * e,
                          -c,
                          1});
    vector<double> temp = resolvent.getCubicEquationSolution();
    double y = temp[0];
    double t[2];
    t[0] = pow(b / 2, 2) - c + y;
    Polynomial quadratic(2);
    quadratic[2] = 1.0;
    if (abs(t[0]) < zeroEps)
    {
        quadratic[1] = b / 2;
        quadratic[0] = y / 2 - sqrt(pow(y / 2, 2) - e);
        temp = quadratic.getQuadraticEquationSolution();
        solution.insert(solution.end(), temp.begin(), temp.end());
        quadratic[0] = y / 2 + sqrt(pow(y / 2, 2) - e);
        temp = quadratic.getQuadraticEquationSolution();
        solution.insert(solution.end(), temp.begin(), temp.end());
    }
    else if (t[0] > 0)
    {
        t[0] = sqrt(t[0]);
        t[1] = b * y / 2 - d;
        quadratic[1] = b / 2 - t[0];
        quadratic[0] = 0.5 * (y - t[1] / t[0]);
        temp = quadratic.getQuadraticEquationSolution();
        solution.insert(solution.end(), temp.begin(), temp.end());
        quadratic[1] = b / 2 + t[0];
        quadratic[0] = 0.5 * (y + t[1] / t[0]);;
        temp = quadratic.getQuadraticEquationSolution();
        solution.insert(solution.end(), temp.begin(), temp.end());
    }
    return solution;
}


/* О П Е Р А Т О Р Ы */


Polynomial &Polynomial::operator+=(const Polynomial &polynomial)
{
    coeffs.resize(std::max(coeffs.size(), polynomial.coeffs.size()));
    std::transform(polynomial.coeffs.cbegin(), polynomial.coeffs.cend(), coeffs.cbegin(), coeffs.begin(), std::plus<double>());
    correctDegree();
    return *this;
}


Polynomial &Polynomial::operator-=(const Polynomial &polynomial)
{
    *this += (-polynomial);
    return *this;
}


Polynomial &Polynomial::operator*=(const Polynomial &polynomial)
{
    *this = *this * polynomial;
    return *this;
}


Polynomial &Polynomial::operator%=(const Polynomial &polynomial)
{
    *this = *this % polynomial;
    return *this;
}


Polynomial operator+(const Polynomial &p1, const Polynomial &p2)
{
    Polynomial p(std::max(p1.coeffs.size(), p2.coeffs.size()) - 1);
    std::copy(p1.coeffs.cbegin(), p1.coeffs.cend(), p.coeffs.begin());
    std::transform(p2.coeffs.cbegin(), p2.coeffs.cend(), p.coeffs.cbegin(), p.coeffs.begin(), std::plus<double>());
    p.correctDegree();
    return p;
}


Polynomial operator-(const Polynomial &p1, const Polynomial &p2)
{
    return (-p2) + p1;
}


Polynomial operator*(const Polynomial &p1, const Polynomial &p2)
{
    Polynomial p(p1.coeffs.size() + p2.coeffs.size() - 2);
    for (decltype(p1.coeffs.size()) i = 0; i < p1.coeffs.size(); ++i)
        for (decltype(p2.coeffs.size()) j = 0; j < p2.coeffs.size(); ++j)
            p[i + j] += p1[i] * p2[j];
    p.correctDegree();
    return p;
}


Polynomial operator*(const Polynomial &polynomial, const double &value)
{
    Polynomial p(polynomial.deg);
    std::transform(polynomial.coeffs.cbegin(), polynomial.coeffs.cend(),
                   p.coeffs.begin(), std::bind(std::multiplies<double>(), std::placeholders::_1, value));
    p.correctDegree();
    return p;
}


Polynomial operator*(const double &value, const Polynomial &polynomial)
{
    return polynomial * value;
}


Polynomial Polynomial::operator-() const
{
    Polynomial p(deg);
    std::transform(coeffs.cbegin(), coeffs.cend(), p.coeffs.begin(), std::negate<double>());
    p.correctDegree();
    return p;
}


Polynomial operator%(const Polynomial &p1, const Polynomial &p2)
{
    Polynomial r = p1;
    int d;
    while ((d = r.degree() - p2.degree()) >= 0)
    {
        Polynomial t(d);
        t[d] = r[r.degree()] / p2[p2.degree()];
        r -= p2 * t;
    }
    return r;
}


double Polynomial::operator()(const double &x) const
{
    auto index = coeffs.size() - 1;
    double res = coeffs[index];
    while (index--)
        res = res * x + coeffs[index];
    return res;
}


double &Polynomial::operator[](vector<double>::size_type index)
{
    return coeffs[index];
}


const double &Polynomial::operator[](vector<double>::size_type index) const
{
    return coeffs[index];
}
