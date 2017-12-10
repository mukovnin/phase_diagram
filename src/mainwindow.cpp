#include "mainwindow.h"
#include <QtWidgets>
#include <bitset>
#include <functional>


MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent),
      diagramSize(QSize(500, 500)),
      imgDiagram(QImage(diagramSize, QImage::Format_RGB32)),
      worker(diagramSize),
      settings("Mukovnin", "PhaseDiagram")
{
    phasesInfoDialog = new PhasesInfoDialog(this);

    // Изначально область построения диаграммы залита белым цветом
    imgDiagram.fill(Qt::white);

    // Определяется путь к исполняемому файлу gnuplot
    QString path;
    if (settings.contains("gnuplot"))
        path = settings.value("gnuplot", "").toString();
    else
        path = QDir::currentPath() + "/gnuplot/bin/gnuplot.exe";
    if (QFile::exists(path))
        gnuplotFileName = path;

    // Создание элементов главного окна
    createMenu();
    createOptionsBox();
    createDiagramBox();
    createLegendBox();

    QHBoxLayout *mainLayout = new QHBoxLayout;
    mainLayout->addWidget(gbOptions, 0);
    mainLayout->addWidget(gbDiagram, 1);
    mainLayout->addWidget(gbLegend, 0);

    setCentralWidget(new QWidget);
    centralWidget()->setLayout(mainLayout);
    resize(minimumSizeHint());

    prbProgress = new QProgressBar;
    lblCursorPos = new QLabel;
    lblStatus = new QLabel("Введите параметры и нажмите кнопку \"Применить\".");
    statusBar()->addWidget(lblStatus);

    setWindowTitle("Модельный термодинамический потенциал с симметрией 3m");

    // Центрирование
    move((QApplication::desktop()->screenGeometry().width() - width()) / 2,
         (QApplication::desktop()->screenGeometry().height() - height()) / 2);

    // Состояние: диаграмма не построена
    setDiagramCreated(false);

    /* Соединение сигналов и слотов.
     * При нажатии кнопки "Применить" срабатывает слот start(),
     * в котором проверяются введённые пользователем параметры.
     * Если они корректны, объект worker перемещается в поток thread
     * и поток запускает метод worker.calculate().
     * Объект worker периодически посылает сигналы processed() о проценте выполнения.
     * Как только работа завершена, worker посылает сигнал finished()
     * и поток thread завершается, в результате чего запускается слот threadFinished() главного окна.
     */
    connect(&thread, SIGNAL(started()), this, SLOT(threadStarted()));
    connect(&thread, SIGNAL(started()), &worker, SLOT(calculate()));
    connect(&thread, SIGNAL(finished()), this, SLOT(threadFinished()));
    connect(&worker, SIGNAL(processed(int)), prbProgress, SLOT(setValue(int)));
    connect(&worker, SIGNAL(finished()), &thread, SLOT(quit()));
    connect(btnStart, SIGNAL(clicked()), this, SLOT(start()));
    // Диаграмма перерисовывается, если пользователь изменил настройки её отображения в меню.
    connect(actShowLines, SIGNAL(triggered(bool)), this, SLOT(drawDiagram()));
    connect(actShowIsosym, SIGNAL(triggered(bool)), this, SLOT(drawDiagram()));
    connect(actionGroup, SIGNAL(triggered(QAction*)), this, SLOT(drawDiagram()));
    /* Выбор любого пункта меню "Графики -> Показать график зависимости..." запускает слот showSurface(),
     * внутри которого по sender'у определяется, какой именно график нужно показать.
     */
    for (QAction* action: actShowGraph)
        connect(action, SIGNAL(triggered(bool)), this, SLOT(showSurface()));
}


MainWindow::~MainWindow()
{
    // Сохранение пути к исполняемому файлу gnuplot
    if (!gnuplotFileName.isEmpty())
        settings.setValue("gnuplot", gnuplotFileName);
}


