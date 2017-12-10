#include <QPoint>
#include <QSize>
#include <algorithm>
#include <bitset>
#include <cmath>
#include "worker.h"
#include "polynomial.h"
#include "twovarspolynomial.h"


Worker::Worker(QSize size, QObject *parent)
    : QObject(parent)
{
    // Резервирование места в двумерном векторе
    data.resize(size.width());
    for (auto &vec : data)
        vec.resize(size.height());
}


Worker::~Worker()
{

}


/* Решает уравнения состояния,
 * проверяет выполнение условий термодинамической устойчивости
 * и возвращает список стабильных фаз для коэффициентов coeffs
 */

std::vector<PhaseInfo> Worker::getPhases()
{
    std::vector<PhaseInfo> info;

    // Фаза 1
    if (coeffs.a[0] > 0)
        info.push_back({.type = 1, .phi = 0.0, .n = {0.0, 0.0}});

    // Фазы 2 и 3
    Polynomial equation({
                            2 * coeffs.a[0],
                            3 * coeffs.b[0],
                            4 * coeffs.a[1],
                            5 * coeffs.d[0],
                            6 * (coeffs.a[2] + coeffs.b[1]),
                            7 * coeffs.d[1],
                            8 * (coeffs.a[3] + coeffs.d[2])
                        });
    auto solution = equation.roots();
    for (double value : solution)
    {
        std::array<double, 2> N {value, 0.0};
        double f;
        if (isPhaseStableN(N, f))
            info.push_back({.type = N[0] < 0 ? (unsigned)2 : (unsigned)3, .phi = f, .n = {N[0], N[1]}});
    }

    // Фаза 4
    std::vector<std::array<double, 2>> inv;
    Polynomial B({coeffs.d[0], 2 * coeffs.d[1]});
    Polynomial C({coeffs.a[0], 2 * coeffs.a[1], 3 * coeffs.a[2], 4 * coeffs.a[3]});
    if (coeffs.d[2] == 0.0)
    {
        if (coeffs.b[1] == 0.0)
        {
            equation = {coeffs.b[0], coeffs.d[0], coeffs.d[1]};
            solution = equation.roots();
            for (double value : solution)
            {
                double BB = B(value);
                if (std::abs(BB) > eps)
                    inv.push_back({value, -C(value) / BB});
            }
        }
        else
        {
            Polynomial E({2 * coeffs.b[1]});
            Polynomial F({coeffs.b[0], coeffs.d[0], coeffs.d[1]});
            equation = B * F - C * E;
            solution = equation.roots();
            for (double value : solution)
                inv.push_back({value, -F(value) / E(value)});
        }
    }
    else
    {
        Polynomial A({coeffs.d[2]});
        Polynomial D = B * B - 4 * A * C;
        Polynomial E({2 * coeffs.b[1], 2 * coeffs.d[2]});
        Polynomial F({coeffs.b[0], coeffs.d[0], coeffs.d[1]});
        Polynomial G = B * E - 2 * A * F;
        equation = E * E * D - G * G;
        solution = equation.roots();
        for (double value : solution)
        {
            double DD = D(value);
            if (DD >= 0)
            {
                double t;
                if (E(value) * G(value) >= 0)
                    t = 0.5 * (-B(value) + sqrt(D(value))) / A(value);
                else
                    t = 0.5 * (-B(value) - sqrt(D(value))) / A(value);
                inv.push_back({value, t});
            }
        }
    }
    for (auto item : inv)
    {
        double f;
        if (item[0] > 0 && isPhaseStableI(item, f))
        {
            Polynomial eq({-1 * item[1], -3 * item[0], 0.0, 4});
            auto r = eq.roots();
            if (!r.empty())
            {
                double sq = item[0] - pow(r[0], 2);
                if (sq > eps)
                    info.push_back({.type = 4, .phi = f, .n = {r[0], sqrt(sq)}});
            }
        }
    }

    return info;
}


/* Возвращает true, если при данных значениях coeffs
 * фаза с параметром порядка N термодинамически стабильна,
 * т.е. выполнены условия минимума потенциала как функции компонент N[0] и N[1].
 * Если возвращается true, в phi помещается потенциал фазы.
 */
