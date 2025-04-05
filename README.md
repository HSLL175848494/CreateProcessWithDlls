# HSLL::CreateProcessWithDlls - 进程创建与DLL注入工具类

## 功能概述
本工具类用于创建挂起状态的进程并注入多个DLL，支持32/64位进程。主要特性：
- 以挂起状态创建目标进程
- 通过远程线程注入多个DLL
- 支持向目标进程内存写入数据
- 完善的错误代码系统

## 使用方法

### 基本流程
1. 实例化类对象（自动创建挂起进程）
2. 调用`LoadDlls`注入DLL
3. 调用`ResumeProcess`恢复进程执行

### 头文件依赖
- Windows SDK
- \<windows.h\>
- \<tlhelp32.h\>

---

## 示例代码
```cpp
#include "CreateProcessWithDlls.h"

int main() {
    // 1. 创建进程（示例路径需替换为实际路径）
    HSLL::CreateProcessWithDlls creator("C:\\target.exe");
    
    if (creator.errorCode != 0) {
        printf("创建进程失败: %s\n", creator.GetErrorInfo());
        return 1;
    }

    // 2. 注入DLL
    const char* dlls[] = { 
        "C:\\inject1.dll",
        "D:\\path\\inject2.dll"
    };
    
    if (!creator.LoadDlls(dlls, 2)) {
        printf("注入失败: %s\n", creator.GetErrorInfo());
        return 1;
    }

    // 3. 恢复进程执行
    if (!creator.ResumeProcess()) {
        printf("恢复失败: %s\n", creator.GetErrorInfo());
        return 1;
    }

    return 0;
}
```

---

## 主要接口说明

### 构造函数
```cpp
CreateProcessWithDlls(LPCSTR processPath)
```
- 参数：目标进程的完整路径
- 自动以CREATE_SUSPENDED方式创建进程

### DLL注入方法
```cpp
BOOL LoadDlls(LPCSTR* dlls, DWORD num)
```
- 参数：
  - dlls: DLL路径数组
  - num: DLL数量
- 注意：必须在使用ResumeProcess前调用

### 进程控制
```cpp
BOOL ResumeProcess()
```
- 恢复挂起的进程执行
- 必须在所有注入操作完成后调用

### 内存写入
```cpp
BOOL WriteMemory(DWORD64 offset, BYTE* code, DWORD size)
```
- 参数：
  - offset: 相对于进程基址的偏移量
  - code: 要写入的数据指针
  - size: 数据大小

### 错误处理
```cpp
LPCSTR GetErrorInfo()
```
- 返回最后一次操作的错误描述

---

## 注意事项
1. 单实例限制：
   - LoadDlls/ResumeProcess 每个对象只能调用一次
   - 需要多次注入时请创建新实例

2. 架构匹配：
   - 需确保注入DLL与目标进程架构（32/64位）一致
   - 编译时需正确配置目标平台（x86/x64）

3. 路径规范：
   - 使用完整绝对路径
   - 推荐使用转义双反斜杠（`C:\\path\\file.dll`）

4. 生命周期：
   - 对象析构时会自动关闭进程/线程句柄
   - 禁止复制/移动类实例

---

## 许可信息
自由使用于合法场景，禁止用于恶意软件开发。因滥用造成的法律责任自行承担。
```
