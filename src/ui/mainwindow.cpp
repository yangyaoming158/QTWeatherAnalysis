#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "jsonhelper.h"
#include <QMessageBox>
#include "dbmanager.h"
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
    QString cityId = ui->lineEdit_City->text().trimmed();
    if (cityId.isEmpty()) return;

    // 1. 先尝试从数据库获取缓存
    // 注意：cityId 在这里就是用户输入的拼音，如 "beijing"
    QByteArray cachedData = DBManager::getInstance().getWeatherCache(cityId);

    if (!cachedData.isEmpty()) {
        qDebug() << "命中缓存，使用数据库数据 -> " << cityId;

        // 2. 如果有缓存且没过期，直接解析显示
        TodayWeather weather = JsonHelper::parseWeatherJson(cachedData);
        updateUI(weather);
    }
    else {
        qDebug() << "无缓存或已过期，发起网络请求 -> " << cityId;

        // 3. 如果没缓存，发起网络请求
        m_weatherMgr->getWeather(cityId);
    }
}

void MainWindow::onWeatherReceived(QString cityId, QByteArray data)
{
    // 1. 解析数据
    TodayWeather weather = JsonHelper::parseWeatherJson(data);

    // 2. 更新界面
    updateUI(weather);

    // 3. 【新增】存入数据库缓存
    // 这里的 weather.city 是中文名（如“北京”），cityId 是拼音（如“beijing”）
    DBManager::getInstance().cacheWeather(cityId, weather.city, data);

    qDebug() << "数据已更新并缓存数据库";
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
    // 如果没有数据，直接返回，防止崩溃
    if (forecast.isEmpty()) return;

    QChart *chart = new QChart();

    // --- 1. 背景透明化 ---
    chart->setBackgroundRoundness(0);
    chart->setBackgroundVisible(false); // 隐藏图表背景，透出窗口背景

    // --- 2. 设置标题 ---
    chart->setTitle("未来气温趋势");
    chart->setTitleBrush(Qt::white); // 标题白色
    chart->legend()->setVisible(false); // 隐藏图例

    // --- 3. 创建曲线 ---
    QLineSeries *highSeries = new QLineSeries();
    QLineSeries *lowSeries = new QLineSeries();

    // --- 4. 填充数据并计算极值 ---
    int minTemp = 100; // 初始设一个很大的数
    int maxTemp = -100; // 初始设一个很小的数
    QStringList categories;

    for (int i = 0; i < forecast.size(); ++i) {
        highSeries->append(i, forecast[i].high);
        lowSeries->append(i, forecast[i].low);
        categories << forecast[i].week; // X轴标签

        // 动态计算最大最小值，用于自动缩放Y轴
        if (forecast[i].low < minTemp) minTemp = forecast[i].low;
        if (forecast[i].high > maxTemp) maxTemp = forecast[i].high;
    }

    chart->addSeries(highSeries);
    chart->addSeries(lowSeries);

    // --- 5. 线条美化与数值显示 ---
    QPen highPen(Qt::red);
    highPen.setWidth(3);
    highSeries->setPen(highPen);
    highSeries->setPointLabelsVisible(true);        // 显示数值
    highSeries->setPointLabelsFormat("@yPoint°");   // 格式：数值°
    highSeries->setPointLabelsColor(Qt::white);     // 白色字

    QPen lowPen(Qt::blue);
    lowPen.setWidth(3);
    lowSeries->setPen(lowPen);
    lowSeries->setPointLabelsVisible(true);
    lowSeries->setPointLabelsFormat("@yPoint°");
    lowSeries->setPointLabelsColor(Qt::white);

    // --- 6. 坐标轴设置 ---
    QBarCategoryAxis *axisX = new QBarCategoryAxis();
    axisX->append(categories);
    axisX->setLabelsColor(Qt::white);
    axisX->setGridLineVisible(false); // 隐藏竖向网格线，更清爽
    chart->addAxis(axisX, Qt::AlignBottom);
    highSeries->attachAxis(axisX);
    lowSeries->attachAxis(axisX);

    QValueAxis *axisY = new QValueAxis();
    // 动态设置范围：上下各留 3 度缓冲空间
    axisY->setRange(minTemp - 3, maxTemp + 3);
    // 修复乱码：使用 Unicode 编码显示 °C
    axisY->setLabelFormat(QString("%d%1C").arg(QChar(0x00B0)));
    axisY->setLabelsColor(Qt::white);
    axisY->setGridLineVisible(true);
    chart->addAxis(axisY, Qt::AlignLeft);
    highSeries->attachAxis(axisY);
    lowSeries->attachAxis(axisY);

    // --- 7. 显示 ---
    ui->chartView->setChart(chart);
    ui->chartView->setRenderHint(QPainter::Antialiasing); // 抗锯齿
    ui->chartView->setStyleSheet("background: transparent"); // View 本身透明
}

