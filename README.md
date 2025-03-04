Example Explanation
For detailed examples, please refer to the file sample.cpp.

Note:
When the LoadDlls function is called, the main thread of the process is resumed for a while to allow the loading of kernel32.dll. This is done to locate the address of the LoadLibraryA function in the current process.

Therefore, if you call the WriteMemory function to modify the program's memory after this point, you may miss the optimal opportunity for modification (i.e., the modified memory might never be used). It is therefore recommended to complete all memory modifications before calling LoadDlls.
