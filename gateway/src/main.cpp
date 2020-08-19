#include <iostream>
#include "Gateway.h"
#include "QTimings.h"

int main()
{
    Gateway nan("127.0.0.1", 3874, 4542);

    QTimings::getShared().start("registration");

    if (!nan.registerToServer())
    {
        QTimings::getShared().stop("registration");
        std::cerr << "Stopping program, could not register to main server" << std::endl;
        std::cout << QTimings::getShared().getPPTimings() << std::endl;
        QTimings::getShared().reset();
        return 2;
    }
    QTimings::getShared().stop("registration");
    std::cout << QTimings::getShared().getPPTimings() << std::endl;
    QTimings::getShared().reset();

    nan.runNAN();
}
