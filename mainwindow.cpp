/*
 *   mainwindow.cpp: main window
 * 
 *   Copyright (C) 2017  Zhengyu Peng, https://zpeng.me
 *
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QDebug>
#include <QMessageBox>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent),
                                          ui(new Ui::MainWindow)
{
    restoreWindowState();
    ui->setupUi(this);
    initUI();
    findLocalIPs();
    loadSettings();

    /******
     * UDP
     ******/
    if (myudp == nullptr)
    {
        myudp = new MyUDP;
    }
    connect(ui->button_UdpSend, SIGNAL(clicked()), this, SLOT(onUdpSendMessage()));         // UDP can send message directly without connection
    connect(ui->lineEdit_UdpSend, SIGNAL(returnPressed()), this, SLOT(onUdpSendMessage())); // UDP can send message directly without connection

    connect(ui->comboBox_Interface, SIGNAL(currentIndexChanged(int)), this, SLOT(ComboxPortSelect(int)));

    // buttons
    connect(ui->button_TcpClient, SIGNAL(clicked()), this, SLOT(onTcpClientButtonClicked()));
    connect(ui->button_TcpServer, SIGNAL(clicked()), this, SLOT(onTcpServerButtonClicked()));
    connect(ui->button_Udp, SIGNAL(clicked()), this, SLOT(onUdpButtonClicked()));
    connect(ui->button_Refresh, SIGNAL(clicked()), this, SLOT(onRefreshButtonClicked()));
}

/******************************************************************************
 ******************************************************************************
 **
 ** TCP Client
 **
 ******************************************************************************
 ******************************************************************************/

/***********************************
 *
 * TCP client start button clicked
 *
 ***********************************/
void MainWindow::onTcpClientButtonClicked()
{
    disconnect(ui->button_TcpClient, SIGNAL(clicked()), this, SLOT(onTcpClientButtonClicked()));

    if (setupConnection(TCPCLIENT))
    {
        ui->statusBar->showMessage(messageTCP + "连接到 " + tcpClientTargetAddr.toString() + ": " + QString::number(tcpClientTargetPort), 0);
        ui->lineEdit_TcpClientTargetIP->setDisabled(true);
        ui->lineEdit_TcpClientTargetPort->setDisabled(true);
        ui->button_TcpClient->setText("停止");

        connect(ui->button_TcpClient, SIGNAL(clicked()), this, SLOT(onTcpClientStopButtonClicked()));
        connect(mytcpclient, SIGNAL(myClientConnected(QString, quint16)), this, SLOT(onTcpClientNewConnection(QString, quint16)));
        connect(mytcpclient, SIGNAL(connectionFailed()), this, SLOT(onTcpClientTimeOut()));
    }

    saveSettings();
}

/***********************************
 *
 * TCP client has a new connection
 *
 ***********************************/
void MainWindow::onTcpClientNewConnection(const QString &from, quint16 port)
{
    disconnect(mytcpclient, SIGNAL(myClientConnected(QString, quint16)), this, SLOT(onTcpClientNewConnection(QString, quint16)));
    disconnect(mytcpclient, SIGNAL(connectionFailed()), this, SLOT(onTcpClientTimeOut()));
    disconnect(ui->button_TcpClient, SIGNAL(clicked()), this, SLOT(onTcpClientStopButtonClicked()));
    connect(mytcpclient, SIGNAL(myClientDisconnected()), this, SLOT(onTcpClientDisconnected()));

    ui->button_TcpClient->setDisabled(false);
    ui->button_TcpClient->setText("断开");

    ui->button_TcpClientSend->setDisabled(false);
    ui->lineEdit_TcpClientSend->setDisabled(false);
    ui->textBrowser_TcpClientMessage->setDisabled(false);

    ui->statusBar->showMessage(messageTCP + "连接到 " + from + ": " + QString::number(port), 0);
    connect(ui->button_TcpClient, SIGNAL(clicked()), this, SLOT(onTcpClientDisconnectButtonClicked()));

    connect(mytcpclient, SIGNAL(newMessage(QString, QString)), this, SLOT(onTcpClientAppendMessage(QString, QString)));
    connect(ui->button_TcpClientSend, SIGNAL(clicked()), this, SLOT(onTcpClientSendMessage()));
    connect(ui->lineEdit_TcpClientSend, SIGNAL(returnPressed()), this, SLOT(onTcpClientSendMessage()));
}

