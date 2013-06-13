/*!
 \file xbn/Merkle.cpp
 \brief LJ Bitcoin Merkle Function Implementations.
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

#include "xbn/Merkle.h"
#include "xbn/Digests.h"
#include "lj/Exception.h"

extern "C" {
#include "nettle/sha2.h"
#include "nettle/ripemd160.h"
}


namespace xbn
{
    namespace 
    {
        void double_sha256(const xbn::merkle::Node& input1,
                const xbn::merkle::Node& input2,
                xbn::merkle::Node& output)
        {
            // Allocate some stuff on the stack.
            uint8_t buffer[SHA256_DIGEST_SIZE];
            struct sha256_ctx ctx;

            sha256_init(&ctx);
            sha256_update(&ctx, SHA256_DIGEST_SIZE, input1.bytes);
            sha256_update(&ctx, SHA256_DIGEST_SIZE, input2.bytes);
            sha256_digest(&ctx, SHA256_DIGEST_SIZE, buffer);
            sha256_update(&ctx, SHA256_DIGEST_SIZE, buffer);
            sha256_digest(&ctx, SHA256_DIGEST_SIZE, output.bytes);
        }
    } // namespace {anonymous}

    namespace merkle
    {
        size_t node_count(size_t leaf_count)
        {
            size_t count = 1;
            while(leaf_count > 1)
            {
                if (leaf_count & 1)
                {
                    ++leaf_count;
                }
                count += leaf_count;
                leaf_count >>= 1;
            }
            return count;
        }

        Tree::Tree(const std::vector<Node>& data) :
                data_(new std::vector<Node>(data))
        {
            if (data.size() == 0)
            {
                // If we got zero digests, we cannot go any fruther.
                data_.reset();
                throw lj::Exception("xbn::merkle::Tree",
                        "cannot create a merkle tree for zero inputs.");
            }

            // prepare to handle all of the branches.
            size_t input_count = data_->size();
            data_->reserve(node_count(input_count));
            auto iter(data_->cbegin());
            
            // calculate all the branches.
            while (input_count > 1)
            {
                if (input_count & 1)
                {
                    // copy an extra element if the count is not divisible by 2
                    data_->push_back(data_->back());
                    ++input_count;
                }

                // calcuate the branches at the next level
                input_count >>= 1;
                for (int h = 0; h < input_count; ++h)
                {
                    const Node& left(*(iter++));
                    const Node& right(*(iter++));
                    data_->emplace_back();
                    double_sha256(left, right, data_->back());
                }
            }
        }

        namespace
        {
            int node_compare(const Node& left, const Node& right)
            {
                return memcmp(left.bytes, right.bytes, SHA256_DIGEST_SIZE);
            }
        }; // namespace xbn::merkle::{anonymous}

        std::list<std::list<xbn::merkle::Node>> as_list(const xbn::merkle::Tree& tree)
        {
            // Note: This function builds up the list "backwards"; things
            // get pushed to the front. This is because the tree may have
            // be calculated from the leaves to the root, but building the
            // levels the correct size is easier when we parse it from 
            // the root to the leaves.

            std::list<std::list<xbn::merkle::Node>> result;
            size_t level_size = 1;
            auto iter = tree.data()->crbegin();
            while (tree.data()->crend() != iter)
            {
                // construct the level we are about to build.
                result.emplace_front();

                // push the number of elements for this level on.
                for (int h = 0;
                        h < level_size && tree.data()->crend() != iter;
                        ++h, ++iter)
                {
                    result.front().push_front(*iter);
                }

                // If we have more than one element, see if the last two elements are equal.
                if (result.front().size() > 1)
                {
                    if (node_compare(*result.front().crbegin(), *(++(result.front().crbegin()))) == 0)
                    {
                        // They are equal, we need to get rid of the element that isn't
                        // actually part of the merkle tree; only part of the calculation.
                        result.front().pop_back();
                    }
                }

                // set the size of the next level as the size of this level times 2.
                level_size = result.front().size() << 1;
            }
            return result;
        }
    }; // namespace xbn::merkle
}; // namespace xbn
