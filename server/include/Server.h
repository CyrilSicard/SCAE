#include <QTcpSocket>
#include <unordered_map>
#include "CommonUtils.hpp"

#pragma once

class Server : public CommonUtils
{
    public:
        Server(const short port);
        ~Server() = default;

        void    receiveNANGRegister(QTcpSocket *socket, const std::string &nid, const std::string &auth);
        void    receiveNANGLogin(QTcpSocket *socket, const std::string &cU, const std::string &cid, const std::string &c1, const std::string &smTime,
                                 const std::string &c2, const std::string &rid, const std::string &cN, const std::string &nangTime);

        bool    runServer();

    protected:
        std::string getMyId() const override;

    private:
        short       port;

        std::string myRandom;
        std::string verifier;

        std::unordered_map<unsigned int, std::string>   hashNames;
};

