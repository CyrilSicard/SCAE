#include <string>
#include <sstream>
#include <iostream>
#include <future>
#include <QTcpSocket>
#include <QByteArray>
#include <QTcpServer>
#include "Gateway.h"
#include "QTimings.h"

Gateway::Gateway(const std::string &host, short port, short open) : CommonUtils(16, 80), host(host), port(port), myPort(open)
{}

std::string Gateway::getMyId() const
{
    return "GatewayNID028734";
}

static  std::string readInput()
{
    std::string answer;
    std::cin >> answer;
    return answer;
}

bool    Gateway::runNAN()
{
    QTcpServer  server;

    if (!server.listen(QHostAddress::Any, this->myPort))
    {
        std::cerr << "Could not start NAN: " << server.errorString().toStdString() << std::endl;
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

        QTimings::getShared().start("connection");
        std::cout << "Received a new connection !" << std::endl;

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
                QTimings::getShared().start("register");
                if (splitted.size() != 2)
                {
                    connection->write("InvalidNumberOfArguments");
                    break;
                }
                this->receiveSMRegister(connection, QByteArray::fromBase64(splitted[0]).toStdString(), QByteArray::fromBase64(splitted[1]).toStdString());
                QTimings::getShared().stop("register");
                std::cout << QTimings::getShared().getPPTimings() << std::endl;
                QTimings::getShared().reset();
                break;

            case '2':
                QTimings::getShared().start("login");
                if (splitted.size() != 4)
                {
                    connection->write("InvalidNumberOfArguments");
                    break;
                }
                this->receiveSMLogin(connection, QByteArray::fromBase64(splitted[0]).toStdString(), QByteArray::fromBase64(splitted[1]).toStdString(),
                        QByteArray::fromBase64(splitted[2]).toStdString(), QByteArray::fromBase64(splitted[3]).toStdString());
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

void    Gateway::receiveSMRegister(QTcpSocket *socket, const std::string &mid, const std::string &auth)
{
#ifdef PRINT_DEBUG
    std::cout << "[REGISTER] client.mid == '" << mid << "' (" << QByteArray::fromStdString(mid).toHex().toStdString() << ")" << std::endl;
    std::cout << "[REGISTER] client.auth == '" << auth << "' (" << QByteArray::fromStdString(auth).toHex().toStdString() << ")" << std::endl;
#endif
    std::ostringstream oss;

    oss << this->verifier << this->getMyId();

    std::string hN = this->hash(oss.str(), "hash-hN");
#ifdef PRINT_DEBUG
    QByteArray copyhN = QByteArray::fromStdString(hN).replace("\r", "\\r");
    std::cout << "[REGISTER] hN == '" << copyhN.toStdString() << "' (" << QByteArray::fromStdString(hN).toHex().toStdString() << ")" << std::endl;
#endif

    oss.str("");
    oss.clear();

    oss << hN << auth;

    std::string vM = this->scalarMul(this->hash(oss.str(), "hash-vM"), 126, "scalar-vM");
#ifdef PRINT_DEBUG
    std::cout << "[REGISTER] vM == '" << vM << "' (" << QByteArray::fromStdString(vM).toHex().toStdString() << ")" << std::endl;
#endif

    oss.str("");
    oss.clear();

    oss << vM << mid;

    std::string hM = this->hash(oss.str(), "hash-hM");
#ifdef PRINT_DEBUG
    std::cout << "[REGISTER] hM == '" << hM << "' (" << QByteArray::fromStdString(hM).toHex().toStdString() << ")" << std::endl;
#endif

    quint32 ip = socket->peerAddress().toIPv4Address();

    this->hashNames.emplace(ip, hM);

    socket->write("1");
    socket->write(QByteArray::fromStdString(vM).toBase64());
}

void    Gateway::receiveSMLogin(QTcpSocket *socket, const std::string &cU, const std::string &cid, const std::string &cL, const std::string &time)
{
#ifdef PRINT_DEBUG
    std::cout << "[LOGIN] client.cU == '" << cU << "' (" << QByteArray::fromStdString(cU).toHex().toStdString() << ")" << std::endl;
    std::cout << "[LOGIN] client.cid == '" << cid << "' (" << QByteArray::fromStdString(cid).toHex().toStdString() << ")" << std::endl;
    std::cout << "[LOGIN] client.C1   == '" << cL << "' (" << QByteArray::fromStdString(cL).toHex().toStdString() << ")" << std::endl;
    std::cout << "[LOGIN] client.time   == '" << time << "' (" << QByteArray::fromStdString(time).toHex().toStdString() << ")" << std::endl;
#endif
    std::string localTime = "TIME";

    std::string hM;
    try
    {
        hM = this->hashNames.at(socket->peerAddress().toIPv4Address());
    }
    catch (const std::out_of_range &e)
    {
        socket->write("IpAddressNotRegistered");
        return;
    }
#ifdef PRINT_DEBUG
    std::cout << "[LOGIN] hM == '" << hM << "' (" << QByteArray::fromStdString(hM).toHex().toStdString() << ")" << std::endl;
#endif

    //TODO: check delta time

    std::string wP = this->applyXOr(cU, hM, "xor-wP");
#ifdef PRINT_DEBUG
    std::cout << "[LOGIN] wP == '" << wP << "' (" << QByteArray::fromStdString(wP).toHex().toStdString() << ")" << std::endl;
#endif

    std::ostringstream tmp;
    tmp << wP << time;

    std::string bi = this->applyXOr(this->hash(tmp.str(), "hash-bi"), cid, "xor-bi");
#ifdef PRINT_DEBUG
    std::cout << "[LOGIN] bi == '" << bi << "' (" << QByteArray::fromStdString(bi).toHex().toStdString() << ")" << std::endl;
#endif

    tmp.str("");
    tmp.clear();

    tmp << this->verifier << this->getMyId();

    std::string hashVnNID = this->hash(tmp.str(), "hash-VnID");

    std::string cN = this->applyXOr(this->applyXOr(cU, hM, "xor-cN-1"), hashVnNID, "xor-cN-2");
#ifdef PRINT_DEBUG
    QByteArray copycN = QByteArray::fromStdString(cN).replace("\r", "\\r");
    std::cout << "[LOGIN] cN == '" << copycN.toStdString() << "' (" << QByteArray::fromStdString(cN).toHex().toStdString() << ")" << std::endl;
#endif

    tmp.str("");
    tmp.clear();

    tmp << cN << hashVnNID;

    std::string rid = this->applyXOr(this->hash(tmp.str(), "hash-rid"), this->myRandom, "xor-rid");
#ifdef PRINT_DEBUG
    std::cout << "[LOGIN] rid == '" << rid << "' (" << QByteArray::fromStdString(rid).toHex().toStdString() << ")" << std::endl;
#endif

    tmp.str("");
    tmp.clear();

    tmp << cL << localTime << cN << this->myRandom;

    std::string c2 = this->hash(tmp.str(), "hash-c2");
#ifdef PRINT_DEBUG
    std::cout << "[LOGIN] C2 == '" << c2 << "' (" << QByteArray::fromStdString(c2).toHex().toStdString() << ")" << std::endl;
#endif

    QTcpSocket  serverSocket;
    QByteArray  output;

    output.append('2');
    output.append(QByteArray::fromStdString(cid).toBase64());
    output.append(':');
    output.append(QByteArray::fromStdString(time).toBase64());
    output.append(':');
    output.append(QByteArray::fromStdString(c2).toBase64());
    output.append(':');
    output.append(QByteArray::fromStdString(rid).toBase64());
    output.append(':');
    output.append(QByteArray::fromStdString(cN).toBase64());
    output.append(':');
    output.append(QByteArray::fromStdString(localTime).toBase64());

    QTimings::getShared().start("send_login");

    serverSocket.connectToHost(QString::fromStdString(this->host), this->port);

    if (!serverSocket.waitForConnected())
    {
        std::cerr << "Error while connecting: " << socket->errorString().toStdString() << std::endl;
        socket->write("UnableToContactServer");
        return;
    }

    serverSocket.write(output);

    if (!serverSocket.waitForBytesWritten())
    {
        std::cerr << "Error while flushing: " << serverSocket.errorString().toStdString() << std::endl;
        socket->write("UnableToContactServer");
        serverSocket.close();
        return;
    }

    if (!serverSocket.waitForReadyRead())
    {
        std::cerr << "No informations to read: " << serverSocket.errorString().toStdString() << std::endl;
        socket->write("UnableToReadServer");
        serverSocket.close();
        return;
    }

    QByteArray  rawResult = serverSocket.readAll();

    serverSocket.disconnectFromHost();
    serverSocket.close();

    QTimings::getShared().stop("send_login");

    if (rawResult.front() != '2')
    {
        std::cerr << "The server returned an error: " << rawResult.toStdString() << std::endl;
        socket->write("ServerError:");
        socket->write(rawResult);
        return;
    }

    rawResult.remove(0, 1);
    QList<QByteArray>  results = rawResult.split(':');
    if (results.size() != 3)
    {
        std::cerr << "Wrong amount of arguments, expected " << 3 << ", got " << results.size() << std::endl;
        socket->write("ServerProtocolError");
        return;
    }

    std::string c3 = QByteArray::fromBase64(results[0]).toStdString();
    std::string cS = QByteArray::fromBase64(results[1]).toStdString();
    std::string serverTime = QByteArray::fromBase64(results[2]).toStdString();

#ifdef PRINT_DEBUG
    std::cout << "[LOGIN] server.C3 == '" << c3 << "' (" << QByteArray::fromStdString(c3).toHex().toStdString() << ")" << std::endl;
    std::cout << "[LOGIN] server.Cs == '" << cS << "' (" << QByteArray::fromStdString(cS).toHex().toStdString() << ")" << std::endl;
    std::cout << "[LOGIN] server.time == '" << serverTime << "' (" << QByteArray::fromStdString(serverTime).toHex().toStdString() << ")" << std::endl;
#endif

    std::string yP = this->applyXOr(cS, hashVnNID, "xor-yP");
#ifdef PRINT_DEBUG
    std::cout << "[LOGIN] yP == '" << yP << "' (" << QByteArray::fromStdString(yP).toHex().toStdString() << ")" << std::endl;
#endif

    std::string cM = this->applyXOr(this->applyXOr(cS, hM, "xor-cM-1"), hashVnNID, "xor-cM-2");
#ifdef PRINT_DEBUG
    std::cout << "[LOGIN] cM == '" << cM << "' (" << QByteArray::fromStdString(cM).toHex().toStdString() << ")" << std::endl;
#endif

    tmp.str("");
    tmp.clear();

    tmp << yP << wP << bi << this->myRandom;

    std::string SKn = this->hash(tmp.str(), "hash-SKn");
#ifdef PRINT_DEBUG
    std::cout << "[LOGIN] SKn == '" << SKn << "' (" << QByteArray::fromStdString(SKn).toHex().toStdString() << ")" << std::endl;
#endif

    tmp.str("");
    tmp.clear();

    tmp << c3 << localTime << cM << this->myRandom;

    std::string c4 = this->hash(tmp.str(), "hash-c4");
#ifdef PRINT_DEBUG
    std::cout << "[LOGIN] C4 == '" << c4 << "' (" << QByteArray::fromStdString(c4).toHex().toStdString() << ")" << std::endl;
#endif

    tmp.str("");
    tmp.clear();

    tmp << cM << hM;

    std::string ridM = this->applyXOr(this->hash(tmp.str(), "hash-ridM"), this->myRandom, "xor-ridM");
#ifdef PRINT_DEBUG
    std::cout << "[LOGIN] ridM == '" << ridM << "' (" << QByteArray::fromStdString(ridM).toHex().toStdString() << ")" << std::endl;
#endif

    socket->write("2");
    socket->write(QByteArray::fromStdString(c3).toBase64());
    socket->write(":");
    socket->write(QByteArray::fromStdString(cS).toBase64());
    socket->write(":");
    socket->write(QByteArray::fromStdString(serverTime).toBase64());
    socket->write(":");
    socket->write(QByteArray::fromStdString(c4).toBase64());
    socket->write(":");
    socket->write(QByteArray::fromStdString(cM).toBase64());
    socket->write(":");
    socket->write(QByteArray::fromStdString(localTime).toBase64());
    socket->write(":");
    socket->write(QByteArray::fromStdString(ridM).toBase64());
}

bool    Gateway::registerToServer()
{
    std::string nid = this->getMyId();
    std::string bi = this->newRandom();
#ifdef PRINT_DEBUG
    std::cout << "[MY_REG] bi == '" << bi << "' (" << QByteArray::fromStdString(bi).toHex().toStdString() << ")" << std::endl;
#endif

    std::ostringstream strs;

    strs.str("");
    strs.clear();

    strs << nid << bi;

    std::string ai = this->hash(strs.str(), "calcA");
#ifdef PRINT_DEBUG
    std::cout << "[MY_REG] ai == '" << ai << "' (" << QByteArray::fromStdString(ai).toHex().toStdString() << ")" << std::endl;
#endif

    QTcpSocket  socket;
    QByteArray  output;

    output.append('1');
    output.append(QByteArray::fromStdString(nid).toBase64());
    output.append(':');
    output.append(QByteArray::fromStdString(ai).toBase64());

    QTimings::getShared().start("send_register");

    std::cout << "Connecting to " << this->host << ":" << this->port << " ..." << std::endl;
    socket.connectToHost(QString::fromStdString(this->host), this->port);

    if (!socket.waitForConnected())
    {
        std::cerr << "Error while connecting: " << socket.errorString().toStdString() << std::endl;
        return false;
    }

    socket.write(output);

    if (!socket.waitForBytesWritten())
    {
        std::cerr << "Error while flushing: " << socket.errorString().toStdString() << std::endl;
        socket.close();
        return false;
    }

    if (!socket.waitForReadyRead())
    {
        std::cerr << "No informations to read: " << socket.errorString().toStdString() << std::endl;
        socket.close();
        return false;
    }

    QByteArray  rawResult = socket.readAll();

    socket.disconnectFromHost();
    socket.close();

    QTimings::getShared().stop("send_register");

    if (rawResult.front() != '1')
    {
        std::cerr << "The server returned an error: " << rawResult.toStdString() << std::endl;
        return false;
    }

    rawResult.remove(0, 1);
    QByteArray  result = QByteArray::fromBase64(rawResult);

    std::string vn = result.toStdString();
#ifdef PRINT_DEBUG
    std::cout << "[MY_REG] vn == '" << vn << "' (" << QByteArray::fromStdString(vn).toHex().toStdString() << ")" << std::endl;
#endif

    this->myRandom = bi;
    this->verifier = vn;
    return true;
}
