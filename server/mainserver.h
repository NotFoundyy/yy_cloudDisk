#ifndef MAINSERVER_H
#define MAINSERVER_H

#include <QFile>
#include <QDir>
#include <QFileInfo>
#include <QDebug>
#include <QTcpServer>
#include <QTcpSocket>
#include <QUdpSocket>
#include <QThread>
#include <QDateTime>
#include <QTime>
#include <QThread>
#include <random>
#include <QDataStream>
#include "protocol.h"
#include "db.h"
#define PORT_MAIN 8899
#if _MSC_VER >= 1600
#pragma execution_character_set("utf-8")
#endif

class MainServer : public QObject
{
    Q_OBJECT
public:
    MainServer()
    {
        DB.initialization();
        QDir dir;
        dir.mkdir("./data");
        m_server = new QTcpServer;
        if(m_server->listen(QHostAddress::Any, PORT_MAIN))
            qDebug() << "服务器启动...";
        else
            exit(-1);

        connect(m_server, &QTcpServer::newConnection, this, [=](){
            QTcpSocket* client = m_server->nextPendingConnection();
            connect(client, &QTcpSocket::readyRead, this, [=]() {
                if (sizeof(Msg) != client->bytesAvailable())
                {
                    OnFile(client);
                }
                else
                {
                    OnMsg(client);
                }
            });
        });
    }

    ~MainServer()
    {
        m_server->deleteLater();
    }

signals:
    void tcpReceiveStart(quint64 fileSize, QString filePath, QString srcFile);

private slots:
    // 发送信息
    void SendMsg(QTcpSocket* sock, Msg msg)
    {
        sock->write((char*)&msg, sizeof(Msg));
    }

    // 响应消息
    void OnMsg(QTcpSocket* client)
    {
        // 传递消息
        Msg msg;
        client->read((char*)&msg, sizeof(msg));

        // 登录
        if ("LOGIN" == QString(msg.type))
        {
            QString inputPwdHash = QString(msg.msg2);  // 客户端已发送MD5哈希
            QString storedPwdHash = DB.GetUserPassword(msg.msg1);  // 需新增方法获取数据库中的哈希密码

            // qDebug() << "Login Request - User:" << msg.msg1
            //          << "Input Hash:" << inputPwdHash
            //          << "Stored Hash:" << storedPwdHash;
            if (inputPwdHash == storedPwdHash) {
                msg.setContent("OK");
            } else {
                msg.setContent("ERR");
            }
            SendMsg(client, msg);
        }

        // 注册
        if ("REG" == QString(msg.type))
        {
            if (!DB.DuplicationUser(msg.msg1))
            {
                msg.setContent("OK");
                DB.AddUser(msg.msg1, msg.msg2);

                // 开辟空间
                QDir dir;
                dir.mkdir(QString("./data/%1").arg(msg.msg1));
                dir.mkdir(QString("./data/%1/.recycle").arg(msg.msg1));
            }
            else
                msg.setContent("ERR");
            SendMsg(client, msg);
        }

        // 获取空间列表
        if ("LIST" == QString(msg.type))
        {
            msg.setContent(DB.FileList(msg.user));
            SendMsg(client, msg);
        }

        // 获取回收站列表
        if ("RECYCLE" == QString(msg.type))
        {
            msg.setContent(DB.RecycleList(msg.user));
            SendMsg(client, msg);
        }

        // 获取操作记录
        if ("LOG" == QString(msg.type))
        {
            msg.setContent(DB.LogList(msg.user));
            SendMsg(client, msg);
        }

        // 删除
        if ("DEL" == QString(msg.type))  HandleDel(msg);

        // 彻底删除
        if ("ERASE" == QString(msg.type)) HandleErase(msg);

        // 恢复
        if ("RECOVER" == QString(msg.type)) HandleRecover(msg);

        // 处理共享
        if ("SHARE" == QString(msg.type)) HandleShare(client, msg);

        // 处理下载共享
        if ("DOWN_SHARE" == QString(msg.type)) HandleDownShare(client, msg);

        // 处理上传
        if ("UP" == QString(msg.type)) HandleUpFile(msg);

        // 处理下载
        if ("DOWN" == QString(msg.type)) HandleDownFile(client, msg);

        // 处理修改用户名
        if ("MOD_USER" == QString(msg.type)) HandleModUser(client, msg);

        // 处理修改密码
        if("MOD_PWD" == QString(msg.type)) HandleModPwd(msg);
    }


