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

#include <cstdlib>
#include <istream>
#include <vector>
#include <map>
#include <string>

/*
using std::istream;
using std::vector;
using std::map;
using std::string;
*/

namespace OpenProp {
    class Element;
    
    class ElementIterator {
    public:
        bool more() {
            bool result = int_more();
            if(!result)
                delete this;
            return result;
        };
        virtual Element *next() = 0;
        
    protected:
        virtual bool int_more() = 0;
    };
    
    class Value {
    public:
        Value(const char *str) : str(str) {}
        Value(const Value &v) : str(v.str) {}
        
        operator const char *() { return str; }
        operator int() { return atoi(str); }
        operator long() { return atol(str); }
        operator double() { return atof(str); }
        operator bool() {
            if(!str) return false;
            if(!str[0]) return false; //empty string
            if(str[0] == '0' && !str[1]) return false; //string "0"
            if(strlen(str) == 5 &&
               toupper(str[0]) == 'F' &&
               toupper(str[1]) == 'A' &&
               toupper(str[2]) == 'L' &&
               toupper(str[3]) == 'S' &&
               toupper(str[4]) == 'E')
                return false;
            
            return true;
        }
        
        bool exists() {
            return str;
        }
        
    private:
        const char *str;
    };
    
    class File {
    public:
        static File *Load(std::istream &input);
        ~File();
        
        Element *getElement(const char *path);
        
        ElementIterator *getElements();
        ElementIterator *getElements(const char *type);
        
        Value getValue(const char *path);
        
    private:
        File( Element *record );
        Element *root;
    };
    
    class Element {
    public:
        const char *getName();
        const char *getType();
        
        virtual bool isProperty() = 0;
        virtual bool isRecord() = 0;
        
        virtual Value getValue() = 0;
        Value getValue(const char *path);
        
        virtual int getNumElements() = 0;
        virtual Element *getElement(const char *path) = 0;
        virtual Element *getElement(int offset) = 0;
        virtual ElementIterator *getElements() = 0;
        virtual ElementIterator *getElements(const char *type) = 0;
        
    protected:
        Element(std::string name, std::string type);
        std::string name, type;
    };
}