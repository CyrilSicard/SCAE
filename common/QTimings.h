#include <QElapsedTimer>
#include <string>
#include <map>
#include <vector>

#pragma once

class QTimings
{
    public:
        QTimings() = default;

        void    start(const std::string &name);
        void    stop(const std::string &name);
        void    stopAndStart(const std::string &nameStop, const std::string &nameStart);

        void    reset();

        std::string getPPTimings() const;

        static QTimings   &getShared() { return QTimings::sharedInstance; };

    private:
        static  QTimings sharedInstance;

        QElapsedTimer timer;

        std::map<std::string, qint64>    starts;
        std::map<std::string, qint64>    timings;
        std::vector<std::pair<std::string, qint64>>     timings2;
};

