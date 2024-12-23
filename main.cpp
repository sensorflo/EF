#include <cstdio>
#include <iostream>

#include <Windows.h>

int main()
{
    SetConsoleOutputCP(CP_UTF8);
    setvbuf(stdout, nullptr, _IOFBF, 1000); // https://stackoverflow.com/questions/45575863/how-to-print-utf-8-strings-to-stdcout-on-windows
    return 0;
}
