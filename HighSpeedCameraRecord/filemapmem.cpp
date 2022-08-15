#include "filemapmem.h"

// ����
filemapmem::filemapmem(const char* fname, const char* mname)
{
	// �����ļ��ں˶��󣬳ɹ�����������fh
	hFile = CreateFile(
		fname,                          // ָ���ļ�����ָ��
		GENERIC_READ | GENERIC_WRITE,   // ����ģʽ��д / ����
		FILE_SHARE_READ,                // ����ģʽ,�ɹ��� 
		NULL,                           // ָ��ȫ���Ե�ָ��
		//OPEN_EXISTING,                // ��δ������ļ�������ڣ����豸���Ҫ��
		OPEN_ALWAYS,                    // ����ļ��������򴴽���
		FILE_ATTRIBUTE_NORMAL,          // �ļ����ԣ�Ĭ������
		NULL);                          // ���ڸ����ļ���� 

	if (hFile == INVALID_HANDLE_VALUE)
	{
		std::cout << "Error   in   CreateFile   ! " << std::endl;
	}

	// �����ļ�ӳ���ں˶��󣬾��������MapFileH
	hFileMapping = CreateFileMapping(
		hFile,                           // �����ļ����
		NULL,                            // ��ȫ����
		PAGE_READWRITE,                  // ��������
		0,                               // ��λ�ļ���С,4G �ߵ������������0x100000000 ->4G
		0x40000000,                      // ��λ�ļ���С,0x40000000Ϊ1G 
		mname);                          // �����ڴ�����

    // �������ɹ�
	if (hFileMapping == NULL)
	{
		std::cout << "Error   in   CreateFileMapping! " << std::endl;
	}

	// ���ļ�����ӳ�䵽���̵ĵ�ַ�ռ� 
	pMapHandle = MapViewOfFile(
		hFileMapping,                   // �ļ�ӳ����
		FILE_MAP_ALL_ACCESS,            // ӳ���ļ������ݷ��ʷ�ʽ��Ҫ�������CreateFileMapping���õı�������һ��
		0,                              // ��ʾ�ļ�ӳ����ʼƫ�Ƶĸ�32λ
		0,                              // ��ʾ�ļ�ӳ����ʼƫ�Ƶĵ�32λ
		0);                             // �ļ�Ҫӳ����ֽ�����Ϊ0��ʾӳ������ӳ���ļ�
//
	if (pMapHandle == NULL)
	{
		std::cout << "Error   in   MapViewOfFile! " << std::endl;
	}
}


// ����
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