#include <QtWidgets>
#include "phasesinfodialog.h"


PhasesInfoDialog::PhasesInfoDialog(QWidget *parent)
    : QDialog(parent)
{
    // Таблица коэффициентов

    QStringList lst;
    lst << "\u03B11" << "\u03B12" << "\u03B13" << "\u03B14" << "\u03B21" << "\u03B22" << "\u03B41" << "\u03B42" << "\u03B43";

    tblCoefficients = new QTableWidget;
    tblCoefficients->setColumnCount(9);
    tblCoefficients->setRowCount(1);
    tblCoefficients->verticalHeader()->hide();
    tblCoefficients->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    tblCoefficients->setHorizontalHeaderLabels(lst);
    tblCoefficients->verticalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);

    QGroupBox *gbCoefficients = new QGroupBox("Коэффициенты модельного потенциала");
    gbCoefficients->setLayout(new QVBoxLayout);
    gbCoefficients->layout()->addWidget(tblCoefficients);
    gbCoefficients->setFixedHeight(100);

    // Таблица стабильных фаз

    lst.clear();
    lst << "Тип фазы" << "\u03B71" << "\u03B72" << "\u03A6";

    tblPhases = new QTableWidget;
    tblPhases->setColumnCount(4);
    tblPhases->verticalHeader()->hide();
    tblPhases->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    tblPhases->setHorizontalHeaderLabels(lst);    
    tblPhases->verticalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);

    QGroupBox *gbPhases = new QGroupBox("Термодинамически устойчивые фазы");
    gbPhases->setLayout(new QVBoxLayout);
    gbPhases->layout()->addWidget(tblPhases);

    // Настройка диалога

    setFixedSize(600, 450);
    setWindowTitle("Список термодинамически устойчивых фаз");

    QVBoxLayout *lytVBox = new QVBoxLayout;
    lytVBox->addWidget(gbCoefficients);
    lytVBox->addWidget(gbPhases);
    setLayout(lytVBox);
}


void PhasesInfoDialog::show(const DiagramPoint &point, Coefficients coefficients)
{
    tblPhases->clearContents();
    tblPhases->setRowCount(0);
    coefficients.a[0] = point.y;
    coefficients.b[0] = point.x;

    // Заполнение таблицы коэффициентов
    for (int i = 0; i < 9; ++i)
        tblCoefficients->setItem(0, i, new QTableWidgetItem(QString("%1").arg(coefficients.c[i])));

    // Заполнение таблицы стабильных фаз
    int currentRow = 0;
    for (auto phase : point.phases)
    {
        // Получение списка доменов данной фазы...
        auto domens = getDomens({phase.n[0], phase.n[1]}, phase.type);
        // ...расширение таблицы...
        tblPhases->setRowCount(tblPhases->rowCount() + domens.size());
        // ...и внесение списка в таблицу
        tblPhases->setItem(currentRow, 0, new QTableWidgetItem(QString("%1").arg(phase.type)));
        tblPhases->setItem(currentRow, 3, new QTableWidgetItem(QString("%1").arg(phase.phi)));
        for (auto item : domens)
        {
            for (int i = 1; i < 3; ++i)
                    tblPhases->setItem(currentRow, i, new QTableWidgetItem(QString("%1").arg(item[i - 1])));
            ++currentRow;
        }
    }

    // Ячейки таблицы не должны быть редактируемыми
    for (int i = 0; i < tblPhases->rowCount(); ++i)
        for (int j = 0; j < tblPhases->columnCount(); ++j)
        {
            QTableWidgetItem *item = tblPhases->item(i, j);
            if (item)
                item->setFlags(item->flags() & ~Qt::ItemIsEditable);
        }

    if (isHidden())
        QDialog::show();
}


std::vector<std::array<double, 2>> PhasesInfoDialog::getDomens(const std::array<double, 2> n, const unsigned phaseType)
{
    std::vector<std::array<double, 2>> res;
    // Включение в список исходного решения
    res.push_back(n);
    // Вычисление симметрийно связанных решений (только для несимметричных фаз 2, 3, 4)
    if (phaseType == 2 || phaseType == 3)
    {
        // 2 домена
        res.push_back({-0.5 * n[0], 0.5 * sqrt(3) * n[0]});
        res.push_back({-0.5 * n[0], -0.5 * sqrt(3) * n[0]});
    }
    else if (phaseType == 4)
    {
        // 5 доменов
        res.push_back({-0.5 * (n[0] - sqrt(3) * n[1]), 0.5 * (n[1] + sqrt(3) * n[0])});
        res.push_back({-0.5 * (n[0] + sqrt(3) * n[1]), 0.5 * (n[1] - sqrt(3) * n[0])});
        res.reserve(2 * res.size());
        for (auto i = res.begin(); i != res.begin() + 3; ++i)
            res.push_back({(*i)[0], -(*i)[1]});
    }
    return res;
}