/***********************************
 *
 * TCP client stop button clicked
 *
 ***********************************/
void MainWindow::onTcpClientStopButtonClicked()
{
    disconnect(ui->button_TcpClient, SIGNAL(clicked()), this, SLOT(onTcpClientStopButtonClicked()));

    ui->statusBar->showMessage(messageTCP + "停止", 2000);
    disconnect(mytcpclient, SIGNAL(myClientConnected(QString, quint16)), this, SLOT(onTcpClientNewConnection(QString, quint16)));
    disconnect(mytcpclient, SIGNAL(connectionFailed()), this, SLOT(onTcpClientTimeOut()));
    ui->button_TcpClient->setText("连接");
    mytcpclient->abortConnection();
    ui->lineEdit_TcpClientTargetIP->setDisabled(false);
    ui->lineEdit_TcpClientTargetPort->setDisabled(false);

    ui->button_TcpClientSend->setDisabled(true);
    ui->lineEdit_TcpClientSend->setDisabled(true);
    ui->textBrowser_TcpClientMessage->setDisabled(true);

    connect(ui->button_TcpClient, SIGNAL(clicked()), this, SLOT(onTcpClientButtonClicked()));
}

/***********************************
 *
 * TCP client connection time out
 *
 ***********************************/
void MainWindow::onTcpClientTimeOut()
{
    ui->statusBar->showMessage(messageTCP + "连接超时", 2000);
    disconnect(ui->button_TcpClient, SIGNAL(clicked()), this, SLOT(onTcpClientStopButtonClicked()));
    disconnect(mytcpclient, SIGNAL(myClientConnected(QString, quint16)), this, SLOT(onTcpClientNewConnection(QString, quint16)));
    disconnect(mytcpclient, SIGNAL(connectionFailed()), this, SLOT(onTcpClientTimeOut()));

    ui->button_TcpClient->setText("连接");
    ui->lineEdit_TcpClientTargetIP->setDisabled(false);
    ui->lineEdit_TcpClientTargetPort->setDisabled(false);

    mytcpclient->closeClient();
    connect(ui->button_TcpClient, SIGNAL(clicked()), this, SLOT(onTcpClientButtonClicked()));
}

/***********************************
 *
 * TCP client diconnect button clicked
 *
 ***********************************/
void MainWindow::onTcpClientDisconnectButtonClicked()
{
    mytcpclient->disconnectCurrentConnection();
}

/***********************************
 *
 * TCP client disconnected
 *
 ***********************************/
void MainWindow::onTcpClientDisconnected()
{
    ui->statusBar->showMessage(messageTCP + "与服务器断开连接", 2000);
    disconnect(mytcpclient, SIGNAL(myClientDisconnected()), this, SLOT(onTcpClientDisconnected()));
    disconnect(mytcpclient, SIGNAL(newMessage(QString, QString)), this, SLOT(onTcpClientAppendMessage(QString, QString)));
    disconnect(ui->button_TcpClientSend, SIGNAL(clicked()), this, SLOT(onTcpClientSendMessage()));
    disconnect(ui->lineEdit_TcpClientSend, SIGNAL(returnPressed()), this, SLOT(onTcpClientSendMessage()));
    disconnect(ui->button_TcpClient, SIGNAL(clicked()), this, SLOT(onTcpClientDisconnectButtonClicked()));
    ui->button_TcpClient->setText("连接");

    ui->button_TcpClientSend->setDisabled(true);
    ui->lineEdit_TcpClientSend->setDisabled(true);
    ui->textBrowser_TcpClientMessage->setDisabled(true);

    ui->button_TcpClient->setDisabled(false);
    ui->lineEdit_TcpClientTargetIP->setDisabled(false);
    ui->lineEdit_TcpClientTargetPort->setDisabled(false);

    mytcpclient->closeClient();
    mytcpclient->close();

    connect(ui->button_TcpClient, SIGNAL(clicked()), this, SLOT(onTcpClientButtonClicked()));
}

