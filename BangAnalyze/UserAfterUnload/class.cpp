#define BUILDING_DLL
#include "class.h"

#include <stdio.h>

void Foo::HelloWorld()
{
	printf("Hello world!\n");
}

Foo* CreateFoo()
{
	return new Foo;
}

extern "C" int* GetIntAddress()
{
	static int value = 123456;
	return &value;
}
