#ifndef DB_H
#define DB_H

#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlQueryModel>
#include <QDebug>
#include <qdir.h>
#include <QSqlRecord>
#include <QRandomGenerator>
#include <QDateTime>
#include <QSqlError>
#define DB Db::Instance()


// 单例模式的类
class Db
{
public:
    // 静态成员函数，用于获取唯一的实例
    static Db& Instance()
    {
        static Db instance; // 在第一次调用时创建唯一实例
        return instance;
    }

    // 防止拷贝构造函数和赋值操作符的调用
    Db(Db const&) = delete;
    void operator=(Db const&) = delete;

    void initialization(){}

    // 是否重名
    bool DuplicationUser(QString user)
    {
        QSqlQueryModel model;
        model.setQuery(QString("SELECT * FROM users WHERE user='%1'").arg(user));
        return model.rowCount() > 0;
    }


    // 登录验证
    bool CheckLogin(QString user, QString pwd)
    {
        QSqlQueryModel model;
        QSqlQuery query;
        query.prepare("SELECT * FROM users WHERE user = ? AND pwd = ?");
        query.addBindValue(user);
        query.addBindValue(pwd);
        query.exec();
        model.setQuery(std::move(query));
        return model.rowCount() > 0;
    }


    // 获取共享文件路径
    QString FetchShareFilePath(QString link, QString pwd)
    {
        QSqlQueryModel model;
        QSqlQuery query;
        query.prepare("SELECT file FROM share WHERE link = ? AND pwd = ?");
        query.addBindValue(link);
        query.addBindValue(pwd);
        query.exec();
        model.setQuery(std::move(query));
        if (model.rowCount() <= 0)
            return QString();

        QSqlRecord record = model.record(0);

        return record.value("file").toString();
    }

    // 获取用户密码哈希值
    QString GetUserPassword(const QString &user) {
        QSqlQuery query;
        query.prepare("SELECT pwd FROM users WHERE user = ?");
        query.addBindValue(user);
        query.exec();
        if (query.next()) {
            return query.value(0).toString();
        }
        return QString();
    }

    // 注册成功，保存用户信息
    void AddUser(QString user, QString pwd) {
        QSqlQuery query;
        query.prepare("INSERT INTO users (user, pwd) VALUES (?, ?)");
        query.addBindValue(user);
        query.addBindValue(pwd);  // 此处pwd应为客户端传来的MD5哈希值
        query.exec();
    }

    // 添加上传下载记录
    void AddLog(QString user, QString file, QString type)
    {
        QSqlQuery query;
        query.prepare("INSERT INTO log (user, file, type, dt) VALUES (?, ?, ?, ?)");
        query.addBindValue(user);
        query.addBindValue(file);
        query.addBindValue(type);
        query.addBindValue(QDateTime::currentMSecsSinceEpoch());
        query.exec();
    }


    // 获取文件共享信息  link;pwd
    QString FetchShare(QString file, QString user)
    {
        QString sql = QString("SELECT link,pwd FROM share WHERE file='%1' AND user='%2'").arg(file).arg(user);
        m_model.setQuery(sql);

        // 首次分享
        if(m_model.rowCount() < 1)
        {
            QString link = RandomString(12);
            QString pwd = RandomString(4);
            // 入库
            sql = QString("INSERT INTO share VALUES('%1','%2','%3','%4')").arg(file).arg(user).arg(link).arg(pwd);
            m_model.setQuery(sql);
            return QString("%1;%2").arg(link).arg(pwd);
        }

        QSqlRecord record =  m_model.record(0);
        return QString("%1;%2").arg(record.value(0).toString()).arg(record.value(1).toString());
    }


    // 目录列表
    QString FileList(QString user)
    {
        QString str;
        QStringList list = GetFiles(QString("./data/%1").arg(user));
        for(int i = 0; i < list.size(); i++)
        {
            if(!str.isEmpty())
                str += ";";
            if(!list.at(i).contains("/.recycle"))
            {
                QString s = list.at(i);
                str += s.replace(QString("./data/%1/").arg(user), "");
            }
        }

        return str;
    }