/***********************************
 *
 * TCP client append a message
 * to message browser
 *
 ***********************************/
void MainWindow::onTcpClientAppendMessage(const QString &from, const QString &message)
{
    if (from.isEmpty() || message.isEmpty())
    {
        return;
    }

    QTextCursor cursor(ui->textBrowser_TcpClientMessage->textCursor());
    cursor.movePosition(QTextCursor::End);

    if (from == "System")
    {
        QColor color = ui->textBrowser_TcpClientMessage->textColor();
        ui->textBrowser_TcpClientMessage->setTextColor(Qt::gray);
        ui->textBrowser_TcpClientMessage->append(message);
        ui->textBrowser_TcpClientMessage->setTextColor(color);
    }
    else
    {
        QTextTable *table = cursor.insertTable(1, 2, tableFormat);
        table->cellAt(0, 0).firstCursorPosition().insertText('<' + from + "> ");
        table->cellAt(0, 1).firstCursorPosition().insertText(message);
    }
    QScrollBar *bar = ui->textBrowser_TcpClientMessage->verticalScrollBar();
    bar->setValue(bar->maximum());
}

/***********************************
 *
 * Send message through TCP client
 *
 ***********************************/
void MainWindow::onTcpClientSendMessage()
{
    QString text = ui->lineEdit_TcpClientSend->text();
    if (text.isEmpty())
    {
        return;
    }

    mytcpclient->sendMessage(text);

    onTcpClientAppendMessage("Me", text);
    ui->lineEdit_TcpClientSend->clear();
}

/******************************************************************************
 ******************************************************************************
 **
 ** TCP Server
 **
 ******************************************************************************
 ******************************************************************************/

/***********************************
 *
 * TCP server listen button clicked
 *
 ***********************************/
void MainWindow::onTcpServerButtonClicked()
{
    disconnect(ui->button_TcpServer, SIGNAL(clicked()), this, SLOT(onTcpServerButtonClicked()));

    if (setupConnection(TCPSERVER))
    {
        ui->statusBar->showMessage(messageTCP + "监听 " + localAddr.toString() + ": " + QString::number(tcpServerListenPort), 0);
        connect(ui->button_TcpServer, SIGNAL(clicked()), this, SLOT(onTcpServerStopButtonClicked()));
        ui->button_TcpServer->setText("停止");
        ui->lineEdit_TcpServerListenPort->setDisabled(true);
        connect(mytcpserver, SIGNAL(myServerConnected(QString, quint16)), this, SLOT(onTcpServerNewConnection(QString, quint16)));
    }
    else
    {
        ui->statusBar->showMessage(messageTCP + "Failed to listen to: " + localAddr.toString() + ": " + QString::number(tcpServerListenPort), 2000);
        connect(ui->button_TcpServer, SIGNAL(clicked()), this, SLOT(onTcpServerButtonClicked()));
    }

    saveSettings();
}

/***********************************
 *
 * TCP server has a new connection
 *
 ***********************************/
void MainWindow::onTcpServerNewConnection(const QString &from, quint16 port)
{
    ui->statusBar->showMessage(messageTCP + "Connected with " + from + ": " + QString::number(port), 0);
    disconnect(mytcpserver, SIGNAL(myServerConnected(QString, quint16)), this, SLOT(onTcpServerNewConnection(QString, quint16)));

    disconnect(ui->button_TcpServer, SIGNAL(clicked()), this, SLOT(onTcpServerStopButtonClicked()));
    ui->button_TcpServer->setText("断开");
    connect(ui->button_TcpServer, SIGNAL(clicked()), this, SLOT(onTcpServerDisconnectButtonClicked()));

    connect(mytcpserver, SIGNAL(myServerDisconnected()), this, SLOT(onTcpServerDisconnected()));
    connect(mytcpserver, SIGNAL(newMessage(QString, QString)), this, SLOT(onTcpServerAppendMessage(QString, QString)));

    ui->button_TcpServerSend->setDisabled(false);
    ui->lineEdit_TcpServerSend->setDisabled(false);
    ui->textBrowser_TcpServerMessage->setDisabled(false);
    connect(ui->button_TcpServerSend, SIGNAL(clicked()), this, SLOT(onTcpServerSendMessage()));
    connect(ui->lineEdit_TcpServerSend, SIGNAL(returnPressed()), this, SLOT(onTcpServerSendMessage()));
}

