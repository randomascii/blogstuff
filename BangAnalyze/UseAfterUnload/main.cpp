#include "class.h"

#include <stdio.h>
#include <Windows.h>

int main(int argc, const char* argv[])
{
	if (argc < 2)
	{
		printf("Usage: %s -export|-virtual|-global\n", argv[0]);
		return 0;
	}
	auto* arg = argv[1];

	auto dll = LoadLibrary("class.dll");
	if (!dll)
	{
		printf("Couldn't load DLL. Exiting.\n");
		return 0;
	}
	auto* getHello = reinterpret_cast<decltype(&CreateFoo)>(GetProcAddress(dll, "CreateFoo"));
	if (!getHello)
	{
		printf("Couldn't find import. Exiting.\n");
		return 0;
	}

	auto* getInt = reinterpret_cast<decltype(&GetIntAddress)>(GetProcAddress(dll, "GetIntAddress"));
	if (!getInt)
	{
		printf("Couldn't find import. Exiting.\n");
		return 0;
	}

	if (strcmp(arg, "-export") == 0)
	{
		printf("Calling exported function.\n");
		(void)getHello();
		printf("Unloading DLL.\n");
		FreeLibrary(dll);
		printf("Calling exported function.\n");
		(void)getHello();
	}
	else if (strcmp(arg, "-virtual") == 0)
	{
		auto* hello = getHello();
		printf("Calling virtual function.\n");
		hello->HelloWorld();
		printf("Unloading DLL.\n");
		FreeLibrary(dll);
		printf("Calling virtual function.\n");
		hello->HelloWorld();
	}
	else if (strcmp(arg, "-global") == 0)
	{
		int* value = getInt();
		printf("Incrementing value.\n");
		*value += 1;
		printf("Unloading DLL.\n");
		FreeLibrary(dll);
		printf("Incrementing value.\n");
		*value += 1;
	}
	else
	{
		printf("Unrecognized argument '%s'\n", arg);
	}
}
