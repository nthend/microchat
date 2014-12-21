#include <cstdio>
#include <cstring>
#include <iostream>
#include <fstream>
#include <string>

#include "chatdaemon.hpp"
#include "chatdatabase.hpp"

int main ()
{
	printf("starting server ...\n");

	ChatDaemon daemon;

	printf("server started.\n");
	
	std::string com;
	while(com != "exit")
	{
		std::getline(std::cin,com);
	}
	
	printf("stopping server ...\n");
  
  return 0;
}