/***********************************
 *
 * TCP server stop button clicked
 *
 ***********************************/
void MainWindow::onTcpServerStopButtonClicked()
{
    disconnect(ui->button_TcpServer, SIGNAL(clicked()), this, SLOT(onTcpServerStopButtonClicked()));

    ui->statusBar->showMessage(messageTCP + "Stopped", 2000);
    disconnect(mytcpserver, SIGNAL(myServerConnected(QString, quint16)));
    mytcpserver->stopListening();
    ui->button_TcpServer->setText("开始");
    ui->lineEdit_TcpServerListenPort->setDisabled(false);

    ui->button_TcpServerSend->setDisabled(true);
    ui->lineEdit_TcpServerSend->setDisabled(true);
    ui->textBrowser_TcpServerMessage->setDisabled(true);

    connect(ui->button_TcpServer, SIGNAL(clicked()), this, SLOT(onTcpServerButtonClicked()));
}

/***********************************
 *
 * TCP server disconnect button clicked
 *
 ***********************************/
void MainWindow::onTcpServerDisconnectButtonClicked()
{
    mytcpserver->stopConnection();
}

/***********************************
 *
 * TCP server disconnected
 *
 ***********************************/
void MainWindow::onTcpServerDisconnected()
{
    ui->statusBar->showMessage(messageTCP + "Client disconnected, listerning to " + localAddr.toString() + ": " + QString::number(tcpServerListenPort), 0);

    ui->button_TcpServerSend->setDisabled(true);
    ui->lineEdit_TcpServerSend->setDisabled(true);
    ui->textBrowser_TcpServerMessage->setDisabled(true);

    disconnect(ui->button_TcpServer, SIGNAL(clicked()), this, SLOT(onTcpServerDisconnectButtonClicked()));
    disconnect(mytcpserver, SIGNAL(myServerDisconnected()), this, SLOT(onTcpServerDisconnected()));
    disconnect(mytcpserver, SIGNAL(newMessage(QString, QString)), this, SLOT(onTcpServerAppendMessage(QString, QString)));
    disconnect(ui->button_TcpServerSend, SIGNAL(clicked()), this, SLOT(onTcpServerSendMessage()));
    disconnect(ui->lineEdit_TcpServerSend, SIGNAL(returnPressed()), this, SLOT(onTcpServerSendMessage()));

    connect(ui->button_TcpServer, SIGNAL(clicked()), this, SLOT(onTcpServerStopButtonClicked()));
    ui->button_TcpServer->setText("停止");
    connect(mytcpserver, SIGNAL(myServerConnected(QString, quint16)), this, SLOT(onTcpServerNewConnection(QString, quint16)));
}

/***********************************
 *
 * TCP server append a message
 * to message browser
 *
 ***********************************/
void MainWindow::onTcpServerAppendMessage(const QString &from, const QString &message)
{
    if (from.isEmpty() || message.isEmpty())
    {
        return;
    }

    QTextCursor cursor(ui->textBrowser_TcpServerMessage->textCursor());
    cursor.movePosition(QTextCursor::End);

    if (from == "System")
    {
        QColor color = ui->textBrowser_TcpServerMessage->textColor();
        ui->textBrowser_TcpServerMessage->setTextColor(Qt::gray);
        ui->textBrowser_TcpServerMessage->append(message);
        ui->textBrowser_TcpServerMessage->setTextColor(color);
    }
    else
    {
        QTextTable *table = cursor.insertTable(1, 2, tableFormat);
        table->cellAt(0, 0).firstCursorPosition().insertText('<' + from + "> ");
        table->cellAt(0, 1).firstCursorPosition().insertText(message);
    }
    QScrollBar *bar = ui->textBrowser_TcpServerMessage->verticalScrollBar();
    bar->setValue(bar->maximum());
}

