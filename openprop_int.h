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
#include "openprop.h"
#include <string>
#include <vector>

namespace OpenProp {
    class Record : public Element {
    public:
        Record(std::string name, std::string type);
        virtual ~Record();
        void append(Element *);
        
        virtual bool isProperty();
        virtual bool isRecord();
        
        virtual Value getValue();
        
        virtual int getNumElements();
        virtual Element *getElement(const char *path);
        virtual Element *getElement(int offset);
        
        virtual ElementIterator *getElements();
        virtual ElementIterator *getElements(const char *type);
        
    protected:
        std::vector<Element *> subrecords;
        std::map<std::string, int> offsetmap;
        std::map<std::string, bool> multimap;
    };
    
    class Property : public Element {
    public:
        Property(std::string name, std::string type, std::string value);
        
        virtual bool isProperty();
        virtual bool isRecord();
        
        virtual Value getValue();
        
        virtual int getNumElements();
        virtual Element *getElement(const char *path);
        virtual Element *getElement(int offset);
        virtual ElementIterator *getElements();
        virtual ElementIterator *getElements(const char *type);
    protected:
        std::string value;
    };
}