#include "chatwindow.h"
#include "ui_chatwindow.h"
#include "utils.h"
#include <QMessageBox>
#include <QDateTime>

extern DataPool g_dataPool;

ChatWindow::ChatWindow(UserProfile me, FriendProfile friendInfo, QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::ChatWindow),
    fileState(StateFileAborted)
{
    ui->setupUi(this);
    showButtons(true, false, false, false);

    setUserInfo(me);
    setFriendInfo(friendInfo);

    inputBox = new InputBox();
    connect(inputBox, SIGNAL(returnPressed()), this, SLOT(on_pushButtonSend_clicked()));
    inputBox->setMaximumHeight(80);
    ui->verticalLayoutInput->addWidget(inputBox);

    QString num;
    for(int i = 12; i <= 24; i+=2)
    {
        num.setNum(i);
        ui->comboBoxFontSize->addItem(num);
    }

    connect(&g_dataPool, SIGNAL(newMessage(quint32,QString)),
            this, SLOT(newMessage(quint32,QString)));
    connect(&g_dataPool, SIGNAL(fileRequest(quint32,QString,quint64)),
            this, SLOT(fileRequest(quint32,QString,quint64)));
    connect(&g_dataPool, SIGNAL(connectionAborted(quint32,Connection::ConnectionType)),
            this, SLOT(connectionAborted(quint32,Connection::ConnectionType)));
    connect(&g_dataPool, SIGNAL(fileRequestResult(quint32,bool)),
            this, SLOT(fileRequestResult(quint32,bool)));
    connect(&g_dataPool, SIGNAL(fileDataReady(quint32,QByteArray)),
            this, SLOT(fileDataReady(quint32,QByteArray)));
    connect(&g_dataPool, SIGNAL(fileReceived(quint32)),
            this, SLOT(fileReceived(quint32)));
    connect(&fileSendingTimer, SIGNAL(timeout()), this, SLOT(fileSendData()));
}

ChatWindow::~ChatWindow()
{
    delete ui;
}

void ChatWindow::fileSendData()
{
    if(sendingFile.atEnd())
    {
        fileState = StateFileSent;
        g_dataPool.finishSendFile(friendInfo.account);
        fileSendingTimer.stop();
        sendingFile.close();
        ui->labelFileState->setText("发送完毕");
        showButtons(true, false, false, false);
        return;
    }
    g_dataPool.sendFileData(friendInfo.account, sendingFile.read(1024L * 1024L));
}

void ChatWindow::fileReceived(quint32 peerUID)
{
    if(peerUID != friendInfo.account)
        return;

    fileState = StateFileReceived;
    receivedFile.close();
    g_dataPool.abortSendFile(peerUID);
}

void ChatWindow::connectionAborted(quint32 peerUID, Connection::ConnectionType type)
{
    if(peerUID != friendInfo.account)
        return;
    switch(type)
    {
    case Connection::message_connection:
        break;

    case Connection::file_connection:
        sendingFile.close();
        receivedFile.close();
        switch(fileState)
        {
        case StateFileAborted:
            ui->labelFileState->setText("已取消");
            break;

        case StateFileRejected:
            ui->labelFileState->setText("已拒绝");
            break;

        case StateFileSent:
            ui->labelFileState->setText("发送成功");
            break;

        case StateFileReceived:
            ui->labelFileState->setText("接收完成");
            break;
        }
        showButtons(true, false, false, false);
        fileState = StateFileAborted;
        break;

    case Connection::audio_connection:
        break;

    default:
        break;
    }
}

void ChatWindow::setUserInfo(UserProfile info)
{
    me = info;
}

void ChatWindow::setFriendInfo(FriendProfile info)
{
    friendInfo = info;
    if(friendInfo.displayName.isEmpty())
        this->setWindowTitle(friendInfo.nickName);
    else
        this->setWindowTitle(friendInfo.displayName + " (" + friendInfo.nickName + ")");
}

