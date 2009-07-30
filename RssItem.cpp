#include "RssItem.h"

const char RssItem::LUNAR_CLASS_NAME[] = "RssItem";
Lunar<RssItem>::RegType RssItem::LUNAR_METHODS[] = {
LUNAR_STRING_GETTER(RssItem, title),
LUNAR_STRING_GETTER(RssItem, link),
LUNAR_STRING_GETTER(RssItem, description),
LUNAR_STRING_GETTER(RssItem, author),
LUNAR_STRING_GETTER(RssItem, date),
{0,0,0,0}
};

