#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "jsonhelper.h"
#include <QMessageBox>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    // 1. 初始化网络管理器
    m_weatherMgr = new WeatherManager(this);

    // 2. 连接信号
    connect(m_weatherMgr, &WeatherManager::weatherReceived, this, &MainWindow::onWeatherReceived);
    connect(m_weatherMgr, &WeatherManager::errorOccurred, this, [](QString err){
        qDebug() << "Error:" << err;
    });

    // 3. 默认查北京
    m_weatherMgr->getWeather("beijing");
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_btn_Search_clicked()
{
    QString city = ui->lineEdit_City->text().trimmed();
    if (!city.isEmpty()) {
        m_weatherMgr->getWeather(city);
    }
}

void MainWindow::onWeatherReceived(QString cityId, QByteArray data)
{
    // 1. 解析数据
    TodayWeather weather = JsonHelper::parseWeatherJson(data);

    // 2. 更新界面
    updateUI(weather);
}

void MainWindow::updateUI(const TodayWeather &weather)
{
    // 更新文本信息
    ui->lbl_City->setText(weather.city);
    ui->lbl_Temp->setText(weather.wendu + "°C");
    ui->lbl_Type->setText(weather.type);
    ui->lbl_Shidu->setText("湿度: " + weather.shidu + "%");

    // 绘制图表
    drawTempChart(weather.forecast);
}

void MainWindow::drawTempChart(const QList<DayWeather> &forecast)
{
    // 如果没有提升 QChartView，这行代码会报错，请确保在 UI 设计器里完成了提升
    // 并且 CMakeLists.txt 里包含了 Charts 模块

    QChart *chart = new QChart();
    chart->setTitle("未来几天温度趋势");

    // 创建高温曲线
    QLineSeries *highSeries = new QLineSeries();
    highSeries->setName("最高温");
    // 创建低温曲线
    QLineSeries *lowSeries = new QLineSeries();
    lowSeries->setName("最低温");

    // 填充数据
    QStringList categories;
    for (int i = 0; i < forecast.size(); ++i) {
        highSeries->append(i, forecast[i].high);
        lowSeries->append(i, forecast[i].low);
        categories << forecast[i].week; // X轴标签：星期几
    }

    chart->addSeries(highSeries);
    chart->addSeries(lowSeries);

    // 创建坐标轴
    QBarCategoryAxis *axisX = new QBarCategoryAxis();
    axisX->append(categories);
    chart->addAxis(axisX, Qt::AlignBottom);
    highSeries->attachAxis(axisX);
    lowSeries->attachAxis(axisX);

    QValueAxis *axisY = new QValueAxis();
    axisY->setRange(-10, 40); // 根据实际情况动态调整更好，这里先写死
    axisY->setLabelFormat("%d°C");
    chart->addAxis(axisY, Qt::AlignLeft);
    highSeries->attachAxis(axisY);
    lowSeries->attachAxis(axisY);

    // 显示图表
    ui->chartView->setChart(chart);
    ui->chartView->setRenderHint(QPainter::Antialiasing); // 抗锯齿，线条更平滑
}
