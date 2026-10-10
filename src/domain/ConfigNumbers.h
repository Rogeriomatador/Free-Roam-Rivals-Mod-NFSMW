#pragma once
#include <cerrno>
#include <cmath>
#include <cstdlib>
#include <limits>
#include <cctype>
namespace frr::domain {
inline bool configNumberEnd(const char* begin,const char* end) {
    if(begin==end) return false;
    while(*end&&std::isspace(static_cast<unsigned char>(*end))) ++end;
    return !*end;
}
inline float finiteConfigFloat(const char* text,float fallback) {
    if(!text) return fallback;
    errno=0;char* end=nullptr;const float value=std::strtof(text,&end);
    return errno!=ERANGE&&std::isfinite(value)&&configNumberEnd(text,end)?value:fallback;
}
inline unsigned finiteConfigUnsigned(const char* text,unsigned fallback) {
    if(!text) return fallback;
    const char* start=text;while(*start&&std::isspace(static_cast<unsigned char>(*start))) ++start;
    if(*start=='-') return fallback;
    errno=0;char* end=nullptr;const auto value=std::strtoul(text,&end,0);
    return errno!=ERANGE&&value<=std::numeric_limits<unsigned>::max()&&configNumberEnd(text,end)?static_cast<unsigned>(value):fallback;
}
}
