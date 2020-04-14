#include <QTcpSocket>
#include <unordered_map>
#include "CommonUtils.hpp"

#pragma once

class Gateway : public CommonUtils
{
    public:
        Gateway(const std::string &host, const short port, const short openPort);
        ~Gateway() = default;

        bool    registerToServer();

        void    receiveSMRegister(QTcpSocket *socket, const std::string &mid, const std::string &auth);
        void    receiveSMLogin(QTcpSocket *socket, const std::string &cU, const std::string &cid, const std::string &cL, const std::string &time);

        bool    runNAN();

    protected:
        std::string getMyId() const override;

    private:
        std::string host;
        short       port;
        short       myPort;

        std::string myRandom;
        std::string verifier;

        std::unordered_map<unsigned int, std::string>   hashNames;
};

