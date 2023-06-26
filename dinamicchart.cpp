#include "dinamicchart.h"

DinamicChart::DinamicChart(QWidget *parent)
    : QMainWindow(parent)
{
    setupChart();

    timer = new QTimer(this);

    timer->start(100);

    chartView->chart()->axisY(series)->setRange(0, 100);
    chartRunning = true;
    setMouseTracking(true);

    setupButton();
    setupConnection();
}

DinamicChart::~DinamicChart()
{
    delete timer;
    delete series;
    delete axisX;
}

void DinamicChart::setupConnection()
{
    connect(timer, &QTimer::timeout, this, &DinamicChart::updateChart);
    connect(stopAction, &QAction::triggered, this, &DinamicChart::toggleChart);
    connect(saveImageAction, &QAction::triggered, this, &DinamicChart::saveChartImage);
    connect(loadCsvAction, &QAction::triggered, this, &DinamicChart::loadChartFromCSV);
    connect(saveCsvAction, &QAction::triggered, this, &DinamicChart::saveChartToCSV);
    connect(series, &QLineSeries::hovered, this, &DinamicChart::showDataToolTip);
}

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

void DinamicChart::setupChart()
{
    series = new QLineSeries();

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
    // chartView->setAttribute(Qt::WA_AlwaysShowToolTips);

    chartView->setAttribute(Qt::WA_TransparentForMouseEvents);

    setCentralWidget(chartView);
}

void DinamicChart::showDataToolTip(QPointF point, bool state)
{
    if (state) {
        QDateTime xDateTime = QDateTime::fromMSecsSinceEpoch(static_cast<qint64>(point.x()));
        QString tooltipText = QString("Дата: %1\nЗначение: %2").arg(xDateTime.toString("dd/MM/yyyy hh:mm:ss")).arg(point.y());
                                  QToolTip::showText(chartView->mapToGlobal(QCursor::pos()), tooltipText, chartView);
    } else {
        QToolTip::hideText();
    }
}

void DinamicChart::updateChart()
{
    qreal value = QRandomGenerator::global()->bounded(1000);
    qint64 timestamp = QDateTime::currentMSecsSinceEpoch();

    if (chartRunning) {
        series->append(timestamp, value);
        qint64 range = 10000;
        qint64 minTimestamp = timestamp - range;
        qint64 maxTimestamp = timestamp;
        QDateTime minDateTime = QDateTime::fromMSecsSinceEpoch(minTimestamp);
        QDateTime maxDateTime = QDateTime::fromMSecsSinceEpoch(maxTimestamp);
        chartView->chart()->axisX(series)->setMin(minDateTime);
        chartView->chart()->axisX(series)->setMax(maxDateTime);

        qreal minY = std::numeric_limits<qreal>::max();
        qreal maxY = std::numeric_limits<qreal>::min();
        for (int i = 0; i < series->count(); ++i) {
            QPointF point = series->at(i);
            if (point.y() < minY) {
                minY = point.y();
            }
            if (point.y() > maxY) {
                maxY = point.y();
            }
        }
        qreal padding = 0.1 * (maxY - minY);
        chartView->chart()->axisY(series)->setMin(minY - padding);
        chartView->chart()->axisY(series)->setMax(maxY + padding);

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

                QThread::msleep(1);
            }
        }
    }
    else {
        series->append(timestamp, 0);
    }

    chartView->chart()->update();
}


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


void DinamicChart::stopChart()
{
    timer->stop();

    stopAction->setText("Continue");

    chartRunning = false;
}

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


void DinamicChart::saveChartImage()
{
    QString fileName = QFileDialog::getSaveFileName(this, "Save Image", "", "Images (*.png *.jpg)");
    if (!fileName.isEmpty()) {
        QImage image = chartView->grab().toImage();
        image.save(fileName);
    }
}


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



void DinamicChart::mousePressEvent(QMouseEvent *event)
{

    if (event->button() == Qt::LeftButton) {
        lastMousePos = event->pos();
        isMousePressed = true;
    }
}
void DinamicChart::moveChartLeft(int delta)
{
    QDateTimeAxis* axisX = dynamic_cast<QDateTimeAxis*>(chartView->chart()->axisX(series));
    if (axisX) {
        qint64 minX = axisX->min().toMSecsSinceEpoch();
        qint64 maxX = axisX->max().toMSecsSinceEpoch();
        qreal dampingFactor = 0.05;

        qint64 newMinX = minX - dampingFactor * delta;
        qint64 newMaxX = maxX - dampingFactor * delta;

        axisX->setMin(QDateTime::fromMSecsSinceEpoch(newMinX));
        axisX->setMax(QDateTime::fromMSecsSinceEpoch(newMaxX));

        chartView->chart()->scroll(delta, 0);
        chartView->chart()->update();
    }
}

void DinamicChart::moveChartRight(int delta)
{
    QDateTimeAxis* axisX = dynamic_cast<QDateTimeAxis*>(chartView->chart()->axisX(series));
    if (axisX) {
        qint64 minX = axisX->min().toMSecsSinceEpoch();
        qint64 maxX = axisX->max().toMSecsSinceEpoch();
        qreal dampingFactor = 0.05;

        qint64 newMinX = minX + dampingFactor * delta;
        qint64 newMaxX = maxX + dampingFactor * delta;

        axisX->setMin(QDateTime::fromMSecsSinceEpoch(newMinX));
        axisX->setMax(QDateTime::fromMSecsSinceEpoch(newMaxX));

        chartView->chart()->scroll(-delta, 0);
        chartView->chart()->update();
    }
}

void DinamicChart::mouseMoveEvent(QMouseEvent* event)
{
    if (isMousePressed) {
        QPoint delta = event->pos() - lastMousePos;
        lastMousePos = event->pos();

        int deltaX = delta.x();
        int maxDeltaX = 10000;

        if (deltaX < -maxDeltaX) {
            deltaX = -maxDeltaX;
        } else if (deltaX > maxDeltaX) {
            deltaX = maxDeltaX;
        }

        if (deltaX > 0) {
            moveChartRight(deltaX);
        } else if (deltaX < 0) {
            moveChartLeft(-deltaX);
        }
    } else {
        QWidget::mouseMoveEvent(event);
    }
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

