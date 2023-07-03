#include "dinamicchart.h"

DinamicChart::DinamicChart(QWidget *parent)
    : QMainWindow(parent), chartRunning(true)
{
    setupChart();

    setMouseTracking(true);
    setupTimer();
    setupButton();
    setupConnection();
}


DinamicChart::~DinamicChart()
{
    delete timer;
    delete series;
    delete axisX;
}

// устанавливает соединения сигналов и слотов для таймера, кнопок и серии графика.
void DinamicChart::setupConnection()
{
    connect(timer, &QTimer::timeout, this, &DinamicChart::updateChart);
    connect(stopAction, &QAction::triggered, this, &DinamicChart::toggleChart);
    connect(saveImageAction, &QAction::triggered, this, &DinamicChart::saveChartImage);
    connect(loadCsvAction, &QAction::triggered, this, &DinamicChart::loadChartFromCSV);
    connect(saveCsvAction, &QAction::triggered, this, &DinamicChart::saveChartToCSV);
    connect(series, &QLineSeries::hovered, this, &DinamicChart::showDataToolTip);
}

// создает и запускает таймер с интервалом 100 миллисекунд.
void DinamicChart::setupTimer()
{
    timer = new QTimer(this);

    timer->start(100);
}

// создает меню функций и кнопку "Функции" для отображения меню.
void DinamicChart::setupButton()
{
    functionsMenu = new QMenu(this);

    stopAction = functionsMenu->addAction("Stop");
    saveImageAction = functionsMenu->addAction("Save Image");
    loadCsvAction = functionsMenu->addAction("Load from CSV");
    saveCsvAction = functionsMenu->addAction("Save CSV");

    QPushButton* functionsButton = new QPushButton("Функции", this);
    functionsButton->setMenu(functionsMenu);
    functionsButton->setGeometry(0, 25, 100, 30);
}

// создает график, серию, оси и представление графика, а также настраивает начальные значения осей.
void DinamicChart::setupChart()
{
    series = new QLineSeries();
    series->setName("Line");
    QChart *chart = new QChart();
    chart->addSeries(series);

    axisX = new QDateTimeAxis();
    axisX->setTickCount(10);

    axisX->setFormat("hh:mm:ss");

    chart->addAxis(axisX, Qt::AlignBottom);

    QValueAxis *axisY = new QValueAxis();
    chart->addAxis(axisY, Qt::AlignLeft);

    series->attachAxis(axisX);
    series->attachAxis(axisY);

    chartView = new QChartView(chart);

    chartView->setAttribute(Qt::WA_TransparentForMouseEvents);

    qint64 currentTimestamp = QDateTime::currentMSecsSinceEpoch();
    qint64 range = 10000;
    qint64 minTimestamp = currentTimestamp - range;
    qint64 maxTimestamp = currentTimestamp;
    QDateTime minDateTime = QDateTime::fromMSecsSinceEpoch(minTimestamp);
    QDateTime maxDateTime = QDateTime::fromMSecsSinceEpoch(maxTimestamp);
    axisX->setRange(minDateTime, maxDateTime);
    axisY->setRange(0, 1000);

    setCentralWidget(chartView);
}


// отображает всплывающую подсказку с данными точки графика при наведении мыши на точку.
void DinamicChart::showDataToolTip(QPointF point, bool state)
{
    if (state) {
        QDateTime xDateTime = QDateTime::fromMSecsSinceEpoch(static_cast<qint64>(point.x()));
        QString tooltipText = QString("Дата: %1\nЗначение: %2").arg(xDateTime.toString("dd/MM/yyyy hh:mm:ss")).arg(point.y());

        QPoint tooltipPos = chartView->mapToGlobal(QCursor::pos());
        QToolTip::showText(tooltipPos, tooltipText, chartView);
    } else {
        QToolTip::hideText();
    }
}

