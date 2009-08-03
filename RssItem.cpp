#include "RssItem.h"
#include <iostream>
#include <ctime>

const char RssItem::LUNAR_CLASS_NAME[] = "RssItem";
Lunar<RssItem>::RegType RssItem::LUNAR_METHODS[] = {
LUNAR_STRING_GETTER(RssItem, title),
LUNAR_STRING_GETTER(RssItem, link),
LUNAR_STRING_GETTER(RssItem, description),
LUNAR_STRING_GETTER(RssItem, author),
LUNAR_STRING_GETTER(RssItem, date),
LUNAR_INTEGER_GETTER(RssItem, date_ts, long long),
{0,0,0,0}
};

long long RssItem::date_ts() const {
    struct tm ts;
    if(!strptime(_date.c_str(), "%a, %d %b %Y %T", &ts))
        std::cerr << "time parse failed." << std::endl;
    return mktime(&ts);
}