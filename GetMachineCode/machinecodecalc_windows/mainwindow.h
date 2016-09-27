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
    QStringList GetMacAddress(); // ��ȡ����mac��ַ
    QString GetCpuId(); // ��ȡcpu id
    QString GetBaseBoardSn(); // ��ȡ�������к�
    QString GetBiosSn(); // ��ȡBIOS���к�
    QString GetDiskSn(); // ��ȡ�������к�
    QString GetMachineCode(); // �õ�����Ψһ������

public:
    void SetUpMainWindow(); // �Ѳ�ѯ��������չʾ��������

public slots:
    void OnClicked(); // �����ť�Ĳۺ��� ���ڼ��� ������

private:
    QString GetWMIInfo(int); // windows����淶����ѯ���Ӳ����Ϣ
    BOOL GetCpuIdByCmd(char*); // ͨ��dosָ���ѯcpu id
    QString GetMachineCodeDigestMessage(); // �õ�����ժҪ����Ϣ

private:
    Ui::MainWindow *ui;
};

#endif // MAINWINDOW_H