// обновляет график, добавляя случайное значение в серию и выполняя анимацию для новой точки.
void DinamicChart::updateChart()
{
    qreal value = QRandomGenerator::global()->bounded(100);
    qint64 timestamp = QDateTime::currentMSecsSinceEpoch();

    if (chartRunning) {
        series->append(timestamp, value);

        if (series->count() >= 2) {
            QPointF prevPoint = series->at(series->count() - 2);
            QPointF currentPoint = series->at(series->count() - 1);

            int steps = 100;
            qreal stepSize = 1.0 / steps;

            for (int i = 0; i <= steps; ++i) {
                qreal t = i * stepSize;

                qreal x = prevPoint.x() + t * (currentPoint.x() - prevPoint.x());
                qreal y = prevPoint.y() + t * (currentPoint.y() - prevPoint.y());

                series->replace(series->count() - 1, QPointF(x, y));

                QCoreApplication::processEvents();

                if (!chartRunning) {
                    break;
                }

                QThread::msleep(1);
            }
        }
    }
    else {
        series->append(timestamp, 0);
    }
}

// приостанавливает или возобновляет обновление графика при нажатии на кнопку "Stop"/"Continue".
void DinamicChart::toggleChart()
{
    if (chartRunning)
    {
        stopAction->setText("Continue");
    }
    else
    {
        timer->start(100);
        stopAction->setText("Stop");
    }

    chartRunning = !chartRunning;
}

// останавливает обновление графика и изменяет текст кнопки на "Continue".
void DinamicChart::stopChart()
{
    timer->stop();

    stopAction->setText("Continue");

    chartRunning = false;
}

// обрабатывает событие прокрутки колеса мыши для масштабирования графика.
void DinamicChart::wheelEvent(QWheelEvent *event)
{
    if (event->angleDelta().y() > 0)
        chartView->chart()->zoomIn();
    else
        chartView->chart()->zoomOut();
}

void DinamicChart::zoomIn()
{
    qreal scaleFactor = 1.05;
    zoomChart(scaleFactor);
}

void DinamicChart::zoomOut()
{
    qreal scaleFactor = 0.95;
    zoomChart(scaleFactor);
}

void DinamicChart::zoomChart(qreal scaleFactor)
{
    QChart *chart = chartView->chart();
    QAbstractAxis *axisX = chart->axisX(series);
    QAbstractAxis *axisY = chart->axisY(series);

    qreal minX = dynamic_cast<QDateTimeAxis*>(axisX)->min().toMSecsSinceEpoch();
    qreal maxX = dynamic_cast<QDateTimeAxis*>(axisX)->max().toMSecsSinceEpoch();
    qreal minY = dynamic_cast<QValueAxis*>(axisY)->min();
    qreal maxY = dynamic_cast<QValueAxis*>(axisY)->max();

    qreal deltaX = (maxX - minX) * (scaleFactor - 1.0) / 2.0;
    qreal deltaY = (maxY - minY) * (scaleFactor - 1.0) / 2.0;

    qreal newMinX = minX - deltaX;
    qreal newMaxX = maxX + deltaX;
    qreal newMinY = minY - deltaY;
    qreal newMaxY = maxY + deltaY;

    dynamic_cast<QDateTimeAxis*>(axisX)->setMin(QDateTime::fromMSecsSinceEpoch(newMinX));
    dynamic_cast<QDateTimeAxis*>(axisX)->setMax(QDateTime::fromMSecsSinceEpoch(newMaxX));
    dynamic_cast<QValueAxis*>(axisY)->setMin(newMinY);
    dynamic_cast<QValueAxis*>(axisY)->setMax(newMaxY);

    chartView->chart()->update();
}

// обновляет диапазон оси X в соответствии с данными серии графика.
void DinamicChart::updateAxisXRange()
{
    QDateTimeAxis *axisX = qobject_cast<QDateTimeAxis*>(chartView->chart()->axisX(series));
    if (axisX) {
        qreal minX = series->at(0).x();
        qreal maxX = series->at(series->count() - 1).x();

        QDateTime minDateTime = QDateTime::fromMSecsSinceEpoch(static_cast<qint64>(minX));
        QDateTime maxDateTime = QDateTime::fromMSecsSinceEpoch(static_cast<qint64>(maxX));

        axisX->setRange(minDateTime, maxDateTime);
    }
}

