#pragma once
/*!
 \file lj/Args.h
 \brief LJ argument parser
 \author Jason Watson
 
 Copyright (c) 2013, Jason Watson
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

#include "lj/Exception.h"
#include <cassert>
#include <list>
#include <map>
#include <string>

namespace lj
{
    class ArgParser;

    //! Argument base class.
    /*!
     \author Jason Watson
     \version 1.0
     \date June 13, 2014
     */
    class Arg
    {
    public:
        enum class Type {
            k_flag,
            k_setting,
            k_list
        }; // namespace lj::Arg::Type

        Arg(ArgParser& parser,
                const std::string& a_short_name,
                const std::string& a_long_name,
                const std::string& a_description,
                const lj::Arg::Type a_type);
        virtual ~Arg() {}
        inline const std::string& long_name() const { return long_name_; }
        inline const std::string& short_name() const { return short_name_; }
        inline const std::string& description() const { return description_; }
        inline const lj::Arg::Type& type() const { return type_; }
        inline bool present() const { return present_; }
        inline bool required() const { return required_; }
        inline Arg& required(const bool is_required) { required_ = is_required; return *this; }
        inline void present(const bool is_present) { present_ = is_present; }
        virtual void consume(const std::string& arg) = 0;

    private:
        const std::string long_name_;
        const std::string short_name_;
        const std::string description_;
        const lj::Arg::Type type_;
        bool present_;
        bool required_;
    }; // namespace lj::Arg

    //! Class for parsing arguments.
    /*!
     \author Jason Watson
     \version 1.0
     \date June 13, 2014
     */
    class ArgParser 
    {
    public:
        ArgParser(const int argc, const char** argv)
        {
            for (int h = 0; h < argc; ++h)
            {
                args_.push_back(std::string(argv[h]));
            }

            if (args_.size() > 0)
            {
                cmd_ = args_.front();
                args_.pop_front();
            }
        }
        virtual ~ArgParser() {}
        virtual void attach(Arg* arg)
        {
            assert(arg);

            if (arg->short_name().empty() && arg->long_name().empty())
            {
                // A nameless argument is useless
                throw lj::Exception("Argument Parser", arg->description() + " does not have a name");
            }

            // Check to see if the argument names are already used.
            auto map_iter = lookup_map_.find(arg->short_name());
            if (map_iter != lookup_map_.end())
            {
                throw lj::Exception("Argument Parser", arg->short_name() + " is already used by " + map_iter->second->long_name());
            }
            map_iter = lookup_map_.find(arg->long_name());
            if (map_iter != lookup_map_.end())
            {
                throw lj::Exception("Argument Parser", arg->long_name() + " is already used by " + map_iter->second->description());
            }

            // Validate some other constraints.
            if (arg->short_name().compare("--") == 0 && arg->type() != lj::Arg::Type::k_list)
            {
                throw lj::Exception("Argument Parser", "The double dash argument must accept a list.");
            }

            // Add it to the parsing maps.
            if (!arg->short_name().empty())
            {
                lookup_map_.insert(std::pair<std::string, Arg*>(arg->short_name(), arg));
            }
            if (!arg->long_name().empty())
            {
                lookup_map_.insert(std::pair<std::string, Arg*>(arg->long_name(), arg));
            }

            // Add it to the help list.
            help_list_.push_back(arg);
        }

        virtual void parse()
        {
            Arg* arg = nullptr;
            bool double_dash = false;
            for (const std::string& cl_arg : args_)
            {
                if (!arg)
                {
                    // First, split up any self assigned flags.
                    std::string lookup_arg;
                    std::string value_arg;
                    const int equal_pos = cl_arg.find('=');
                    if (equal_pos != std::string::npos)
                    {
                        lookup_arg = cl_arg.substr(0, equal_pos);
                        value_arg = cl_arg.substr(equal_pos + 1);
                    }
                    else
                    {
                        lookup_arg = cl_arg;
                    }

                    // Find the argument object for consuming this.
                    auto map_iter = lookup_map_.find(lookup_arg);
                    if (map_iter == lookup_map_.end())
                    {
                        throw lj::Exception("Invalid Argument", command() + " doesn't know how to deal with " + cl_arg);
                    }
                    arg = map_iter->second;
                    arg->present(true);

                    if (arg->short_name().compare("--") == 0)
                    {
                        // goto double dash mode. Everything is collected into the last arg.
                        double_dash = true;
                    }
                    else if (arg->type() == lj::Arg::Type::k_flag || equal_pos != std::string::npos)
                    {
                        // Only assign the value if we have a flag or a self assigned flag.
                        arg->consume(value_arg);
                        arg = nullptr;
                    }
                }
                else
                {
                    arg->consume(cl_arg);
                    if (!double_dash)
                    {
                        arg = nullptr;
                    }
                }
            }

            for (Arg* an_arg : help_list_)
            {
                if (an_arg->required() && !an_arg->present())
                {
                    throw lj::Exception("Missing Argument", an_arg->long_name() + " is a required, but not present.");
                }
            }
        }

        virtual const std::string& command() const { return cmd_; }
        virtual const std::list<std::string>& args() const { return args_; }
    private:
        std::map<std::string, Arg*> lookup_map_;
        std::list<Arg*> help_list_;
        std::string cmd_;
        std::list<std::string> args_;
    }; // namespace lj::ArgParser
    
    Arg::Arg(ArgParser& parser,
                const std::string& a_short_name,
                const std::string& a_long_name,
                const std::string& a_description,
                const lj::Arg::Type a_type) :
                long_name_(a_long_name),
                short_name_(a_short_name),
                description_(a_description),
                type_(a_type),
                present_(false),
                required_(false)
    {
        parser.attach(this);
    }

    //! Flag Argument Type.
    /*!
     \author Jason Watson
     \version 1.0
     \date June 13, 2014
     */
    class FlagArg : public Arg
    {
    public:
        FlagArg(ArgParser& parser,
                const std::string& a_short_name,
                const std::string& a_long_name,
                const std::string& a_description) :
                Arg(parser, a_short_name, a_long_name, a_description, lj::Arg::Type::k_flag),
                value_(false) {}
        virtual ~FlagArg() {}
        inline void value(const bool a_value) { value_ = a_value; }
        inline bool boolean() const { return value_; }
        virtual void consume(const std::string& arg) override { value(true); }
    private:
        bool value_;
    }; // namespace lj::FlagArg

    //! Setting Argument Type.
    /*!
     \author Jason Watson
     \version 1.0
     \date June 13, 2014
     */
    class SettingArg : public Arg
    {
    public:
        SettingArg(ArgParser& parser, 
                const std::string& a_short_name,
                const std::string& a_long_name,
                const std::string& a_description,
                const std::string& a_default) :
                Arg(parser, a_short_name, a_long_name, a_description, lj::Arg::Type::k_setting),
                value_(a_default) {}
        virtual ~SettingArg() {}
        inline void value(const std::string& a_value) { value_ = a_value; }
        inline const std::string& str() const { return value_; }
        virtual void consume(const std::string& arg) override { value(arg); }
    private:
        std::string value_;
    }; // namespace lj::SettingArg

    //! List Argument Type.
    /*!
     \author Jason Watson
     \version 1.0
     \date June 13, 2014
     */
    class ListArg : public Arg
    {
    public:
        ListArg(ArgParser& parser,
                const std::string& a_long_name,
                const std::string& a_short_name,
                const std::string& a_description,
                const std::list<std::string>& a_default) :
                Arg(parser, a_short_name, a_long_name, a_description, lj::Arg::Type::k_list),
                default_value_(a_default) {}
        virtual ~ListArg() {}
        inline void value(const std::list<std::string>& a_value) { value_ = a_value; }
        inline const std::list<std::string>& list() const { return present() ? value_ : default_value_; }
        virtual void consume(const std::string& arg) override { value_.push_back(arg); }
    private:
        std::list<std::string> default_value_;
        std::list<std::string> value_;
    }; // namespace lj::ListArg
}; // namespace lj
