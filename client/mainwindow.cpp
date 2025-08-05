#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QFileInfo>
#include <QDateTime>
#include <QDir>
#include <QFileDialog>
#include <QInputDialog>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow), m_serverAddress("127.0.0.1"), m_serverPort(8899), m_sock(new QTcpSocket),m_downloadProgress(nullptr), // 初始化进度条指针
    m_currentDownloadSize(0)
{
    ui->setupUi(this);

    // 无边框
    setWindowFlags(Qt::FramelessWindowHint);
    setAttribute(Qt::WA_TranslucentBackground);
    setWindowIcon(QIcon("./res/up.png"));

    // 连接服务器
    m_sock->connectToHost(m_serverAddress, m_serverPort);

    // 连接错误
    connect(m_sock, SIGNAL(error(QAbstractSocket::SocketError)), this, SLOT(Error(QAbstractSocket::SocketError)));

    // 接收消息
    connect(m_sock, &QTcpSocket::readyRead, this, [=]()
    {
        if(m_sock->bytesAvailable() <= 0)
            return;
        if (sizeof(Msg) != m_sock->bytesAvailable())
            OnFile();
        else
            OnMsg();
    });

}


MainWindow::~MainWindow()
{
    delete m_sock;
    delete ui;
}


#define LIST     ui->stackedWidget->setCurrentIndex(1);\
        Msg msg("LIST");\
        msg.setUser(m_user);\
        SendMsg(msg);

// 初始化主界面
void MainWindow::InitMain()
{
    LIST;

    connect(ui->btn_head, &QPushButton::clicked, this, [=](){ ui->stackedWidget->setCurrentWidget(ui->page_me); });

    // 1.初始化导航
    connect(ui->btn_up_bar, &QPushButton::clicked, this, [=](){ ui->stackedWidget->setCurrentIndex(0); });

    connect(ui->btn_down_bar, &QPushButton::clicked, this, [=]()
    {
        LIST;
    });

    connect(ui->btn_share_bar, &QPushButton::clicked, this, [=](){ ui->stackedWidget->setCurrentIndex(2); });

    connect(ui->btn_record_bar, &QPushButton::clicked, this, [=]()
    {
        ui->stackedWidget->setCurrentIndex(3);
        Msg msg("LOG");
        msg.setUser(m_user);
        SendMsg(msg);
    });

    connect(ui->btn_bin_bar, &QPushButton::clicked, this, [=]()
    {
        ui->stackedWidget->setCurrentIndex(4);
        Msg msg("RECYCLE");
        msg.setUser(m_user);
        SendMsg(msg);
    });

    connect(ui->btn_set_bar, &QPushButton::clicked, this, [=]()
    {
        ui->stackedWidget->setCurrentIndex(5);
    });

    connect(ui->btn_quit_bar, &QPushButton::clicked, this, [=](){ exit(0); });

    connect(ui->btn_min, &QPushButton::clicked, this, [=](){ showMinimized(); });
    connect(ui->btn_close, &QPushButton::clicked, this, [=](){ exit(0); });

    // 设置本地目录
    connect(ui->btn_setdir, &QPushButton::clicked, this, [=]()
    {
        // 打开目录对话框
        QString directory = QFileDialog::getExistingDirectory(this, "选择目录", QDir::homePath());

        // 如果用户选择了目录，则输出选择的目录路径
        if (!directory.isEmpty())
        {
            ui->ed_dir->setText(directory);
            m_set->setValue("dir", directory);
            m_set->sync();
        }
    });


    // 2.初始化空间列表
    // 右键菜单
    QMenu *menuList = new QMenu(this);
    QAction *actShare = menuList->addAction("分享");
    QAction *actDown = menuList->addAction("下载");
    QAction *actDel = menuList->addAction("删除");

    connect(actShare, &QAction::triggered, this, [=]()
    {
        Msg msg("SHARE");
        msg.setUser(m_user);
        msg.setContent(ui->lw_down->currentItem()->text());
        SendMsg(msg);
    });

    connect(actDown, &QAction::triggered, this, [=]()
    {
        QStringList list;
        for(auto item : ui->lw_down->selectedItems())
            list.append(item->text());

        // 依次下载多个项目
        DownFile(list);
    });

    connect(actDel, &QAction::triggered, this, [=]()
    {
        QStringList list;
        for(auto item : ui->lw_down->selectedItems())
        {
            list.append(item->text());
            delete ui->lw_down->takeItem(ui->lw_down->row(item));
        }

        // 删除多个项目
        DelFile(list);
    });

    // 设置选择模式为多选
    ui->lw_down->setSelectionMode(QAbstractItemView::MultiSelection);
    ui->lw_down->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->lw_down, &QListWidget::customContextMenuRequested, this, [=](const QPoint &pos)
    {
        Q_UNUSED(pos);
        QListWidget *w = qobject_cast<QListWidget*>(sender());
        if(w->currentItem())
        {
            // 弹出式菜单
            menuList->exec(QCursor::pos());
        }
    });


    // 3.初始化回收站
    // 右键菜单
    QMenu *menuRecycle = new QMenu(this);
    QAction *actRecover = menuRecycle->addAction("恢复");
    QAction *actForceDel = menuRecycle->addAction("删除");

    connect(actRecover, &QAction::triggered, this, [=]()
    {
        QStringList list;
        for(auto item : ui->lw_recycle->selectedItems())
        {
            list.append(item->text());
            delete ui->lw_recycle->takeItem(ui->lw_recycle->row(item));
        }

        // 依次恢复多个项目
        RecoverFile(list);
    });

    connect(actForceDel, &QAction::triggered, this, [=]()
    {
        QStringList list;
        for(auto item : ui->lw_recycle->selectedItems())
        {
            list.append(item->text());
            delete ui->lw_recycle->takeItem(ui->lw_recycle->row(item));
        }

        // 删除多个项目
        DelFile(list, true);
    });

    // 设置选择模式为多选
    ui->lw_recycle->setSelectionMode(QAbstractItemView::MultiSelection);
    ui->lw_recycle->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->lw_recycle, &QListWidget::customContextMenuRequested, this, [=](const QPoint &pos)
    {
        Q_UNUSED(pos);
        QListWidget *w = qobject_cast<QListWidget*>(sender());
        if(w->currentItem())
        {
            // 弹出式菜单
            menuRecycle->exec(QCursor::pos());
        }
    });

    // 4.初始化操作列表
    ui->tw_record->setColumnCount(3);
    ui->tw_record->setHorizontalHeaderLabels({"文件", "操作", "日期"});
    ui->tw_record->setColumnWidth(0, 328);
    ui->tw_record->setColumnWidth(1, 100);
    ui->tw_record->setColumnWidth(2, 200);
    ui->tw_record->verticalHeader()->hide();

    // 5.配置设置
    m_set = new QSettings("./config.ini", QSettings::IniFormat);
    ui->ed_dir->setReadOnly(true);

    QFile file("config.ini");
    if(!file.exists())
    {
        // 创建本地目录
        QDir dir("./data");
        m_set->setValue("dir", "./data");
        m_set->sync();
    }
    file.close();

    // 取值
    QString dataPath = m_set->value("dir").toString();
    ui->ed_dir->setText(dataPath);
    QDir dir(dataPath);
    if(!dir.exists(dataPath))
    {
        if(!dir.mkdir(dataPath))
        {
            qWarning() << "创建目录 " << dataPath << " 失败 ";
        }
    }
}

