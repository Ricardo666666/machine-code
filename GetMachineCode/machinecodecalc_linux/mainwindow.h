#ifndef MAINWINDOW_H
#define MAINWINDOW_H

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
    QString GetMachineCode(); // 得到本机唯一机器码

public:
    void SetUpMainWindow(); // 把查询到的数据展示到主界面

public slots:
    void OnClicked(); // 点击按钮的槽函数 用于计算 机器码

private:
    QString GetMachineCodeDigestMessage(); // 得到用于摘要的消息

private:
    Ui::MainWindow *ui;
};

#endif // MAINWINDOW_H