void MainWindow::createMenu()
{
    // Меню "Файл"
    QMenu *fileMenu = new QMenu("&Файл");
    actSave = fileMenu->addAction("&Сохранить диаграмму в файл...", this, SLOT(save()), Qt::CTRL | Qt::Key_S);
    fileMenu->addSeparator();
    fileMenu->addAction("&Выход", this, SLOT(close()));
    menuBar()->addMenu(fileMenu);

    // Меню "Графики"
    QMenu *graphsMenu = new QMenu("&Графики");
    graphsMenu->addAction("&Указать расположение исполняемого файла gnuplot...", this, SLOT(setGnuplotPath()));
    graphsMenu->addSeparator();
    const QString names[3]
         {"&равновесного потенциала \u03A6",
          "&первой компоненты параметра порядка \u03B71",
          "&второй компоненты параметра порядка \u03B72"};
    for (int i = 0; i < 3; ++i)
        actShowGraph[i] = graphsMenu->addAction
                (QString("График зависимости %1 от \u03B11 и \u03B21").arg(names[i]));
    menuBar()->addMenu(graphsMenu);

    // Меню "Параметры"
    QMenu *optionsMenu = new QMenu("&Параметры");
    actShowLines = optionsMenu->addAction("&Показывать линии фазовых переходов первого рода");
    actShowLines->setCheckable(true);
    actShowIsosym = optionsMenu->addAction("П&оказывать области сосуществования изосимметрийных модификаций фаз 2 и 3");
    actShowIsosym->setCheckable(true);
    menuBar()->addMenu(optionsMenu);

    // Подменю "Режим отображения фаз"
    QMenu *viewModeMenu = new QMenu("&Режим отображения фаз");
    optionsMenu->addMenu(viewModeMenu);
    actShowAllStable = viewModeMenu->addAction("&Показывать наборы устойчивых фаз");
    actShowAllStable->setCheckable(true);
    actShowAllStable->setChecked(true);
    actShowMostStable = viewModeMenu->addAction("П&оказывать только наиболее устойчивую фазу");
    actShowMostStable->setCheckable(true);
    actionGroup = new QActionGroup(this);
    actionGroup->addAction(actShowAllStable);
    actionGroup->addAction(actShowMostStable);

    // Меню "Справка"
    QMenu *helpMenu = new QMenu("&Справка");
    helpMenu->addAction("&Показать выражение модельного потенциала...", this, SLOT(showPotential()));
    helpMenu->addAction("&О программе", this, SLOT(about()));
    menuBar()->addMenu(helpMenu);
}


void MainWindow::createOptionsBox()
{
    // Настройка таблиц для ввода диапазонов Альфа1 и Бета1 и значений прочих коэффициентов

    tblValues = new QTableWidget;
    tblValues->setColumnCount(1);
    tblValues->setRowCount(7);
    tblValues->verticalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    tblValues->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    tblValues->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Minimum);

    tblRanges = new QTableWidget;
    tblRanges->setColumnCount(2);
    tblRanges->setRowCount(2);
    tblRanges->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    tblRanges->verticalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    tblRanges->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Maximum);

    QStringList lst;
    lst << "\u03B12" << "\u03B13" << "\u03B14" << "\u03B22" << "\u03B41" << "\u03B42" << "\u03B43";
    tblValues->setVerticalHeaderLabels(lst);

    lst.clear();
    lst << "Значение";
    tblValues->setHorizontalHeaderLabels(lst);

    lst.clear();
    lst << "\u03B11" << "\u03B21";
    tblRanges->setVerticalHeaderLabels(lst);

    lst.clear();
    lst << "Мин" << "Макс";
    tblRanges->setHorizontalHeaderLabels(lst);

    for (int i = 0; i < 2; ++i)
    {
        tblRanges->setItem(i, 0, new QTableWidgetItem("-10"));
        tblRanges->setItem(i, 1, new QTableWidgetItem("10"));
    }

    int arr[7] {1, 1, 0, 1, 1, 0, 0};
    for (int i = 0; i < 7; ++i)
        tblValues->setItem(i, 0, new QTableWidgetItem(QString("%1").arg(arr[i])));

    btnStart = new QPushButton("Применить");
    btnStart->setDefault(true);

    QVBoxLayout *lytVBox = new QVBoxLayout;
    lytVBox->addWidget(tblValues);
    lytVBox->addWidget(tblRanges);
    lytVBox->addWidget(btnStart);

    gbOptions = new QGroupBox("Входные параметры");
    gbOptions->setLayout(lytVBox);
}


