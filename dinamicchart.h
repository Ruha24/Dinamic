#ifndef DINAMICCHART_H
#define DINAMICCHART_H

#include <QMainWindow>
#include <QtWidgets>
#include <QtCharts>
#include <QChartView>
#include <QFileDialog>

class DinamicChart : public QMainWindow
{
    Q_OBJECT

public:
    explicit DinamicChart(QWidget *parent = nullptr);
    ~DinamicChart();


protected:
    void wheelEvent(QWheelEvent *event) override;


    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;


private slots:
    void stopChart();
    void toggleChart();
    void saveChartImage();
    void loadChartFromCSV();
    void saveChartToCSV();

private:

    void zoomChart(qreal scaleFactor);
    void moveChartLeft(int delta);
    void moveChartRight(int delta);
    void setupChart();
    void setupConnection();
    void setupButton();
    void updateChart();
    void zoomIn();
    void zoomOut();
    void updateAxisXRange();
    void showDataToolTip(QPointF point, bool state);

    QTimer* timer;
    QChartView* chartView;
    QLineSeries* series;
    QDateTimeAxis* axisX;

    QMenu* functionsMenu;
    QAction* stopAction;
    QAction* saveImageAction;
    QAction* loadCsvAction;
    QAction* saveCsvAction;
    bool chartRunning;

    QPoint lastMousePos;
    bool isMousePressed = false;
};
#endif // DINAMICCHART_H
