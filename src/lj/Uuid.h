#pragma once
/*!
 \file lj/Uuid.h
 \brief LJ Uuid header.
 \author Jason Watson

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

#include <initializer_list>
#include <string>
#include <stdint.h>

namespace lj
{
    /*!
     \brief Uuid value.

     Unique ID value.
     \since 1.0
     */
    class Uuid
    {
    public:
        static const Uuid k_nil; //!< constant nil value.
        static const Uuid k_ns_dns; //!< constant for DNS namespace.
        static const Uuid k_ns_url; //!< constant for URL namespace.
        static const Uuid k_ns_oid; //!< constant for ISO OID namespace.
        static const Uuid k_ns_x500; //!< constant for the X.500 DN namespace.

        /*!
         \brief Default constructor.

         Create a random Uuid.
         */
        Uuid();

        /*!
         \brief Initializer list constructor.

         Construct a Uuid from 16 constant bytes.
         \param d 16 bytes.
         */
        Uuid(std::initializer_list<uint8_t> d);

        /*!
         \brief Array constructor.

         Construct a Uuid from a 16 byte array.
         \param d 16 bytes.
         */
        explicit Uuid(const uint8_t d[16]);

        /*!
         \brief Copy constructor.
         \param o The other object.
         */
        Uuid(const Uuid& o);

        /*!
         \brief String constructor.

         Expects the input string in the format of
         {xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx}
         \param o The string.
         */
        explicit Uuid(const std::string& o);
        
        /*!
         \brief Version 5 constructor.

         The namespace and name are hased to create a version 5 UUID.
         \param ns The namespace uuid
         \param name The name.
         \param name_sz The length of the id byte array.
         */
        Uuid(const Uuid& ns, const void* name, const size_t name_sz);

        /*!
         \brief Version 5 constructor.

         The namespace and name are hased to create a version 5 UUID.
         Identical to Uuid::Uuid(const Uuid&, const void*, const size_t)
         except that it gets the size from the string.
         \param ns The namespace uuid
         \param name The name.
         */
        Uuid(const Uuid& ns, const std::string& name);

        /*!
         \brief Id constructor.

         ID is calculated into the Uuid.

         \note two Uuids created from the same ID are not
         guarenteed to be equal. They have a high probability of
         being not equal.
         \param o The id.
         */
        explicit Uuid(const uint64_t o);

        //! Destructor.
        ~Uuid();

        /*!
         \brief Assignment operator.
         \param o The right hand value.
         \return reference to this.
         */
        Uuid& operator=(const Uuid& o);

        /*!
         \brief Equality operator.
         \param o The right hand value.
         \return true if the two are equal, false otherwise.
         */
        bool operator==(const Uuid& o) const;

        /*!
         \brief Inequality operator.
         \param o The right hand value.
         \return false if the two are equal, true otherwise.
         */
        inline bool operator!=(const Uuid& o) const { return !((*this) == o); };

        /*!
         \brief Less than operator.
         \param o The right hand value.
         \return true if this object is less than \c o, false otherwise.
         */
        bool operator<(const Uuid& o) const;

        /*!
         \brief Less than or equal operator.
         \param o The right hand value.
         \return true if this object is less than or equal to \c o, false otherwise.
         */
        inline bool operator<=(const Uuid& o) const { return ((*this) == o) || ((*this) < o); };

        /*!
         \brief Greater than operator.
         \param o The right hand value.
         \return true if this object is greater than \c o, false otherwise.
         */
        inline bool operator>(const Uuid& o) const { return (o < (*this)); };

        /*!
         \brief Greater than or equal operator.
         \param o The right hand value.
         \return true if this object is greater than or equal to \c o, false otherwise.
         */
        inline bool operator>=(const Uuid& o) const { return (o <= (*this)); };

        /*!
         \brief String conversion

         Outputs in the format {xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx}.

         \return The string representation.
         */
        operator std::string() const;

        /*!
         \brief Integer conversion

         Converts the Uuid to the embedded id value.

         \return The embedded id value.
         */
        operator uint64_t() const;

        /*!
         \brief Debug string conversion.

         Outputs in the format {xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx}/yyyy,
         where yyyy is the embedded id value.

         \return The string representation.
         */
        std::string str() const;

        /*!
         \brief Data accessor.

         size will always be set to 16.

         \param sz Pointer to a location to store the data size.
         \return Pointer to the data.
         */
        inline const uint8_t* const data(size_t* sz) const { *sz = 16; return data_; };
    private:
        uint8_t data_[16];
    }; // clase lj::Uuid
}; // namespace lj