void MainWindow::createDiagramBox()
{    
    lblDiagram = new QLabel;
    lblDiagram->setScaledContents(true);
    lblDiagram->setPixmap(QPixmap::fromImage(imgDiagram));

    // Нужно обрабатывать клики и перемещения курсора по диаграмме
    lblDiagram->setMouseTracking(true);
    lblDiagram->installEventFilter(this);

    QVBoxLayout *lytVBox = new QVBoxLayout;
    lytVBox->addWidget(lblDiagram);

    gbDiagram = new QGroupBox("Фазовая диаграмма");
    gbDiagram->setLayout(lytVBox);
}


void MainWindow::createLegendBox()
{
    gbLegend = new QGroupBox("Обозначения на диаграмме");
    QGridLayout *lytGrid = new QGridLayout;

    // Установленный i-й бит означает присутствие i-й фазы
    const unsigned arr[16] {0b0001, 0b0010, 0b0100, 0b1000, 0b0011, 0b0101, 0b1001, 0b0110,
                            0b1010, 0b1100, 0b0111, 0b1011, 0b1101, 0b1110, 0b1111, 0b0000};

    // Построение легенды
    for (int i = 0; i < 20; ++i)
    {
        // Формирование подписи
        QString s;
        if (i < 16)
        {
            std::bitset<4> bs(arr[i]);
            int count = bs.count();
            s = count == 0 ? "Нет устойч. фаз" : count > 1 ? "Фазы" : "Фаза";
            for (size_t j = 0; j < 4; ++j)
                if (bs.test(j))
                    s += QString(" %1").arg(j + 1);
        }
        else
        {
            const QString strArr[4] {tr("Неск. фаз 2"), tr("Неск. фаз 3"),
                                     tr("Неск. фаз 4"), tr("Первородный ФП")};
            s = strArr[i - 16];
        }

        // Определение цвета
        QColor color = i < 16 ? QColor(colors[arr[i]]) : QColor(colors[i]);

        // Отображение цвета
        QLabel *lblColor = new QLabel;
        lblColor->setMinimumWidth(120);
        lblColor->setMaximumHeight(15);
        QPalette pal = lblColor->palette();
        pal.setColor(lblColor->backgroundRole(), color);
        lblColor->setPalette(pal);
        lblColor->setAutoFillBackground(true);

        // Отображение подписи
        QLabel *lblText = new QLabel(s);
        lblText->setAlignment(Qt::AlignCenter);

        QVBoxLayout *lytVBox = new QVBoxLayout;
        lytVBox->addWidget(lblColor, 0, Qt::AlignHCenter);
        lytVBox->addWidget(lblText, 0, Qt::AlignHCenter);

        lytGrid->addLayout(lytVBox, i / 2, i % 2);
    }

    gbLegend->setLayout(lytGrid);
}