/***********************************
 *
 * Send message through TCP server
 *
 ***********************************/
void MainWindow::onTcpServerSendMessage()
{
    QString text = ui->lineEdit_TcpServerSend->text();
    if (text.isEmpty())
    {
        return;
    }

    mytcpserver->sendMessage(text);

    onTcpServerAppendMessage("Me", text);
    ui->lineEdit_TcpServerSend->clear();
}

/******************************************************************************
 ******************************************************************************
 **
 ** UDP
 **
 ******************************************************************************
 ******************************************************************************/

/***********************************
 *
 * UDP listen button clicked
 *
 ***********************************/
void MainWindow::onUdpButtonClicked()
{
    disconnect(ui->button_Udp, SIGNAL(clicked()), this, SLOT(onUdpButtonClicked()));

    if (setupConnection(UDPSERVER))
    {
        ui->statusBar->showMessage(messageUDP + "监听 " + localAddr.toString() + ": " + QString::number(udpListenPort), 0);
        connect(ui->button_Udp, SIGNAL(clicked()), this, SLOT(onUdpStopButtonClicked()));
        ui->button_Udp->setText("停止");

        ui->button_UdpSend->setDisabled(false);
        ui->lineEdit_UdpSend->setDisabled(false);
        ui->textBrowser_UdpMessage->setDisabled(false);

        ui->lineEdit_UdpListenPort->setDisabled(true);
        connect(myudp, SIGNAL(newMessage(QString, QString)), this, SLOT(onUdpAppendMessage(QString, QString)));
    }
    else
    {
        ui->statusBar->showMessage(messageUDP + "监听失败: " + localAddr.toString() + ": " + QString::number(udpListenPort), 2000);
        connect(ui->button_Udp, SIGNAL(clicked()), this, SLOT(onUdpButtonClicked()));
    }

    saveSettings();
}

/***********************************
 *
 * UDP stop button clicked
 *
 ***********************************/
void MainWindow::onUdpStopButtonClicked()
{
    disconnect(ui->button_Udp, SIGNAL(clicked()), this, SLOT(onUdpStopButtonClicked()));

    ui->statusBar->showMessage(messageUDP + "停止了", 2000);
    disconnect(myudp, SIGNAL(newMessage(QString, QString)), this, SLOT(onUdpAppendMessage(QString, QString)));
    ui->button_Udp->setText("开始");
    myudp->unbindPort();
    ui->lineEdit_UdpListenPort->setDisabled(false);

    connect(ui->button_Udp, SIGNAL(clicked()), this, SLOT(onUdpButtonClicked()));
}

/***********************************
 *
 * UDP append a message
 * to message browser
 *
 ***********************************/
void MainWindow::onUdpAppendMessage(const QString &from, const QString &message)
{
    //if (from.isEmpty() || message.isEmpty())
    //{
    //    return;
    //}

    QTextCursor cursor(ui->textBrowser_UdpMessage->textCursor());
    cursor.movePosition(QTextCursor::End);

    if (from == "System")
    {
        QColor color = ui->textBrowser_UdpMessage->textColor();
        ui->textBrowser_UdpMessage->setTextColor(Qt::gray);
        ui->textBrowser_UdpMessage->append(message);
        ui->textBrowser_UdpMessage->setTextColor(color);
    }
    else
    {
        QTextTable *table = cursor.insertTable(1, 2, tableFormat);
        table->cellAt(0, 0).firstCursorPosition().insertText('<' + from + "> ");
        table->cellAt(0, 1).firstCursorPosition().insertText(message);
    }
    QScrollBar *bar = ui->textBrowser_UdpMessage->verticalScrollBar();
    bar->setValue(bar->maximum());
}

/***********************************
 *
 * Send message through UDP
 *
 ***********************************/