// сохраняет изображение графика в картинку
void DinamicChart::saveChartImage()
{
    QString fileName = QFileDialog::getSaveFileName(this, "Save Image", "", "Images (*.png *.jpg)");
    if (!fileName.isEmpty()) {
        QImage image = chartView->grab().toImage();
        image.save(fileName);
    }
}


// сохраняет данные серии графика в файл CSV
void DinamicChart::saveChartToCSV()
{
    QString fileName = QFileDialog::getSaveFileName(this, "Save Chart", "", "CSV Files (*.csv)");
    if (!fileName.isEmpty()) {
        QFile file(fileName);
        if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream stream(&file);
            stream << "X,Y\n";

            QList<QPointF> points = series->points();
            for (const QPointF& point : points) {
                QString xStr = QString::number(point.x(), 'f', 2);
                QString yStr = QString::number(point.y(), 'f', 2);
                stream << xStr << "," << yStr << "\n";
            }
            file.close();
        }
    }
}



// загружает данные серии графика из файла CSV.
void DinamicChart::loadChartFromCSV()
{
    QString fileName = QFileDialog::getOpenFileName(this, "Load Chart", "", "CSV Files (*.csv)");
    if (!fileName.isEmpty()) {
        QFile file(fileName);
        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            series->clear();

            QTextStream stream(&file);
            QString header = stream.readLine();
            while (!stream.atEnd()) {
                QString line = stream.readLine();
                QStringList values = line.split(',');
                if (values.size() >= 2) {
                    double x = values.at(0).toDouble();
                    double y = values.at(1).toDouble();
                    series->append(x, y);
                }
            }
            file.close();

            updateAxisXRange();
            chartView->chart()->update();
        }
    }
}

// обрабатывает нажатие кнопкой мыши
void DinamicChart::mousePressEvent(QMouseEvent *event)
{

    if (event->button() == Qt::LeftButton) {
        lastMousePos = event->pos();
        isMousePressed = true;
    }
}

// Передвижение графика влево
void DinamicChart::moveChartLeft(int delta)
{
    QDateTimeAxis* axisX = dynamic_cast<QDateTimeAxis*>(chartView->chart()->axisX(series));
    if (axisX) {
        qint64 minX = axisX->min().toMSecsSinceEpoch();
        qint64 maxX = axisX->max().toMSecsSinceEpoch();

        qreal fullRange = maxX - minX;
        qreal currentRange = axisX->max().toMSecsSinceEpoch() - axisX->min().toMSecsSinceEpoch();

        qint64 newMinX = minX - (delta * currentRange / fullRange);
        qint64 newMaxX = newMinX + currentRange;

        axisX->setMin(QDateTime::fromMSecsSinceEpoch(newMinX));
        axisX->setMax(QDateTime::fromMSecsSinceEpoch(newMaxX));

        chartView->chart()->scroll(delta, 0);
    }
}

// Передвижение графика вправо
void DinamicChart::moveChartRight(int delta)
{
    QDateTimeAxis* axisX = dynamic_cast<QDateTimeAxis*>(chartView->chart()->axisX(series));
    if (axisX) {
        qint64 minX = axisX->min().toMSecsSinceEpoch();
        qint64 maxX = axisX->max().toMSecsSinceEpoch();

        qreal fullRange = maxX - minX;
        qreal currentRange = axisX->max().toMSecsSinceEpoch() - axisX->min().toMSecsSinceEpoch();

        qint64 newMaxX = maxX + (delta * currentRange / fullRange);
        qint64 newMinX = newMaxX - currentRange;

        axisX->setMin(QDateTime::fromMSecsSinceEpoch(newMinX));
        axisX->setMax(QDateTime::fromMSecsSinceEpoch(newMaxX));

        chartView->chart()->scroll(-delta, 0);
    }
}


