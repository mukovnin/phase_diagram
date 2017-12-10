#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QThread>
#include <QProcess>
#include <QSettings>
#include <QTemporaryFile>
#include "worker.h"
#include "phasesinfodialog.h"

QT_BEGIN_NAMESPACE
class QAction;
class QActionGroup;
class QGroupBox;
class QLabel;
class QImage;
class QMenu;
class QMenuBar;
class QPushButton;
class QTableWidget;
class QProgressBar;
class QProcess;
class QTemporaryFile;
class QSettings;
QT_END_NAMESPACE

// Главное окно

class MainWindow : public QMainWindow
{
    Q_OBJECT
private:
    // Размер двумерного массива, представляющего диаграмму: инициализируется в конструкторе, по умолчанию 500х500
    const QSize diagramSize;
    // Цвета для обозначения областей на диаграмме
    const QRgb colors[20] {0xffffff, 0x008000, 0x000080, 0xff7f00, 0x800080, 0xffff00, 0x5959ab, 0x5c3317,
                           0x800000, 0x70db93, 0x4d4dff, 0x97694f, 0xff1cae, 0x99cc32, 0x80aead, 0xff0000,
                           0xc0d9d9, 0x38b0de, 0xd8bfd8, 0x00ffff};

    QAction *actSave;            // Сохранение
    QAction *actShowLines;       // Показ линий первородных фазовых переходов
    QAction *actShowGraph[3];    // Отображение трёхмерных графиков
    QAction *actShowIsosym;      // Отображение областей с изосимметрийными низкосимметричными фазами
    QAction *actShowMostStable;  // Отображение только наиболее стабильной фазы
    QAction *actShowAllStable;   // Отображение всех стабильных фаз
    QActionGroup *actionGroup;   // Группа для actShowMostStable и actShowAllStable

    QImage imgDiagram;
    QLabel *lblDiagram;
    QLabel *lblStatus;
    QLabel *lblCursorPos;
    QGroupBox *gbLegend;
    QGroupBox *gbOptions;
    QGroupBox *gbDiagram;
    QPushButton *btnStart;
    QTableWidget *tblValues;
    QTableWidget *tblRanges;
    QProgressBar *prbProgress;

    Worker worker;                          // Объект, занимающийся расчётами
    QThread thread;                         // Поток, в котором происходит работа worker'а
    PhasesInfoDialog *phasesInfoDialog;     // Диалог с подробной информацией о фазах в данной точке диаграммы
    QProcess gnuplot;                       // Запущенный процесс gnuplot
    QTemporaryFile file;                    // Временный файл для построения графика в gnuplot
    QSettings settings;                     // Сохранение настроек
    QString gnuplotFileName;                // Путь к исполняемому файлу gnuplot

    // Создание элементов главного окна
    void createMenu();
    void createLegendBox();
    void createOptionsBox();
    void createDiagramBox();

    // Признак того, что диаграмма построена
    bool diagramCreated;
    /* Установка параметров worker'а,
     * вызывается перед запуском расчётов, считывая введённые пользователем в элементах главного окна данные */
    bool setWorkerOptions();

    // Меняет значение флага diagramCreated, управляя доступностью пунктов меню
    void setDiagramCreated(bool flag);

protected:
    /* Фильтр событий главного окна:
     * обрабатывает перемещение курсора по построенной диаграмме (меняет текст в lblCursorPos)
     * и клики на диаграмме (показывает диалог phasesInfoDialog) */
    virtual bool eventFilter(QObject *pobj, QEvent *pe);

/* С Л О Т Ы */
private slots:
    void drawDiagram();     // Рисует построенную диаграмму на imgDiagram
    void about();           // Показать диалог "О программе"
    void save();            // Показать диалог сохранения диаграммы
    void setGnuplotPath();  // Показать диалог выбора исполняемого файла gnuplot
    void showSurface();     // Показать один из трёхмерных графиков
    void showPotential();   // Показать диалог с выражением для потенциала
    void start();           // Нажатие кнопки "Применить" - запуск расчётов, если введённые параметры корректны
public slots:    
    void threadStarted();   // Поток с расчётами стартовал
    void threadFinished();  // Поток с расчётами завершился

public:
    MainWindow(QWidget *parent = 0);
    ~MainWindow();
};

#endif // MAINWINDOW_H
