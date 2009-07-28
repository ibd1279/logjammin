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

#include "openprop_int.h"
#include "parser.h"

#include <istream>
#include <list>

namespace OpenProp {
    
    class CharStream {
    public:
        CharStream(std::istream &input) : input(input) {}
        
        bool eof() { return input.eof(); }
        char get() {
            char c;
            c = input.get();
            return c;
        }
        
        char peek() {
            return input.peek();
        }
        
        void put(char c) { input.putback(c); }
        
    private:
        std::istream &input;
    };
    
    class Token {
    public:
        enum Type {
            NONE,
            ID,
            STRING,
            EQUALS,
            COLON,
            DOUBLECOLON,
            LBRACE,
            RBRACE,
            SEMI,
            ERROR,
            _EOF
        };
        
        Token() : type(NONE), text("") {}
        Token(Type type, const std::string &text) : type(type), text(text) {}
        Token(Type type, const char ch) : type(type), text() {
            text.push_back(ch);
        }
        
        Type getType() {return type;}
        const std::string &getText() {return text;}
        
    private:
        Type type;
        std::string text;
    };
    
    void int_itoa(int number, std::string &str) {
        if(number == 0) return;
        int digit = number % 10;
        int_itoa(number / 10, str);
        str.push_back('0' + digit);
    }
    
    void itoa(int number, std::string &str) {
        if(number == 0) {
            str.push_back('0');
        } else if(number < 0) {
            str.push_back('-');
            int_itoa(number, str);
        } else {
            int_itoa(number, str);
        }
    }
    
    //FIXME: We may want to do nested comments at some point
    void lexBlockComment(CharStream &input) {
        char c;
        if(input.peek() != '/') return;
        c = input.get();
        c = input.get();
        if(c == '*') {
            while(1) {
                c = input.get();
                if(c == '*') {
                    c = input.get();
                    if( c == '/' ) return;
                }
            }
        } else if(c == '/') {
            while(c != '\n' && c != '\r' && !input.eof()) {
                c = input.get();
            }
        }
    }
    
    Token lex(std::istream &inp) {
        CharStream input(inp);
        if(input.eof()) return Token(Token::_EOF, "");
        char c;
        while(!input.eof()) {
            switch(input.peek()) {
                case '/':
                    lexBlockComment(input);
                    continue;
                    
                case ';':
                    c = input.get();
                    return Token( Token::SEMI, c);
                    
                case '{':
                    c = input.get();
                    return Token( Token::LBRACE, c);
                    
                case '}':
                    c = input.get();
                    return Token( Token::RBRACE, c);
                    
                case '=':
                    c = input.get();
                    return Token( Token::EQUALS, c);
                    
                case ':': 
                {
                    c = input.get();
                    std::string tempstr;
                    tempstr.push_back(c);
                    //FIXME: Do we really need the doublecolon?
                    if(input.peek() == ':') {
                        c = input.get();
                        tempstr.push_back(c);
                        return Token( Token::DOUBLECOLON, tempstr );
                    }
                    
                    return Token( Token::COLON, tempstr );
                }
                    /*
                     
                     case '0':
                     case '1':
                     case '2':
                     case '3':
                     case '4':
                     case '5':
                     case '6':
                     case '7':
                     case '8':
                     case '9':
                     case '-':
                     {
                     c = input.get();
                     input.putback(c);
                     int number;
                     input >> number;
                     string tempstr = "";
                     itoa(number, tempstr);
                     return Token( Token::INT, tempstr );
                     }
                     */
                    
                case '"':
                {
                    c = input.get();
                    std::string tempstr = "";
                    do {
                        if(input.eof()) return Token(Token::ERROR, "");
                        c = input.get();
                        if(c == '\\') {
                            if(input.eof()) return Token(Token::ERROR, "");
                            c = input.get();
                            switch(c) {
                                case 'n':
                                    tempstr.push_back('\n');
                                    break;
                                    
                                case '\n':
                                    break;
                                    
                                default:
                                    tempstr.push_back(c);
                                    break;
                            }
                        } else if(c == '"') {
                            break;
                        } else {
                            tempstr.push_back(c);
                        }
                    } while(1);
                    
                    return Token(Token::STRING, tempstr);
                }
                case 'a':
                case 'b':
                case 'c':
                case 'd':
                case 'e':
                case 'f':
                case 'g':
                case 'h':
                case 'i':
                case 'j':
                case 'k':
                case 'l':
                case 'm':
                case 'n':
                case 'o':
                case 'p':
                case 'q':
                case 'r':
                case 's':
                case 't':
                case 'u':
                case 'v':
                case 'w':
                case 'x':
                case 'y':
                case 'z':
                    
                case 'A':
                case 'B':
                case 'C':
                case 'D':
                case 'E':
                case 'F':
                case 'G':
                case 'H':
                case 'I':
                case 'J':
                case 'K':
                case 'L':
                case 'M':
                case 'N':
                case 'O':
                case 'P':
                case 'Q':
                case 'R':
                case 'S':
                case 'T':
                case 'U':
                case 'V':
                case 'W':
                case 'X':
                case 'Y':
                case 'Z':
                    
                case '_':
                { //IDENTIFIER
                    c = input.get();
                    std::string tempstr = "";
                    while(1) {
                        tempstr.push_back(c);
                        if(input.eof()) return Token( Token::_EOF, "" );
                        char tempc = input.peek();
                        if((tempc < 'a' || tempc > 'z') &&
                           (tempc < 'A' || tempc > 'Z') &&
                           (tempc < '0' || tempc > '9') &&
                           tempc != '_')
                            break;
                        c = input.get();
                    }
                    
                    return Token( Token::ID, tempstr );
                }
            }
            c = input.get();
        }
        return Token( Token::_EOF, "" );
    }
    std::list<Token> tokList;
    
