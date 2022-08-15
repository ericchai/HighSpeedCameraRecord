#pragma once

#include "windows.h"
#include <string>
#include <iostream>

class filemapmem
{
public:
	filemapmem(const char* fname,const char* mname);
	~filemapmem();
public:
	HANDLE hFile;
	HANDLE hFileMapping;
	LPVOID pMapHandle;

	//std::string fName;
	//std::string mName;
};