    // 回收站列表
    QString RecycleList(QString user)
    {
        QString str;
        QStringList list = GetFiles(QString("./data/%1/.recycle").arg(user));
        for(int i = 0; i < list.size(); i++)
        {
            if(!str.isEmpty())
                str += ";";
            QString s = list.at(i);
            str += s.replace(QString("./data/%1/.recycle/").arg(user), "");
        }

        return str;
    }


    // 记录列表
    QString LogList(QString user)
    {
        QString str;
        QSqlQuery q;
        QString sql = QString("SELECT file,type,dt FROM log WHERE user='%1'").arg(user);

        if (q.exec(sql))
        {
            while (q.next())
            {
                QString line;
                line += q.value(0).toString();  line += "|";
                line += q.value(1).toString();  line += "|";
                line += q.value(2).toString();

                if (!str.isEmpty())
                    str += ";";
                str += line;
            }
        }

        return str;
    }


    // 修改用户名
    void UpdateUser(const QString& user, const QString& newUser)
    {
        QSqlQuery q;
        QString sql = QString("UPDATE users SET user='%2' WHERE user='%1'").arg(user).arg(newUser);
        if(!q.exec(sql))
        {
            qWarning() << "修改用户名错误：" << q.lastError().text();
        }
    }


    // 修改密码
    void UpdatePwd(const QString& user, const QString& pwd)
    {
        QSqlQuery q;
        QString sql = QString("UPDATE users SET pwd='%2' WHERE user='%1'").arg(user).arg(pwd);
        if(!q.exec(sql))
        {
            qWarning() << "修改密码错误：" << q.lastError().text();
        }
    }


private:
    // 私有构造函数，防止类被实例化
    Db()
    {
        if(!CreateDB())
        {
            qWarning() << "连接数据库失败:" << m_db.lastError().text();
            exit(-1);
        }
        CreateTable();
    }

    bool CreateDB()
    {
        // 添加数据库驱动
        m_db = QSqlDatabase::addDatabase("QSQLITE");

        // 设置数据库名字（文件名）
        m_db.setDatabaseName("database.db");

        // 打开数据库
        return m_db.open();
    }


    void CreateTable()
    {
        // 1、创建用户信息表
        QSqlQuery query;
        QString str = QString("CREATE TABLE users ("
                              "user TEXT PRIMARY KEY NOT NULL,"
                              "pwd TEXT NOT NULL);");
        query.exec(str);
        m_model.setQuery("SELECT * FROM project");
        if(m_model.rowCount() < 2)
        {
            query.exec(QString("INSERT INTO users VALUES('tom', '123123')"));
            query.exec(QString("INSERT INTO users VALUES('张三', '123123')"));
        }

        // 2.创建记录表
        str = QString("CREATE TABLE log ("
                                      "id INTEGER PRIMARY KEY NOT NULL,"
                                      "user TEXT NOT NULL,"
                                      "file TEXT NOT NULL,"
                                      "type TEXT NOT NULL,"
                                      "dt TEXT NOT NULL);");
        query.exec(str);

        // 3.创建文件共享表
        str = QString("CREATE TABLE share ("
                                      "file TEXT NOT NULL,"
                                      "user TEXT NOT NULL,"
                                      "link TEXT NOT NULL,"
                                      "pwd TEXT NOT NULL);");
        query.exec(str);
    }


    // 获取文件列表
    QStringList GetFiles(QString path)
    {
        QStringList list;

        QDir dir(path);
        QFileInfoList infolist = dir.entryInfoList(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot);
        for(int i = 0; i < infolist.size(); i++)
        {
            QFileInfo info = infolist.at(i);
            QString curPath = info.filePath();

            if(info.isDir())
            {
                QStringList files = GetFiles(curPath);
                list.append(files);
            }else
            {
                list.append(curPath);
            }
        }

        return list;
    }


    // 随机序列
    QString RandomString(int length)
    {
        QString str;
        QString characters = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";

        // 使用 QRandomGenerator 生成随机数
        QRandomGenerator generator = QRandomGenerator::securelySeeded();

        // 生成指定长度的随机字符串
        for (int i = 0; i < length; ++i)
        {
            // 生成随机索引，从字符集合中选择字符
            int index = generator.bounded(characters.length());
            str.append(characters.at(index));
        }

        return str;
    }


public:
    QSqlDatabase m_db;
    QSqlQueryModel m_model;
};

#endif // DB_H
