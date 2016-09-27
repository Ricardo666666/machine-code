#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <cstdint>
#include <cstring>
#include <string>
#include <iostream>
#include <sstream>

#include <QtGlobal>
#include <QUuid>
#include <QList>
#include <QAxObject>
#include <QNetworkInterface>

#include <boost/uuid/sha1.hpp>

MainWindow::MainWindow(QWidget *parent) :
	QMainWindow(parent),
	ui(new Ui::MainWindow)
{
	ui->setupUi(this);

	SetUpMainWindow();

	QObject::connect(ui->pushButton, SIGNAL(clicked()), this, SLOT(OnClicked())); // 关联按钮和计算机器码槽
}

MainWindow::~MainWindow()
{
	delete ui;
}

// 获取mac地址列表 利用Qt提供的网络模块完成mac地址的查找
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
				mac_address = iface.hardwareAddress().toUpper();
				mac_address_list.push_back(mac_address);
			}
		}
	}

	return mac_address_list;
}

// 获取cpu id 内部调用利用dos指令的函数完成查找
QString MainWindow::GetCpuId()
{
	char output[256] = {0};

	GetCpuIdByCmd(output); 

	return QString(output);
}

// 获取主板序列号 通过VMI查出
QString MainWindow::GetBaseBoardSn()
{
	const QString& board_str = GetWMIInfo(2);

	return board_str;
}

// 获取BIOS序列号 通过VMI查出
QString MainWindow::GetBiosSn()
{
	const QString& bios_str = GetWMIInfo(4);

	return bios_str;
}

// 获取硬盘序列号 通过VMI查出
QString MainWindow::GetDiskSn()
{
	const QString& disk_str = GetWMIInfo(1);

	return disk_str;
}

// 获取唯一机器码 采用sha1算法 生成160位的摘要
QString MainWindow::GetMachineCode()
{
	const QString& message = GetMachineCodeDigestMessage(); // 用于摘要的消息
	const std::string& str = message.toStdString(); // 如果用QString直接转为char*会出现不以'\0'结尾的情况
	const char* p_char = str.c_str();

	std::uint32_t digest[5] = {0}; // 存放摘要的数组

	boost::uuids::detail::sha1 sha;

	sha.process_block(p_char, p_char + std::strlen(p_char)); //用sha1算法计算摘要
	sha.get_digest(digest); // 获取摘要

	std::stringstream ostring;

	for (int i = 0; i < 5; ++i)
	{
		ostring << std::hex << digest[i]; // 以16进制的字符串形式表示机器码
	}

	return QString(ostring.str().c_str());
}

// 把查询到的数据展示在主窗口中
void MainWindow::SetUpMainWindow()
{
	QStringList mac_list = GetMacAddress(); // 得到所有的本机mac地址 e.g. 本地连接

	foreach (QString mac_str, mac_list) // 把查找到的mac地址展示在主界面上
	{
		ui->lineEdit->insert(mac_str);
		ui->lineEdit->insert("  ");
	}

	const QString& cpu_str = GetCpuId();

	ui->lineEdit_2->insert(cpu_str);

	const QString& board_str = GetBaseBoardSn();
	ui->lineEdit_3->insert(board_str);

	const QString& bios_str = GetBiosSn();
	ui->lineEdit_4->insert(bios_str);

	const QString& disk_str = GetDiskSn();
	ui->lineEdit_5->insert(disk_str);

	ui->textBrowser->append(GetMachineCodeDigestMessage()); // 展示用于计算摘要的信息
	ui->textBrowser->append(tr("\n点击按钮计算机器码 :"));
}

// 按钮点击的槽函数 用于计算机器码
void MainWindow::OnClicked()
{
	const QString& machine_code = GetMachineCode();
	ui->textBrowser->append(machine_code);
}

// 执行获取cpu id的dos指令来获取cpu id
BOOL MainWindow::GetCpuIdByCmd(char* lpszCpu)
{
	const long MAX_COMMAND_SIZE = 10000; // 命令行输出缓冲大小

#ifdef UNICODE
	WCHAR szFetCmd[] = L"wmic cpu get processorid"; // 获取CPU序列号命令行
#else
	CHAR szFetCmd[] = "wmic cpu get processorid";
#endif

	const std::string strEnSearch = "ProcessorId"; // CPU序列号的前导信息

	BOOL bret = FALSE;
	HANDLE hReadPipe = NULL; //读取管道
	HANDLE hWritePipe = NULL; //写入管道
	PROCESS_INFORMATION pi;   //进程信息
	STARTUPINFO	si;	  //控制命令行窗口信息
	SECURITY_ATTRIBUTES sa;   //安全属性

	char szBuffer[MAX_COMMAND_SIZE + 1] = { 0 }; // 放置命令行结果的输出缓冲区
	std::string	strBuffer;
	unsigned long count = 0;
	long ipos = 0;

	memset(&pi, 0, sizeof(pi));
	memset(&si, 0, sizeof(si));
	memset(&sa, 0, sizeof(sa));

	pi.hProcess = NULL;
	pi.hThread = NULL;
	si.cb = sizeof(STARTUPINFO);
	sa.nLength = sizeof(SECURITY_ATTRIBUTES);
	sa.lpSecurityDescriptor = NULL;
	sa.bInheritHandle = TRUE;

	// 创建管道
	bret = CreatePipe(&hReadPipe, &hWritePipe, &sa, 0);

	if (!bret)
	{
		goto END;
	}

	// 设置命令行窗口的信息为指定的读写管道
	GetStartupInfo(&si);

	si.hStdError = hWritePipe;
	si.hStdOutput = hWritePipe;
	si.wShowWindow = SW_HIDE; //隐藏命令行窗口
	si.dwFlags = STARTF_USESHOWWINDOW | STARTF_USESTDHANDLES;

	// 创建获取命令行的进程
	bret = CreateProcess(NULL, szFetCmd, NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi);

	if (!bret)
	{
		goto END;
	}

	// 读取返回的数据
	WaitForSingleObject(pi.hProcess, 500/*INFINITE*/);
	bret = ReadFile(hReadPipe, szBuffer, MAX_COMMAND_SIZE, &count, 0);

	if (!bret)
	{
		goto END;
	}

	// 查找CPU序列号
	bret = FALSE;
	strBuffer = szBuffer;
	ipos = strBuffer.find(strEnSearch);

	if (ipos < 0) // 没有找到
	{
		goto END;
	}
	else
	{
		strBuffer = strBuffer.substr(ipos + strEnSearch.length());
	}

	memset(szBuffer, 0x00, sizeof(szBuffer));
	strcpy_s(szBuffer, strBuffer.c_str());

	// 去掉空白字符
	int j = 0;
	for (size_t i = 0; i < strlen(szBuffer); i++)
	{
		if (szBuffer[i] != ' ' && szBuffer[i] != '\n' && szBuffer[i] != '\r')
		{
			lpszCpu[j] = szBuffer[i];
			j++;
		}
	}

	bret = TRUE;

END:
	// 关闭所有的句柄
	CloseHandle(hWritePipe);
	CloseHandle(hReadPipe);
	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);

	return bret;
}

