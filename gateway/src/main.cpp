#include <iostream>
#include "Gateway.h"

int main()
{
    Gateway nan("localhost", 3874, 4542);

    if (!nan.registerToServer())
    {
        std::cerr << "Stopping program, could not register to main server" << std::endl;
        return 2;
    }

    nan.runNAN();
}
