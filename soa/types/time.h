
#pragma once

#include<chrono>

namespace Datacratic {

constexpr std::chrono::hours operator ""h(unsigned long long h)
{
        return std::chrono::hours(h);
}
constexpr std::chrono::duration<long double, ratio<3600,1>> operator ""h(long double h)
{
        return std::chrono::duration<long double, std::ratio<3600,1>>(h);
}


constexpr std::chrono::minutes operator ""min(unsigned long long m)
{
        return std::chrono::minutes(m);
}
 constexpr std::chrono::duration<long double,
                                 std::ratio<60,1>> operator ""min(long double m)
{
        return std::chrono::duration<long double, ratio<60,1>> (m);
}

constexpr std::chrono::seconds operator ""s(unsigned long long s)
{
        return std::chrono::seconds(s);
}
constexpr std::chrono::duration<long double> operator ""s(long double s)
{
        return std::chrono::duration<long double>(s);
}

constexpr std::chrono::milliseconds operator ""ms(unsigned long long ms)
{
        return chrono::milliseconds(ms);
}
constexpr std::chrono::duration<long double, std::milli> operator ""ms(long double ms)
{
        return std::chrono::duration<long double, std::milli>(ms);
}


constexpr std::chrono::microseconds operator ""us(unsigned long long us)
{
        return std::chrono::microseconds(us);
}
constexpr std::chrono::duration<long double, std::micro> operator ""us(long double us)
{
        return std::chrono::duration<long double, std::micro>(us);
}

constexpr std::chrono::nanoseconds operator ""ns(unsigned long long ns)
{
        return chrono::nanoseconds(ns);
}
constexpr std::chrono::duration<long double, std::nano> operator ""ns(long double ns)
{
        return std::chrono::duration<long double, std::nano>(ns);
}
}
