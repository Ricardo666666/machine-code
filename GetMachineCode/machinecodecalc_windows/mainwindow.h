#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#ifdef WIN32
#include "Windows.h"
#endif

#include <QMainWindow>
#include <QString>
#include <QStringList>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

public:
    QStringList GetMacAddress(); // 获取本机mac地址
    QString GetCpuId(); // 获取cpu id
    QString GetBaseBoardSn(); // 获取主板序列号
    QString GetBiosSn(); // 获取BIOS序列号
    QString GetDiskSn(); // 获取磁盘序列号
    QString GetMachineCode(); // 得到本机唯一机器码

public:
    void SetUpMainWindow(); // 把查询到的数据展示到主界面

public slots:
    void OnClicked(); // 点击按钮的槽函数 用于计算 机器码

private:
    QString GetWMIInfo(int); // windows管理规范来查询多个硬件信息
    BOOL GetCpuIdByCmd(char*); // 通过dos指令查询cpu id
    QString GetMachineCodeDigestMessage(); // 得到用于摘要的消息

private:
    Ui::MainWindow *ui;
};

#endif // MAINWINDOW_H
