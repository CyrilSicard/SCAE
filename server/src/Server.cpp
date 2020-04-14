#include <string>
#include <sstream>
#include <iostream>
#include <future>
#include <QTcpSocket>
#include <QByteArray>
#include <QTcpServer>
#include "Server.h"

Server::Server(short port) : CommonUtils(16, 80), port(port)
{}

std::string Server::getMyId() const
{
    return "ServerSID928462";
}

static  std::string readInput()
{
    std::string answer;
    std::cin >> answer;
    return answer;
}

bool    Server::runServer()
{
    QTcpServer  server;

    if (!server.listen(QHostAddress::Any, this->port))
    {
        std::cerr << "Could not start Server: " << server.errorString().toStdString() << std::endl;
        return false;
    }

    std::string prevInput;
    std::cin >> prevInput;

    std::future<std::string> *consoleInput = new std::future<std::string>(std::async(readInput));
    std::atomic<bool> serverIsOn(true);
    bool timeOutCheck = false;

    while (serverIsOn && server.isListening())
    {
        if (consoleInput->wait_for(std::chrono::milliseconds(2)) == std::future_status::ready)
        {
            prevInput = consoleInput->get();
            delete consoleInput;
            if (prevInput.compare("stop") == 0 || prevInput.compare("end") == 0)
            {
                serverIsOn = false;
                consoleInput = nullptr;
            }
            else
            {
                consoleInput = new std::future<std::string>(std::async(readInput));
            }
        }

        if (!server.waitForNewConnection(100, &timeOutCheck))
        {
            if (!timeOutCheck)
            {
                serverIsOn = false;
            }
            continue;
        }

        QTcpSocket  *connection = server.nextPendingConnection();
        if (connection == nullptr)
        {
            continue;
        }

        if (!connection->waitForReadyRead())
        {
            connection->close();
            continue;
        }

        QByteArray  message = connection->readAll();
        char type = message.front();

        QList<QByteArray> splitted;

        if (type >= '0' && type <= '9')
        {
            message.remove(0, 1);
            splitted = message.split(':');
        }

        switch (type)
        {
            case '1':
                if (splitted.size() != 2)
                {
                    connection->write("InvalidNumberOfArguments");
                    break;
                }
                this->receiveNANGRegister(connection, splitted[0].toStdString(), splitted[1].toStdString());
                break;

            case '2':
                if (splitted.size() != 8)
                {
                    connection->write("InvalidNumberOfArguments");
                    break;
                }
                this->receiveNANGLogin(connection, splitted[0].toStdString(), splitted[1].toStdString(), splitted[2].toStdString(), splitted[3].toStdString(),
                        splitted[4].toStdString(), splitted[5].toStdString(), splitted[6].toStdString(), splitted[7].toStdString());
                break;

            default:
                connection->write("WrongProtocol");
                break;
        }

        connection->flush();
        connection->close();
        connection = nullptr;
    }

    return true;
}

void    Server::receiveNANGRegister(QTcpSocket *socket, const std::string &nid, const std::string &auth)
{
    std::ostringstream oss;

    std::string e = this->newRandom();

    oss << this->getMyId() << e;

    std::string mI = this->hash(oss.str());

    oss.str("");
    oss.clear();

    oss << mI << auth;

    std::string vN = this->scalarMul(this->hash(oss.str()), 126);

    oss.str("");
    oss.clear();

    oss << vN << nid;

    std::string hN = this->hash(oss.str());

    quint32 ip = socket->peerAddress().toIPv4Address();

    this->hashNames.emplace(ip, hN);

    socket->write(QByteArray::fromStdString(vN).toBase64());
}

void    Server::receiveNANGLogin(QTcpSocket *socket, const std::string &cU, const std::string &cid, const std::string &cL, const std::string &smTime,
                                 const std::string &c2, const std::string &rid, const std::string &cN, const std::string &nangTime)
{
    std::string localTime = nullptr;

    std::string hN;
    try
    {
        hN = this->hashNames.at(socket->peerAddress().toIPv4Address());
    }
    catch (const std::out_of_range &e)
    {
        socket->write("IpAddressNotRegistered");
        return;
    }

    //TODO: check delta time

    std::string wP = this->applyXOr(cN, hN);

    std::ostringstream tmp;
    tmp << wP << smTime;

    std::string bi = this->applyXOr(this->hash(tmp.str()), cid);

    tmp.str("");
    tmp.clear();

    tmp << cN << hN;

    std::string bj = this->applyXOr(this->hash(tmp.str()), rid);

    tmp.str("");
    tmp.clear();

    tmp << cid << bi << wP;

    std::string c1_bis = this->hash(tmp.str());

    tmp.str("");
    tmp.clear();

    tmp << c1_bis << nangTime << cN << bj;

    std::string c2_bis = this->hash(tmp.str());

    if (c2_bis.compare(c2) != 0)
    {
        socket->write("WrongID");
        return;
    }

    std::string y = this->newRandom();

    std::string yP = this->scalarMul(y, 256);

    std::string cS = this->applyXOr(yP, this->hash(hN));

    tmp.str("");
    tmp.clear();

    tmp << yP << wP << bi << bj;

    std::string SKs = this->hash(tmp.str());

    tmp.str("");
    tmp.clear();

    tmp << SKs << localTime << yP;

    std::string c3 = this->hash(tmp.str());

    QByteArray  output;

    output.append('2');
    output.append(QByteArray::fromStdString(c3).toBase64());
    output.append(':');
    output.append(QByteArray::fromStdString(cS).toBase64());
    output.append(':');
    output.append(QByteArray::fromStdString(localTime).toBase64());

    socket->write(output);
}
