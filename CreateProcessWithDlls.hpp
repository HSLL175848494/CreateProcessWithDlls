#ifndef HSLL_CREATEPROCESSWITHDLLS
#define HSLL_CREATEPROCESSWITHDLLS

#include <windows.h>
#include <tlhelp32.h>

namespace HSLL
{
	class CreateProcessWithDlls
	{
		BOOL errorCode;
		DWORD codeSize;
		BYTE* binaryCode;
		DWORD64 imageBase;
		STARTUPINFOA startInfo{};
		PROCESS_INFORMATION processInfo{};

		static constexpr LPCSTR errorInfo[11] = {
		"未发生错误",
		"创建进程失败",
		"获取进程基址失败",
		"该函数不允许多次调用",
		"dlls不允许为nullptr而num不允许为0"
		"获取进程LoadLibraryA函数地址失败",
		"在目标进程中申请内存失败",
		"在目标进程中写入dll地址字符串失败",
		"修改内存属性失败",
		"创建远程线程失败",
		"写入指令失败"
		};


		static constexpr BYTE binaryCode32[38] = {
		0x53,               // push ebx
		0x56,               // push esi
		0x57,               // push edi
		0x8B, 0x44, 0x24, 0x10, // mov eax, [esp+10h] ; 获取参数地址
		0x8B, 0x18,         // mov ebx, [eax]        ; LoadLibraryA函数指针
		0x8B, 0x78, 0x08,   // mov edi, [eax+8]      ; 模块数量
		0x8D, 0x70, 0x0C,   // lea esi, [eax+0Ch]    ; 字符串数组起始地址
		0x85, 0xFF,         // test edi, edi         ; 检查模块数量是否为0
		0x74, 0x0F,         // je 跳出循环           ; 数量为0直接结束
		0x56,               // push esi              ; 压入字符串参数
		0xFF, 0xD3,         // call ebx              ; 调用LoadLibraryA
		0x46,               // inc esi               ; 跳到下一个字符
		0x80, 0x3E, 0x00,   // cmp byte ptr [esi], 0 ; 检查字符串结束符
		0x75, 0xFA,         // jne 回退到inc esi     ; 继续遍历字符串
		0x46,               // inc esi               ; 跳过null终止符
		0x83, 0xEF, 0x01,   // sub edi, 1            ; 模块计数器减1
		0x75, 0xED,         // jne 循环开始          ; 继续加载下一个模块
		0x5F,               // pop edi
		0x5E,               // pop esi
		0x5B,               // pop ebx
		0xC3                // ret
		};

		static constexpr BYTE binaryCode64[81] = {
		0x48, 0x89, 0x5C, 0x24, 0x10, // mov qword ptr [rsp+10h],rbx
		0x56,                         // push rsi
		0x48, 0x83, 0xEC, 0x20,       // sub rsp,20h
		0x8B, 0x41, 0x08,             // mov eax,dword ptr [rcx+8]
		0x48, 0x8D, 0x59, 0x0C,       // lea rbx,[rcx+0Ch]
		0x48, 0x8B, 0x31,             // mov rsi,qword ptr [rcx]
		0x85, 0xC0,                   //test eax,eax
		0x74, 0x2E,                   // je ...
		0x48, 0x89, 0x7C, 0x24, 0x30, // mov qword ptr [rsp+30h],rdi
		0x8B, 0xF8,                   // mov edi,eax
		0x90,                         // nop
		0x48, 0x8B, 0xCB,             // mov rcx,rbx
		0xFF, 0xD6,                   // call rsi
		0x80, 0x3B, 0x00,             // cmp byte ptr [rbx],0
		0x74, 0x0E,                   // je ...
		0x66, 0x0F, 0x1F, 0x44, 0x00, 0x00, // nop word ptr [rax+rax]
		0x48, 0xFF, 0xC3,             // inc rbx
		0x80, 0x3B, 0x00,             // cmp byte ptr [rbx],0
		0x75, 0xF8,                   // jne ...
		0x48, 0xFF, 0xC3,             // inc rbx
		0x48, 0x83, 0xEF, 0x01,       // sub rdi,1
		0x75, 0xDF,                   // jne ...
		0x48, 0x8B, 0x7C, 0x24, 0x30, // mov rdi,qword ptr [rsp+30h]
		0x48, 0x8B, 0x5C, 0x24, 0x38, // mov rbx,qword ptr [rsp+38h]
		0x48, 0x83, 0xC4, 0x20,       // add rsp,20h
		0x5E,                         // pop rsi
		0xC3                          // ret
		};

		BOOL GetProcessImageBase()
		{
			CONTEXT	read{};
			read.ContextFlags = CONTEXT_ALL;
			GetThreadContext(processInfo.hThread, &read);
#ifdef _WIN64
			if (!ReadProcessMemory(processInfo.hProcess, (LPCVOID)(read.Rdx + 0x10), &imageBase, 8, nullptr))
#else
			if (!ReadProcessMemory(processInfo.hProcess, (LPCVOID)(read.Ebx + 0x8), &imageBase, 4, nullptr))
#endif // _WIN32
				return FALSE;
			return TRUE;
		}

