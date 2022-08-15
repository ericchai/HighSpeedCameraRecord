#include "filemapmem.h"

// 构造
filemapmem::filemapmem(const char* fname, const char* mname)
{
	// 创建文件内核对象，成功其句柄保存于fh
	hFile = CreateFile(
		fname,                          // 指向文件名的指针
		GENERIC_READ | GENERIC_WRITE,   // 访问模式（写 / 读）
		FILE_SHARE_READ,                // 共享模式,可共享 
		NULL,                           // 指向安全属性的指针
		//OPEN_EXISTING,                // 如何创建，文件必须存在，由设备提出要求
		OPEN_ALWAYS,                    // 如果文件不存在则创建它
		FILE_ATTRIBUTE_NORMAL,          // 文件属性，默认属性
		NULL);                          // 用于复制文件句柄 

	if (hFile == INVALID_HANDLE_VALUE)
	{
		std::cout << "Error   in   CreateFile   ! " << std::endl;
	}

	// 创建文件映射内核对象，句柄保存于MapFileH
	hFileMapping = CreateFileMapping(
		hFile,                           // 物理文件句柄
		NULL,                            // 安全设置
		PAGE_READWRITE,                  // 保护设置
		0,                               // 高位文件大小,4G 高低组合起来就是0x100000000 ->4G
		0x40000000,                      // 低位文件大小,0x40000000为1G 
		mname);                          // 共享内存名称

    // 创建不成功
	if (hFileMapping == NULL)
	{
		std::cout << "Error   in   CreateFileMapping! " << std::endl;
	}

	// 将文件数据映射到进程的地址空间 
	pMapHandle = MapViewOfFile(
		hFileMapping,                   // 文件映射句柄
		FILE_MAP_ALL_ACCESS,            // 映射文件的数据访问方式，要与上面的CreateFileMapping设置的保护属性一致
		0,                              // 表示文件映射起始偏移的高32位
		0,                              // 表示文件映射起始偏移的低32位
		0);                             // 文件要映射的字节数，为0表示映射整个映射文件
//
	if (pMapHandle == NULL)
	{
		std::cout << "Error   in   MapViewOfFile! " << std::endl;
	}
}


// 析构
filemapmem::~filemapmem()
{
	if (pMapHandle)
	{
		UnmapViewOfFile(pMapHandle);
		pMapHandle = nullptr;
	}
	if (hFileMapping)
	{
		CloseHandle(hFileMapping);
		pMapHandle = nullptr;
	}
	if (hFile)
	{
		CloseHandle(hFile);
		pMapHandle = nullptr;
	}
}