// 发送消息
void MainWindow::SendMsg(const Msg &msg)
{
    m_sock->write((char*)&msg, sizeof(msg));
}

// 下载文件
void MainWindow::DownFile(QStringList fileList)
{
    foreach (const QString &fileName, fileList) {
        // 发送下载请求
        Msg msg("DOWN");
        msg.setUser(m_user);
        msg.setContent(fileName);
        SendMsg(msg);

        // 等待文件接收完成（根据实际协议调整）
        while (m_recBytes < m_fileSize && !m_downloadProgress->wasCanceled()) {
            QApplication::processEvents();
        }
    }
}


// 删除文件
void MainWindow::DelFile(QStringList fileList, bool erase)
{
    if (fileList.isEmpty()) {
        ERR("请先选择要删除的文件");
        return;
    }

    // 根据操作类型生成提示信息
    QString title, message, confirmButtonText;
    QMessageBox::Icon icon = QMessageBox::Question;

    if (erase) {
        title = "永久删除确认";
        message = QString("您确定要永久删除以下 %1 个文件吗？\n\n"
                          "⚠️ 此操作不可撤销，文件将被彻底清除！\n\n"
                          "删除文件列表：\n%2")
                      .arg(fileList.size())
                      .arg(fileList.join("\n"));
        confirmButtonText = "永久删除";
        icon = QMessageBox::Warning;
    } else {
        title = "移动到回收站";
        message = QString("您确定要将以下 %1 个文件移动到回收站吗？\n\n"
                          "🗑️ 文件将在回收站保留15天，到期后自动清除。\n\n"
                          "操作文件列表：\n%2")
                      .arg(fileList.size())
                      .arg(fileList.join("\n"));
        confirmButtonText = "移动到回收站";
    }

    // 创建自定义确认弹窗
    QMessageBox msgBox(this);
    msgBox.setWindowTitle(title);
    msgBox.setText(message);
    msgBox.setIcon(icon);
    msgBox.addButton("取消", QMessageBox::RejectRole);
    QPushButton *confirmButton = msgBox.addButton(confirmButtonText, QMessageBox::AcceptRole);
    confirmButton->setStyleSheet(erase ? "color: red;" : "color: green;");

    // 显示弹窗并等待用户选择
    msgBox.exec();
    if (msgBox.clickedButton() != confirmButton) {
        return; // 用户取消操作
    }

    // 拼接文件列表为分号分隔的字符串
    QString content = fileList.join(";");

    // 构造消息
    Msg msg;
    msg.setUser(m_user);
    msg.setContent(content);
    msg.setType(erase ? "ERASE" : "DEL");

    // 发送删除请求
    SendMsg(msg);

    // 从UI列表中移除项目
    QListWidget* targetList = erase ? ui->lw_recycle : ui->lw_down;
    for (auto fileName : fileList) {
        QList<QListWidgetItem*> items = targetList->findItems(fileName, Qt::MatchExactly);
        foreach (QListWidgetItem* item, items) {
            delete targetList->takeItem(targetList->row(item));
        }
    }

    // 显示操作结果
    QString resultMsg;
    if (erase) {
        resultMsg = QString("已永久删除 %1 个文件").arg(fileList.size());
    } else {
        resultMsg = QString("已移动 %1 个文件到回收站").arg(fileList.size());
    }
    OK(resultMsg); // 弹窗提示
}


