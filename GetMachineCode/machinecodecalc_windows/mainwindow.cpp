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

	QObject::connect(ui->pushButton, SIGNAL(clicked()), this, SLOT(OnClicked())); // ������ť�ͼ���������
}

MainWindow::~MainWindow()
{
	delete ui;
}

// ��ȡmac��ַ�б� ����Qt�ṩ������ģ�����mac��ַ�Ĳ���
QStringList MainWindow::GetMacAddress()
{
	QString mac_address;
	QStringList mac_address_list;
	QList<QNetworkInterface> interface_list = QNetworkInterface::allInterfaces();

	foreach (QNetworkInterface iface, interface_list)
	{
		//��֤��ȡ���Ǳ���IP��ַ, ��Ծ������, �������������� ֮��������ַ
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

// ��ȡcpu id �ڲ���������dosָ��ĺ�����ɲ���
QString MainWindow::GetCpuId()
{
	char output[256] = {0};

	GetCpuIdByCmd(output); 

	return QString(output);
}

// ��ȡ�������к� ͨ��VMI���
QString MainWindow::GetBaseBoardSn()
{
	const QString& board_str = GetWMIInfo(2);

	return board_str;
}

// ��ȡBIOS���к� ͨ��VMI���
QString MainWindow::GetBiosSn()
{
	const QString& bios_str = GetWMIInfo(4);

	return bios_str;
}

// ��ȡӲ�����к� ͨ��VMI���
QString MainWindow::GetDiskSn()
{
	const QString& disk_str = GetWMIInfo(1);

	return disk_str;
}

// ��ȡΨһ������ ����sha1�㷨 ����160λ��ժҪ
QString MainWindow::GetMachineCode()
{
	const QString& message = GetMachineCodeDigestMessage(); // ����ժҪ����Ϣ
	const std::string& str = message.toStdString(); // �����QStringֱ��תΪchar*����ֲ���'\0'��β�����
	const char* p_char = str.c_str();

	std::uint32_t digest[5] = {0}; // ���ժҪ������

	boost::uuids::detail::sha1 sha;

	sha.process_block(p_char, p_char + std::strlen(p_char)); //��sha1�㷨����ժҪ
	sha.get_digest(digest); // ��ȡժҪ

	std::stringstream ostring;

	for (int i = 0; i < 5; ++i)
	{
		ostring << std::hex << digest[i]; // ��16���Ƶ��ַ�����ʽ��ʾ������
	}

	return QString(ostring.str().c_str());
}

// �Ѳ�ѯ��������չʾ����������
void MainWindow::SetUpMainWindow()
{
	QStringList mac_list = GetMacAddress(); // �õ����еı���mac��ַ e.g. ��������

	foreach (QString mac_str, mac_list) // �Ѳ��ҵ���mac��ַչʾ����������
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

	ui->textBrowser->append(GetMachineCodeDigestMessage()); // չʾ���ڼ���ժҪ����Ϣ
	ui->textBrowser->append(tr("\n�����ť��������� :"));
}

// ��ť����Ĳۺ��� ���ڼ��������
void MainWindow::OnClicked()
{
	const QString& machine_code = GetMachineCode();
	ui->textBrowser->append(machine_code);
}

// ִ�л�ȡcpu id��dosָ������ȡcpu id
BOOL MainWindow::GetCpuIdByCmd(char* lpszCpu)
{
	const long MAX_COMMAND_SIZE = 10000; // ��������������С

#ifdef UNICODE
	WCHAR szFetCmd[] = L"wmic cpu get processorid"; // ��ȡCPU���к�������
#else
	CHAR szFetCmd[] = "wmic cpu get processorid";
#endif

	const std::string strEnSearch = "ProcessorId"; // CPU���кŵ�ǰ����Ϣ

	BOOL bret = FALSE;
	HANDLE hReadPipe = NULL; //��ȡ�ܵ�
	HANDLE hWritePipe = NULL; //д��ܵ�
	PROCESS_INFORMATION pi;   //������Ϣ
	STARTUPINFO	si;	  //���������д�����Ϣ
	SECURITY_ATTRIBUTES sa;   //��ȫ����

	char szBuffer[MAX_COMMAND_SIZE + 1] = { 0 }; // ���������н�������������
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

	// �����ܵ�
	bret = CreatePipe(&hReadPipe, &hWritePipe, &sa, 0);

	if (!bret)
	{
		goto END;
	}

	// ���������д��ڵ���ϢΪָ���Ķ�д�ܵ�
	GetStartupInfo(&si);

	si.hStdError = hWritePipe;
	si.hStdOutput = hWritePipe;
	si.wShowWindow = SW_HIDE; //���������д���
	si.dwFlags = STARTF_USESHOWWINDOW | STARTF_USESTDHANDLES;

	// ������ȡ�����еĽ���
	bret = CreateProcess(NULL, szFetCmd, NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi);

	if (!bret)
	{
		goto END;
	}

	// ��ȡ���ص�����
	WaitForSingleObject(pi.hProcess, 500/*INFINITE*/);
	bret = ReadFile(hReadPipe, szBuffer, MAX_COMMAND_SIZE, &count, 0);

	if (!bret)
	{
		goto END;
	}

	// ����CPU���к�
	bret = FALSE;
	strBuffer = szBuffer;
	ipos = strBuffer.find(strEnSearch);

	if (ipos < 0) // û���ҵ�
	{
		goto END;
	}
	else
	{
		strBuffer = strBuffer.substr(ipos + strEnSearch.length());
	}

	memset(szBuffer, 0x00, sizeof(szBuffer));
	strcpy_s(szBuffer, strBuffer.c_str());

	// ȥ���հ��ַ�
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
	// �ر����еľ��
	CloseHandle(hWritePipe);
	CloseHandle(hReadPipe);
	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);

	return bret;
}

// ͨ��VMI(windows����淶)��ѯӲ������Ϣ
QString MainWindow::GetWMIInfo(int type)
{
	/*
	0.��ǰԭ��������ַ��
	SELECT MACAddress  FROM Win32_NetworkAdapter WHERE (MACAddress IS NOT NULL) AND (NOT (PNPDeviceID LIKE 'ROOT%'))
	1.Ӳ�����к�
	SELECT PNPDeviceID FROM Win32_DiskDrive WHERE(SerialNumber IS NOT NULL) AND (MediaType LIKE 'Fixed hard disk%')
	2.��ȡ�������к�
	SELECT SerialNumber FROM Win32_BaseBoard WHERE (SerialNumber IS NOT NULL)
	3.������ID
	SELECT ProcessorId  FROM Win32_Processor WHERE (ProcessorId IS NOT NULL)
	4.BIOS���к�
	SELECT SerialNumber  FROM Win32_BIOS WHERE (SerialNumber IS NOT NULL)
	5.�����ͺ�
	SELECT Product FROM Win32_BaseBoard WHERE (Product IS NOT NULL)
	*/

	//ע��qt����wmiʱ���Բ�ѯ���Ҫ����ϸ����� like ֮��ľ�����ؾ�ȷ�����н������

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

	//��Ҫinclude <windows.h>
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

// �õ�����ժҪ����Ϣ(���Ǹ����⴮�ַ������ɻ�����)
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