bool Worker::isPhaseStableN(const std::array<double, 2> &N, double &phi)
{
    /* potentialN - термодинамический потенциал как функция двух переменных N[0] и N[1]:
     * a[3] * (N[1])^8 + (a[2]*(N[0])^2 - 3*d[1]*N[0] + 9*d[2] + 4*a[3]) * (N[1])^6... и т.д.
     */
    TwoVarsPolynomial potentialN(8);
    potentialN[8] = {coeffs.a[3]};
    potentialN[6] = {coeffs.a[2],
                     -3 * coeffs.d[1],
                     9 * coeffs.d[2] + 4 * coeffs.a[3]};
    potentialN[4] = {coeffs.a[1],
                     -3 * coeffs.d[0],
                     3 * (coeffs.a[2] + 3 * coeffs.b[1]),
                     -5 * coeffs.d[1],
                     3 * (2 * coeffs.a[3] + coeffs.d[2])};
    potentialN[2] = {coeffs.a[0],
                     -3 * coeffs.b[0],
                     2 * coeffs.a[1],
                     -2 * coeffs.d[0],
                     3 * (coeffs.a[2] - 2 * coeffs.b[1]),
                     -coeffs.d[1],
                     4 * coeffs.a[3] - 5 * coeffs.d[2]};
    potentialN[0] = {0.0,
                     0.0,
                     coeffs.a[0],
                     coeffs.b[0],
                     coeffs.a[1],
                     coeffs.d[0],
                     coeffs.a[2] + coeffs.b[1],
                     coeffs.d[1],
                     coeffs.a[3] + coeffs.d[2]};
    TwoVarsPolynomial diffX(potentialN), diffY(potentialN), diffXY(potentialN);
    diffX.differentiate(2, 0);
    diffY.differentiate(0, 2);
    diffXY.differentiate(1, 1);
    double first = diffX(N[0], N[1]);
    if (first <= 0 || first * diffY(N[0], N[1]) - pow(diffXY(N[0], N[1]), 2) <= 0)
        return false;
    phi = potentialN(N[0], N[1]);
    return true;
}


/* Возвращает true, если при данных значениях coeffs
 * фаза 4 с инвариантами I термодинамически стабильна,
 * т.е. выполнены условия минмимума потенциала как функции I[0] и I[1].
 * Если возвращается true, в phi помещается потенциал фазы.
 */
bool Worker::isPhaseStableI(const std::array<double, 2> &I, double &phi)
{
    TwoVarsPolynomial potentialI(2);
    potentialI[2] = {coeffs.b[1],
                     coeffs.d[2]
                    };
    potentialI[1] = {coeffs.b[0],
                     coeffs.d[0],
                     coeffs.d[1]
                    };
    potentialI[0] = {0.0,
                     coeffs.a[0],
                     coeffs.a[1],
                     coeffs.a[2],
                     coeffs.a[3]
                    };
    TwoVarsPolynomial diffX(potentialI), diffY(potentialI), diffXY(potentialI);
    diffX.differentiate(2, 0);
    diffY.differentiate(0, 2);
    diffXY.differentiate(1, 1);
    double first = diffX(I[0], I[1]);
    if (first <= 0 || first * diffY(I[0], I[1]) - pow(diffXY(I[0], I[1]), 2) <= 0)
        return false;
    phi = potentialI(I[0], I[1]);
    return true;
}

// Расчёт и заполнение массива data
void Worker::calculate()
{
    // Стартовые значения Альфа1 и Бета1
    double startY = coeffs.a[0], startX = coeffs.b[0];

    for (decltype(data.size()) i = 0; i < data.size(); ++i)
    {
        coeffs.b[0] = startX + i * dX;

        for (decltype(data[i].size()) j = 0; j < data[i].size(); ++j)
        {
            coeffs.a[0] = startY + (data[i].size() - 1 - j) * dY;

            DiagramPoint &dp = data[i][j];
            dp.x = coeffs.b[0];
            dp.y = coeffs.a[0];

            dp.phases = getPhases();

            // Выяснение наиболее устойчивой фазы, т.е. фазы с миниммальным потенциалом
            auto pos = std::min_element(dp.phases.begin(),
                                        dp.phases.end(),
                                        [] (PhaseInfo &first, PhaseInfo &second) {return first.phi < second.phi;});
            dp.stablest = pos == dp.phases.end() ? -1 : pos - dp.phases.begin();

            if (i && j)
            {
                // Выяснение, не происходит ли в данной точке фазовый переход первого рода
                std::bitset<4> bs[3];
                std::for_each(data[i][j].phases.cbegin(), data[i][j].phases.cend(), [&bs](const PhaseInfo &item){bs[0].set(item.type - 1);});
                std::for_each(data[i - 1][j].phases.cbegin(), data[i - 1][j].phases.cend(), [&bs](const PhaseInfo &item){bs[1].set(item.type - 1);});
                std::for_each(data[i][j - 1].phases.cbegin(), data[i][j - 1].phases.cend(), [&bs](const PhaseInfo &item){bs[2].set(item.type - 1);});
                data[i][j].transition = (bs[0].count() > 1 && bs[1].count() > 1 && bs[0] == bs[1] && data[i][j].stablest != data[i - 1][j].stablest) ||
                                        (bs[0].count() > 1 && bs[2].count() > 1 && bs[0] == bs[2] && data[i][j].stablest != data[i][j - 1].stablest);
            }
        }

        // Сигнал о проценте выполнения
        emit processed(100 * i / (data.size() - 1));
    }

    // Возвращение a[0] и b[0] их начальных значений
    coeffs.a[0] = startY, coeffs.b[0] = startX;

    // Сигнал о завершении
    emit finished();
}


