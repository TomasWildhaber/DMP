#pragma once
#include <iostream>

#ifdef PLATFORM_WINDOWS

int Main(int argc, char** argv)
{
	std::cout << "Works!" << std::endl;

	std::cin.get();
	return 0;
}

#ifndef DISTRIBUTION_CONFIG
int main(int argc, char** argv)
{
	return Main(argc, argv);
}
#else
int WinMain(int argc, char** argv)
{
	return Main(argc, argv);
}
#endif

#endif