bool MainWindow::eventFilter(QObject *pobj, QEvent *pe)
{
    if (pobj == lblDiagram && diagramCreated)
    {
        /* При перемещении курсора по построенной диаграмме
         * в lblCursorPos выводится позиция в координатах Альфа1 - Бета1.
         * По клику на диаграмме показывается диалог phasesInfoDialog
         * с подробной информацией о выбранной точке диаграммы.
         */
        if (pe->type() == QEvent::Leave)
            lblCursorPos->setText(tr("Курсор вне диаграммы"));
        else if (pe->type() == QEvent::Enter || pe->type() == QEvent::MouseMove || pe->type() == QEvent::MouseButtonPress)
        {
            QPointF pf;
            if (pe->type() == QEvent::Enter)
                pf = static_cast<QEnterEvent*>(pe)->localPos();
            else
                pf = static_cast<QMouseEvent*>(pe)->localPos();
            QPoint pos(pf.x() * diagramSize.width() / lblDiagram->width(), pf.y() * diagramSize.height() / lblDiagram->height());
            if (pe->type() == QEvent::MouseButtonPress)
                phasesInfoDialog->show(worker.getDiagramPoint(pos), worker.getCoefficients());
            else
            {
                pf = worker.getXY(pos);
                lblCursorPos->setText(tr("Курсор:   \u03B1<sub>1</sub> = %1   \u03B2<sub>1</sub> = %2").arg(pf.y()).arg(pf.x()));
            }
        }
    }
    return false;
}


bool MainWindow::setWorkerOptions()
{
    // Проверка данных, введённых пользователем
    bool ok[11];    // Если все ok = true, все текстовые данные удалось преобразовать в числа
    const QString s[11] {"\u03B12", "\u03B13", "\u03B14", "\u03B22", "\u03B41", "\u03B42", "\u03B43",
                         "\u03B11 (min)", "\u03B11 (max)", "\u03B21 (min)", "\u03B21 (max)"};
    Coefficients c;

    c.a[1] = tblValues->item(0, 0)->text().toDouble(&ok[0]);
    c.a[2] = tblValues->item(1, 0)->text().toDouble(&ok[1]);
    c.a[3] = tblValues->item(2, 0)->text().toDouble(&ok[2]);
    c.b[1] = tblValues->item(3, 0)->text().toDouble(&ok[3]);
    c.d[0] = tblValues->item(4, 0)->text().toDouble(&ok[4]);
    c.d[1] = tblValues->item(5, 0)->text().toDouble(&ok[5]);
    c.d[2] = tblValues->item(6, 0)->text().toDouble(&ok[6]);

    double maxX, sX, maxY, sY;

    c.a[0] = tblRanges->item(0, 0)->text().toDouble(&ok[7]);
    maxY = tblRanges->item(0, 1)->text().toDouble(&ok[8]);

    c.b[0] = tblRanges->item(1, 0)->text().toDouble(&ok[9]);
    maxX = tblRanges->item(1, 1)->text().toDouble(&ok[10]);

    auto pos = std::find(std::begin(ok), std::end(ok), false);
    if (pos != std::end(ok))
    {
        QString msg = QString("Указанное в таблице значение %1 не удалось преобразовать в число.").arg(s[pos - std::begin(ok)]);
        QMessageBox::warning(this, "Ошибка преобразования", msg);
        return false;
    }

    // Проверка границ диапазонов
    sX = (maxX - c.b[0]) / diagramSize.width();
    sY = (maxY - c.a[0]) / diagramSize.height();

    if (sX <= 0 || sY <= 0)
    {
        int i = sX < 0 ? 9 : 7;
        QString msg = QString("В указанных параметрах %1 больше либо равно %2.").arg(s[i]).arg(s[i + 1]);
        QMessageBox::warning(this, "Ошибка диапазона", msg);
        return false;
    }

    // Установка параметров worker'а
    worker.setParameters(c, sX, sY);
    return true;
}


