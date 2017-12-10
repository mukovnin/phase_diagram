#ifndef POLYNOMIAL_H
#define POLYNOMIAL_H

#include <initializer_list>
#include <vector>

/* Класс, реализующий основные операции с полиномами одной переменной */

class Polynomial
{
private:
    // Если коэффициент полинома по модулю меньше zeroEps, он считается равным нулю
    static constexpr double zeroEps = 1e-10;
    // Погрешность нахождения корней
    static constexpr double rootEps = 1e-5;
    // Вектор коэффициентов
    std::vector<double> coeffs;
    // Степень
    std::size_t deg;
    // Вектор полиномов, образующих стандартную систему Штурма
    std::vector<Polynomial> sturmSystem;
    // Метод сравнивает модули коэффициентов с величиной zeroEps и корректирует степень полинома
    void correctDegree();
    // Возвращает корень полинома на отрезке lX..rX, найденный бинарным поиском
    double findRootBisection(double lX, double rX) const;
    /* Функция поиска корня полинома на отрезке l..r методом Ньютона.
     * В параметре х передаётся начальное приближение и возвращается корень.
     * maxN - максимально допустимое количество итераций.
     * dP - производная исследуемого полинома.
     * Функция возвращает true, если метод Ньютона сходится
     * и удалось найти корень, не превысив максимальное число итераций.
     */
    bool findRootNewton(double l, double r, double &x, unsigned maxN, const Polynomial &dP) const;
    // Возвращает нижнюю границу корней полинома
    double getLowRootsLimit() const;
    // Возвращает верхнюю границу корней полинома
    double getHighRootsLimit() const;
    // Создаёт стандартную систему Штурма
    void createSturmSystem();
    // Находит все вещественные корни полинома на отрезке l..r и заносит их в вектор vec
    void searchRoots(double l, double r, std::vector<double> &vec);
    // 4 функции аналитически решают уравнения степеней от 1 до 4 и возвращают вектор корней
    std::vector<double> getLinearEquationSolution() const;
    std::vector<double> getQuadraticEquationSolution() const;
    std::vector<double> getCubicEquationSolution() const;
    std::vector<double> getQuarticEquationSolution() const;
public:
    // Конструктор, создаёт полином по списку его коэффициентов
    Polynomial(const std::initializer_list<double> &coefficients);
    // Конструктор, создаёт полином заданной степени
    Polynomial(const std::size_t degree);
    // Конструктор, создаёт полином первой степени
    Polynomial();
    // Пустой деструктор
    virtual ~Polynomial();
    // Возвращает текущую степень полинома
    std::size_t degree() const;
    // Дифференцирует полином и возвращает ссылку на него
    Polynomial& differentiate();
    // Возвращает вектор всех вещественных корней полинома
    std::vector<double> roots();
    // Составные операторы присваивания
    Polynomial& operator+=(const Polynomial &polynomial);
    Polynomial& operator-=(const Polynomial &polynomial);
    Polynomial& operator*=(const Polynomial &polynomial);
    Polynomial& operator%=(const Polynomial &polynomial);
    // Оператор изменения знака полинома
    Polynomial operator-() const;
    // Оператор вычисления полинома
    double operator()(const double &x) const;
    // Операторы индексирования (возвращают нужный коэффициент полинома)
    double &operator[](std::vector<double>::size_type index);
    const double &operator[](std::vector<double>::size_type index) const;
    /* Операторы:
     * сложение, вычитание, умножение, вычисление остатка от деления двух полиномов;
     * умножение полинома на число.
     */
    friend Polynomial operator+(const Polynomial &p1, const Polynomial &p2);
    friend Polynomial operator-(const Polynomial &p1, const Polynomial &p2);
    friend Polynomial operator*(const Polynomial &p1, const Polynomial &p2);
    friend Polynomial operator*(const Polynomial &polynomial, const double &value);
    friend Polynomial operator*(const double &value, const Polynomial &polynomial);
    friend Polynomial operator%(const Polynomial &p1, const Polynomial &p2);
};

#endif // POLYNOMIAL_H
