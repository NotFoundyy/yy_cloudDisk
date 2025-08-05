# CloudDisk API 文档

本文档详细说明 CloudDisk 云盘系统的通信协议和API接口。

## 📋 目录

- [协议概述](#协议概述)
- [消息格式](#消息格式)
- [API接口](#api接口)
- [错误处理](#错误处理)
- [示例代码](#示例代码)

## 🔧 协议概述

CloudDisk 使用基于TCP的自定义协议进行客户端与服务器之间的通信。所有通信都通过 `Msg` 结构体进行数据交换。

### 通信流程

1. **连接建立**：客户端连接到服务器的8899端口
2. **消息交换**：使用 `Msg` 结构体进行请求和响应
3. **文件传输**：文件数据直接通过TCP流传输
4. **连接关闭**：操作完成后关闭连接

## 📨 消息格式

### Msg 结构体定义

```cpp
struct Msg {
    char type[16];      // 消息类型，固定16字节
    char msg1[64];      // 字段1，固定64字节
    char msg2[64];      // 字段2，固定64字节
    char content[164];  // 消息主体，固定164字节
    char user[16];      // 用户ID，固定16字节
    int fileSize;       // 文件大小，4字节整数
};
```

### 字段说明

| 字段 | 类型 | 大小 | 说明 |
|------|------|------|------|
| `type` | char[16] | 16字节 | 消息类型标识符 |
| `msg1` | char[64] | 64字节 | 主要参数字段 |
| `msg2` | char[64] | 64字节 | 次要参数字段 |
| `content` | char[164] | 164字节 | 消息内容或文件列表 |
| `user` | char[16] | 16字节 | 操作用户ID |
| `fileSize` | int | 4字节 | 文件大小（字节） |

## 🔌 API接口

### 用户管理接口

#### 1. 用户登录 (LOGIN)

**请求格式**：
```cpp
Msg msg("LOGIN", username, password_hash);
msg.setUser(username);
```

**参数说明**：
- `type`: "LOGIN"
- `msg1`: 用户名
- `msg2`: 密码的MD5哈希值
- `user`: 用户名

**响应格式**：
```cpp
// 成功
msg.setContent("OK");

// 失败
msg.setContent("ERR");
```

**示例**：
```cpp
// 客户端发送登录请求
Msg loginMsg("LOGIN", "admin", "5f4dcc3b5aa765d61d8327deb882cf99");
loginMsg.setUser("admin");
clientSocket->write((char*)&loginMsg, sizeof(Msg));

// 服务器响应
if (验证成功) {
    loginMsg.setContent("OK");
} else {
    loginMsg.setContent("ERR");
}
```

#### 2. 用户注册 (REG)

**请求格式**：
```cpp
Msg msg("REG", username, password_hash);
```

**参数说明**：
- `type`: "REG"
- `msg1`: 用户名
- `msg2`: 密码的MD5哈希值

**响应格式**：
```cpp
// 成功
msg.setContent("OK");

// 用户名已存在
msg.setContent("ERR");
```

### 文件管理接口

#### 3. 获取文件列表 (LIST)

**请求格式**：
```cpp
Msg msg("LIST");
msg.setUser(username);
```

**参数说明**：
- `type`: "LIST"
- `user`: 用户名

**响应格式**：
```cpp
// 文件列表，用分号分隔
msg.setContent("file1.txt;file2.pdf;document.docx");
```

#### 4. 文件上传 (UP)

**请求格式**：
```cpp
Msg msg("UP", filename, "");
msg.setUser(username);
msg.setSize(fileSize);
```

**参数说明**：
- `type`: "UP"
- `msg1`: 文件名
- `user`: 用户名
- `fileSize`: 文件大小

**传输流程**：
1. 发送消息头
2. 发送文件数据
3. 服务器接收并保存文件

**示例**：
```cpp
// 1. 发送上传请求
Msg uploadMsg("UP", "test.txt", "");
uploadMsg.setUser("admin");
uploadMsg.setSize(fileSize);
socket->write((char*)&uploadMsg, sizeof(Msg));

// 2. 发送文件数据
QFile file("test.txt");
if (file.open(QIODevice::ReadOnly)) {
    QByteArray data = file.readAll();
    socket->write(data);
    file.close();
}
```

#### 5. 文件下载 (DOWN)

**请求格式**：
```cpp
Msg msg("DOWN");
msg.setUser(username);
msg.setContent("file1.txt;file2.pdf");  // 要下载的文件列表
```

**参数说明**：
- `type`: "DOWN"
- `user`: 用户名
- `content`: 要下载的文件列表，用分号分隔

**响应流程**：
1. 服务器发送文件信息
2. 服务器发送文件数据
3. 客户端接收并保存文件

**示例**：
```cpp
// 客户端请求下载
Msg downloadMsg("DOWN");
downloadMsg.setUser("admin");
downloadMsg.setContent("test.txt");
socket->write((char*)&downloadMsg, sizeof(Msg));

// 服务器响应文件信息
Msg response("DOWN");
response.setMsg1("test.txt");
response.setSize(1024);
socket->write((char*)&response, sizeof(Msg));

// 服务器发送文件数据
QFile file("./data/admin/test.txt");
if (file.open(QIODevice::ReadOnly)) {
    QByteArray data = file.readAll();
    socket->write(data);
    file.close();
}
```

#### 6. 文件删除 (DEL)

**请求格式**：
```cpp
Msg msg("DEL");
msg.setUser(username);
msg.setContent("file1.txt;file2.pdf");  // 要删除的文件列表
```

**参数说明**：
- `type`: "DEL"
- `user`: 用户名
- `content`: 要删除的文件列表，用分号分隔

**响应格式**：
```cpp
// 删除成功（无响应，直接处理）
```

### 回收站接口

#### 7. 获取回收站列表 (RECYCLE)

**请求格式**：
```cpp
Msg msg("RECYCLE");
msg.setUser(username);
```

**参数说明**：
- `type`: "RECYCLE"
- `user`: 用户名

**响应格式**：
```cpp
// 回收站文件列表，用分号分隔
msg.setContent("deleted_file1.txt;deleted_file2.pdf");
```

#### 8. 文件恢复 (RECOVER)

**请求格式**：
```cpp
Msg msg("RECOVER");
msg.setUser(username);
msg.setContent("file1.txt;file2.pdf");  // 要恢复的文件列表
```

**参数说明**：
- `type`: "RECOVER"
- `user`: 用户名
- `content`: 要恢复的文件列表，用分号分隔

#### 9. 彻底删除 (ERASE)

**请求格式**：
```cpp
Msg msg("ERASE");
msg.setUser(username);
msg.setContent("file1.txt;file2.pdf");  // 要彻底删除的文件列表
```

**参数说明**：
- `type`: "ERASE"
- `user`: 用户名
- `content`: 要彻底删除的文件列表，用分号分隔

### 文件分享接口

#### 10. 生成分享链接 (SHARE)

**请求格式**：
```cpp
Msg msg("SHARE");
msg.setUser(username);
msg.setContent(filename);  // 要分享的文件名
```

**参数说明**：
- `type`: "SHARE"
- `user`: 用户名
- `content`: 要分享的文件名

**响应格式**：
```cpp
// 返回分享链接和密码
msg.setMsg1("https://share.example.com/abc123def456");
msg.setMsg2("1234");  // 访问密码
```

#### 11. 下载分享文件 (DOWN_SHARE)

**请求格式**：
```cpp
Msg msg("DOWN_SHARE", share_link, password);
```

**参数说明**：
- `type`: "DOWN_SHARE"
- `msg1`: 分享链接
- `msg2`: 访问密码

**响应格式**：
```cpp
// 成功
msg.setMsg1(filename);
msg.setSize(fileSize);
// 然后发送文件数据

// 失败
msg.setMsg1("ERR");
```

### 日志接口

#### 12. 获取操作记录 (LOG)

**请求格式**：
```cpp
Msg msg("LOG");
msg.setUser(username);
```

**参数说明**：
- `type`: "LOG"
- `user`: 用户名

**响应格式**：
```cpp
// 操作记录，格式：文件名|操作类型|时间戳;文件名|操作类型|时间戳
msg.setContent("test.txt|UP|1640995200000;document.pdf|DOWN|1640995300000");
```

### 账户设置接口

#### 13. 修改用户名 (MOD_USER)

**请求格式**：
```cpp
Msg msg("MOD_USER");
msg.setUser(old_username);
msg.setContent(new_username);
```

**参数说明**：
- `type`: "MOD_USER"
- `user`: 原用户名
- `content`: 新用户名

**响应格式**：
```cpp
// 成功
msg.setContent(new_username);

// 用户名已存在
msg.setContent("DUP");

// 失败
msg.setContent("ERR");
```

#### 14. 修改密码 (MOD_PWD)

**请求格式**：
```cpp
Msg msg("MOD_PWD");
msg.setUser(username);
msg.setContent(new_password_hash);
```

**参数说明**：
- `type`: "MOD_PWD"
- `user`: 用户名
- `content`: 新密码的MD5哈希值

## ⚠️ 错误处理

### 常见错误类型

1. **连接错误**
   - 服务器未启动
   - 网络连接中断
   - 端口被占用

2. **认证错误**
   - 用户名不存在
   - 密码错误
   - 用户未登录

3. **文件操作错误**
   - 文件不存在
   - 权限不足
   - 磁盘空间不足

4. **协议错误**
   - 消息格式错误
   - 不支持的操作类型
   - 参数缺失

### 错误处理建议

```cpp
// 客户端错误处理示例
void handleError(QAbstractSocket::SocketError error) {
    switch (error) {
        case QAbstractSocket::ConnectionRefusedError:
            qDebug() << "连接被拒绝，请检查服务器是否启动";
            break;
        case QAbstractSocket::RemoteHostClosedError:
            qDebug() << "服务器连接已关闭";
            break;
        case QAbstractSocket::HostNotFoundError:
            qDebug() << "找不到服务器主机";
            break;
        case QAbstractSocket::SocketTimeoutError:
            qDebug() << "连接超时";
            break;
        default:
            qDebug() << "网络错误：" << error;
            break;
    }
}
```

## 💡 示例代码

### 完整的登录流程

```cpp
class CloudDiskClient {
private:
    QTcpSocket *socket;
    
public:
    bool login(const QString &username, const QString &password) {
        // 连接到服务器
        socket = new QTcpSocket();
        socket->connectToHost("127.0.0.1", 8899);
        
        if (!socket->waitForConnected(5000)) {
            qDebug() << "连接服务器失败";
            return false;
        }
        
        // 构造登录消息
        QString passwordHash = getMd5Hash(password);
        Msg loginMsg("LOGIN", username, passwordHash);
        loginMsg.setUser(username);
        
        // 发送登录请求
        socket->write((char*)&loginMsg, sizeof(Msg));
        
        // 等待响应
        if (socket->waitForReadyRead(5000)) {
            Msg response;
            socket->read((char*)&response, sizeof(Msg));
            
            if (QString(response.content) == "OK") {
                qDebug() << "登录成功";
                return true;
            } else {
                qDebug() << "登录失败";
                return false;
            }
        }
        
        return false;
    }
    
private:
    QString getMd5Hash(const QString &input) {
        QByteArray hash = QCryptographicHash::hash(
            input.toUtf8(), QCryptographicHash::Md5);
        return hash.toHex();
    }
};
```

### 文件上传示例

```cpp
bool uploadFile(const QString &username, const QString &filePath) {
    QFileInfo fileInfo(filePath);
    QString fileName = fileInfo.fileName();
    qint64 fileSize = fileInfo.size();
    
    // 构造上传消息
    Msg uploadMsg("UP", fileName, "");
    uploadMsg.setUser(username);
    uploadMsg.setSize(fileSize);
    
    // 发送消息头
    socket->write((char*)&uploadMsg, sizeof(Msg));
    
    // 发送文件数据
    QFile file(filePath);
    if (file.open(QIODevice::ReadOnly)) {
        QByteArray data = file.readAll();
        socket->write(data);
        file.close();
        
        // 等待上传完成
        if (socket->waitForBytesWritten()) {
            qDebug() << "文件上传成功";
            return true;
        }
    }
    
    return false;
}
```

### 文件下载示例

```cpp
bool downloadFile(const QString &username, const QString &fileName) {
    // 构造下载请求
    Msg downloadMsg("DOWN");
    downloadMsg.setUser(username);
    downloadMsg.setContent(fileName);
    
    // 发送下载请求
    socket->write((char*)&downloadMsg, sizeof(Msg));
    
    // 等待文件信息响应
    if (socket->waitForReadyRead(5000)) {
        Msg response;
        socket->read((char*)&response, sizeof(Msg));
        
        if (QString(response.type) == "DOWN") {
            QString receivedFileName = QString(response.msg1);
            qint64 fileSize = response.fileSize;
            
            // 接收文件数据
            QByteArray fileData;
            while (fileData.size() < fileSize) {
                if (socket->waitForReadyRead(5000)) {
                    fileData.append(socket->readAll());
                } else {
                    break;
                }
            }
            
            // 保存文件
            if (fileData.size() == fileSize) {
                QFile file(QString("./download/%1").arg(receivedFileName));
                if (file.open(QIODevice::WriteOnly)) {
                    file.write(fileData);
                    file.close();
                    qDebug() << "文件下载成功";
                    return true;
                }
            }
        }
    }
    
    return false;
}
```

## 📝 注意事项

1. **字符编码**：所有字符串都使用UTF-8编码
2. **文件大小限制**：建议单个文件不超过100MB
3. **并发连接**：服务器支持多客户端同时连接
4. **超时设置**：建议设置合理的连接和读取超时时间
5. **错误重试**：网络操作建议实现重试机制
6. **数据验证**：客户端应验证服务器响应的数据完整性

## 🔄 版本兼容性

- **协议版本**：v1.0
- **Qt版本**：支持Qt 5.12+和Qt 6.0+
- **向后兼容**：新版本保持与旧版本的协议兼容性

---

**注意**：本文档描述的API接口可能会在后续版本中有所调整，请关注项目的更新日志。 