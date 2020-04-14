#include <stdexcept>
#include <QStringBuilder>
#include <iostream>
#include "QTimings.h"

QTimings    QTimings::sharedInstance;

void    QTimings::start(const std::string &name)
{
    if (!this->timer.isValid())
    {
        this->timer.start();
    }
    this->starts.emplace(name, this->timer.nsecsElapsed());
}

void    QTimings::stop(const std::string &name)
{
    try
    {
        qint64 ret = this->starts.at(name);
        qint64 now = this->timer.nsecsElapsed();

        //this->timings.emplace(name, (now - ret));
        this->timings2.push_back(std::make_pair(name, (now - ret)));
    }
    catch (std::out_of_range e)
    {
        std::cerr << "Not a timing name: " << name << ". Details: " << e.what() << std::endl;
    }
}

void    QTimings::stopAndStart(const std::string &nameStop, const std::string &nameStart)
{
    if (!this->timer.isValid())
    {
        this->timer.start();
    }
    qint64 now = this->timer.nsecsElapsed();
    try
    {
        qint64 ret = this->starts.at(nameStop);

        //this->timings.emplace(nameStop, (now - ret));
        this->timings2.push_back(std::make_pair(nameStop, (now - ret)));
    }
    catch (std::out_of_range e)
    {
        std::cerr << "Not a timing name: " << nameStop << ". Details: " << e.what() << std::endl;
    }
    this->starts.emplace(nameStart, now);
}

void    QTimings::reset()
{
    this->timer.invalidate();
    this->timings.clear();
    this->timings2.clear();
    this->starts.clear();
}

std::string QTimings::getPPTimings() const
{
    QString result;

    result.append("Timings :\n");
    for (auto entry : this->timings2)
    {
        result.append(" - ").append(entry.first.c_str()).append(": ");
        result.append(QString::number((((double) entry.second) / 1000000.0))).append(" ms\n");
    }

    return result.toStdString();
}