void MainWindow::drawDiagram()
{
    // Рисование диаграммы, если массив с данными готов
    if (!diagramCreated)
        return;
    // Положение координатных осей
    QPoint zero = worker.getZeroIndexes();
    // Назначение цвета каждому пикселу
    for (int i = 0; i < diagramSize.width(); ++i)
        for (int j = 0; j < diagramSize.height(); ++j)
        {
            QRgb c;
            QPoint p(i, j);
            if (i == zero.x() || j == zero.y())
                c = 0x000000;   // Здесь проходит координатная ось
            else
            {
                bool detected = false;
                if (actShowIsosym->isChecked())
                {
                    for (unsigned k = 2; k <= 4; ++k)
                        if (worker.getIsosymmetricCount(p, k) > 1)
                        {
                            // Пользователь хочет видеть области сосуществования изосимметрийных фаз
                            // и в данной точке диаграммы такие фазы сосуществуют
                            c = colors[k + 14];
                            detected = true;
                        }
                }
                if (!detected)
                {
                    if (actShowLines->isChecked() && worker.isTransition(p))
                        // Пользователь хочет видеть линии первородных фазовых переходов
                        // и данная точка диаграммы принадлежит такой линии
                        c = colors[19];
                    else
                    {
                        // Ничего необычного нет, нужно просто отобразить самую устойчивую фазу или набор всех устойчивых фаз
                        std::bitset<4> bs;
                        if (actShowMostStable->isChecked())
                        {
                            // Установка в bs бита, соответствующего номеру наиболее устойчивой фазы
                            unsigned stablest = worker.getStablestPhaseType(p);
                            if (stablest)
                                bs.set(stablest - 1);
                        }
                        else
                            // Установка в bs битов, соответствующих устойчивым фазам
                            for (size_t k = 1; k <= 4; ++k)
                                if (worker.isPhaseStable(p, k))
                                    bs.set(k - 1);
                        // Определение цвета
                        c = colors[bs.to_ulong()];
                    }
                }
            }
            // Рисование пиксела
            imgDiagram.setPixel(p, c);
        }
    // Отображение картинки из imgDiagram на lblDiagram
    lblDiagram->setPixmap(QPixmap::fromImage(imgDiagram));
}


void MainWindow::showSurface()
{   
    if (gnuplotFileName.isEmpty())
    {
        // Неясно, где искать gnuplot.exe
        QMessageBox::warning
            (this,
             "Расположение gnuplot",
             "Для построения графика необходимо, чтобы на этом компьютере была установлена программа gnuplot. "
             "Она распространяется по свободной лицензии и может быть загружена <A HREF=\"http://www.gnuplot.info/download.html\">по этой ссылке</A>. "
             "После установки в меню \"Графики\" выберите пункт \"Указать расположение исполняемого файла gnuplot...\" и задайте путь к файлу gnuplot.exe."
            );
        return;
    }

    // Подготовка временного файла
    if (!file.open())
    {
        QMessageBox::warning(this, "Ошибка", "График не будет показан, т.к. не удалось сформировать файл данных для gnuplot.");
        return;
    }
    file.resize(0);
    file.setTextModeEnabled(true);

    // Запуск и инициализация gnuplot
    if (gnuplot.state() == QProcess::NotRunning)
    {
        gnuplot.start(QString("\"%1\"").arg(gnuplotFileName).toLocal8Bit());
        gnuplot.write("set termoption enhanced\n");
        gnuplot.write("set xlabel \"{/Symbol b}1\"\n");
        gnuplot.write("set ylabel \"{/Symbol a}1\"\n");
        gnuplot.write("set palette rgb 33,13,10\n");
        gnuplot.write("set key noautotitle\n");
    }

    // Массив функторов, посредством одного из которых будут получаться данные для трёхмерного графика
    std::function<double(const Worker&, const QPoint&)> methods[3] =
            {&Worker::getStablestPhasePotential, &Worker::getStablestPhaseFirstOrderParameter, &Worker::getStablestPhaseSecondOrderParameter};

    // Массив заголовков графиков
    QString titles[3] = {"Thermodynamic potential", "First order parameter component", "Second order parameter component"};

    /* В sender - выбранный пользователем action.
     * Вычисляем нужный index в массивах функторов и заголовков.
     */
    auto index = std::find(std::begin(actShowGraph), std::end(actShowGraph), sender()) - std::begin(actShowGraph);

    // Получение данных о требуемых точках графика и запись их во временный файл
    for (int i = 0; i < diagramSize.width(); i+=5)
    {
        for (int j = 0; j < diagramSize.height(); j+=5)
        {
            QPoint p(i, j);
            QPointF pf = worker.getXY(p);

            if (worker.getStablestPhaseType(p))
            {
                double z = methods[index](worker, p);
                file.write(QString("%1 %2 %3\n").arg(pf.x()).arg(pf.y()).arg(z).toLocal8Bit());
            }
        }
        file.write("\n");
    }
    file.close();

    // Установка заголовка графика и фактическое рисование
    gnuplot.write(QString("set title \" %1 \"\n").arg(titles[index]).toLocal8Bit());
    gnuplot.write(QString("splot \"" + file.fileName() + "\" with pm3d\n").toLocal8Bit());
}


