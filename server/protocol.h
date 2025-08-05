#ifndef PROTOCOL_H
#define PROTOCOL_H
#include <QString>
#include <QDebug>
#include <stdlib.h>
#include <string.h>
#include <QFile>
#define Q_DEBUG_PAUSE for(int i = 0; i < 1000000; i++)

// 自定义协议
struct Msg
{
    char type[16];      // 消息类型，LOGIN登录 REG注册 DEL删除文件 ERASE彻底删除 RECOVER恢复文件
                        // LIST空间 RECYCLE回收站 UP上传 DOWN下载
                        // DOWN_SHARE下载分享文件 LOG上传下载记录
                        // MOD_USER更改用户名 MOD_PWD修改密码
    char msg1[64];      // 字段1：用户名|共享链接|
    char msg2[64];      // 字段2：密码|密钥|
    char content[164];  // 消息主体
    char user[16];      // 接收方ID
    int fileSize;       // 文件总大小

    Msg()
    {
        memset(type, '\0', sizeof(type));
        memset(msg1, '\0', sizeof(msg1));
        memset(msg2, '\0', sizeof(msg2));
        memset(content, '\0', sizeof(content));
        memset(user, '\0', sizeof(user));
        fileSize = 0;
    }

    Msg(QString _type, QString _msg1 = QString(), QString _msg2 = QString())
    {
        memcpy(type, _type.toStdString().c_str(), sizeof(type));
        memcpy(msg1, _msg1.toStdString().c_str(), sizeof(msg1));
        memcpy(msg2, _msg2.toStdString().c_str(), sizeof(msg2));

        memset(content, '\0', sizeof(content));
        memset(user, '\0', sizeof(user));
        fileSize = 0;
    }

    void setType(QString _type)
    {
        memcpy(type, _type.toStdString().c_str(), sizeof(type));
    }

    void setMsg1(QString _msg1)
    {
        memcpy(msg1, _msg1.toStdString().c_str(), sizeof(msg1));
    }

    void setMsg2(QString _msg2)
    {
        memcpy(msg2, _msg2.toStdString().c_str(), sizeof(msg2));
    }

    void setContent(QString _content)
    {
        memcpy(content, _content.toStdString().c_str(), sizeof(content));
    }

    void setUser(QString _user)
    {
        memcpy(user, _user.toStdString().c_str(), sizeof(user));
    }

    void setSize(int size)
    {
        fileSize = size;
    }

    void show()
    {
        qDebug() << "type：" << type << " msg1：" << msg1 << " msg2：" << msg2 << " user：" << user << " size：" << fileSize;
        qDebug() << "content：" << content << endl;
    }

    QString info()
    {
        return QString("type：%1  msg1：%2   msg2：%3   user：%4   size:%5   content：%6").
                arg(type).arg(msg1).arg(msg2).arg(user).arg(fileSize).arg(content);
    }
};

#endif // PROTOCOL_H