/* Установка параметров: коэффициентов потенциала (для Альфа1 и Бета1 передаются их стартовые значения)
 * и шагов изменения Альфа1 (stepY) и Бета1 (stepX).
 * Функция должна вызываться перед запуском calculate().
 */
void Worker::setParameters(const Coefficients coefficients, const double stepX, const double stepY)
{
   coeffs = coefficients;
   dX = stepX;
   dY = stepY;
}


unsigned Worker::getStablestPhaseType(const QPoint &point) const
{
    /* Возвращает тип наиболее стабильной фазы в точке point.
     * Если стабильных фаз нет, возвращает 0.
     */
    const DiagramPoint &dp = data[point.x()][point.y()];
    return dp.stablest == -1 ? 0 : dp.phases[dp.stablest].type;
}


/* Следующие три функции возвращают потенциал и компоненты параметра порядка наиболее устойчивой фазы в точке point.
 * Эти функции должны вызываться после предварительной проверки, есть ли в данной точке стабильные фазы.
 */

double Worker::getStablestPhasePotential(const QPoint &point) const
{
    const DiagramPoint &dp = data[point.x()][point.y()];
    return dp.phases[dp.stablest].phi;
}


double Worker::getStablestPhaseFirstOrderParameter(const QPoint &point) const
{
    const DiagramPoint &dp = data[point.x()][point.y()];
    return dp.phases[dp.stablest].n[0];
}


double Worker::getStablestPhaseSecondOrderParameter(const QPoint &point) const
{
    const DiagramPoint &dp = data[point.x()][point.y()];
    return dp.phases[dp.stablest].n[1];
}


QPoint Worker::getZeroIndexes() const
{
    // Возвращает пиксельные координаты, определяющие положение координатных осей на диаграмме
    int i = -coeffs.b[0] / dX;
    int j = static_cast<int>(data[0].size()) - 1 + static_cast<int>(coeffs.a[0] / dY);
    return QPoint(i, j);
}


bool Worker::isPhaseStable(const QPoint &point, const unsigned phase) const
{
    // Возвращает true, если фаза phase стабильна в точке point
    auto &vec = data[point.x()][point.y()].phases;
    auto pos = std::find_if(vec.cbegin(),
                            vec.cend(),
                            [phase] (const PhaseInfo &item) {return item.type == phase;});
    return pos != vec.cend();
}


bool Worker::isTransition(const QPoint &point) const
{
    // Возвращает true, если точка point лежит на линии фазового перехода первого рода
    return data[point.x()][point.y()].transition;
}


unsigned Worker::getIsosymmetricCount(const QPoint &point, unsigned phase) const
{
    /* Возвращает количество стабильных фаз типа phase в точке point,
     * т.е. число изосимметрийных модификаций фазы данного типа.
     */
    auto &vec = data[point.x()][point.y()].phases;
    return std::count_if(vec.cbegin(),
                         vec.cend(),
                         [phase] (const PhaseInfo &item) {return item.type == phase;});
}


QPointF Worker::getXY(const QPoint &point) const
{
    /* Возвращает пару вещественных координат х (Бета1) и у (Альфа1),
     * соответствующую паре "пиксельных" координат point.
     */
    const DiagramPoint &dp = data[point.x()][point.y()];
    return QPointF(dp.x, dp.y);
}


Coefficients Worker::getCoefficients() const
{
    return coeffs;
}


const DiagramPoint &Worker::getDiagramPoint(const QPoint &point) const
{
    // Возвращает константную ссылку на структуру с информацией о данной точке диаграммы
    return data[point.x()][point.y()];
}
