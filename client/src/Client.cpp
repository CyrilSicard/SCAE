#include <string>
#include <sstream>
#include <iostream>
#include <future>
#include <QTcpSocket>
#include <QByteArray>
#include <QTcpServer>
#include "Client.h"

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

    std::ostringstream strs;

    strs.str("");
    strs.clear();

    strs << mid << bi;

    std::string aj = this->hash(strs.str(), "calcA");

    QTcpSocket  socket;
    QByteArray  output;

    output.append('1');
    output.append(QByteArray::fromStdString(mid));
    output.append(':');
    output.append(QByteArray::fromStdString(aj));

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

    std::string vm = result.toStdString();

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

    std::string localTime;

    std::string w = this->newRandom();

    std::string wP = this->scalarMul(w, 256);

    std::string cU = this->applyXOr(wP, this->hash(strs.str()));

    strs.str("");
    strs.clear();

    strs << wP << localTime;

    std::string cid = this->applyXOr(this->hash(strs.str()), this->myRandom);

    strs.str("");
    strs.clear();

    strs << cid << this->myRandom << wP;

    std::string c1 = this->hash(strs.str());

    QTcpSocket  socket;
    QByteArray  output;

    output.append('2');
    output.append(QByteArray::fromStdString(cU));
    output.append(':');
    output.append(QByteArray::fromStdString(cid));
    output.append(':');
    output.append(QByteArray::fromStdString(c1));
    output.append(':');
    output.append(QByteArray::fromStdString(localTime));

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

    if (rawResult.front() != '2')
    {
        std::cerr << "The server returned an error: " << rawResult.toStdString() << std::endl;
        return false;
    }

    QList<QByteArray> splitted = rawResult.right(1).split(':');

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

    std::string hashVmMID = this->hash(strs.str());

    std::string yP = this->applyXOr(cS, hashVmMID);

    strs.str("");
    strs.clear();

    strs << cM << hashVmMID;

    std::string bj = this->applyXOr(this->hash(strs.str()), rid);

    strs.str("");
    strs.clear();

    strs << yP << wP << this->myRandom << bj;

    std::string SKm = this->hash(strs.str());

    strs.str("");
    strs.clear();

    strs << SKm << t3 << yP;

    std::string c3_bis = this->hash(strs.str());

    strs.str("");
    strs.clear();

    strs << c3_bis << t4 << cM << bj;

    std::string c4_bis = this->hash(strs.str());

    return (c4_bis.compare(c4) == 0);
}