    // 接收文件
    bool ReceiveFile(QTcpSocket* client)
    {
        QFile file(m_curFile);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Append))
        {
            qDebug() << "文件打开失败";
            return false;
        }

        bool tail = false;
        quint64 now;

        QByteArray buff;
        quint64 residue = m_fileSize - m_recBytes;          // 上次文件未读取的量
        now = client->bytesAvailable() - residue;           // 本次还有多少未处理
        if (client->bytesAvailable() > residue)
        {
            buff = client->read(residue);
            tail = true;
            //qDebug() << "上个文件已全部接收,还有 " << client->bytesAvailable() - residue << " 数据未处理";
        }
        else
        {
            //qDebug() << "全部读取";
            buff = client->readAll();
            QByteArray tmp = buff.left(sizeof(Msg));
            Msg msg = (Msg)tmp;
            if (QString(msg.type) == "UP")
            {
                qDebug() << "元信息与文件黏在一起";
                HandleUpFile(msg);
                buff.remove(0, sizeof(Msg));
            }
        }

        // 写入数据并关闭文件
        file.write(buff);
        file.close();

        // 统计本文件读取字节数
        m_recBytes += buff.size();
        //qDebug() << "已处理了：" << buff.size() << endl;

        // 处理剩余的
        if (tail)
        {
            //qDebug() << "处理剩余的 " << now << " 包";
            if (now > sizeof(Msg))
            {
                Msg msg;
                client->read((char*)&msg, sizeof(msg));

                // 接收文件
                if ("UP" == QString(msg.type))
                {
                    qDebug() << "[二次解包]";
                    HandleUpFile(msg);
                    ReceiveFile(client);
                }               
            }
            if (client->bytesAvailable() > 0)
            {
                //qDebug() << "剩余：" << client->bytesAvailable() << " 未处理";
            }
        }

        return true;
    }


    // 响应文件
    void OnFile(QTcpSocket* client)
    {
        while (!ReceiveFile(client))
            OnMsg(client);
    }


    // 处理上传
    void HandleUpFile(const Msg& msg)
    {
        m_curFile = QString("./data/%1/%2").arg(msg.user).arg(msg.msg1);
        m_fileSize = msg.fileSize;
        m_recBytes = 0;
        DB.AddLog(msg.user, msg.msg1, msg.type);
        qDebug() << "接收到文件: " << m_curFile << "大小：" << msg.fileSize;
    }


    // 处理下载
    void HandleDownFile(QTcpSocket* client, const Msg& msg)
    {
        QStringList list = QString(msg.content).split(";");
        for (auto fileName : list)
        {
            QString filePath = QString("./data/%1/%2").arg(msg.user).arg(fileName);
            DB.AddLog(msg.user, fileName, msg.type);

            // 发送
            QFileInfo info(filePath);
            Msg msg("DOWN");
            msg.setMsg1(info.fileName());
            msg.setSize(info.size());
            SendMsg(client, msg);
            qDebug() << "发送文件: " << info.fileName() << " 大小：" << info.size();
            Q_DEBUG_PAUSE;

            QFile file(filePath);
            if(!file.open(QIODevice::ReadOnly))
            {
                qWarning() << "打开文件失败:" << filePath << " " << file.errorString();
                return;
            }
            QByteArray buff = file.readAll();
            client->write(buff);
            file.close();
            Q_DEBUG_PAUSE;
        }
    }


    // 处理删除
    void HandleDel(Msg msg)
    {
        QStringList list = QString(msg.content).split(";");
        for (auto fileName : list)
        {
            // 记录原路径
            QString srcPath = QString("./data/%1/%2").arg(msg.user).arg(fileName);
            QFile file(srcPath);
            qDebug() << file.exists() << "|" << srcPath << "|";

            // 移动文件
            QString dstPath = QString("./data/%1/.recycle/%2").arg(msg.user).arg(fileName);
            if (!file.rename(dstPath))
                qDebug() << "移动失败 " << file.errorString() << " 移动后：" << dstPath;
        }
    }


    // 处理清除
    void HandleErase(Msg msg)
    {
        QStringList list = QString(msg.content).split(";");
        for (auto fileName : list)
        {
            // 记录原路径
            QString srcPath = QString("./data/%1/.recycle/%2").arg(msg.user).arg(fileName);
            QFile file(srcPath);
            file.remove();
        }
    }


    // 处理恢复
    void HandleRecover(Msg msg)
    {
        QStringList list = QString(msg.content).split(";");
        for (auto fileName : list)
        {
            // 记录原路径
            QString srcPath = QString("./data/%1/.recycle/%2").arg(msg.user).arg(fileName);
            QFile file(srcPath);

            qDebug() << file.exists() << "|" << srcPath << "|";

            // 移动文件
            QString dstPath = QString("./data/%1/%2").arg(msg.user).arg(fileName);
            if (!file.rename(dstPath))
                qDebug() << "移动失败 " << file.errorString() << " 移动后：" << dstPath;
        }
    }


    // 处理共享
    void HandleShare(QTcpSocket* sock, Msg msg)
    {
        QString file = QString("./data/%1/%2").arg(msg.user).arg(msg.content);
        QString linkPwd = DB.FetchShare(file, msg.user);
        QStringList list = linkPwd.split(";");
        msg.setMsg1(list.at(0));
        msg.setMsg2(list.at(1));
        SendMsg(sock, msg);
    }


    // 处理下载功能
    void HandleDownShare(QTcpSocket* sock, Msg msg)
    {
        QString filePath = DB.FetchShareFilePath(msg.msg1, msg.msg2);
        if (filePath.isEmpty())
        {
            msg.setMsg1("ERR");
            SendMsg(sock, msg);
        }
        else
        {
            QFileInfo info(filePath);
            msg.setMsg1(info.fileName());
            msg.setSize(info.size());
            SendMsg(sock, msg);
            Q_DEBUG_PAUSE;

            // 发送文件            
            qDebug() << "发送文件: " << info.fileName() << " 大小：" << info.size();
            Q_DEBUG_PAUSE;
            DB.AddLog(msg.user, info.fileName(), "DOWN");
            QFile file(filePath);
            file.open(QIODevice::ReadOnly);
            QByteArray buff = file.readAll();
            sock->write(buff);
            file.close();
        }
    }


    // 处理修改用户名
    void HandleModUser(QTcpSocket* sock, Msg msg)
    {
        if(DB.DuplicationUser(msg.content))
        {
            msg.setContent("DUP");
            SendMsg(sock, msg);
            return;
        }

        QString newUser = msg.content;
        QString oldPath = QString("./data/%1").arg(msg.user);
        QString newPath = QString("./data/%1").arg(msg.content);


        // 修改目录
        QDir dir;
        if(!dir.exists(oldPath))
        {
            msg.setContent("ERR");
            SendMsg(sock, msg);
            return;
        }

        if(!dir.rename(oldPath, newPath))
        {
            qDebug() << "重命名失败：" << newPath;
        }

        // 修改用户名
        DB.UpdateUser(msg.user, msg.content);

        // 修改成功
        msg.setContent(newUser);
        SendMsg(sock, msg);
    }


    // 处理修改密码
    void HandleModPwd(Msg msg)
    {
        DB.UpdatePwd(msg.user, msg.content);
    }


private:
    QTcpServer *m_server;       // tcp服务
    QString m_curFile;          // 正在读取的文件
    quint64 m_recBytes = 0;     // 已经读取的大小
    quint64 m_fileSize = 0;     // 总大小
};


#endif // MAINSERVER_H
