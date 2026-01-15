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

    // 【核心修复 1】: 必须最先初始化数据库！
    // 只有这一步成功了，后面的 initModel 拿到的才是活的连接
    if (!DBManager::getInstance().initDB()) {
        QMessageBox::critical(this, "错误", "数据库初始化失败，程序无法正常运行！");
    }

    // 【核心修复 2】: 确保这行代码在 initDB 之后
    initModel();

    // 1. 初始化网络管理器
    m_weatherMgr = new WeatherManager(this);

    // 1. 初始化变量 (确保默认为 false)
    m_isNight = false;
    // 2. 连接信号
    connect(m_weatherMgr, &WeatherManager::weatherReceived, this, &MainWindow::onWeatherReceived);
    // 【新增】手动连接，确保只连一次
    connect(ui->btn_Theme, &QPushButton::clicked, this, &MainWindow::switchTheme);
    connect(m_weatherMgr, &WeatherManager::errorOccurred, this, [](QString err){
        qDebug() << "Error:" << err;
    });
    // 3. 默认查北京
    m_weatherMgr->getWeather("beijing");
    // 【新增】程序启动时，加载默认主题
    updateStyle();



}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_btn_Search_clicked()
{
    QString cityId = ui->lineEdit_City->text().trimmed();
    if (cityId.isEmpty()) cityId = "beijing"; // 默认

    QByteArray cachedData = DBManager::getInstance().getWeatherCache(cityId);

    if (!cachedData.isEmpty()) {
        qDebug() << "Hit Cache (命中缓存) ->" << cityId;

        TodayWeather weather = JsonHelper::parseWeatherJson(cachedData);
        updateUI(weather);

        // --- 【新增代码：补全历史数据】 ---
        // 即使是缓存的数据，也要尝试存入历史表，保证历史表里有数据
        for (const DayWeather &day : weather.forecast) {
            DBManager::getInstance().insertHistoryData(cityId, day.date, day.high, day.low);
        }
        // -------------------------------
    }
    else {
        // ... 无缓存逻辑保持不变 ...
        qDebug() << "无缓存或已过期，发起网络请求 -> " << cityId;
        m_weatherMgr->getWeather(cityId);
    }

    // --- 刷新表格 ---
    m_model->setFilter(QString("city_id = '%1'").arg(cityId));
    m_model->select();
}

void MainWindow::onWeatherReceived(QString cityId, QByteArray data)
{
    // 1. 解析 & 2. 更新界面 & 3. 存缓存 (原代码不变)
    TodayWeather weather = JsonHelper::parseWeatherJson(data);
    updateUI(weather);
    DBManager::getInstance().cacheWeather(cityId, weather.city, data);

    // 4. 存历史数据 (原代码不变)
    for (const DayWeather &day : weather.forecast) {
        DBManager::getInstance().insertHistoryData(cityId, day.date, day.high, day.low);
    }

    // --- 【新增代码：核心修复】 ---
    // 数据存进去了，现在通知表格刷新显示！
    // 1. 确保过滤器是对的 (只显示当前城市)
    m_model->setFilter(QString("city_id = '%1'").arg(cityId));
    // 2. 重新从数据库载入
    m_model->select();

    qDebug() << "历史表已刷新，行数：" << m_model->rowCount();
}

void MainWindow::updateUI(const TodayWeather &weather)
{
    ui->lbl_Temp->setStyleSheet("font-size: 72px;"); // 恢复大字体显示温度
    // 更新文本信息
    ui->lbl_City->setText(weather.city);
    ui->lbl_Temp->setText(weather.wendu + "°C");
    ui->lbl_Type->setText(weather.type);
    ui->lbl_Shidu->setText("湿度: " + weather.shidu + "%");

    // 绘制图表
    drawTempChart(weather.forecast,"未来气温趋势");
}

