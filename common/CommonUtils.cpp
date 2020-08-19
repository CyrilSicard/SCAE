#include <sstream>
#include <QRandomGenerator>
#include <QString>
#include "QTimings.h"
#include "CommonUtils.hpp"
#include "FiniteFieldElement.cpp"

CommonUtils::CommonUtils(int a, int b) : generatedCurve(a, b)
{
    this->generatedCurve.CalculatePoints();
}

std::string CommonUtils::newRandom() const
{
    QRandomGenerator gen;
    quint32 generated = gen.generate();

    return QString::number(generated).toStdString();
}

std::string CommonUtils::hash(const std::string &text, const std::string &operationName, const QCryptographicHash::Algorithm algorithm) const
{
    if (!operationName.empty())
    {
        QTimings::getShared().start(operationName + "_hash");
    }
    std::string result = QCryptographicHash::hash(QByteArray::fromStdString(text), algorithm).toHex().toStdString();
    if (!operationName.empty())
    {
        QTimings::getShared().stop(operationName + "_hash");
    }
    return result;
}

std::string CommonUtils::applyXOr(const std::string &inA, const std::string &inB, const std::string &operationName) const
{
    if (!operationName.empty())
    {
        QTimings::getShared().start(operationName + "_xor");
    }
    QByteArray arrA = QByteArray::fromStdString(inA);
    QByteArray arrB = QByteArray::fromStdString(inB);

    QByteArray  result;
    int zeroCount = 0;

    for (int i = 0; i < std::max(arrA.length(), arrB.length()); ++i)
    {
        char cA = (i >= arrA.size()) ? 0 : arrA.at(i);
        char cB = (i >= arrB.size()) ? 0 : arrB.at(i);

        char tmp = (cA ^ cB);
        if (tmp <= '\0')
        {
            zeroCount += 1;
            continue;
        }
        else if (zeroCount > 0)
        {
            result.append(zeroCount, '\0');
            zeroCount = 0;
        }

        result.append(tmp);
    }
    if (!operationName.empty())
    {
        QTimings::getShared().stop(operationName + "_xor");
    }
    return result.toStdString();
}

std::string CommonUtils::scalarMul(const std::string &text, int pointIndex, const std::string &operationName)
{
    if (!operationName.empty())
    {
        QTimings::getShared().start(operationName + "_scalar");
    }

    ec_t::Point point = this->generatedCurve[pointIndex];

    std::ostringstream strs;

    for (unsigned int i = 0; i < text.length(); ++i)
    {
        int cA = (i < text.length()) ? text.at(i) : 1;

        ec_t::ffe_t c1(cA * point.x());
        ec_t::ffe_t c2(cA * point.y());
        point *= cA;

        strs << ((c1.i() ^ c2.i()) % 255);
    }
    if (!operationName.empty())
    {
        QTimings::getShared().stop(operationName + "_scalar");
    }

    return strs.str();
}