void ChatWindow::showButtons(bool chooseFile, bool cancel, bool accept, bool reject)
{
    ui->pushButtonChooseFile->setVisible(chooseFile);
    ui->pushButtonFileCancel->setVisible(cancel);
    ui->pushButtonAcceptFile->setVisible(accept);
    ui->pushButtonRejectFile->setVisible(reject);
}

void ChatWindow::fileRequest(quint32 peerUID, QString fileName, quint64 fileSize)
{
    if(peerUID != friendInfo.account)
        return;

    this->fileName = fileName;
    ui->labelFileName->setText(fileName);
    ui->labelFileSize->setText(getFileSizeAsString(fileSize));
    ui->labelFileState->setText("对方发送文件");
    showButtons(false, false, true, true);
}

QString ChatWindow::getFileSizeAsString(qint64 size)
{
    double d;
    QString unit;

    if(size < 1024)
    {
        d = size;
        unit = "B";
    }
    else if(size < 1024L * 1024L)
    {
        d = (double)size / 1024.0;
        unit = "KB";
    }
    else if(size < 1024L * 1024L * 1024L)
    {
        d = (double)size / 1024.0 / 1024.0;
        unit = "MB";
    }
    else
    {
        d = (double)size / 1024.0 / 1024.0 / 1024.0;
        unit = "GB";
    }
    return QString("%1 %2").arg(d, 0, 'f', 2).arg(unit);
}

void ChatWindow::on_pushButtonChooseFile_clicked()
{
    QString path;

    path = QFileDialog::getOpenFileName(this, "选择文件");
    if(path.isNull())
        return;

    sendingFile.setFileName(path);
    if(!sendingFile.open(QFile::ReadOnly))
    {
        QMessageBox::warning(this, "提示", QString("无法打开文件 %1").arg(path));
        return;
    }

    QFileInfo info(path);

    ui->labelFileName->setText(info.fileName());
    ui->labelFileSize->setText(getFileSizeAsString(info.size()));
    ui->labelFileState->setText("等待对方接收");
    if(!establishConnection(Connection::file_connection, path))
    {
        QMessageBox::warning(this, "提示", "对方可能不在线，发送文件失败");
        ui->labelFileState->setText("无传输");
        return;
    }
    showButtons(false, true, false, false);
}

void ChatWindow::on_toolButtonClear_clicked()
{
    ui->textEditChatMsg->clear();
}

void ChatWindow::on_fontComboBox_currentFontChanged(const QFont &f)
{
    QFont font;

    font = inputBox->font();
    font.setFamily(f.family());
    inputBox->setFont(font);
    inputBox->setFocus();
}

void ChatWindow::on_pushButtonClose_clicked()
{
    this->close();
}

void ChatWindow::on_comboBoxFontSize_currentIndexChanged(const QString &size)
{
    QFont font;

    font = inputBox->font();
    font.setPointSize(size.toInt());
    inputBox->setFont(font);
    inputBox->setFocus();
}

bool ChatWindow::establishConnection(enum Connection::ConnectionType type, QString path)
{
    AddrInfo addr;
    QTcpSocket *socket;

    if(g_dataPool.isConnected(friendInfo.account, type))
        return true;
    if(!api.getIpAndPort(Utils::getInstance()->getTcpSocket(), friendInfo.account, addr))
        return false;
    socket = new QTcpSocket();
    socket->connectToHost(addr.ipAddr, addr.port);
    if(!socket->waitForConnected(5000))
        return false;
    switch(type)
    {
    case Connection::message_connection:
        g_dataPool.setMessageConnection(socket, me.account, friendInfo.account);
        break;

    case Connection::file_connection:
        g_dataPool.setFileConnection(socket, me.account, path, friendInfo.account);
        break;

    case Connection::audio_connection:
        g_dataPool.setAudioConnection(socket, me.account, friendInfo.account);
        break;

    default:
        break;
    }
    return true;
}