/* Прочее, см. описание в заголовочном файле */


void MainWindow::showPotential()
{
    QMessageBox::information
        (this,
         "Модельный потенциал",
         "\u03b7<sub>1</sub> и \u03b7<sub>2</sub> \u2014 компоненты параметры порядка"
         "<br>I<sub>1</sub> = \u03b7<sub>1</sub><sup>2</sup> + \u03b7<sub>2</sub><sup>2</sup> и "
         "I<sub>2</sub> = \u03b7<sub>1</sub><sup>3</sup> - 3\u03b7<sub>1</sub>\u03b7<sub>2</sub><sup>2</sup>  \u2014 инварианты"
         "<br>\u03a6 = \u03b1<sub>1</sub>I<sub>1</sub> + \u03b1<sub>2</sub>I<sub>1</sub><sup>2</sup> + \u03b1<sub>3</sub>I<sub>1</sub><sup>3</sup> + "
         "\u03b1<sub>4</sub>I<sub>1</sub><sup>4</sup> + \u03b2<sub>1</sub>I<sub>2</sub> + \u03b2<sub>2</sub>I<sub>2</sub><sup>2</sup> + "
         "\u03b4<sub>1</sub>I<sub>1</sub>I<sub>2</sub> + \u03b4<sub>2</sub>I<sub>1</sub><sup>2</sup>I<sub>2</sub> + "
         "\u03b4<sub>3</sub>I<sub>1</sub>I<sub>2</sub><sup>2</sup> \u2014 потенциал"
        );
}


void MainWindow::setDiagramCreated(bool flag)
{
    diagramCreated = flag;
    actSave->setEnabled(diagramCreated);
    for (auto action : actShowGraph)
        action->setEnabled(diagramCreated);
}


void MainWindow::save()
{
    QString path = QFileDialog::getSaveFileName(this, "Сохранение диаграммы", "", "*.bmp");
    if (!path.isEmpty())
        imgDiagram.save(path, "BMP");
}


void MainWindow::setGnuplotPath()
{
    QString path = QFileDialog::getOpenFileName(this, "Файл gnuplot", "", "");
    if (!path.isEmpty())
        gnuplotFileName = path;
}


void MainWindow::start()
{
    if (setWorkerOptions())
    {
        worker.moveToThread(&thread);
        thread.start();
    }
}


void MainWindow::threadStarted()
{
    setDiagramCreated(false);
    lblStatus->setText("Подождите...");
    statusBar()->removeWidget(lblCursorPos);
    statusBar()->addWidget(prbProgress);
    prbProgress->show();
}


void MainWindow::threadFinished()
{
    setDiagramCreated(true);
    drawDiagram();
    prbProgress->reset();
    lblStatus->setText("Для получения полной информации нажмите левую кнопку мыши в нужной точке диаграммы.");
    statusBar()->removeWidget(prbProgress);
    statusBar()->addWidget(lblCursorPos);
    lblCursorPos->show();
}


void MainWindow::about()
{
    QMessageBox::about(this, "О программе",
                       "Программа \"Phase Diagram 1.0\"\n \u00A9 Алексей Муковнин, 2011\n(2017 - портирование на Qt 5.9.0)");
}
