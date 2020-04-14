#include <QTcpSocket>
#include "CommonUtils.hpp"

#pragma once

class Client : public CommonUtils
{
    public:
        Client(const std::string &host, const short port);
        ~Client() = default;

        bool    registerToNAN();
        bool    loginToNAN();

    protected:
        std::string getMyId() const override;

    private:
        std::string host;
        short       port;

        std::string myRandom;
        std::string verifier;
};