void MainWindow::onUdpSendMessage()
{
    QString text = ui->lineEdit_UdpSend->text();
    if (text.isEmpty())
    {
        return;
    }

    udpTargetAddr.setAddress(ui->lineEdit_UdpTargetIP->text());
    udpTargetPort = ui->lineEdit_UdpTargetPort->text().toInt();
    myudp->sendMessage(udpTargetAddr, udpTargetPort, text);

    onUdpAppendMessage("Me", text);
    ui->lineEdit_UdpSend->clear();
}

/******************************************************************************
 ******************************************************************************/

/******************************************************************************
 ******************************************************************************/

/***********************************
 *
 * UI initialization
 *
 ***********************************/
void MainWindow::initUI()
{
    QString rule = "(?:[0-1]?[0-9]?[0-9]|2[0-4][0-9]|25[0-5])";
    ui->lineEdit_TcpClientTargetIP->setValidator(new QRegExpValidator(QRegExp("^" + rule + "\\." + rule + "\\." + rule + "\\." + rule + "$"), this));
    ui->lineEdit_UdpTargetIP->setValidator(new QRegExpValidator(QRegExp("^" + rule + "\\." + rule + "\\." + rule + "\\." + rule + "$"), this));
    ui->lineEdit_TcpClientTargetPort->setValidator(new QIntValidator(0, 65535, this));
    ui->lineEdit_TcpServerListenPort->setValidator(new QIntValidator(0, 65535, this));
    ui->lineEdit_UdpListenPort->setValidator(new QIntValidator(0, 65535, this));

    ui->textBrowser_TcpClientMessage->setFocusPolicy(Qt::NoFocus);
    ui->textBrowser_TcpServerMessage->setFocusPolicy(Qt::NoFocus);

    ui->lineEdit_TcpClientSend->setFocusPolicy(Qt::StrongFocus);
    ui->lineEdit_TcpClientSend->setFocus();

    ui->button_TcpClientSend->setDisabled(true);
    ui->lineEdit_TcpClientSend->setDisabled(true);
    ui->textBrowser_TcpClientMessage->setDisabled(true);

    ui->button_TcpServerSend->setDisabled(true);
    ui->lineEdit_TcpServerSend->setDisabled(true);
    ui->textBrowser_TcpServerMessage->setDisabled(true);

    tableFormat.setBorder(0);

}

/***********************************
 *
 * Setup connections
 *
 ***********************************/
bool MainWindow::setupConnection(quint8 type)
{
    bool isSuccess = false;
    localAddr.setAddress(ui->label_LocalIP->text());

    switch (type)
    {
    case TCPSERVER:
        tcpServerListenPort = ui->lineEdit_TcpServerListenPort->text().toInt();
        if (mytcpserver == nullptr)
        {
            mytcpserver = new MyTCPServer;
        }
        isSuccess = mytcpserver->listen(localAddr, tcpServerListenPort);
        break;
    case TCPCLIENT:
        isSuccess = true;
        tcpClientTargetAddr.setAddress(ui->lineEdit_TcpClientTargetIP->text());
        tcpClientTargetPort = ui->lineEdit_TcpClientTargetPort->text().toInt();
        if (mytcpclient == nullptr)
        {
            mytcpclient = new MyTCPClient;
        }
        mytcpclient->connectTo(tcpClientTargetAddr, tcpClientTargetPort);
        break;
    case UDPSERVER:
        udpListenPort = ui->lineEdit_UdpListenPort->text().toInt();
        isSuccess = myudp->bindPort(localAddr, udpListenPort);
        break;
    }
    return isSuccess;
}

/***********************************
 *
 * Find IP of local WiFi connections
 *
 ***********************************/