// 通过VMI(windows管理规范)查询硬件的信息
QString MainWindow::GetWMIInfo(int type)
{
	/*
	0.当前原生网卡地址：
	SELECT MACAddress  FROM Win32_NetworkAdapter WHERE (MACAddress IS NOT NULL) AND (NOT (PNPDeviceID LIKE 'ROOT%'))
	1.硬盘序列号
	SELECT PNPDeviceID FROM Win32_DiskDrive WHERE(SerialNumber IS NOT NULL) AND (MediaType LIKE 'Fixed hard disk%')
	2.获取主板序列号
	SELECT SerialNumber FROM Win32_BaseBoard WHERE (SerialNumber IS NOT NULL)
	3.处理器ID
	SELECT ProcessorId  FROM Win32_Processor WHERE (ProcessorId IS NOT NULL)
	4.BIOS序列号
	SELECT SerialNumber  FROM Win32_BIOS WHERE (SerialNumber IS NOT NULL)
	5.主板型号
	SELECT Product FROM Win32_BaseBoard WHERE (Product IS NOT NULL)
	*/

	//注意qt调用wmi时，对查询语句要求很严格，所以 like 之类的句子务必精确才能有结果出来

	QString hwInfo;
	QStringList sqlCmd;

	sqlCmd << tr("SELECT MACAddress  FROM Win32_NetworkAdapter WHERE (MACAddress IS NOT NULL) AND (NOT (PNPDeviceID LIKE 'ROOT%'))");
	sqlCmd << tr("SELECT PNPDeviceID  FROM  Win32_DiskDrive WHERE( PNPDeviceID IS NOT NULL) AND (MediaType LIKE 'Fixed%')");
	sqlCmd << tr("SELECT SerialNumber FROM Win32_BaseBoard WHERE (SerialNumber IS NOT NULL)");
	sqlCmd << tr("SELECT ProcessorId  FROM Win32_Processor WHERE (ProcessorId IS NOT NULL)");
	sqlCmd << tr("SELECT SerialNumber  FROM Win32_BIOS WHERE (SerialNumber IS NOT NULL)");
	sqlCmd << tr("SELECT Product FROM Win32_BaseBoard WHERE (Product IS NOT NULL)");

	QStringList columnName;

	columnName << tr("MACAddress");
	columnName << tr("PNPDeviceID");
	columnName << tr("SerialNumber");
	columnName << tr("ProcessorId");
	columnName << tr("SerialNumber");
	columnName << tr("Product");

	QAxObject *objIWbemLocator = new QAxObject(tr("WbemScripting.SWbemLocator"));
	QAxObject *objWMIService = objIWbemLocator->querySubObject("ConnectServer(QString&,QString&)", QString("."), QString("root\\cimv2"));
	QString query;

	if(type < sqlCmd.size())
	{
		query=sqlCmd.at(type);
	}

	QAxObject *objInterList = objWMIService->querySubObject("ExecQuery(QString&))", query);
	QAxObject *enum1 = objInterList->querySubObject("_NewEnum");

	//需要include <windows.h>
	IEnumVARIANT* enumInterface = NULL;
	enum1->queryInterface(IID_IEnumVARIANT, (void**)&enumInterface);
	enumInterface->Reset();

	for (int i = 0; i < objInterList->dynamicCall("Count").toInt(); ++i)
	{
		VARIANT *theItem = (VARIANT*)malloc(sizeof(VARIANT));

		if (enumInterface->Next(1, theItem, NULL) != S_FALSE)
		{
			QAxObject *item = new QAxObject((IUnknown *)theItem->punkVal);

			if(item)
			{
				if(type < columnName.size())
				{
					QByteArray datagram(columnName.at(type).toAscii());
					const char* tempConstChar = datagram.data();
					qDebug() << "the query is " << query << " and cn is " << QString::fromAscii(tempConstChar);
					hwInfo = item->dynamicCall(tempConstChar).toString();
				}

				qDebug() <<" string is " << hwInfo;
			}
			else
			{
				qDebug() <<" item is null";
			}
		}
		else
		{
			qDebug() <<" item is false";
		}
	}

	return hwInfo;
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