// 恢复文件
void MainWindow::RecoverFile(QStringList fileList)
{
    QString content = QString();
    for(auto s : fileList)
    {
        if(!content.isEmpty())
            content += ";";
        content += s;
    }

    // 下载请求
    Msg msg("RECOVER");
    msg.setUser(m_user);
    msg.setContent(content);
    SendMsg(msg);
}


// 初始文件
void MainWindow::HandleFile(const Msg& msg)
{
    // 重置状态
    if (m_downloadProgress) {
        delete m_downloadProgress;
        m_downloadProgress = nullptr;
    }

    // 初始化进度条
    m_downloadProgress = new QProgressDialog("正在下载文件...", "取消", 0, 100, this);
    m_downloadProgress->setWindowModality(Qt::WindowModal);
    m_downloadProgress->setMinimumDuration(0);
    m_currentDownloadFile = msg.msg1; // 记录文件名
    m_currentDownloadSize = 0;

    // 初始化文件接收状态
    m_curFile = QString("%1/%2").arg(ui->ed_dir->text().trimmed()).arg(msg.msg1);
    m_fileSize = msg.fileSize;
    m_recBytes = 0;
    qDebug() << "开始接收文件: " << m_curFile << "大小：" << m_fileSize;
}


// 接收文件
bool MainWindow::ReceiveFile()
{
    QFile file(m_curFile);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Append))
    {
        qDebug() << "文件打开失败:" << file.errorString() << " " << m_curFile;
        return false;
    }

    QByteArray buff = m_sock->readAll(); // 读取所有可用数据
    file.write(buff);
    file.close();

    // 更新已接收字节数
    m_recBytes += buff.size();
    m_currentDownloadSize = m_recBytes;

    // 计算并更新进度
    int progressValue = static_cast<int>((m_currentDownloadSize * 100) / m_fileSize);
    m_downloadProgress->setValue(progressValue);
    QApplication::processEvents(); // 刷新界面

    // 检查是否取消
    if (m_downloadProgress->wasCanceled()) {
        ERR("下载已取消");
        file.remove(); // 删除未完成的文件
        return false;
    }

    // 检查是否完成
    if (m_recBytes >= m_fileSize) {
        m_downloadProgress->close();
        OK(QString("文件 %1 下载完成").arg(m_currentDownloadFile));
        // 重置状态
        m_curFile.clear();
        m_recBytes = 0;
        m_fileSize = 0;
        delete m_downloadProgress;
        m_downloadProgress = nullptr;
    }

    return true;
}


