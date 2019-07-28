#pragma once

class Foo
{
public:
	virtual void HelloWorld();
};

#if defined(BUILDING_DLL)
extern "C" __declspec(dllexport) Foo* CreateFoo();
extern "C" __declspec(dllexport) int* GetIntAddress();
#else
extern "C" __declspec(dllimport) Foo* CreateFoo();
extern "C" __declspec(dllimport) int* GetIntAddress();
#endif
