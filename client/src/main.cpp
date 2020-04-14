#include "Client.h"

int main()
{
    Client cli("localhost", 4542);

    cli.registerToNAN();

    cli.loginToNAN();
}