// 响应消息
void MainWindow::OnMsg()
{
    Msg msg;
    m_sock->read((char*)&msg, sizeof(Msg));

    if("LIST" == QString(msg.type))
    {
        qDebug() << "收到列表：" << msg.content;
        QStringList list = QString(msg.content).split(";");
        ui->lw_down->clear();
        for(auto file : list)
        {
            ui->lw_down->addItem(file);
        }
    }

    if("RECYCLE" == QString(msg.type))
    {
        qDebug() << "收到回收站列表：" << msg.content;
        QStringList list = QString(msg.content).split(";");
        ui->lw_recycle->clear();
        for(auto file : list)
        {
            ui->lw_recycle->addItem(file);
        }
    }

    if("LOG" == QString(msg.type))
    {
        qDebug() << "收到记录列表：" << msg.content;
        if(QString(msg.content).isEmpty())
            return;

        QStringList list = QString(msg.content).split(";");
        ui->tw_record->clearContents();
        int rowCount = ui->tw_record->rowCount();
        for (int row = rowCount - 1; row >= 0; --row)
                ui->tw_record->removeRow(row);
        for(auto file : list)
        {
            QStringList line = file.split("|");

            QString type = "下载";
            if(line.at(1) == "UP")
                type = "上传";

            quint64 tt = line.at(2).toULongLong();
            QDateTime datetime = QDateTime::fromMSecsSinceEpoch(tt);

            int row = ui->tw_record->rowCount();
            ui->tw_record->insertRow(row);
            ui->tw_record->setItem(row, 0, new QTableWidgetItem(line.at(0)));
            ui->tw_record->setItem(row, 1, new QTableWidgetItem(type));
            ui->tw_record->setItem(row, 2, new QTableWidgetItem(datetime.toString("yyyy-MM-dd hh:mm:ss")));
        }
    }

    if("SHARE" == QString(msg.type))
    {
        QString str = QString("链接：%1\n密钥：%2\n").arg(msg.msg1).arg(msg.msg2);
        OK(str);
    }

    if("DOWN_SHARE" == QString(msg.type))
    {
        if("ERR" == QString(msg.msg1))
        {
            ERR("文件链接或提取码错误");
        }
        else
        {
            HandleFile(msg);
        }
    }

    if("DOWN" == QString(msg.type)) HandleFile(msg);


    // 用户名修改
    if("MOD_USER" == QString(msg.type))
    {
        if("DUP" == QString(msg.content))
        {
            ERR("已存在该用户名，修改失败");
            return;
        }
        else if("ERR" == QString(msg.content))
        {
            ERR("修改失败");
            return;
        }
        else
        {
            ui->label_admin->setText(msg.content);
            ui->lb_name->setText(msg.content);
            qDebug() << "msg.user=" << msg.user << " msg.content=" << msg.content;
            OK("修改成功");
        }
    }
}


// 响应文件
void MainWindow::OnFile()
{
    //qDebug() << "内核缓冲区大小：" << m_sock->bytesAvailable();

    // 仅在存在有效文件时接收
    if (!m_curFile.isEmpty() && m_fileSize > 0) {
        ReceiveFile();
    } else {
        // 无文件接收时直接处理消息
        OnMsg();
    }
}


// 生成MD5哈希的辅助函数
QString MainWindow::GetMd5Hash(const QString &input) {
    QByteArray data = input.toUtf8();
    QByteArray hash = QCryptographicHash::hash(data, QCryptographicHash::Md5);
    return QString(hash.toHex());  // 返回32位十六进制字符串
}

// 设置登录账户
void MainWindow::setCurrentUser(const QString &user)
{
    m_user = user;
    ui->label_admin->setText(user);
    ui->lb_name->setText(user);

    // 界面初始化
    InitMain();

    // 修改用户名
    connect(ui->btn_mod_user, &QPushButton::clicked, this, [=]()
    {
        auto name = QInputDialog::getText(this, "用户名", "输入新用户名");
        if(name.isEmpty())
            return;

        // 更改用户名
        Msg msg;
        msg.setUser(m_user);
        msg.setContent(name);
        msg.setType("MOD_USER");
        SendMsg(msg);
    });


    // 修改密码
    connect(ui->btn_mod_pwd, &QPushButton::clicked, this, [=]()
    {
        auto pwd1 = ui->ed_pwd1->text().trimmed();
        auto pwd2 = ui->ed_pwd2->text().trimmed();
        if(pwd1.isEmpty() || pwd2.isEmpty())
        {
            ERR("密码不能为空");
            return;
        }

        if(pwd1 != pwd2)
        {
            ERR("两次密码不一致");
            return;
        }

        if(pwd1.size() < 6)
        {
            ERR("密码不能低于6位");
            return;
        }

        // 对密码进行MD5加密
        QString hashedPwd = GetMd5Hash(pwd1);

        // 更改密码
        Msg msg;
        msg.setUser(m_user);
        msg.setType("MOD_PWD");
        msg.setContent(hashedPwd);  // 替换为哈希值
        SendMsg(msg);
        OK("密码修改成功");
    });

}


