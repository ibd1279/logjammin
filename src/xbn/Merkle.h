#pragma once
/*!
 \file xbn/Merkle.h
 \brief LJ Bitcoin Merkle Functions.
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

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace xbn
{
    namespace merkle
    {
        //! Get the amount of memory necessary for a given leaf count.
        /*!
         \par Uneven levels.
         This implementation duplicates the values for uneven levels.
         while this increases the memory consumption, it simplifies
         the logic necessary for creating the tree, and reduces some
         memory copy operations.
         \param leaf_count The number of leaf nodes for the tree.
         \return The tital number of nodes that must be allocated.
         */
        size_t node_count(size_t leaf_count);

        //! Node in a Merkle tree.
        /*!
         Merkle tree nodes are 256bit digest results.
         */
        struct Node
        {
            uint8_t bytes[32]; //!< The node bytes.
        }; // struct xbn::merkle::Node

        //! The Merkle tree.
        class Tree
        {
        public:
            //! Construct a new tree from the provided data.
            /*!
             \par
             The merkle tree copies what it needs from the provided
             \c data param. It does not use the pointer after
             construction.
             \param data The leaf data
             */
            Tree(const std::vector<Node>& data);

            //! Copy constructor
            /*!
             \param orig The original object to copy from.
             */
            Tree(const Tree& orig) :
                    data_(orig.data_)
            {
            }

            //! Destructor
            ~Tree() {}

            //! Copy assignment operator
            /*!
             \param orig The original object to copy from.
             */
            Tree& operator=(const Tree& orig)
            {
                // protect against self assignment.
                if (data_.get() != orig.data_.get())
                {
                    data_ = orig.data_;
                }
                return *this;
            }

            //! Get the merkle root.
            /*!
             \return The merkle root for this tree.
             */
            inline const xbn::merkle::Node& root() const
            {
                return data_->back();
            }

            //! Get the merkle tree data.
            /*!
             \param[out] sz The size of the data.
             \return The merkle data.
             */
            inline const std::shared_ptr<std::vector<xbn::merkle::Node>> data() const
            {
                return data_;
            }

            //! Get the merkle tree size.
            /*!
             \return The merkle tree size.
             */
            inline size_t size() const { return data_->size(); }
        private:
            std::shared_ptr<std::vector<xbn::merkle::Node>> data_;
        }; // class xbn::merkle::Tree
    }; // namespace xbn::merkle
}; // namespace xbn
