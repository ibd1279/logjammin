/*!
 \file test/bcn/Digests.cpp
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

#include "testhelper.h"
#include "xbn/Digests.h"
#include "xbn/Merkle.h"
#include "lj/Base64.h"
#include "test/xbn/DigestsTest_driver.h"

void testDoubleSha256()
{
    std::string input("hello");
    uint8_t output[32];
    xbn::double_sha256(input, output);
    std::string result(xbn::as_string(output, 32));
    std::string expected("503d8319a48348cdc610a582f7bf754b5833df65038606eb48510790dfc99595");
    TEST_ASSERT_MSG(expected + " == " + result,
            expected.compare(result) == 0);
}

void testRipemd160Sha256()
{
    std::string input("hello");
    uint8_t output[20];
    xbn::ripemd160_sha256(input, output);
    std::string result(xbn::as_string(output, 20));
    std::string expected("0f7ddc6655f050b4a83183747c2b7230c2c8a9b6");
    TEST_ASSERT_MSG(expected + " == " + result,
            expected.compare(result) == 0);
}

void testStringConversion()
{
    std::list<std::string> string_inputs{
            "727218593f8166e7d0a00e3b49cb7787aaf3cdd158e211d4cd8bd9c10f74710b",
            "c20253c83e3ea464eb82c645ae515c2cef88ab25fa1803ce145040d5fc7f8c0e",
            "283e2e2e84049009ab0a95423d1ebccba3668b5be88d6969cd5623e50265ff71",
            "283e2e2e84049009ab0a95423d1ebccba3668b5be88d6969cd5623e50265ff71",
            "e92396c3e15db75758dacec17ea2170abb1ee5d48f207f3397efb9b251916e2b",
            "ed2458ac6125d62c59982ac03732ff537716447bba73d7a12998dbdf03581e88",
            "8e7ad2a0e2ab40fd7da9816f91e34d15596697cb053b76751fb888a69823c884",
    };

    for (auto& str : string_inputs)
    {
        size_t sz;
        std::unique_ptr<uint8_t[]> tmp(xbn::as_bytes(str, &sz));
        std::string result(xbn::as_string(tmp.get(), sz));
        TEST_ASSERT_MSG(str + " == " + result, str.compare(result) == 0);

        xbn::merkle::Node n;
        sz = sizeof(n);
        xbn::as_bytes(str, n.bytes, &sz);
        result = xbn::as_string(n.bytes, sz);
        TEST_ASSERT_MSG(str + " == " + result, str.compare(result) == 0);
    }
}

void testMerkleRootBlock172010()
{
    //transactions
    std::list<std::string> multiple_inputs{
            "727218593f8166e7d0a00e3b49cb7787aaf3cdd158e211d4cd8bd9c10f74710b",
            "c20253c83e3ea464eb82c645ae515c2cef88ab25fa1803ce145040d5fc7f8c0e",
            "283e2e2e84049009ab0a95423d1ebccba3668b5be88d6969cd5623e50265ff71",
    };

    // convert to inputs.
    std::vector<xbn::merkle::Node> inputs;
    for (auto& str : multiple_inputs)
    {
        inputs.emplace_back();
        size_t sz = sizeof(xbn::merkle::Node);
        xbn::as_bytes(str, inputs.back().bytes, &sz);
    }

    // construct the tree.
    xbn::merkle::Tree tree(inputs);

    // convert to strings for asserting.
    std::vector<std::string> tree_data;
    for (auto& node : *(tree.data()))
    {
        tree_data.push_back(xbn::as_string(node.bytes, sizeof(xbn::merkle::Node)));
    }

    // asset
    std::vector<std::string> tree_expected{
            "727218593f8166e7d0a00e3b49cb7787aaf3cdd158e211d4cd8bd9c10f74710b",
            "c20253c83e3ea464eb82c645ae515c2cef88ab25fa1803ce145040d5fc7f8c0e",
            "283e2e2e84049009ab0a95423d1ebccba3668b5be88d6969cd5623e50265ff71",
            "283e2e2e84049009ab0a95423d1ebccba3668b5be88d6969cd5623e50265ff71",
            "e92396c3e15db75758dacec17ea2170abb1ee5d48f207f3397efb9b251916e2b",
            "ed2458ac6125d62c59982ac03732ff537716447bba73d7a12998dbdf03581e88",
            "8e7ad2a0e2ab40fd7da9816f91e34d15596697cb053b76751fb888a69823c884",
    };
    TEST_ASSERT(tree_expected.size() == tree_data.size());
    for(int h = 0; h < tree_data.size(); ++h)
    {
        TEST_ASSERT_MSG(tree_expected.at(h) + " == " + tree_data.at(h),
                tree_expected.at(h).compare(tree_data.at(h)) == 0);
    }
}

void testNodeCount()
{
    size_t result = xbn::merkle::node_count(1);
    TEST_ASSERT(result == 1);
    result = xbn::merkle::node_count(2);
    TEST_ASSERT(result == 3);
    result = xbn::merkle::node_count(3);
    TEST_ASSERT(result == 7);
    result = xbn::merkle::node_count(4);
    TEST_ASSERT(result == 7);
    result = xbn::merkle::node_count(5);
    TEST_ASSERT(result == 13);
    result = xbn::merkle::node_count(19);
    TEST_ASSERT(result == 43);
    result = xbn::merkle::node_count(65);
    TEST_ASSERT(result == 141);
    result = xbn::merkle::node_count(129);
    TEST_ASSERT(result == 271);
    result = xbn::merkle::node_count(192);
    TEST_ASSERT(result == 385);
}

int main(int argc, char** argv)
{
    return Test_util::runner("bcn::Digests", tests);
}

