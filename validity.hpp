#pragma once
#include <QString>

bool is_valid_ip(const QString& ip)
{
    return !ip.isEmpty(); //for now
}

bool is_valid_mac(const QString& mac)
{
    return mac.length() == 17;
}

bool is_valid_name(const QString& name)
{
    return name.size();
}
