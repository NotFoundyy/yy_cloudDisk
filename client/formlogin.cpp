#include "formlogin.h"
#include "ui_formlogin.h"

#include <QMessageBox>

FormLogin::FormLogin(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::FormLogin)
{
    ui->setupUi(this);
    // 无边框
    setWindowFlags(Qt::FramelessWindowHint);
    setAttribute(Qt::WA_TranslucentBackground);
    setWindowIcon(QIcon("./res/up.png"));

    initLogin();
}

FormLogin::~FormLogin()
{
    delete ui;
}

// 生成MD5哈希的辅助函数
QString GetMd5Hash(const QString &input) {
    QByteArray data = input.toUtf8();
    QByteArray hash = QCryptographicHash::hash(data, QCryptographicHash::Md5);
    return QString(hash.toHex());  // 返回32位十六进制字符串
}

// 初始化登录
void FormLogin::initLogin()
{
    m_sock = new QTcpSocket;
    m_sock->connectToHost(m_serverAddress, m_serverPort);

    connect(m_sock, &QTcpSocket::connected, this, [=]()
    {
        qDebug() << "连接成功";
    });

    // 接收消息
    connect(m_sock, &QTcpSocket::readyRead, this, [=]()
    {
        if (sizeof(Msg) == m_sock->bytesAvailable())
        {
            Msg msg;
            m_sock->read((char*)&msg, sizeof(Msg));
            qDebug() << "接收到消息" << msg.type << " " << msg.content;

            if("LOGIN" == QString(msg.type))
            {
                // 登录成功
                if("OK" == QString(msg.content))
                {
                    // 记录登录账户
                    QString currentUser = QString(msg.msg1);
                    m_sock->close();

                    this->hide();
                    MainWindow *m = new MainWindow;
                    m->setCurrentUser(currentUser);
                    m->show();
                }
                else
                {
                    ERR("用户名或密码错误");
                }
            }

            if("REG" == QString(msg.type))
            {
                if("OK" == QString(msg.content))
                    OK("注册成功");
                else
                    ERR("已有该账户，请换名称后重新注册");
            }
        }
    });

    // 登录
    connect(ui->btn_log, &QPushButton::clicked, this, [=]()
    {
        // 判空
        QString user = ui->edit_user_logPage->text().trimmed();
        QString pwd = ui->edit_pwd_logPage->text().trimmed();
        if(user.isEmpty() || pwd.isEmpty())
        {
            QMessageBox::critical(this, " ", "账号和密码不能为空");
            return;
        }

        // 登录
        // 添加MD5加密
        QString hashedPwd = GetMd5Hash(pwd);  // 确保调用此函数

        Msg msg("LOGIN", user, hashedPwd);  // 发送哈希值
        m_sock->write((char*)&msg, sizeof(msg));
    });

    // 注册
    connect(ui->btn_reg, &QPushButton::clicked, this, [=]()
    {
        // 判空
        QString user = ui->edit_user_regPage->text().trimmed();
        QString pwd = ui->edit_pwd_regPage->text().trimmed();
        QString cfg = ui->edit_cfg_regPage->text().trimmed();
        if(user.isEmpty() || pwd.isEmpty())
        {
            QMessageBox::critical(this, " ", "账号和密码不能为空");
            return;
        }

        if(pwd != cfg)
        {
            QMessageBox::critical(this, " ", "两次密码不相同");
            return;
        }

        if(pwd.size() < 6)
        {
            QMessageBox::critical(this, " ", "密码需大于 5 位");
            return;
        }

        // 对密码进行MD5加密
        QString hashedPwd = GetMd5Hash(pwd);

        // 发送加密后的密码
        Msg msg("REG", user, hashedPwd);  // 替换msg.msg2为哈希值
        m_sock->write((char*)&msg, sizeof(msg));
    });

    // 转到登录注册
    connect(ui->btn_log_sw, &QPushButton::clicked, this, [=](){ ui->stackLR->setCurrentIndex(0); });
    connect(ui->btn_reg_sw, &QPushButton::clicked, this, [=](){ ui->stackLR->setCurrentIndex(1); });

    // 退出
    connect(ui->btn_quit, &QPushButton::clicked, this, [=](){ exit(0); });
}


void FormLogin::mousePressEvent(QMouseEvent *e)
{
    if(e->button() == Qt::LeftButton)
    {
        m_pressed = true;
        m_pos = e->pos();
    }
}


void FormLogin::mouseReleaseEvent(QMouseEvent *e)
{
    if(e->button() == Qt::LeftButton)
        m_pressed = false;
}


void FormLogin::mouseMoveEvent(QMouseEvent *e)
{
    move(e->pos() - m_pos + pos());		// 当前位置减去相对距离
}
