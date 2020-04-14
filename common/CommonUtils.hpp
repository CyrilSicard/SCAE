#include <string>
#include <QCryptographicHash>
#include "FiniteFieldElement.hpp"

#pragma once

class CommonUtils
{
    public:
        CommonUtils(int a, int b);
        virtual ~CommonUtils() = default;

    protected:
        using ec_t = Cryptography::EllipticCurve<263>;

        virtual std::string newRandom() const;
        virtual std::string hash(const std::string &text, const std::string &operationName = "", const QCryptographicHash::Algorithm algorithm = QCryptographicHash::Algorithm::Md5) const;
        virtual std::string applyXOr(const std::string &inA, const std::string &inB, const std::string &operationName = "") const;
        virtual std::string scalarMul(const std::string &text, int pointIndex, const std::string &operationName = "");

        virtual std::string getMyId() const = 0;


        ec_t    generatedCurve;

    private:
};

