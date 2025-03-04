#include"CreateProcessWithDlls.hpp"
#include<iostream>

int main()
{
	//�������̲���ͣ
	HSLL::CreateProcessWithDlls tool("Sakura.exe");

	//�жϽ����Ƿ񴴽��ɹ�,tool.errorCode>0��ʾ���һ�ε��÷����˴���
	if (tool.errorCode)
	{
		std::cout << tool.GetErrorInfo() << std::endl;
		return -1;
	}

	//�����ڹ������ɹ����޸��ڴ�
	BYTE code[4] = { 0xa,0xb,0xc,0xd };
	if (!tool.WriteMemory(0x584, code, 4))
	{
		std::cout << tool.GetErrorInfo() << std::endl;
		return -1;
	}

	//����dlls
	LPCSTR dlls[3] = { "test1.dll","test2.dll","test3.dll" };
	if (!tool.LoadDlls(dlls, 3))
	{
		std::cout << tool.GetErrorInfo() << std::endl;
		return -1;
	}

	//�ָ����߳�
	if (tool.ResumeProcess())
	{
		std::cout << tool.GetErrorInfo() << std::endl;
		return -1;
	}

	return 0;
}

