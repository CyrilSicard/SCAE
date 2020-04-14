#include <string>
#include <sstream>
#include <iostream>
#include <future>
#include <QTcpSocket>
#include <QByteArray>
#include <QTcpServer>
#include "Gateway.h"

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
                this->receiveSMRegister(connection, splitted[0].toStdString(), splitted[1].toStdString());
                break;

            case '2':
                if (splitted.size() != 4)
                {
                    connection->write("InvalidNumberOfArguments");
                    break;
                }
                this->receiveSMLogin(connection, splitted[0].toStdString(), splitted[1].toStdString(), splitted[2].toStdString(), splitted[3].toStdString());
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

void    Gateway::receiveSMRegister(QTcpSocket *socket, const std::string &mid, const std::string &auth)
{
    std::ostringstream oss;

    oss << this->verifier << this->getMyId();

    std::string hN = this->hash(oss.str());

    oss.str("");
    oss.clear();

    oss << hN << auth;

    std::string vM = this->scalarMul(this->hash(oss.str()), 126);

    oss.str("");
    oss.clear();

    oss << vM << mid;

    std::string hM = this->hash(oss.str());

    quint32 ip = socket->peerAddress().toIPv4Address();

    this->hashNames.emplace(ip, hM);

    socket->write(QByteArray::fromStdString(vM).toBase64());
}

void    Gateway::receiveSMLogin(QTcpSocket *socket, const std::string &cU, const std::string &cid, const std::string &cL, const std::string &time)
{
    std::string localTime = nullptr;

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

    //TODO: check delta time

    std::string wP = this->applyXOr(cU, hM);

    std::ostringstream tmp;
    tmp << wP << time;

    std::string bi = this->applyXOr(this->hash(tmp.str()), cid);

    tmp.str("");
    tmp.clear();

    tmp << this->verifier << this->getMyId();

    std::string hashVnNID = this->hash(tmp.str());

    std::string cN = this->applyXOr(this->applyXOr(cU, hM), hashVnNID);

    tmp.str("");
    tmp.clear();

    tmp << cN << hashVnNID;

    std::string rid = this->applyXOr(this->hash(tmp.str()), this->myRandom);

    tmp.str("");
    tmp.clear();

    tmp << cL << localTime << cN << this->myRandom;

    std::string c2 = this->hash(tmp.str());

    QTcpSocket  serverSocket;
    QByteArray  output;

    output.append('2');
    output.append(QByteArray::fromStdString(cU).toBase64());
    output.append(':');
    output.append(QByteArray::fromStdString(cid).toBase64());
    output.append(':');
    output.append(QByteArray::fromStdString(cL).toBase64());
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

    if (rawResult.front() != '2')
    {
        std::cerr << "The server returned an error: " << rawResult.toStdString() << std::endl;
        socket->write("ServerError:");
        socket->write(rawResult);
        return;
    }

    QList<QByteArray>  results = rawResult.right(1).split(':');
    if (results.size() != 3)
    {
        std::cerr << "Wrong amount of arguments" << std::endl;
        socket->write("ServerProtocolError");
        return;
    }

    std::string c3 = QByteArray::fromBase64(results[0]).toStdString();
    std::string cS = QByteArray::fromBase64(results[1]).toStdString();
    std::string serverTime = QByteArray::fromBase64(results[2]).toStdString();

    tmp.str("");
    tmp.clear();

    std::string yP = this->applyXOr(cS, hashVnNID);

    std::string cM = this->applyXOr(this->applyXOr(cS, hM), hashVnNID);

    tmp << yP << wP << bi << this->myRandom;

    std::string SKn = this->hash(tmp.str());

    tmp.str("");
    tmp.clear();

    tmp << c3 << localTime << cM << this->myRandom;

    std::string c4 = this->hash(tmp.str());

    tmp.str("");
    tmp.clear();

    tmp << cM << hM;

    std::string ridM = this->applyXOr(this->hash(tmp.str()), this->myRandom);

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

    std::ostringstream strs;

    strs.str("");
    strs.clear();

    strs << nid << bi;

    std::string ai = this->hash(strs.str(), "calcA");

    QTcpSocket  socket;
    QByteArray  output;

    output.append('1');
    output.append(QByteArray::fromStdString(nid));
    output.append(':');
    output.append(QByteArray::fromStdString(ai));

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

    if (rawResult.front() != '1')
    {
        std::cerr << "The server returned an error: " << rawResult.toStdString() << std::endl;
        return false;
    }

    QByteArray  result = QByteArray::fromBase64(rawResult.right(1));

    std::string vn = result.toStdString();

    this->myRandom = bi;
    this->verifier = vn;
    return true;
}
