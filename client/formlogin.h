#ifndef FORMLOGIN_H
#define FORMLOGIN_H

#include <QWidget>
#include <QTcpSocket>
#include <QAbstractSocket>
#include <QMouseEvent>
#include <QCryptographicHash>
#include "mainwindow.h"
#include "../server/protocol.h"
#define ERR(str) QMessageBox::critical(this, "警告", str);
#define OK(str) QMessageBox::information(this, "提示", str)


namespace Ui {
class FormLogin;
}

class FormLogin : public QWidget
{
    Q_OBJECT

public:
    explicit FormLogin(QWidget *parent = nullptr);
    ~FormLogin();

    // 初始化登录
    void initLogin();

private:
    void mousePressEvent(QMouseEvent* e);
    void mouseReleaseEvent(QMouseEvent* e);
    void mouseMoveEvent(QMouseEvent* e);

private:
    Ui::FormLogin *ui;
    bool m_pressed = false;                 // 按下状态
    QPoint m_pos;                           // 按下时的鼠标位置
    QString m_serverAddress = "127.0.0.1";  // 服务器地址
    quint64 m_serverPort = 8899;            // 服务器端口
    QTcpSocket *m_sock;                     // 客户端
};

#endif // FORMLOGIN_H
