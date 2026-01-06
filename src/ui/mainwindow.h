#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QtCharts> // 引入图表库
#include "weathermanager.h"
#include "weatherdata.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    // 搜索按钮点击
    void on_btn_Search_clicked();

    // 接收到天气数据的槽函数
    void onWeatherReceived(QString cityId, QByteArray data);
    void on_btn_History_clicked();


    void switchTheme();

private:
    Ui::MainWindow *ui;
    WeatherManager *m_weatherMgr;
    bool m_isNight; // 记录当前是否是夜间模式
    void updateStyle(); // 切换样式的辅助函数

    // 【核心】更新 UI 显示
    void updateUI(const TodayWeather &weather);

    // 【核心】绘制温度折线图
    void drawTempChart(const QList<DayWeather> &list,QString title = "气温趋势");
};
#endif // MAINWINDOW_H
