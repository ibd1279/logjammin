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
    
    //! Open Property Element Iterator
    /*!
     \par
     An object used to iterate over elements inside a OpenProp.
     structure.
     \author Brian N. Chin naerbnic@ucla.edu     
     \version 1.0
     \date 2003
     */
    class ElementIterator {
    public:
        //! Test if there are more elements left to iterator over.
        /*!
         \par
         As long as the iterator is processed to completion, the object
         does not need to be freed.
         \return true if there are more elements, false otherwise.
         */
        bool more() {
            bool result = int_more();
            if(!result)
                delete this;
            return result;
        };
        //! Get the next element
        /*!
         \return The next element in the iterator.
         */
        virtual Element *next() = 0;
        
    protected:
        //! Internal more method.
        /*!
         \par
         The method is used to test if there are more elements. This version
         does not delete the object upon the end of the iterator.
         \return true if there are more elements, false otherwise.
         */
        virtual bool int_more() = 0;
    };
    
    //! Open Property value object.
    /*!
     \par
     provides useful methods for converting an Open Property data into
     C structures.
     \author Brian N. Chin naerbnic@ucla.edu
     \author Jason Watson
     \version 1.0
     \date 2003 & 2009
     */
    class Value {
    public:
        //! Create a new Value object.
        /*!
         \par
         Creates a new Value object from the provided string.
         */
        Value(const char *str) : str(str) {}
        //! Create a new Value object as a clone.
        /*!
         \par
         Creates a new Value object as a clone of another Value object.
         */
        Value(const Value &v) : str(v.str) {}
        
        //! Get the value as a C String.
        /*!
         \par
         Do NOT release this memory.
         \return Value as a C String.
         */
        operator const char *() { return str; }
        
        //! Get the value as an int.
        /*!
         \return Value as an int.
         */
        operator int() { return atoi(str); }
        
        //! Get the value as a long.
        /*!
         \return Value as a long.
         */
        operator long() { return atol(str); }
        
        //! Get the value as a double.
        /*!
         \return Value as a double.
         */
        operator double() { return atof(str); }
        
        //! Get the value as a boolean.
        /*!
         \par
         a NULL value is treated as false.  An empty string is
         treated as false., the string "0" is treated as false. The string
         "false" in any case is treated as false.  All other values are 
         treated as true.
         \return Value as a bool.
         */
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
        
        //! Check if the value exists.
        /*!
         \return If the value exists.
         */
        bool exists() {
            return str;
        }
        
    private:
        const char *str;
    };
    
    //! Open Property File.
    /*!
     \par
     This is the root object for all OpenProperty structures.
     \author Brian N. Chin naerbnic@ucla.edu     
     \version 1.0
     \date 2003
     */
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
    
    //! Open Property Element.
    /*!
     \par
     Object that represents a configuration node in the Open Property
     configuration.
     \author Brian N. Chin naerbnic@ucla.edu     
     \version 1.0
     \date 2003
     */
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