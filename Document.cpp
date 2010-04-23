/*
 \file Document.cpp
 \author Jason Watson
 Copyright (c) 2010, Jason Watson
 All rights reserved.
 
 Redistribution and use in source and binary forms, with or without
 modification, are permitted provided that the following conditions are met:
 
 * Redistributions of source code must retain the above copyright notice,
 this list of conditions and the following disclaimer.
 
 * Redistributions in binary form must reproduce the above copyright notice,
 this list of conditions and the following disclaimer in the documentation
 and/or other materials provided with the distribution.
 
 * Neither the name of the LogJammin nor the names of its contributors
 may be used to endorse or promote products derived from this software
 without specific prior written permission.
 
 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 POSSIBILITY OF SUCH DAMAGE.
 */

#include "build/default/config.h"
#include "Document.h"
#define BUFFER_SIZE 1024

namespace tokyo {
    namespace {
        enum WhatShouldThisBe {
            DOC_SIZE,
            FIELD_TYPE,
            FIELD_NAME,
            FIELD_VALUE,
            STRING_END,
            DOC_END
        };
        size_t field_length(DocumentNodeType t) {
            switch(t) {
                case INT32_NODE:
                    return 4;
                case INT64_NODE:
                case TIMESTAMP_NODE:
                case DOUBLE_NODE:
                    return 8;
                case STRING_NODE:
                    return 5;
                case DOC_NODE:
                case ARRAY_NODE:
                    return 5;
                case BOOL_NODE:
                    return 1;
                case NULL_NODE:
                    return 0;
                default:
                    return -1;
            };
        }
    };
    StreamingBSONParser::StreamingBSONParser() {
        buffer = new char[BUFFER_SIZE + 1];
    }
    StreamingBSONParser::~StreamingBSONParser() {
        delete[] buffer;
    }
    void StreamingBSONParser::parse(std::istream is) {
        DocumentNodeType t = DOC_NODE;
        WhatShouldThisBe looking_at = DOC_SIZE;
        std::list<long long> doc_sizes;
        size_t sz = 0;
        size_t curr = 0;
        size_t read_after = 0;
        size_t docsz = 0, tmp = 0;
        
        while(is.good()) {
            is.read(buffer + read_after, BUFFER_SIZE - read_after);
            sz = is.gcount() + read_after;
            curr = 0;
            buffer[sz] = 0;
            
            while(1) {
                if((looking_at == DOC_SIZE && sz - curr < 4) ||
                   (looking_at == FIELD_TYPE && sz - curr < 1) ||
                   (looking_at == DOC_END && sz - curr < 1) ||
                   (looking_at == FIELD_NAME && sz - curr == strlen(buffer + curr)) ||
                   (looking_at == FIELD_VALUE && sz - curr < field_length(t)) ||
                   (looking_at == STRING_END && sz - curr < 1)) {
                    read_after = sz - curr;
                    memmove(buffer, buffer + curr, read_after);
                    break;
                }
                switch(looking_at) {
                    case DOC_SIZE:
                        tmp = 0;
                        memcpy(&tmp, buffer + curr, 4);
                        start_doc(tmp);
                        
                        doc_sizes.push_back(docsz - tmp);
                        docsz = tmp;
                        
                        curr += 4;
                        docsz -= 4;
                        
                        looking_at = FIELD_TYPE;
                        
                        break;
                    case FIELD_TYPE:
                        t = (DocumentNodeType)buffer[curr];
                        
                        curr++;
                        docsz--;
                        
                        looking_at = FIELD_NAME;
                        
                        break;
                    case FIELD_NAME:
                        tmp = strlen(buffer + curr);
                        start_field(t, std::string(buffer + curr));
                        
                        curr += tmp + 1;
                        docsz -= tmp + 1;
                        
                        looking_at = FIELD_VALUE;
                        break;
                    case STRING_END:
                        tmp = (sz - curr > docsz) ? docsz : (sz - curr);
                        bytes(buffer + curr, tmp);
                        
                        docsz -= tmp;
                        curr += tmp;
                        
                        if(!docsz) {
                            docsz = doc_sizes.back();
                            doc_sizes.pop_back();
                            looking_at = FIELD_TYPE;
                        }
                        break;
                    case FIELD_VALUE:
                        switch(t) {
                            case BOOL_NODE:
                                bytes(buffer + curr, 1);
                                curr++;
                                docsz--;
                                looking_at = FIELD_TYPE;
                                break;
                            case INT32_NODE:
                                bytes(buffer + curr, 4);
                                curr += 4;
                                docsz -= 4;
                                looking_at = FIELD_TYPE;
                                break;
                            case TIMESTAMP_NODE:
                            case DOUBLE_NODE:
                            case INT64_NODE:
                                bytes(buffer + curr, 8);
                                curr += 8;
                                docsz -= 8;
                                looking_at = FIELD_TYPE;
                                break;
                            case NULL_NODE:
                                bytes(buffer, 0);
                                looking_at = FIELD_TYPE;
                                break;
                            case STRING_NODE:
                                tmp = 0;
                                memcpy(&tmp, buffer + curr, 4);
                                bytes(buffer + curr, 4);
                                
                                curr += 4;
                                docsz -= 4;

                                doc_sizes.push_back(docsz - tmp);
                                docsz = tmp;

                                looking_at = STRING_END;
                                break;
                            case DOC_NODE:
                            case ARRAY_NODE:
                                tmp = 0;
                                memcpy(&tmp, buffer + curr, 4);
                                bytes(buffer + curr, 4);
                                
                                curr += 4;
                                docsz -= 4;
                                
                                doc_sizes.push_back(docsz - tmp);
                                docsz = tmp;
                                
                                looking_at = DOC_SIZE;
                                break;
                            default:
                                break;
                        };
                        break;
                    case DOC_END:
                        end_doc();
                        docsz--;
                        curr++;
                        
                        docsz = doc_sizes.back();
                        doc_sizes.pop_back();
                        
                        looking_at = FIELD_TYPE;
                        break;
                };
                if(docsz <= 1) {
                    looking_at = DOC_END;
                }
            }
        }
    }
    
};