void DinamicChart::mouseMoveEvent(QMouseEvent* event)
{
    if (isMousePressed) {
        // Вычисление изменения положения мыши относительно предыдущей позиции
        QPoint delta = event->pos() - lastMousePos;
        lastMousePos = event->pos();

        int deltaX = delta.x();
        int maxDeltaX = 10000;

        // Ограничение значения deltaX до диапазона [-maxDeltaX, maxDeltaX]
        if (deltaX < -maxDeltaX) {
            deltaX = -maxDeltaX;
        } else if (deltaX > maxDeltaX) {
            deltaX = maxDeltaX;
        }

        // Перемещение графика вправо или влево в зависимости от значения deltaX
        if (deltaX > 0) {
            moveChartRight(deltaX);
        } else if (deltaX < 0) {
            moveChartLeft(-deltaX);
        }
    } else {
        QPointF mousePoint = chartView->chart()->mapToValue(event->pos());
        // Проверка, находится ли позиция мыши внутри области графика
        bool contains = chartView->chart()->plotArea().contains(event->pos());

        qreal threshold = 5.0;

        bool isCloseToLine = false;
        for (const auto& series : chartView->chart()->series()) {
            auto lineSeries = qobject_cast<QLineSeries*>(series);
            if (lineSeries) {
                for (int i = 0; i < lineSeries->count() - 1; ++i) {
                    QPointF p1 = lineSeries->at(i);
                    QPointF p2 = lineSeries->at(i + 1);
                    // Вычисление расстояния от точки мыши до линии
                    qreal distance = pointToLineDistance(mousePoint, p1, p2);

                    // Проверка, находится ли расстояние в пределах порогового значения
                    if (distance <= threshold) {
                        isCloseToLine = true;
                        break;
                    }
                }

                if (isCloseToLine)
                    break;
            }
        }

        // Показ подсказки с данными, если позиция мыши близка к линии графика
        if (contains && isCloseToLine && mousePoint.x() >= axisX->min().toMSecsSinceEpoch() && mousePoint.x() <= axisX->max().toMSecsSinceEpoch()) {
            showDataToolTip(mousePoint, true);
        } else {
            showDataToolTip(mousePoint, false);
        }
    }
    QWidget::mouseMoveEvent(event);
}


qreal DinamicChart::pointToLineDistance(const QPointF& point, const QPointF& lineP1, const QPointF& lineP2)
{
    qreal dx = lineP2.x() - lineP1.x();
    qreal dy = lineP2.y() - lineP1.y();

    qreal len_sq = dx * dx + dy * dy;

    if (len_sq == 0.0) {
        // Линия является точкой, возвращаем расстояние до точки lineP1
        qreal dx1 = point.x() - lineP1.x();
        qreal dy1 = point.y() - lineP1.y();
        return qSqrt(dx1 * dx1 + dy1 * dy1);
    }

    qreal t = ((point.x() - lineP1.x()) * dx + (point.y() - lineP1.y()) * dy) / len_sq;

    if (t < 0.0) {
        // Ближе к lineP1
        qreal dx2 = point.x() - lineP1.x();
        qreal dy2 = point.y() - lineP1.y();
        return qSqrt(dx2 * dx2 + dy2 * dy2);
    } else if (t > 1.0) {
        // Ближе к lineP2
        qreal dx3 = point.x() - lineP2.x();
        qreal dy3 = point.y() - lineP2.y();
        return qSqrt(dx3 * dx3 + dy3 * dy3);
    }

    // Ближе к отрезку
    qreal projX = lineP1.x() + t * dx;
    qreal projY = lineP1.y() + t * dy;

    qreal dx4 = point.x() - projX;
    qreal dy4 = point.y() - projY;

    return qSqrt(dx4 * dx4 + dy4 * dy4);
}


void DinamicChart::mouseReleaseEvent(QMouseEvent *event)
{

    if (event->button() == Qt::LeftButton) {
        if (isMousePressed) {
            isMousePressed = false;
            return;
        }
    }

    QWidget::mouseReleaseEvent(event);
}