void MainWindow::drawTempChart(const QList<DayWeather> &list, QString title)
{
    if (list.isEmpty()) return;

    // 【核心修改 1】根据日夜模式决定文字颜色
    // 夜间(true) -> 白色； 日间(false) -> 黑色
     QColor textColor = m_isNight ? Qt::white : Qt::black;
    // 网格线颜色也微调一下，日间深一点，夜间淡一点
    QColor gridColor = m_isNight ? QColor(255, 255, 255, 40) : QColor(0, 0, 0, 40);

    QChart *chart = new QChart();
    chart->setBackgroundRoundness(0);
    chart->setBackgroundVisible(false);
    chart->setTitle(title);

    // 【修改】标题颜色不再写死 Qt::white
    chart->setTitleBrush(textColor);
    chart->legend()->setVisible(false);

    QLineSeries *highSeries = new QLineSeries();
    QLineSeries *lowSeries = new QLineSeries();

    QStringList categories;
    int minTemp = 100;
    int maxTemp = -100;

    for (int i = 0; i < list.size(); ++i) {
        highSeries->append(i, list[i].high);
        lowSeries->append(i, list[i].low);

        QDate date = QDate::fromString(list[i].date, "yyyy-MM-dd");
        categories << date.toString("MM/dd");

        if (list[i].low < minTemp) minTemp = list[i].low;
        if (list[i].high > maxTemp) maxTemp = list[i].high;
    }

    chart->addSeries(highSeries);
    chart->addSeries(lowSeries);

    // --- 线条与数值 ---
    QPen highPen(Qt::red); highPen.setWidth(3); highSeries->setPen(highPen);
    highSeries->setPointLabelsVisible(true);
    highSeries->setPointLabelsFormat("@yPoint°");
    highSeries->setPointLabelsColor(textColor); // 【修改】跟随主题

    QPen lowPen(Qt::blue); lowPen.setWidth(3); lowSeries->setPen(lowPen);
    lowSeries->setPointLabelsVisible(true);
    lowSeries->setPointLabelsFormat("@yPoint°");
    lowSeries->setPointLabelsColor(textColor); // 【修改】跟随主题

    // --- X 轴 ---
    QBarCategoryAxis *axisX = new QBarCategoryAxis();
    axisX->append(categories);
    axisX->setLabelsColor(textColor); // 【修改】跟随主题
    axisX->setGridLineVisible(false);
    axisX->setLinePen(QPen(textColor)); // 轴线颜色
    chart->addAxis(axisX, Qt::AlignBottom);
    highSeries->attachAxis(axisX);
    lowSeries->attachAxis(axisX);

    // --- Y 轴 ---
    QValueAxis *axisY = new QValueAxis();
    axisY->setRange(minTemp - 3, maxTemp + 3);
    // 使用标准 Unicode 防止乱码
    axisY->setLabelFormat("%d C");

    axisY->setLabelsColor(textColor); // 【修改】跟随主题

    // 网格线设为虚线
    QPen gridPen(gridColor);
    gridPen.setStyle(Qt::DashLine);
    axisY->setGridLinePen(gridPen);

    chart->addAxis(axisY, Qt::AlignLeft);
    highSeries->attachAxis(axisY);
    lowSeries->attachAxis(axisY);

    // 显示
    ui->chartView->setChart(chart);
    ui->chartView->setRenderHint(QPainter::Antialiasing);
    ui->chartView->setStyleSheet("background: transparent");
}

void MainWindow::on_btn_History_clicked()
{
    // 1. 获取输入框的城市 ID
    QString cityId = ui->lineEdit_City->text().trimmed();
    if (cityId.isEmpty()) cityId = "beijing"; // 默认

    // 2. 【修改】获取中文名 (如果数据库有缓存，就能显示中文，否则显示拼音)
    QString cityName = DBManager::getInstance().getCityName(cityId);

    // 3. 【修改】调用 getRecentHistory (只查过去6天)
    QList<DayWeather> historyList = DBManager::getInstance().getRecentHistory(cityId);

    // 4. 检查是否有数据
    if (historyList.isEmpty()) {
        QMessageBox::information(this, "暂无历史",
                                 QString("[%1] 暂无历史数据。\n\n"
                                         "原因：系统设计为仅记录昨日及以前的数据。\n"
                                         "请明天再来查看趋势，或查询其他已有数据的城市。").arg(cityName));
        return;
    }

    // 5. 【UI美化】计算时间跨度
    QString startDate = historyList.first().date.mid(5); // "01-06"
    QString endDate = historyList.last().date.mid(5);    // "01-12"

    // 6. 更新界面标签
    ui->lbl_City->setText(cityName); // 显示中文名

    // 调整大字显示
    ui->lbl_Temp->setStyleSheet("font-size: 40px;");
    ui->lbl_Temp->setText("历史回顾");

    ui->lbl_Type->setText(QString("%1 ~ %2").arg(startDate, endDate));
    ui->lbl_Shidu->setText(QString("近 %1 天走势").arg(historyList.count()));

    // 7. 切换 Tab 并画图
    ui->tabWidget->setCurrentIndex(0); // 确保在图表页

    // 这里的标题也加上中文名
    drawTempChart(historyList, cityName + " - 历史气温回顾");
}


