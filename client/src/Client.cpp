#include <string>
#include <sstream>
#include <iostream>
#include <future>
#include <QTcpSocket>
#include <QByteArray>
#include <QTcpServer>
#include "Client.h"
#include "QTimings.h"

Client::Client(const std::string &host, short port) : CommonUtils(16, 80), host(host), port(port)
{}

std::string Client::getMyId() const
{
    return "SmartReaderMID098735";
}

bool    Client::registerToNAN()
{
    std::string mid = this->getMyId();
    std::string bi = this->newRandom();
#ifdef PRINT_DEBUG
    std::cout << "[REGISTER] bi == '" << bi << "' (" << QByteArray::fromStdString(bi).toHex().toStdString() << ")" << std::endl;
#endif

    std::ostringstream strs;

    strs.str("");
    strs.clear();

    strs << mid << bi;

    std::string aj = this->hash(strs.str(), "calcA");
#ifdef PRINT_DEBUG
    std::cout << "[REGISTER] aj == '" << aj << "' (" << QByteArray::fromStdString(aj).toHex().toStdString() << ")" << std::endl;
#endif

    QTcpSocket  socket;
    QByteArray  output;

    output.append('1');
    output.append(QByteArray::fromStdString(mid).toBase64());
    output.append(':');
    output.append(QByteArray::fromStdString(aj).toBase64());

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

    std::string vm = result.toStdString();
#ifdef PRINT_DEBUG
    std::cout << "[REGISTER] vM == '" << vm << "' (" << QByteArray::fromStdString(vm).toHex().toStdString() << ")" << std::endl;
#endif

    this->myRandom = bi;
    this->verifier = vm;
    return true;
}

bool    Client::loginToNAN()
{

    std::ostringstream strs;

    strs.str("");
    strs.clear();

    strs << this->verifier << this->getMyId();

    std::string localTime = "TIME";

    std::string w = this->newRandom();
#ifdef PRINT_DEBUG
    std::cout << "[LOGIN] w == '" << w << "' (" << QByteArray::fromStdString(w).toHex().toStdString() << ")" << std::endl;
#endif

    std::string wP = this->scalarMul(w, 256, "scalar-wP");
#ifdef PRINT_DEBUG
    std::cout << "[LOGIN] wP == '" << wP << "' (" << QByteArray::fromStdString(wP).toHex().toStdString() << ")" << std::endl;
#endif

    std::string hM = this->hash(strs.str(), "hash-cU");
#ifdef PRINT_DEBUG
    std::cout << "[LOGIN] hM == '" << hM << "' (" << QByteArray::fromStdString(hM).toHex().toStdString() << ")" << std::endl;
#endif

    std::string cU = this->applyXOr(wP, hM, "xor-cU");
#ifdef PRINT_DEBUG
    std::cout << "[LOGIN] cU == '" << cU << "' (" << QByteArray::fromStdString(cU).toHex().toStdString() << ")" << std::endl;
#endif

    strs.str("");
    strs.clear();

    strs << wP << localTime;

    std::string cid = this->applyXOr(this->hash(strs.str(), "hash-cid"), this->myRandom, "xor-cid");
#ifdef PRINT_DEBUG
    std::cout << "[LOGIN] cid == '" << cid << "' (" << QByteArray::fromStdString(cid).toHex().toStdString() << ")" << std::endl;
#endif

    strs.str("");
    strs.clear();

    strs << cid << this->myRandom << wP;

    std::string c1 = this->hash(strs.str(), "hash-c1");
#ifdef PRINT_DEBUG
    std::cout << "[LOGIN] C1 == '" << c1 << "' (" << QByteArray::fromStdString(c1).toHex().toStdString() << ")" << std::endl;
#endif

    QTcpSocket  socket;
    QByteArray  output;

    output.append('2');
    output.append(QByteArray::fromStdString(cU).toBase64());
    output.append(':');
    output.append(QByteArray::fromStdString(cid).toBase64());
    output.append(':');
    output.append(QByteArray::fromStdString(c1).toBase64());
    output.append(':');
    output.append(QByteArray::fromStdString(localTime).toBase64());

    QTimings::getShared().start("send_login");

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

    QTimings::getShared().stop("send_login");

    if (rawResult.front() != '2')
    {
        std::cerr << "The server returned an error: " << rawResult.toStdString() << std::endl;
        return false;
    }

    rawResult.remove(0, 1);
    QList<QByteArray> splitted = rawResult.split(':');

    std::string c3 = QByteArray::fromBase64(splitted[0]).toStdString();
    std::string cS = QByteArray::fromBase64(splitted[1]).toStdString();
    std::string t3 = QByteArray::fromBase64(splitted[2]).toStdString();
    std::string c4 = QByteArray::fromBase64(splitted[3]).toStdString();
    std::string cM = QByteArray::fromBase64(splitted[4]).toStdString();
    std::string t4 = QByteArray::fromBase64(splitted[5]).toStdString();
    std::string rid = QByteArray::fromBase64(splitted[6]).toStdString();

    strs.str("");
    strs.clear();

    strs << this->verifier << this->getMyId();

    std::string hashVmMID = this->hash(strs.str(), "hash-VmMID");

    std::string yP = this->applyXOr(cS, hashVmMID, "xor-yP");
#ifdef PRINT_DEBUG
    std::cout << "[LOGIN] yP == '" << yP << "' (" << QByteArray::fromStdString(yP).toHex().toStdString() << ")" << std::endl;
#endif

    strs.str("");
    strs.clear();

    strs << cM << hashVmMID;

    std::string bj = this->applyXOr(this->hash(strs.str(), "hash-bj"), rid, "xor-bj");
#ifdef PRINT_DEBUG
    std::cout << "[LOGIN] bj == '" << bj << "' (" << QByteArray::fromStdString(bj).toHex().toStdString() << ")" << std::endl;
#endif

    strs.str("");
    strs.clear();

    strs << yP << wP << this->myRandom << bj;

    std::string SKm = this->hash(strs.str(), "hash-SKm");
#ifdef PRINT_DEBUG
    std::cout << "[LOGIN] SKm == '" << SKm << "' (" << QByteArray::fromStdString(SKm).toHex().toStdString() << ")" << std::endl;
#endif

    strs.str("");
    strs.clear();

    strs << SKm << t3 << yP;

    std::string c3_bis = this->hash(strs.str(), "hash-c3'");
#ifdef PRINT_DEBUG
    std::cout << "[LOGIN] C3' == '" << c3_bis << "' (" << QByteArray::fromStdString(c3_bis).toHex().toStdString() << ")" << std::endl;
#endif

    strs.str("");
    strs.clear();

    strs << c3_bis << t4 << cM << bj;

    std::string c4_bis = this->hash(strs.str(), "hash-c4'");
#ifdef PRINT_DEBUG
    std::cout << "[LOGIN] C4' == '" << c4_bis << "' (" << QByteArray::fromStdString(c4_bis).toHex().toStdString() << ")" << std::endl;
#endif

    return (c4_bis.compare(c4) == 0);
}
