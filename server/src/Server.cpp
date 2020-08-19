#include <string>
#include <sstream>
#include <iostream>
#include <future>
#include <QTcpSocket>
#include <QByteArray>
#include <QTcpServer>
#include "Server.h"
#include "QTimings.h"

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

#ifdef PRINT_DEBUG
        std::cout << "A new connection appeared" << std::endl;
#endif

        QTimings::getShared().start("connection");

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


#ifdef PRINT_DEBUG
        std::cout << "Reading data:" << std::endl;
#endif
        QByteArray  message = connection->readAll();
        char type = message.front();
#ifdef PRINT_DEBUG
        std::cout << "     '" << message.toStdString() << "'" << std::endl;
#endif

        QList<QByteArray> splitted;

        if (type >= '0' && type <= '9')
        {
            message.remove(0, 1);
            splitted = message.split(':');
        }

        switch (type)
        {
            case '1':
                QTimings::getShared().start("register");
                if (splitted.size() != 2)
                {
                    connection->write("InvalidNumberOfArguments");
                    break;
                }
                this->receiveNANGRegister(connection, QByteArray::fromBase64(splitted[0]).toStdString(), QByteArray::fromBase64(splitted[1]).toStdString());
                QTimings::getShared().stop("register");
                std::cout << QTimings::getShared().getPPTimings() << std::endl;
                QTimings::getShared().reset();
                break;

            case '2':
                QTimings::getShared().start("login");
                if (splitted.size() != 6)
                {
                    connection->write("InvalidNumberOfArguments");
                    break;
                }
                this->receiveNANGLogin(connection, QByteArray::fromBase64(splitted[0]).toStdString(), QByteArray::fromBase64(splitted[1]).toStdString(),
                        QByteArray::fromBase64(splitted[2]).toStdString(), QByteArray::fromBase64(splitted[3]).toStdString(),
                        QByteArray::fromBase64(splitted[4]).toStdString(), QByteArray::fromBase64(splitted[5]).toStdString());
                QTimings::getShared().stop("login");
                std::cout << QTimings::getShared().getPPTimings() << std::endl;
                QTimings::getShared().reset();
                break;

            default:
                connection->write("WrongProtocol");
                break;
        }

        connection->flush();
        connection->close();
        connection = nullptr;

        QTimings::getShared().stop("connection");
    }

    return true;
}

void    Server::receiveNANGRegister(QTcpSocket *socket, const std::string &nid, const std::string &auth)
{
#ifdef PRINT_DEBUG
    std::cout << "[REGISTER] nang.nid == '" << nid << "' (" << QByteArray::fromStdString(nid).toHex().toStdString() << ")" << std::endl;
    std::cout << "[REGISTER] nang.auth == '" << auth << "' (" << QByteArray::fromStdString(auth).toHex().toStdString() << ")" << std::endl;
#endif
    std::ostringstream oss;

    std::string e = this->newRandom();

    oss << this->getMyId() << e;

    std::string mI = this->hash(oss.str(), "hash-mI");
#ifdef PRINT_DEBUG
    std::cout << "[REGISTER] mI == '" << mI << "' (" << QByteArray::fromStdString(mI).toHex().toStdString() << ")" << std::endl;
#endif

    oss.str("");
    oss.clear();

    oss << mI << auth;

    std::string vN = this->scalarMul(this->hash(oss.str(), "hash-vN"), 126, "scalar-vN");
#ifdef PRINT_DEBUG
    std::cout << "[REGISTER] vN == '" << vN << "' (" << QByteArray::fromStdString(vN).toHex().toStdString() << ")" << std::endl;
#endif

    oss.str("");
    oss.clear();

    oss << vN << nid;

    std::string hN = this->hash(oss.str(), "hash-hN");
#ifdef PRINT_DEBUG
    QByteArray copy = QByteArray::fromStdString(hN).replace("\r", "\\r");
    std::cout << "[REGISTER] hN == '" << copy.toStdString() << "' (" << QByteArray::fromStdString(hN).toHex().toStdString() << ")" << std::endl;
#endif

    quint32 ip = socket->peerAddress().toIPv4Address();

    this->hashNames.emplace(ip, hN);

    socket->write("1");
    socket->write(QByteArray::fromStdString(vN).toBase64());
}

