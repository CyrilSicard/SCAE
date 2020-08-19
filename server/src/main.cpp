#include <iostream>
#include "Server.h"

int main()
{
    std::cout << "Hello world!" << std::endl;

    Server serv(3874);

    serv.runServer();

    return 0;
}