void MainWindow::findLocalIPs()
{
    ui->comboBox_Interface->clear();
    interfaceList.clear();
    auto list = QNetworkInterface::allInterfaces();
    foreach (QNetworkInterface interface, list) {
        // 1. 首先判断是不是以太网，过滤WiFi
//        if (interface.type() != QNetworkInterface::Ethernet)
//            continue;
        if (interface.humanReadableName().contains("Loopback"))
            continue;

        // 2. 如果有安装VMware虚拟机的话，会出现两个虚拟网卡
        // 匹配关键字"VMware"。来过滤虚拟网卡
//        if (interface.humanReadableName().contains("VMware"))
//            continue;

        // 3. 一般都会有两个ip地址，一个ipv4一个ipv6地址
        // 根据协议版本，来过滤掉ipv6地址
        foreach (auto entry ,interface.addressEntries()) {
            if (entry.ip().protocol() == QAbstractSocket::IPv4Protocol)
            {
                Port_IP tmp;
                tmp.ip = entry.ip().toString();
                tmp.name = interface.humanReadableName();
                interfaceList.push_back(tmp);
                ui->comboBox_Interface->addItem(tmp.name);
                continue;
            }
        }
    }
}

/***********************************
 *
 * Load settings from local configuration file
 *
 ***********************************/
void MainWindow::loadSettings()
{
    settingsFileDir = QApplication::applicationDirPath() + "/config.ini";
    QSettings settings(settingsFileDir, QSettings::IniFormat);
    ui->lineEdit_TcpClientTargetIP->setText(settings.value("TCP_CLIENT_TARGET_IP", "127.0.0.1").toString());
    ui->lineEdit_TcpClientTargetPort->setText(settings.value("TCP_CLIENT_TARGET_PORT", 1234).toString());

    ui->lineEdit_TcpServerListenPort->setText(settings.value("TCP_SERVER_LISTEN_PORT", 1234).toString());

    ui->lineEdit_UdpListenPort->setText(settings.value("UDP_LISTEN_PORT", 1234).toString());
    ui->lineEdit_UdpTargetIP->setText(settings.value("UDP_TARGET_IP", "127.0.0.1").toString());
    ui->lineEdit_UdpTargetPort->setText(settings.value("UDP_TARGET_PORT", 1234).toString());

    ui->tabWidget->setCurrentIndex(settings.value("TAB_INDEX", 0).toInt());

    int index = settings.value("interfaceIndex", 0).toInt();
    if (interfaceList.count() == 0) return;

    foreach (auto entry ,interfaceList) {
        if(ui->comboBox_Interface->currentText() == entry.name)
            ui->label_LocalIP->setText(entry.ip);
    }
}

void MainWindow::ComboxPortSelect(int index)
{
//    QMessageBox::information(this, "title", "your select item is " + text, NULL);

    foreach (auto entry ,interfaceList) {
        if(ui->comboBox_Interface->currentText() == entry.name)
            ui->label_LocalIP->setText(entry.ip);
    }
}

/***********************************
 *
 * Save settings to local configuration file
 *
 ***********************************/
void MainWindow::saveSettings()
{
    QSettings settings(settingsFileDir, QSettings::IniFormat);
    settings.setValue("TCP_CLIENT_TARGET_IP", ui->lineEdit_TcpClientTargetIP->text());
    settings.setValue("TCP_CLIENT_TARGET_PORT", ui->lineEdit_TcpClientTargetPort->text());

    settings.setValue("TCP_SERVER_LISTEN_PORT", ui->lineEdit_TcpServerListenPort->text());

    settings.setValue("UDP_LISTEN_PORT", ui->lineEdit_UdpListenPort->text());
    settings.setValue("UDP_TARGET_IP", ui->lineEdit_UdpTargetIP->text());
    settings.setValue("UDP_TARGET_PORT", ui->lineEdit_UdpTargetPort->text());

    settings.setValue("INTERFACE_INDEX", ui->comboBox_Interface->currentIndex());

    settings.setValue("TAB_INDEX", ui->tabWidget->currentIndex());
    settings.sync();
}

void MainWindow::onRefreshButtonClicked()
{
    saveSettings();
    findLocalIPs();
    loadSettings();
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    QSettings settings("ZPeng", "SocketTest");
    settings.setValue("geometry", saveGeometry());
    settings.setValue("windowState", saveState());
    QMainWindow::closeEvent(event);
}

void MainWindow::restoreWindowState()
{
    QSettings settings("ZPeng", "SocketTest");
    restoreGeometry(settings.value("geometry").toByteArray());
    restoreState(settings.value("windowState").toByteArray());
}

MainWindow::~MainWindow()
{
    delete ui;
}