void    Server::receiveNANGLogin(QTcpSocket *socket, const std::string &cid, const std::string &smTime,
                                 const std::string &c2, const std::string &rid, const std::string &cN, const std::string &nangTime)
{
#ifdef PRINT_DEBUG
    std::cout << "[LOGIN] client.CID == '" << cid << "' (" << QByteArray::fromStdString(cid).toHex().toStdString() << ")" << std::endl;
    std::cout << "[LOGIN] client.time == '" << smTime << "' (" << QByteArray::fromStdString(smTime).toHex().toStdString() << ")" << std::endl;
    std::cout << "[LOGIN] client.C2   == '" << c2 << "' (" << QByteArray::fromStdString(c2).toHex().toStdString() << ")" << std::endl;
    std::cout << "[LOGIN] client.RID   == '" << rid << "' (" << QByteArray::fromStdString(rid).toHex().toStdString() << ")" << std::endl;
    QByteArray copycN = QByteArray::fromStdString(cN).replace("\r", "\\r");
    std::cout << "[LOGIN] client.Cn   == '" << copycN.toStdString() << "' (" << QByteArray::fromStdString(cN).toHex().toStdString() << ")" << std::endl;
    std::cout << "[LOGIN] gateway.time   == '" << nangTime << "' (" << QByteArray::fromStdString(nangTime).toHex().toStdString() << ")" << std::endl;
#endif
    std::string localTime = "TIME";

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
#ifdef PRINT_DEBUG
    QByteArray copy = QByteArray::fromStdString(hN).replace("\r", "\\r");
    std::cout << "[LOGIN] hN == '" << copy.toStdString() << "' (" << QByteArray::fromStdString(hN).toHex().toStdString() << ")" << std::endl;
#endif

    //TODO: check delta time

    std::string wP = this->applyXOr(cN, hN, "xor-wP");
#ifdef PRINT_DEBUG
    std::cout << "[LOGIN] wP == '" << wP << "' (" << QByteArray::fromStdString(wP).toHex().toStdString() << ")" << std::endl;
#endif

    std::ostringstream tmp;
    tmp << wP << smTime;

    std::string bi = this->applyXOr(this->hash(tmp.str(), "hash-bi"), cid, "xor-bi");
#ifdef PRINT_DEBUG
    std::cout << "[LOGIN] bi == '" << bi << "' (" << QByteArray::fromStdString(bi).toHex().toStdString() << ")" << std::endl;
#endif

    tmp.str("");
    tmp.clear();

    tmp << cN << hN;

    std::string bj = this->applyXOr(this->hash(tmp.str(), "hash-bj"), rid, "xor-bj");
#ifdef PRINT_DEBUG
    std::cout << "[LOGIN] bj == '" << bj << "' (" << QByteArray::fromStdString(bj).toHex().toStdString() << ")" << std::endl;
#endif

    tmp.str("");
    tmp.clear();

    tmp << cid << bi << wP;

    std::string c1_bis = this->hash(tmp.str(), "hash-c1'");
#ifdef PRINT_DEBUG
    std::cout << "[LOGIN] C1' == '" << c1_bis << "' (" << QByteArray::fromStdString(c1_bis).toHex().toStdString() << ")" << std::endl;
#endif

    tmp.str("");
    tmp.clear();

    tmp << c1_bis << nangTime << cN << bj;

    std::string c2_bis = this->hash(tmp.str(), "hash-c2'");
#ifdef PRINT_DEBUG
    std::cout << "[LOGIN] C2' == '" << c2_bis << "' (" << QByteArray::fromStdString(c2_bis).toHex().toStdString() << ")" << std::endl;
#endif

    if (c2_bis.compare(c2) != 0)
    {
        socket->write("WrongID");
        return;
    }

    std::string y = this->newRandom();

    std::string yP = this->scalarMul(y, 256, "scalar-yP");
#ifdef PRINT_DEBUG
    std::cout << "[LOGIN] yP == '" << yP << "' (" << QByteArray::fromStdString(yP).toHex().toStdString() << ")" << std::endl;
#endif

    std::string cS = this->applyXOr(yP, /*this->hash(*/hN/*, "hash-cS")*/, "xor-cS");
#ifdef PRINT_DEBUG
    std::cout << "[LOGIN] cS == '" << cS << "' (" << QByteArray::fromStdString(cS).toHex().toStdString() << ")" << std::endl;
#endif

    tmp.str("");
    tmp.clear();

    tmp << yP << wP << bi << bj;

    std::string SKs = this->hash(tmp.str(), "hash-SKs");
#ifdef PRINT_DEBUG
    std::cout << "[LOGIN] SKs == '" << SKs << "' (" << QByteArray::fromStdString(SKs).toHex().toStdString() << ")" << std::endl;
#endif

    tmp.str("");
    tmp.clear();

    tmp << SKs << localTime << yP;

    std::string c3 = this->hash(tmp.str(), "hash-c3");
#ifdef PRINT_DEBUG
    std::cout << "[LOGIN] C3 == '" << c3 << "' (" << QByteArray::fromStdString(c3).toHex().toStdString() << ")" << std::endl;
#endif

    QByteArray  output;

    output.append('2');
    output.append(QByteArray::fromStdString(c3).toBase64());
    output.append(':');
    output.append(QByteArray::fromStdString(cS).toBase64());
    output.append(':');
    output.append(QByteArray::fromStdString(localTime).toBase64());

    socket->write(output);
}
