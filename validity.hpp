#pragma once
#include <QString>

bool is_valid_mac(const QString& mac)
{
    auto length = mac.length();
    if (length != 17) return false;
    
    for (int i=0 ; i < length ; ++i)
    {
        char ch = mac[i].toLatin1();
        
        if ((i+1)%3 == 0)
        {
            //every 3rd character should be colon
            if (ch != ':') return false;
        }
        else
        {
            //checking if valid hex character
            if ((ch < '0' or ch > '9') and
                (ch < 'a' or ch > 'f') and
                (ch < 'A' or ch > 'F')) return false;
        }
    }
    return true;
}

bool is_valid_name(const QString& name)
{
    auto length = name.length();
    if (length < 1 or length > 48) return false;
    
    for (int i=0 ; i < length ; ++i)
    {
        char ch = name[i].toLatin1();
        
        // valid username characters
        if ((ch < '0' or ch > '9') and
            (ch < 'a' or ch > 'z') and
            (ch < 'A' or ch > 'Z') and
            (ch !='-' && ch !='_') and
            (ch !='(' && ch !=')') and
            (ch !=' ' && ch !='@')) return false;
    }
    return true;
}