// 发生错误
void MainWindow::Error(QAbstractSocket::SocketError e)
{
    switch (static_cast<int>(e))
    {
    case QAbstractSocket::ConnectionRefusedError:
        ERR("连接超时，检查服务器地址和端口号或检查服务器是否已启动");
        exit(-1);
        break;

    case QAbstractSocket::HostNotFoundError:
        ERR("主机地址错误");
        exit(-1);
        break;

    case QAbstractSocket::RemoteHostClosedError:
        ERR("服务器断开");
        exit(-1);
        break;
    }
}


// 上传文件
void MainWindow::on_btn_upload_clicked()
{
    // 创建一个文件对话框
    QFileDialog fileDialog;
    fileDialog.setFileMode(QFileDialog::ExistingFiles);                     // 设置为选择多个文件
    fileDialog.setDirectory(QDir::currentPath());                           // 设置默认打开路径为程序目录
    fileDialog.setNameFilter("文件 (*.*)");    // 设置文件过滤器
    fileDialog.setWindowTitle("选择文件");

    QStringList selectedFiles;

    // 打开文件对话框并等待用户选择文件
    if (fileDialog.exec())
    {
        // 获取用户选择的文件列表
        selectedFiles = fileDialog.selectedFiles();
    }

    if(selectedFiles.isEmpty())
    {
        ERR("请先选择要上传的文件");
        return;
    }

    // 添加进度提示
    QProgressDialog progress("正在上传文件...", "取消", 0, 100, this);
    progress.setWindowModality(Qt::WindowModal);
    progress.setMinimumDuration(0);

    foreach (const QString &filePath, selectedFiles) {
        QFile file(filePath);
        if (!file.open(QIODevice::ReadOnly)) {
            ERR(QString("无法打开文件：%1").arg(file.errorString()));
            continue;
        }

        QFileInfo info(file);
        qint64 fileSize = info.size();
        const qint64 chunkSize = 1024 * 1024; // 每次上传1MB
        qint64 bytesSent = 0;

        // 发送文件元信息
        Msg msg("UP");
        msg.setUser(m_user);
        msg.setMsg1(info.fileName());
        msg.setSize(fileSize);
        SendMsg(msg);

        // 分块上传
        while (!file.atEnd()) {
            if (progress.wasCanceled()) {
                file.close();
                ERR("上传已取消");
                return;
            }

            QByteArray chunk = file.read(chunkSize);
            m_sock->write(chunk);
            if (!m_sock->waitForBytesWritten(5000)) { // 等待5秒确保数据发送
                ERR("网络传输超时");
                file.close();
                return;
            }

            bytesSent += chunk.size();
            int progressValue = static_cast<int>((bytesSent * 100) / fileSize);
            progress.setValue(progressValue);
            QApplication::processEvents(); // 刷新界面
        }

        file.close();
        OK(QString("文件 %1 上传成功！").arg(info.fileName()));
        progress.setValue(0); // 重置进度条为下一个文件准备
    }
}


// 下载分享的文件
void MainWindow::on_btn_down_clicked()
{
    QString link = ui->ed_link->text().trimmed();
    QString pwd = ui->ed_pwd->text().trimmed();
    if(link.isEmpty() || pwd.isEmpty())
    {
        ERR("文件连接和提取码不能为空");
        return;
    }

    // 请求下载共享文件
    Msg msg("DOWN_SHARE", link, pwd);
    msg.setUser(m_user);
    SendMsg(msg);
}



void MainWindow::mousePressEvent(QMouseEvent *e)
{
    if(e->button() == Qt::LeftButton)
    {
        m_pressed = true;
        m_pos = e->pos();
    }
}


void MainWindow::mouseReleaseEvent(QMouseEvent *e)
{
    if(e->button() == Qt::LeftButton)
        m_pressed = false;
}


void MainWindow::mouseMoveEvent(QMouseEvent *e)
{
    move(e->pos() - m_pos + pos());		// 当前位置减去相对距离
}


