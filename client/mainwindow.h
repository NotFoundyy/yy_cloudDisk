#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QMouseEvent>
#include <QMessageBox>
#include <QTcpSocket>
#include <QAbstractSocket>
#include <QMenu>
#include <QAction>
#include <QThread>
#include <QSettings>
#include <QCryptographicHash>
#include <QProgressDialog>
#include "../server/protocol.h"
#define ERR(str) QMessageBox::critical(this, "警告", str);
#define OK(str) QMessageBox::information(this, "提示", str)

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE


class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    // 初始化主界面
    void InitMain();

    // 发送消息
    void SendMsg(const Msg& msg);

    // 下载文件
    void DownFile(QStringList fileList);

    // 删除文件
    void DelFile(QStringList fileList, bool erase = false);

    // 恢复文件
    void RecoverFile(QStringList fileList);

    // 处理文件
    void HandleFile(const Msg& msg);

    // 接收文件
    bool ReceiveFile();

    // 响应消息
    void OnMsg();

    // 响应文件
    void OnFile();

    // 设置登录账户
    void setCurrentUser(const QString& user);

    //hd5加密
    QString GetMd5Hash(const QString &input);

public slots:
    // 发生错误
    void Error(QAbstractSocket::SocketError);

    // 上传文件
    void on_btn_upload_clicked();

    // 下载分享的文件
    void on_btn_down_clicked();

private:
    void mousePressEvent(QMouseEvent* e);
    void mouseReleaseEvent(QMouseEvent* e);
    void mouseMoveEvent(QMouseEvent* e);

private:
    Ui::MainWindow *ui;
    bool m_pressed = false;     // 按下状态
    QPoint m_pos;       		// 按下时的鼠标位置
    QString m_user;             // 登录账户
    QString m_serverAddress;    // 服务器地址
    quint64 m_serverPort;       // 服务器端口
    QTcpSocket *m_sock;         // 客户端
    QSettings *m_set;           // 配置项

    QString m_curFile;          // 正在读取的文件
    quint64 m_recBytes = 0;     // 已经读取的大小
    quint64 m_fileSize = 0;     // 总大小
    QProgressDialog *m_downloadProgress; // 下载进度条
    qint64 m_currentDownloadSize;        // 当前已下载字节数
    QString m_currentDownloadFile;       // 当前下载的文件名
};
#endif // MAINWINDOW_H
