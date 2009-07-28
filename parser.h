/*************************************************************************
 *                                                                       *
 * OpenProp C++ Library, Copyright (C) 2003, Brian N. Chin               *
 * All rights reserved.  Email: naerbnic@ucla.edu                        *
 *                                                                       *
 * This library is free software; you can redistribute it and/or         *
 * modify it under the terms of the BSD-style license that is included   *
 * with this library in the file LICENSE-BSD.TXT.                        *
 *                                                                       *
 * This library is distributed in the hope that it will be useful,       *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See LICENCE file *
 * for more details.                                                     *
 *                                                                       *
 *************************************************************************/
#pragma once
#include <istream>

namespace OpenProp {
    class Element;
    class Record;
    
    void populateList( std::istream &input );
    void clearList();
    bool parseRecordList(Record *record);
    Element *parse();
}