void MainWindow::switchTheme()
{
    m_isNight = !m_isNight;
    updateStyle();

    // 【新增】强制刷新图表颜色
    // 逻辑：如果输入框里有城市，就假装用户又点了一次搜索，或者手动重画
    QString cityId = ui->lineEdit_City->text();
    if (!cityId.isEmpty()) {
        // 重新调用搜索逻辑，这会触发 drawTempChart，使用新的 m_isNight 颜色
        on_btn_Search_clicked();
    }
}

// 核心换肤函数
void MainWindow::updateStyle()
{
    // 【修改点】严格按照你截图中的路径来写 (双斜杠)
    QString qssPath = m_isNight ? "://resources/styles/style_night.qss"
                                : "://resources/styles/style_day.qss";

    QFile file(qssPath);
    if (file.open(QFile::ReadOnly)) {
        QString styleSheet = QLatin1String(file.readAll());
        qApp->setStyleSheet(styleSheet);
        file.close();
        qDebug() << "成功切换主题:" << qssPath;
    } else {
        // 如果这里打印了，说明路径还是不对，或者文件没被编译进去
        qDebug() << "依然找不到文件:" << qssPath;
    }
}

void MainWindow::initModel()
{
    // 获取 DBManager 里的连接名
    QSqlDatabase db = DBManager::getInstance().getDatabase();
    qDebug() << "Model 使用的数据库连接名:" << db.connectionName();
    qDebug() << "数据库是否打开:" << db.isOpen();
    qDebug() << "数据库路径:" << db.databaseName();
    // 重新实例化 Model，确保用的是同一个 db
    m_model = new QSqlTableModel(this, db);
    m_model->setTable("WeatherHistory");

    // 2. 设置列名别名 (Model)
    m_model->setHeaderData(m_model->fieldIndex("city_id"), Qt::Horizontal, "城市ID");
    m_model->setHeaderData(m_model->fieldIndex("date"), Qt::Horizontal, "日期");
    m_model->setHeaderData(m_model->fieldIndex("high"), Qt::Horizontal, "最高温");
    m_model->setHeaderData(m_model->fieldIndex("low"), Qt::Horizontal, "最低温");

    // 3. 绑定到视图 (View)
    ui->tableView_History->setModel(m_model);

    // --- 【新增美化代码】 ---

    // 1. 让列宽自动铺满整个表格 (平均分配宽度)
    // 这一步最关键！加上后就不会左边挤一坨，右边空一大块了
    ui->tableView_History->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);

    // 2. 隐藏最左边的行号 (1, 2, 3, 4...)
    // 也就是垂直表头，通常不需要显示，隐藏后更清爽
    ui->tableView_History->verticalHeader()->setVisible(false);

    // 3. 启用隔行变色 (斑马纹)，提升阅读体验
    ui->tableView_History->setAlternatingRowColors(true);

    // 4. 设置行高 (稍微高一点，不那么挤)
    ui->tableView_History->verticalHeader()->setDefaultSectionSize(40);


    // 4. 视觉优化
    ui->tableView_History->setEditTriggers(QAbstractItemView::NoEditTriggers); // 禁止编辑
    ui->tableView_History->setSelectionBehavior(QAbstractItemView::SelectRows); // 选中整行
    ui->tableView_History->setAlternatingRowColors(true); // 隔行变色

    // 5. 加载数据
    m_model->select();
}

