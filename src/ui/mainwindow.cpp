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

    // 4. 【新增】存历史数据 (遍历 forecast 列表)
    // 这里我们把未来3天的数据都拆开存进去
    for (const DayWeather &day : weather.forecast) {
        // 参数：拼音ID, 日期(2026-01-06), 最高温, 最低温
        DBManager::getInstance().insertHistoryData(cityId, day.date, day.high, day.low);
    }

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
    drawTempChart(weather.forecast,"未来气温趋势");
}

void MainWindow::drawTempChart(const QList<DayWeather> &list,QString title)
{
    if (list.isEmpty()) return;

    // 1. 创建图表对象
    QChart *chart = new QChart();
    chart->setBackgroundRoundness(0);
    chart->setBackgroundVisible(false);
    chart->setTitle("气温趋势"); // 标题可以是通用的
    chart->setTitleBrush(Qt::white);
    chart->legend()->setVisible(false);

    // 2. 创建曲线
    QLineSeries *highSeries = new QLineSeries();
    QLineSeries *lowSeries = new QLineSeries();

    // 3. 填充数据 (核心修改部分)
    QStringList categories;
    int minTemp = 100;
    int maxTemp = -100;

    for (int i = 0; i < list.size(); ++i) {
        // X 轴的位置就是 i (0, 1, 2...)，不管实际日期差几天，它们在图上都是相邻的
        highSeries->append(i, list[i].high);
        lowSeries->append(i, list[i].low);

        // 【修改点】 X 轴标签只显示日期 (MM/dd)
        // 解析数据库里的 "2026-01-06"
        QDate date = QDate::fromString(list[i].date, "yyyy-MM-dd");

        // 格式化为 "01/06" (去掉星期几)
        categories << date.toString("MM/dd");

        // 计算极值用于缩放 Y 轴
        if (list[i].low < minTemp) minTemp = list[i].low;
        if (list[i].high > maxTemp) maxTemp = list[i].high;
    }

    chart->addSeries(highSeries);
    chart->addSeries(lowSeries);

    // 【修改点】使用传入的标题
    chart->setTitle(title);
    chart->setTitleBrush(Qt::white);

    // 4. 设置线条样式 (保持不变)
    QPen highPen(Qt::red); highPen.setWidth(3); highSeries->setPen(highPen);
    highSeries->setPointLabelsVisible(true); highSeries->setPointLabelsFormat("@yPoint°"); highSeries->setPointLabelsColor(Qt::white);

    QPen lowPen(Qt::blue); lowPen.setWidth(3); lowSeries->setPen(lowPen);
    lowSeries->setPointLabelsVisible(true); lowSeries->setPointLabelsFormat("@yPoint°"); lowSeries->setPointLabelsColor(Qt::white);

    // 5. 设置坐标轴
    // X 轴：使用 QBarCategoryAxis，它会按顺序显示我们刚才塞进去的 categories
    QBarCategoryAxis *axisX = new QBarCategoryAxis();
    axisX->append(categories);
    axisX->setLabelsColor(Qt::white);
    axisX->setGridLineVisible(false);
    chart->addAxis(axisX, Qt::AlignBottom);
    highSeries->attachAxis(axisX);
    lowSeries->attachAxis(axisX);

    // Y 轴 (保持不变)
    QValueAxis *axisY = new QValueAxis();
    axisY->setRange(minTemp - 3, maxTemp + 3);
    axisY->setLabelFormat(QString("%d%1C").arg(QChar(0x00B0)));
    axisY->setLabelsColor(Qt::white);
    axisY->setGridLineVisible(true);
    chart->addAxis(axisY, Qt::AlignLeft);
    highSeries->attachAxis(axisY);
    lowSeries->attachAxis(axisY);

    // 6. 显示
    ui->chartView->setChart(chart);
    ui->chartView->setRenderHint(QPainter::Antialiasing);
    ui->chartView->setStyleSheet("background: transparent");
}

void MainWindow::on_btn_History_clicked()
{
    // 1. 获取输入框的城市 ID
    QString cityId = ui->lineEdit_City->text().trimmed();
    if (cityId.isEmpty()) cityId = "beijing"; // 默认

    // 2. 查历史数据
    QList<DayWeather> historyList = DBManager::getInstance().getHistoryData(cityId);

    if (historyList.isEmpty()) {
        QMessageBox::information(this, "提示", "没有找到 " + cityId + " 的历史数据。");
        return;
    }

    // 3. 【核心修复】更新上方的文字标签
    // 因为是看历史，"实时温度"和"湿度"已经没有意义了，容易产生误解，建议清空或改名

    ui->lbl_City->setText(cityId); // 显示当前查询的城市拼音
    ui->lbl_Temp->setText("历史");  // 把温度改成文字，提示模式变化
    ui->lbl_Type->setText("数据回顾"); // 把天气改成描述
    ui->lbl_Shidu->clear();        // 历史数据通常不记录实时湿度，直接清空

    // 4. 画图
    drawTempChart(historyList, cityId + " - 历史气温积累");
}

