#include"CreateProcessWithDlls.hpp"
#include<iostream>

int main()
{
	//创建进程并暂停
	HSLL::CreateProcessWithDlls tool("Sakura.exe");

	//判断进程是否创建成功,tool.errorCode!=0表示最近一次调用发生了错误
	if (tool.errorCode)
	{
		std::cout << tool.GetErrorInfo() << std::endl;
		return -1;
	}

	//可以在构造对象成功后修改内存
	BYTE code[4] = { 0xa,0xb,0xc,0xd };
	if (!tool.WriteMemory(0x584, code, 4))
	{
		std::cout << tool.GetErrorInfo() << std::endl;
		return -1;
	}

	//加载dlls
	LPCSTR dlls[3] = { "test1.dll","test2.dll","test3.dll" };
	if (!tool.LoadDlls(dlls, 3))
	{
		std::cout << tool.GetErrorInfo() << std::endl;
		return -1;
	}

	//恢复主线程
	if (tool.ResumeProcess())
	{
		std::cout << tool.GetErrorInfo() << std::endl;
		return -1;
	}

	return 0;
}