		LPVOID GetRemoteLoadLibraryA()
		{
			HMODULE hKernel32Local = GetModuleHandleA("kernel32.dll");
			if (!hKernel32Local)
				return nullptr;

			FARPROC pLoadLibraryA_Local = GetProcAddress(hKernel32Local, "LoadLibraryA");
			if (!pLoadLibraryA_Local)
				return nullptr;

			ptrdiff_t offset = (BYTE*)pLoadLibraryA_Local - (BYTE*)hKernel32Local;
			HMODULE hKernel32Remote = GetRemoteModuleHandle(GetProcessId(processInfo.hProcess), L"kernel32.dll");
			if (!hKernel32Remote)
				return nullptr;

			return (LPVOID)((BYTE*)hKernel32Remote + offset);
		}

		HMODULE GetRemoteModuleHandle(DWORD pid, LPCWSTR moduleName)
		{
			HANDLE hSnap;
			HMODULE hMod = 0;
			MODULEENTRY32W me{};
			me.dwSize = sizeof(me);

			ResumeThread(processInfo.hThread);

			while (true)
			{
				hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, pid);

				if (hSnap != INVALID_HANDLE_VALUE && Module32FirstW(hSnap, &me))
				{
					do
					{
						if (lstrcmpiW(me.szModule, moduleName) == 0 || lstrcmpiW(me.szExePath, moduleName) == 0)
						{
							hMod = me.hModule;
							break;
						}
					} while (Module32NextW(hSnap, &me));

					if (hMod)
						break;
				}
			}

			CloseHandle(hSnap);
			return hMod;
		}

	public:

		BOOL LoadDlls(LPCSTR* dlls, DWORD num)
		{
			static BOOL once = false;

			if (once)
			{
				errorCode = 3;
				return FALSE;
			}

			if (dlls == nullptr || num == 0)
			{
				errorCode = 4;
				return FALSE;
			}

			LPVOID address = GetRemoteLoadLibraryA();
			if (address == nullptr)
			{
				errorCode = 5;
				return FALSE;
			}

			DWORD allocSize = 12 + codeSize;

			for (DWORD i = 0; i < num; i++)
				allocSize += lstrlenA(dlls[i]) + 1;

			LPVOID remoteAddr = VirtualAllocEx(processInfo.hProcess, nullptr, allocSize, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
			if (!remoteAddr)
			{
				errorCode = 6;
				return FALSE;
			}

			BYTE* temp = new BYTE[allocSize];
			*(DWORD64*)temp = (DWORD64)address;
			*(DWORD*)(temp + 8) = num;

			BYTE* ptr = temp + 12;
			for (DWORD i = 0; i < num; i++)
			{
				DWORD length = lstrlenA(dlls[i]) + 1;
				memcpy(ptr, dlls[i], length);
				ptr += length;
			}

			memcpy(ptr, binaryCode, codeSize);

			if (!WriteProcessMemory(processInfo.hProcess, remoteAddr, temp, allocSize, nullptr))
			{
				VirtualFreeEx(processInfo.hProcess, remoteAddr, 0, MEM_RELEASE);
				errorCode = 7;
				return FALSE;
			}

			delete[] temp;

			DWORD OldProctect;
			if (!VirtualProtectEx(processInfo.hProcess,
				(LPVOID)((DWORD64)remoteAddr + allocSize - codeSize), codeSize, PAGE_EXECUTE, &OldProctect))
			{
				VirtualFreeEx(processInfo.hProcess, remoteAddr, 0, MEM_RELEASE);
				errorCode = 8;
				return FALSE;
			}

			HANDLE hRemoteThread = CreateRemoteThread(processInfo.hProcess, nullptr, 0,
				(LPTHREAD_START_ROUTINE)((DWORD64)remoteAddr + allocSize - codeSize), remoteAddr, 0, nullptr);

			if (!hRemoteThread)
			{
				VirtualFreeEx(processInfo.hProcess, remoteAddr, 0, MEM_RELEASE);
				errorCode = 9;
				return FALSE;
			}

			WaitForSingleObject(hRemoteThread, INFINITE);
			SuspendThread(processInfo.hThread);
			CloseHandle(hRemoteThread);
			VirtualFreeEx(processInfo.hProcess, remoteAddr, 0, MEM_RELEASE);

			errorCode = 0;
			once = true;
			return TRUE;
		}

		BOOL ResumeProcess()
		{
			static BOOL once = false;
			if (once)
			{
				errorCode = 3;
				return FALSE;
			}
			ResumeThread(processInfo.hThread);
			once = true;
			return TRUE;
		}

		BOOL WriteMemory(DWORD64 offset, BYTE* code, DWORD size)
		{
			if (!WriteProcessMemory(processInfo.hProcess, (LPVOID)(imageBase + offset), code, size, nullptr))
			{
				errorCode = 10;
				return FALSE;
			}
			return TRUE;
		}

		CreateProcessWithDlls(LPCSTR proCreateProcessWithDlls) : errorCode(0)
		{
			if (!CreateProcessA(proCreateProcessWithDlls, nullptr, nullptr, nullptr, FALSE, CREATE_SUSPENDED, nullptr, nullptr, &startInfo, &processInfo))
			{
				errorCode = 1;
				return;
			}

			if (!GetProcessImageBase())
				errorCode = 2;

#ifdef _WIN64
			binaryCode = (BYTE*)binaryCode64;
			codeSize = 81;

#else
			binaryCode = (BYTE*)binaryCode32;
			codeSize = 38;
#endif // _WIN32

		}

		CreateProcessWithDlls(const CreateProcessWithDlls&) = delete;
		CreateProcessWithDlls& operator=(const CreateProcessWithDlls&) = delete;
		CreateProcessWithDlls(CreateProcessWithDlls&&) = delete;
		CreateProcessWithDlls& operator=(CreateProcessWithDlls&&) = delete;
	};
}

#endif // !
