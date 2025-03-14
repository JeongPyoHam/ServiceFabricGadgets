#include <stdio.h>
#include <Windows.h>
#include <vector>

DWORD WINAPI ThreadCallback(LPVOID lpParam);

struct SumParameter
{
    int startIndex;
    int endIndex;
    std::vector<int> const * pValues;
    int sum;
};

// Q1: write a program that splits 'values' into N group and sum in each thread in parallel
// Q2: write a program that splits 'values' into N group and iterate to invoke sum thread
int main()
{
    printf("sum thread\r\n");

    std::vector<int> values(100);
    for (int i = 0; i < values.size(); ++i)
    {
        values[i] = i + 1;
    }

    SumParameter* pParam = new SumParameter{ 0, 99, &values, 0 };
    DWORD dwThreadId = 0;
    HANDLE hThread = INVALID_HANDLE_VALUE;

    hThread = CreateThread(
        NULL,   // LPSECURITY_ATTRIBUTES
        0,      // use default stack size
        ThreadCallback,
        (void*)pParam,
        0,      // use default creation flag
        &dwThreadId);

    if (hThread == NULL)
    {
        printf("CreateThread: %d", GetLastError());
        return -1;
    }

    WaitForSingleObject(hThread, INFINITE);

    CloseHandle(hThread);
    
    printf("Sum: %d\r\n", pParam->sum);

    delete pParam;

    return 0;
}

DWORD WINAPI ThreadCallback(LPVOID lpParam)
{
    if (lpParam == NULL)
    {
        return 1;
    }

    SumParameter* pSumParam = (SumParameter *) lpParam;

    if (pSumParam->startIndex > pSumParam->endIndex)
    {
        return 2;
    }

    int sum = 0;
    for (int i = pSumParam->startIndex; i <= pSumParam->endIndex; ++i)
    {
        sum += pSumParam->pValues->at(i);
    }
    pSumParam->sum = sum;

    return 0;
}