void ChatWindow::on_pushButtonSend_clicked()
{
    QString header;
    QString text = inputBox->toHtml();

    header = QString("<span style='font-size:11pt; color:#0000ff;'>%1</span>");
    header = header.arg(Qt::escape(me.name + " "
                        + QDateTime::currentDateTime().toString("yyyy/MM/dd hh:mm:ss")));
    if(!inputBox->toPlainText().isEmpty())
    {
        ui->textEditChatMsg->append(header);
        ui->textEditChatMsg->append(text);
        inputBox->clear();
        if(!establishConnection(Connection::message_connection))
        {
            QMessageBox::warning(this, "提示", "对方可能不在线，无法发送消息");
            return;
        }
        g_dataPool.sendMessage(friendInfo.account, text);
    }
}

void ChatWindow::newMessage(quint32 peerUID, QString msg)
{
    if(peerUID != friendInfo.account)
        return;

    QString header;
    QString name;

    if(friendInfo.displayName.isEmpty())
        name = friendInfo.nickName;
    else
        name = friendInfo.displayName;

    header = QString("<span style='font-size:11pt; color:#00aa00;'>%1</span>");
    header = header.arg(Qt::escape(name + " "
                        + QDateTime::currentDateTime().toString("yyyy/MM/dd hh:mm:ss")));
    ui->textEditChatMsg->append(header);
    ui->textEditChatMsg->append(msg);
}

void ChatWindow::on_toolButtonBold_toggled(bool checked)
{
    QFont font;

    font = inputBox->font();
    font.setBold(checked);
    inputBox->setFont(font);
    inputBox->setFocus();
}

void ChatWindow::on_toolButtonItalic_toggled(bool checked)
{
    QFont font;

    font = inputBox->font();
    font.setItalic(checked);
    inputBox->setFont(font);
    inputBox->setFocus();
}

void ChatWindow::closeEvent(QCloseEvent *e)
{
    inputBox->clear();
    e->accept();
}

void ChatWindow::showEvent(QShowEvent *e)
{
    inputBox->setFocus();
    e->accept();
}

void ChatWindow::on_pushButtonFileCancel_clicked()
{
    fileState = StateFileAborted;
    g_dataPool.abortSendFile(friendInfo.account);
}

void ChatWindow::on_pushButtonRejectFile_clicked()
{
    fileState = StateFileRejected;
    ui->labelFileState->setText("已拒绝接收");
    showButtons(true, false, false, false);
    g_dataPool.sendFileRequestResult(friendInfo.account, false);
    g_dataPool.abortSendFile(friendInfo.account);
}

void ChatWindow::fileRequestResult(quint32 peerUID, bool accepted)
{
    if(peerUID != friendInfo.account)
        return;

    if(accepted)
    {
        ui->labelFileState->setText("传输中");
        showButtons(false, true, false, false);
        fileSendingTimer.setInterval(1000);
        fileSendingTimer.start();
    }
    else
    {
        fileState = StateFileRejected;
        ui->labelFileState->setText("对方拒绝接收文件");
        showButtons(true, false, false, false);
        QMessageBox::information(this, "提示", QString("%1 拒绝接收").arg(friendInfo.nickName));
    }
}

void ChatWindow::on_pushButtonAcceptFile_clicked()
{
    QString path;

    path = QFileDialog::getSaveFileName(this, "保存文件", fileName);
    if(path.isEmpty())
    {
        this->on_pushButtonRejectFile_clicked();
        return;
    }
    receivedFile.setFileName(path);
    if(!receivedFile.open(QFile::WriteOnly | QFile::Truncate))
    {
        QMessageBox::warning(this, "提示", QString("无法创建文件 %1").arg(path));
        this->on_pushButtonRejectFile_clicked();
        return;
    }
    ui->labelFileState->setText("传输中");
    showButtons(false, true, false, false);
    g_dataPool.sendFileRequestResult(friendInfo.account, true);
}

void ChatWindow::fileDataReady(quint32 peerUID, QByteArray data)
{
    if(peerUID != friendInfo.account)
        return;

    receivedFile.write(data);
    receivedFile.flush();
}
