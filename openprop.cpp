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

#include "openprop.h"
#include "openprop_int.h"
#include "parser.h"

#include <memory>
using std::auto_ptr;

#include <cassert>

#include <iostream>
using std::cout;

#include <string>
using std::string;

namespace OpenProp {
    
    File *File::Load(std::istream &input) {
        populateList(input);
        Record *multirec = new Record("<root>", "<root>");
        if(!parseRecordList(multirec)) {
            delete multirec;
            return NULL;
        }
        
        clearList();
        
        return new File(multirec);
    }
    
    File::File( Element *elem ) : root(elem) {}
    File::~File() { delete root; }
    
    Element *File::getElement(const char *path) {
        return root->getElement(path);
    }
    
    ElementIterator *File::getElements() {
        return root->getElements();
    }
    
    ElementIterator *File::getElements(const char *type) {
        return root->getElements(type);
    }
    
    Value File::getValue(const char *path) {
        return root->getValue(path);
    }
    
    //ELEMENT IMPLEMENTATION
    
    Element::Element(std::string name, std::string type) : name(name), type(type) {}
    
    const char *Element::getName() {
        return name.c_str();
    }
    
    const char *Element::getType() {
        return name.c_str();
    }
    
    Value Element::getValue(const char *path) {
        Element *elem = this->getElement(path);
        if(!elem) return Value(NULL);
        
        return Value(elem->getValue());
    }
    
    //MULTIRECORD IMPLEMENTATION
    
    Record::Record(std::string name, std::string type) : Element(name, type) {}
    Record::~Record() {
        for(unsigned int i = 0; i < subrecords.size(); i++) {
            delete subrecords[i];
        }
    } //FIXME: delete children later
    
    void Record::append(Element *elem) {
        subrecords.push_back(elem);
        if(offsetmap.find(elem->getName()) == offsetmap.end() && !multimap[elem->getName()]) {
            offsetmap[elem->getName()] = subrecords.size() - 1;
        } else {
            offsetmap.erase(elem->getName());
            multimap[elem->getName()] = true;
        }
    }
    
    bool Record::isProperty() {
        return false;
    }
    
    bool Record::isRecord() {
        return true;
    }
    
    Value Record::getValue() {
        return Value(NULL);
    }
    
    int Record::getNumElements() {
        return subrecords.size();
    }
    
    Element *Record::getElement(const char *path) {
        Element *nextRecord = NULL;
        int pos = 0;
        
        if(path[0] == '[') {
            int numlen = 0;
            while(path[1 + numlen] != '\0' && path[1 + numlen] != ']')
                numlen++;
            
            if(path[1 + numlen] == '\0') return NULL;
            
            for(int i = 1; i < 1 + numlen; i++)
                if( path[i] < '0' || path[i] > '9' ) 
                    return NULL;
            
            string tempstr(path + 1, path + numlen + 1);
            int offset = atoi(tempstr.c_str());
            nextRecord = this->getElement(offset);
            if(!nextRecord) return NULL;
            pos = 2 + numlen;
        } else {
            if( (path[0] < 'a' || path[0] > 'z') &&
               (path[0] < 'A' || path[0] > 'Z') &&
               (path[0] != '_') )
                return NULL;
            
            pos = 1;
            while((path[pos] >= 'a' && path[pos] <= 'z') ||
                  (path[pos] >= 'A' && path[pos] <= 'Z') ||
                  (path[pos] >= '0' && path[pos] <= '9') ||
                  path[pos] == '_')
                pos++;
            
            string tempstr(path, path + pos);
            if(offsetmap.find(tempstr) == offsetmap.end())
                return NULL;
            
            int offset = offsetmap[tempstr];
            nextRecord = subrecords[offset];
            if(!nextRecord) return NULL; //FIXME: Needed?
        }
        
        if(path[pos] == '\0') return nextRecord;
        else if(path[pos] == '.') return nextRecord->getElement(&path[pos+1]);
        else if(path[pos] == '[') return nextRecord->getElement(&path[pos]);
        else return NULL;
    }
    
    Element *Record::getElement(int offset) {
        if(offset >= subrecords.size()) return NULL;
        return subrecords[offset];
    }
    
    namespace {
        class RecordIterator : public ElementIterator {
        public:
            RecordIterator(std::vector<Element *>::iterator begin, std::vector<Element *>::iterator end) 
            : current(begin), end(end) {}
            
            virtual Element *next() {
                return *current++;
            }
            
        protected:
            virtual bool int_more() {
                return (current != end);
            }
            
        private:
            std::vector<Element *>::iterator current, end;
        };
        
        class FilterRecordIterator : public RecordIterator {
        public:
            FilterRecordIterator(std::vector<Element *>::iterator begin, 
                                 std::vector<Element *>::iterator end,
                                 std::string type)
            : RecordIterator(begin, end), type(type), nextRec(NULL) {
                //Prime it first;
                findNext();
            }
            
            
            virtual Element *next() {
                Element *tempRec = nextRec;
                nextRec = NULL;
                findNext();
                return tempRec;
            }
            
        protected:
            virtual bool int_more() {
                return nextRec;
            }
            
        private:
            void findNext() {
                if(nextRec) return;
                
                while(this->RecordIterator::int_more()) {
                    Element *possNext = this->RecordIterator::next();
                    if(type == possNext->getType())
                        nextRec = possNext;
                    
                    if(nextRec) return;
                }
            }
            
            std::string type;
            Element *nextRec;
        };
    }
    
    ElementIterator *Record::getElements() {
        return new RecordIterator(subrecords.begin(), subrecords.end());
    }
    
    ElementIterator *Record::getElements(const char *type) {
        return new FilterRecordIterator(subrecords.begin(), subrecords.end(), type);
    }
    
    //SCALARRECORD IMPLEMENTATION
    
    Property::Property(string name, string type, string value) 
    : Element(name, type), value(value) {}
    
    bool Property::isProperty() {
        return true;
    }
    
    bool Property::isRecord() {
        return false;
    }
    
    Value Property::getValue() {
        return value.c_str();
    }
    
    int Property::getNumElements() {
        return 0;
    }
    
    Element *Property::getElement(const char *path) {
        return NULL;
    }
    
    Element *Property::getElement(int offset) {
        return NULL;
    }
    
    namespace {
        class EmptyIterator : public ElementIterator {
        public:
            virtual Element *next() {
                assert(!"Next called in empty iterator"); //Should never get here
                return NULL;
            }
            
        protected:
            virtual bool int_more() {
                return false;
            }
        };
    }
    
    ElementIterator *Property::getElements() {
        return new EmptyIterator();
    }
    
    ElementIterator *Property::getElements(const char *type) {
        return new EmptyIterator();
    }
}
