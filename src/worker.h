#ifndef WORKER_H
#define WORKER_H

#include <QObject>
#include <vector>
#include <twovarspolynomial.h>


/* -------------------------------------------------------------------  *
 * Worker - класс, выполняющий всю работу по расчёту фазовой диаграммы  *
 * -------------------------------------------------------------------  *
 *                                                                      */


// Коэффициенты модельного потенциала
struct Coefficients
{
    union
    {
        struct
        {
            double a[4];
            double b[2];
            double d[3];
        };
        double c[9];
    };
};

// Информация о фазе
struct PhaseInfo
{
    unsigned type;  // Тип (от 1 до 4)
    double phi;     // Потенциал
    double n[2];    // Параметр порядка
};

// Точка фазовой диаграммы
struct DiagramPoint
{
    double x, y;                    // Коэффициенты Альфа1 (у) и Бета1 (х)
    bool transition;                // Признак первородного фазового перехода
    std::ptrdiff_t stablest;        // Индекс наиболее устойчивой фазы в векторе phases
    std::vector<PhaseInfo> phases;  // Все устойчивые фазы
};


class Worker : public QObject
{
    Q_OBJECT
private:
    static constexpr double eps = 1e-10;
    // Шаг изменения Альфа1 (dY) и Бета1 (dX)
    double dX, dY;
    // Коэффициенты модельного потенциала
    // (задаются перед началом расчётов и меняются в процессе)
    Coefficients coeffs;
    // Двумерный массив, хранящий информацию для каждой точки диаграммы
    std::vector<std::vector<DiagramPoint>> data;
    // Возвращает набор стабильных фаз для текущих значений коэффициентов потенциала
    std::vector<PhaseInfo> getPhases();
    // Определяет устойчивость фазы по компонентам её параметра порядка
    bool isPhaseStableN(const std::array<double, 2> &N, double &phi);
    // Определяет устойчивость фазы по инвариантам
    bool isPhaseStableI(const std::array<double, 2> &I, double &phi);
public:
    Worker(QSize size, QObject *parent = 0);
    virtual ~Worker();
    /* Установка параметров - коэффициентов потенциала Coefficients
     * (для Coefficients.alpha[0] и Coefficients.beta[0] должны быть установлены стартовые значения)
     * и величин "шагов" по Бета1 (stepX) и Альфа1 (stepY).
     * Функция должна быть вызывана перед вызовом calculate().
     */
    void setParameters(const Coefficients coefficients, const double stepX, const double stepY);
    // Возвращает номер (1..4) наиболее устойчивой фазы в данной точке диаграммы или 0, если стабильных фаз нет
    unsigned getStablestPhaseType(const QPoint &point) const;
    // Возвращает потенциал наиболее устойчивой фазы
    double getStablestPhasePotential(const QPoint &point) const;
    // Возвращают компоненты параметра порядка наиболее устойчивой фазы
    double getStablestPhaseFirstOrderParameter(const QPoint &point) const;
    double getStablestPhaseSecondOrderParameter(const QPoint &point) const;
    /* Возвращает пару индексов, соответствующих перемене знака Бета1 и Альфа1 в массиве данных.
     * Функция служит для определения, в каких точках на диаграмме рисовать координатные оси.
     */
    QPoint getZeroIndexes() const;
    // Возвращает true, если фаза phase стабильна в точке point
    bool isPhaseStable(const QPoint &point, const unsigned phase) const;
    // Возвращает true, если точка point принадлежит линии фазового перехода первого рода
    bool isTransition(const QPoint &point) const;
    // Возвращает количество сосуществующих изосимметрийных модификаций фазы phase в точке point
    unsigned getIsosymmetricCount(const QPoint &point, unsigned phase) const;
    // Возвращает Бета1 (Х) и Альфа1 (Y) по целочисленным индексам массива данных
    QPointF getXY(const QPoint &point) const;
    // Возвращает копию вектора коэффициентов
    Coefficients getCoefficients() const;
    // Возвращает константную ссылку на информацию о фазах в точке point
    const DiagramPoint &getDiagramPoint(const QPoint &point) const;
private slots:
    // Запуск вычислений
    void calculate();
signals:
    // Сигнал о завершении работы
    void finished();
    // Сигнал о выполнении percent % расчётов
    void processed(int percent);
};

#endif // WORKER_H
