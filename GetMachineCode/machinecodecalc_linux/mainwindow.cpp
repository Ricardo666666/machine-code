#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <stdio.h>
#include <cstdint>
#include <cstring>
#include <cstdlib>
#include <cctype>
#include <string>
#include <sstream>
#include <algorithm>

#include <QtGlobal>
#include <QList>
#include <QNetworkInterface>

#include <boost/uuid/sha1.hpp>
#include <boost/algorithm/string.hpp>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    SetUpMainWindow();

    QObject::connect(ui->pushButton, SIGNAL(clicked()), this, SLOT(OnClicked()));
}

MainWindow::~MainWindow()
{
    delete ui;
}

// 获取mac地址列表 利用Qt NetWork模块
QStringList MainWindow::GetMacAddress()
{
    QString mac_address;
    QStringList mac_address_list;
    QList<QNetworkInterface> interface_list = QNetworkInterface::allInterfaces();

    foreach (QNetworkInterface iface, interface_list)
    {
        //保证获取的是本地IP地址, 活跃的网卡, 不是虚拟机，隧道 之类的网络地址
        if(iface.flags().testFlag(QNetworkInterface::IsUp) &&
                !iface.flags().testFlag(QNetworkInterface::IsLoopBack) &&
                !(iface.humanReadableName().contains(tr("VMware"),Qt::CaseInsensitive)) &&
                !(iface.humanReadableName().contains(tr("Tunnel"),Qt::CaseInsensitive)) &&
                !(iface.humanReadableName().contains(tr("Tunneling"),Qt::CaseInsensitive)) &&
                !(iface.humanReadableName().contains(tr("Pseudo"),Qt::CaseInsensitive)))
        {
            if(!(iface.hardwareAddress().isEmpty()))
            {
                //qDebug() << iface.humanReadableName();
                mac_address = iface.hardwareAddress().toUpper();
                mac_address_list.push_back(mac_address);
            }
        }
    }

    return mac_address_list;
}

// 获取cpu id 通过dmidecode指令查出 需要root权限
QString MainWindow::GetCpuId()
{
    FILE* p = popen("dmidecode -t 4 | grep ID", "r"); // 需要root权限才能执行

    if(p == NULL)
    {
        std::perror("popen failed!");
        std::exit(1);
    }

    const int MAXLINE = 256;
    char result_buf[MAXLINE] = {0};

    if(fgets(result_buf, MAXLINE, p) != NULL)
    {
        if(result_buf[std::strlen(result_buf) - 1] == '\n')
        {
            result_buf[std::strlen(result_buf) - 1] = '\0';
        }
    }

    int rc = pclose(p); // 关闭文件

    if(rc == -1)
    {
        std::perror("close file failed!");
        std::exit(1);
    }

    std::string str(result_buf);

    auto f = [](const unsigned char c)
    {
        return std::isspace(c);
    };

    str.erase(std::remove_if(str.begin(), str.end(), f), str.end());

    boost::algorithm::erase_all(str, "ID:");

    return QString(str.c_str());
}

// 计算机器码函数 采用sha1算法 生成160位的摘要
QString MainWindow::GetMachineCode()
{
    const QString& message = GetMachineCodeDigestMessage();
    const std::string& str = message.toStdString();
    const char* p_char = str.c_str();

    std::uint32_t digest[5] = {0};

    boost::uuids::detail::sha1 sha;

    sha.process_block(p_char, p_char + std::strlen(p_char));
    sha.get_digest(digest);

    std::stringstream ostring;

    for (int i = 0; i < 5; ++i)
    {
        ostring << std::hex << digest[i];
    }

    return QString(ostring.str().c_str());
}

// 把查询到的数据展示在主窗口中
void MainWindow::SetUpMainWindow()
{
    QStringList mac_list = GetMacAddress(); // 得到所有的本机mac地址 e.g. eth0 virbr0

    foreach (QString mac_str, mac_list) // 把查找到的mac地址展示在主界面上
    {
        ui->lineEdit->insert(mac_str);
        ui->lineEdit->insert("  ");
    }

    const QString& cpu_str = GetCpuId();

    ui->lineEdit_2->insert(cpu_str);

    ui->textBrowser->append(GetMachineCodeDigestMessage());
}

// 按钮点击的槽 用于计算机器码
void MainWindow::OnClicked()
{
    const QString& machine_code = GetMachineCode();
    ui->textBrowser->append(machine_code);
}

// 得到用于摘要的信息(就是根据这串字符串生成机器码)
QString MainWindow::GetMachineCodeDigestMessage()
{
    const QStringList& mac_list = GetMacAddress();

    QString message = GetCpuId();

    if(!(mac_list.isEmpty()))
    {
        foreach (QString str, mac_list)
        {
            message += str;
        }
    }

    return message;
}
