#pragma once
/*!
 \file lj/Args.h
 \brief LJ argument parser
 
 Copyright (c) 2014, Jason Watson
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
    class Arg_parser;

    /*!
     \brief Argument base class.
     \since 1.0
     */
    class Arg
    {
    public:
        //! Type of Argument.
        enum class Type {
            k_flag, //!< boolean flag argument (present or not).
            k_setting, //!< setting argument (key value strings).
            k_list //!< series of string values.
        }; // enum lj::Arg::Type

        /*!
         \brief Create a new argument.
         \param parser The Arg_parser object.
         \param a_short_name The short name for this argument.
         \param a_long_name The long name for this argument.
         \param a_description Help text for this argument.
         \param a_type The argument type.
         */
        Arg(Arg_parser& parser,
                const std::string& a_short_name,
                const std::string& a_long_name,
                const std::string& a_description,
                const lj::Arg::Type a_type);
        //! Arg destructor
        virtual ~Arg() = default;

        //! Long name for an argument.
        inline const std::string& long_name() const { return long_name_; }

        //! Short name for an argument.
        inline const std::string& short_name() const { return short_name_; }
        
        //! Help description for an argument.
        inline const std::string& description() const { return description_; }
    
        //! Type of argument
        inline const lj::Arg::Type& type() const { return type_; }

        //! Is the argument present?
        inline bool present() const { return present_; }

        //! Is the argument required?
        inline bool required() const { return required_; }

        //! Set if the argument is required.
        inline Arg& required(const bool is_required) { required_ = is_required; return *this; }

        //! Set if the argument is present.
        inline void present(const bool is_present) { present_ = is_present; }

        /*!
         \brief consume a string provided from the command line.
         \param arg The argument value to consume.
         */
        virtual void consume(const std::string& arg) = 0;

    private:
        const std::string long_name_;
        const std::string short_name_;
        const std::string description_;
        const lj::Arg::Type type_;
        bool present_;
        bool required_;
    }; // class lj::Arg

    /*!
     \brief Class for parsing argument.
     \since 1.0

     Sample usage:
         \code
int main(int argc, char* const argv[]) {
    lj::Arg_parser arg_parser(argc, argv);
    lj::Setting_arg config_setting(arg_parser,
            "-c",
            "--config",
            "Location of the configuration file.",
            "");
    lj::Flag_arg verbose_flag(arg_parser,
            "-v",
            "--verbose",
            "Output a bunch of normally internal stuff.");

    arg_parser.parse();
    if (verbose_flag.boolean())
    {
        std::cout << "loading config from: " <<
                config_setting.str() <<
                std::endl;
    }

    return 0;
}
         \endcode
     */
    class Arg_parser 
    {
    public:
        //! Create a new arg parser.
        Arg_parser(const int argc, const char* const* argv)
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

        //! Destructor
        virtual ~Arg_parser() = default;

        /*!
         \brief Attach an argument to this parser.

         Arguments must be attached before calling \c Arg_parser::parse().

         \warning Calling this is handled in the \c Arg::Arg constructor, and
         you shouldn't need to call it yourself.

         \note This does not take ownership of the pointer, and does not
         release the associated memory.

         \param arg Pointer to the argument object to attach.
         */
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

        //! Prase the command line args based on the attached arg objects.
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

        //! The executable name.
        virtual const std::string& command() const { return cmd_; }

        //! The unparsed list of arguments after the executable name.
        virtual const std::list<std::string>& args() const { return args_; }
    private:
        std::map<std::string, Arg*> lookup_map_;
        std::list<Arg*> help_list_;
        std::string cmd_;
        std::list<std::string> args_;
    }; // class lj::Arg_parser
    
    Arg::Arg(Arg_parser& parser,
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

    /*!
     \brief Flag Argument Type.
     \since 1.0

     The default for the flag argument is false. The value will
     be set to true if the flag is provided as an argument.

     \note Repeated settings do not change the value.
     */
    class Flag_arg : public Arg
    {
    public:
        /*!
         \brief Create a boolean flag.
         \param parser The Arg_parser object.
         \param a_short_name The short name for this argument.
         \param a_long_name The long name for this argument.
         \param a_description Help text for this argument.
         */
        Flag_arg(Arg_parser& parser,
                const std::string& a_short_name,
                const std::string& a_long_name,
                const std::string& a_description) :
                Arg(parser, a_short_name, a_long_name, a_description, lj::Arg::Type::k_flag),
                value_(false) {}

        //! Destructor
        virtual ~Flag_arg() = default;

        //! Set the flag value.
        inline void value(const bool a_value) { value_ = a_value; }

        //! Get the flag value.
        inline bool boolean() const { return value_; }

        virtual void consume(const std::string& arg) override { value(true); }
    private:
        bool value_;
    }; // class lj::Flag_arg

    /*!
     \brief Setting Argument Type.
     \since 1.0

     You must provide a default value. If you need to check for the actual
     presence of the setting, use the \c Arg::present() method.

     \note The value for only the last setting encountered is stored.
     */
    class Setting_arg : public Arg
    {
    public:
        /*!
         \brief Create a setting flag.
         \param parser The Arg_parser object.
         \param a_short_name The short name for this argument.
         \param a_long_name The long name for this argument.
         \param a_description Help text for this argument.
         \param a_default The default value if the argument isn't provided.
         */
        Setting_arg(Arg_parser& parser, 
                const std::string& a_short_name,
                const std::string& a_long_name,
                const std::string& a_description,
                const std::string& a_default) :
                Arg(parser, a_short_name, a_long_name, a_description, lj::Arg::Type::k_setting),
                value_(a_default) {}
        //! Destructor
        virtual ~Setting_arg() = default;

        //! Set the setting value.
        inline void value(const std::string& a_value) { value_ = a_value; }

        //! Get the setting value.
        inline const std::string& str() const { return value_; }

        virtual void consume(const std::string& arg) override { value(arg); }
    private:
        std::string value_;
    }; // class lj::Setting_arg

    /*!
     \brief List Argument Type.
     \since 1.0

     You must provide a default value. If you need to check for the actual
     presence of the setting, use the \c Arg::present() method. The default
     will only be used for the value if the setting is not present at all.

     \note
     All encountered settings are additive for the value.
     */
    class List_arg : public Arg
    {
    public:
        /*!
         \brief Create a setting flag.
         \param parser The Arg_parser object.
         \param a_short_name The short name for this argument.
         \param a_long_name The long name for this argument.
         \param a_description Help text for this argument.
         \param a_default The default value if the argument isn't provided.
         */
        List_arg(Arg_parser& parser,
                const std::string& a_long_name,
                const std::string& a_short_name,
                const std::string& a_description,
                const std::list<std::string>& a_default) :
                Arg(parser, a_short_name, a_long_name, a_description, lj::Arg::Type::k_list),
                default_value_(a_default) {}

        //! Destructor
        virtual ~List_arg() = default;

        //! Set the list value.
        inline void value(const std::list<std::string>& a_value) { value_ = a_value; }

        //! Get the list value.
        inline const std::list<std::string>& list() const { return present() ? value_ : default_value_; }

        virtual void consume(const std::string& arg) override { value_.push_back(arg); }
    private:
        std::list<std::string> default_value_;
        std::list<std::string> value_;
    }; // class lj::List_arg
}; // namespace lj
