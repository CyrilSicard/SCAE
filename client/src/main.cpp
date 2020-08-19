#include <iostream>
#include "Client.h"
#include "QTimings.h"

int main()
{
    Client cli("127.0.0.1", 4542);

    QTimings::getShared().start("register");

    if (!cli.registerToNAN())
    {
        std::cerr << "Could not register to NANG !" << std::endl;
        QTimings::getShared().stop("register");

        std::cout << QTimings::getShared().getPPTimings() << std::endl;
        QTimings::getShared().reset();
        return 0;
    }

    QTimings::getShared().stopAndStart("register", "login");

    if (cli.loginToNAN())
    {
        std::cout << "OKAY !!! Logged In !" << std::endl;
    }
    else
    {
        std::cerr << "NOPE !!! Login failed !" << std::endl;
    }

    QTimings::getShared().stop("login");

    std::cout << QTimings::getShared().getPPTimings() << std::endl;
    QTimings::getShared().reset();
}
