#pragma once

#include "windows.h"
#include <string>
#include <iostream>

class filemapmem
{
public:
	filemapmem(const char* fname,const char* mname);
	filemapmem(filemapmem& fm) = delete;
	filemapmem(filemapmem&& fmtemp) = delete;
	~filemapmem();
public:
	HANDLE hFile;
	HANDLE hFileMapping;
	LPVOID pMapHandle;
};

