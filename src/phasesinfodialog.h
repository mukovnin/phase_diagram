#ifndef PHASESINFODIALOG_H
#define PHASESINFODIALOG_H

#include <QDialog>
#include <worker.h>

QT_BEGIN_NAMESPACE
class QTableWidget;
QT_END_NAMESPACE

// Диалог отображает подробную информацию о стабильных в данной точке диаграммы фазах
// Вызывается по клику мыши в нужной точке построенной диаграммы

class PhasesInfoDialog : public QDialog
{
private:
    QTableWidget* tblCoefficients;  // Таблица коэффициентов
    QTableWidget* tblPhases;        // Список стабильных фаз
    /* getDomens - возвращает список доменов (симметрийно связанных решений)
     * n - компоненты параметра порядка
     * phaseType - тип фазы */
    std::vector<std::array<double, 2>> getDomens(const std::array<double, 2> n, const unsigned phaseType);
public:
    PhasesInfoDialog(QWidget *parent = 0);
    /* show - показывает диалог
     * point - структура с информацией о фазах
       coefficients - вектор коэффициентов */
    void show(const DiagramPoint &point, Coefficients coefficients);
};

#endif // PHASESINFODIALOG_H
