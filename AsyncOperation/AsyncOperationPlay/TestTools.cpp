#include "stdafx.h"


#include "TestTools.h"

void PrintCurrentThreadId(wstring const& prefix)
{
    DWORD threadId = ::GetCurrentThreadId();
    std::wcout << prefix << L": " << L"0x" << std::hex << std::setw(8) << std::setfill(L'0') << threadId << endl;
}