    void populateList(std::istream &input) {
        while(1) {
            Token tok = lex(input);
            tokList.push_back(tok);
            if(tok.getType() == Token::_EOF) return;
        }
    }
    
    void clearList() {
        tokList.clear();
    }
    
    bool parseRecordList(Record *record) {
        while(1) {
            Element *nextRec;
            switch(tokList.front().getType()) {
                case Token::ID:
                case Token::LBRACE:
                case Token::STRING:
                    nextRec = parse();
                    break;
                    
                default:
                    return true;
            }
            
            if(!nextRec) return false;
            record->append(nextRec);
            if(tokList.front().getType() == Token::SEMI)
                tokList.pop_front();
        }
    }
    
    Element *parse() {
        Token tok = tokList.front();
        switch(tokList.front().getType()) {
            case Token::ID:
            {
                std::string name = tokList.front().getText();
                std::string type = name;
                
                tokList.pop_front();
                
                if(tokList.front().getType() == Token::COLON) {
                    tokList.pop_front();
                    if(!tokList.front().getType() == Token::ID) 
                        return NULL;
                    type = tokList.front().getText();
                    tokList.pop_front();
                }
                
                
                switch(tokList.front().getType()) {
                    case Token::LBRACE:
                    {
                        tokList.pop_front();
                        Record *multrec = new Record(name, type);
                        if(!parseRecordList(multrec) || tokList.front().getType() != Token::RBRACE) {
                            delete multrec;
                            return NULL;
                        }
                        
                        tokList.pop_front();
                        
                        return multrec;
                    }
                        
                    case Token::EQUALS:
                    {
                        tokList.pop_front();
                        switch(tokList.front().getType()) {
                            case Token::STRING:
                            {
                                Property *prop = new Property(name, type, tokList.front().getText());
                                tokList.pop_front();
                                return prop;
                            }
                            default:
                                return NULL;
                        }
                    }
                        
                    case Token::DOUBLECOLON:
                    {
                        tokList.pop_front();
                        Element *nextRecord = parse();
                        if(!nextRecord) return NULL;
                        Record *multrec = new Record(name, type);
                        multrec->append(nextRecord);
                        return multrec;
                    }
                        
                    case Token::ID:
                    case Token::SEMI:
                    {   //valuless property. Set to something which will evaluate to true
                        return new Property(name, type, "<exists>");
                    }
                        
                    default:
                        return NULL;
                }
            }
                
            case Token::LBRACE:
            {
                tokList.pop_front();
                Record *multrec = new Record("<unnamed>", "<unnamed>");
                if(!parseRecordList(multrec) || tokList.front().getType() != Token::RBRACE) {
                    delete multrec;
                    return NULL;
                }
                tokList.pop_front();
                
                return multrec;
            }
                
            case Token::STRING:
            {
                Property *prop = new Property("<unnamed>", "<unnamed>", tokList.front().getText());
                tokList.pop_front();
                return prop;
            }
                
            default:
                return NULL;
        }
    }
}
