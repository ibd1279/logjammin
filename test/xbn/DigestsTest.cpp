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

void testMerkleRootBlock241347()
{
    //transactions
    std::list<std::string> multiple_inputs{
            "aef4a682805c9c89ad42022946461ffe370b489065a1ceb0200a0cb1c8e3aeeb",
            "3da69017312fafb193867764054c634c73bd6d97aaf5a13466556c74162b7ff0",
            "eac84c926667ea99d08842b1e7d48e94e9397e6cef4fe0b66875fa0a83738d2b",
            "e14c8fd13905b7bd74f99f982e48979f1ada025e04d9aca730df978c4b3b27a4",
            "c116158e903e2ef12606c11d8df28e337e31541628415b8aac8d1a7362192a4d",
            "28d16f0db526d26d1b2e9405716e7f71e903cbd651f2d39d32ff240152bdf9ce",
            "3d8594e871a09c2d51bacd4551cb543be8c5a7a1286e52fc8c2bec3a891bbde4",
            "fa3a9924950fc58ed2a8d001c179bc290b8cd22733d1ddbfb3e5fe14796ade2f",
            "5ff3408b471b43e1977c72148da217fd5d3a49ff3c3d275a0cce9ba23d7013af",
            "8c1c55cc23b02a8daae135243f7728361b216f1bd683566ba7cc7a6052261328",
            "71ed2679c48d5d483d92999023a84192d6b372ec901146d52fb7c7d430e581d6",
            "ef2432b9b1ccdf10d0a24b9d10706979c81e23858437b3934bf7cca780450dc2",
            "c01df6197d4ae50b7d82c9207b5b29a446c48a2eb250975f711b4d9e33887f49",
            "0d4bd8d585ef9e7cd3d477025320ed5aad7a7965f440ba0a81ea3a5d9d32f693",
            "8b2a18a9dd52a93ba4839b98265b6e3f20a01e470ddef88d566272293d2bc2e0",
            "0f1c1e065c6ee632dc2e9c9587a7fa2a0d992f8db34ba15065a03af3adc080ce",
            "cd9d155c99ff0c399c5d2a3fd8c97b11155c6976ef15eab176c0a20b6af1ecdd",
            "e75c816324b44a758ddda94784967f9d0f95f9729a77f929bff9e611dab47153",
            "7e92feaef7a6084267c0fa720908ce703204b58c420ec18da701d2bf918f3aee",
            "2149e576bf6f537b9a08e5c488d0e0faec29269dd7abc07dd2dd9b2e531ceb0b",
            "41c35985383693ac1d754850d0f369820267d40bff3b1ea01b36faf1fb1fc1c5",
            "a13ae2a40b74008bc2bef920e4a013d9ce2cf4e503f17151afc6505924b1461c",
            "490546fac2897bc279b93e039a5b36e90ee0120c5ec22eba889a69def786cd77",
            "aaf93de8bc026dd0c923ee6cd8b67137d5f1fd20b00e5504f8659ca4cfabf75c",
            "37ead40de99f56331d47b3601db7b829a41016eef4822f69966ab9aaa9fbf906",
            "eddd6cba39f1a365e8607f9e84f7988033e2511ec6390f2d71f4ce742a51f526",
            "afcc1834fce4dd92e20a1d13a1c21e2f84dfd7a5c6f567f6ebc1b683de296081",
            "c7081f11473ffca6d03a6062cdf2cfbe68ce11d0f3f9ae65e54165d72bb3f38b",
            "72487604ec1fb853737eb963bb65b79309c39c2230141248b5f03f7e2d747d5c",
            "8b7639600863a2f67ee4ea9e1ef29348a8204578542a226c37f35cdb58a9f09b",
            "5dc68f7d57382c3a238ed8c255ea8b93eef6075c7271883de6d85163604a77ed",
            "f3d72d9817dd77fce0a96aaf91ef126f2a90b0310c5b3ea40f6fa4df4e799f3b",
            "b67261a5a8762d8b550e1a0b0546c5a4ac4b1f19a46ae356c26d86538a54ee42",
            "e35a9ea28c03d0d8086d53aa7848e6563cb2edc588830d618aca450a2702c913",
            "ccc438360b1557657754171ef92baa1af395c6d6218131a8c8f686c72b929d52",
            "547754bf460005b9367e273e33eb051a328114a92535e6d8759316d8204aef13",
            "f225e41ae7e8dcbae3db67386fa7b85aabd5f38c451663ae87775d20614e7200",
            "905635796ddd9fa630da853bfefb3d6ab786f34653f3e7d78af6024715bdc30c",
            "61a1e698fc689222349c334dfae12c38af45b19076015aae4b9561ab8d6a02d9",
            "e2059f301fb014cbee8ec0f7208515ef5b508d4cd388c7eb749e24e9e855cecc",
            "c6970047d3f6c6dad9d3274db395ec5d26f766105a039f8480c8296f7ec88413",
            "e381a4c2eb6cac5da5bf77f274992c0c0a6526f2c7b022252ef2084fab95620a",
            "f62b8bb7bc3566c8f9e85e526800a4c04f5acd7bb17830dc850c10ec22baa451",
            "7b0e9fa2baeba855885c7fd558a3151daebb0f880d045e9fcbe4da670302aa41",
            "e86ca065b5278de44c47ee933f61f5027b40b25085116f87ac09c590188d5954",
            "417f0bff4bf03519690f9b67f1806bb6a14233af22fad0b1ba152275a5990578",
            "e3557bace14969cfd300a38ec6a1afa2d7863452447b3ec7d0c64b168a481f84",
            "6a43428741af630e92dc15be17bae1ad1457a343366f5f7a7cdc85424007ac21",
            "bb68b2e8cc5652b0b41c8479205dcac47d9e4ee101161fe2cbb5ea5ef435971f",
            "2d564eb171f366a6486e24270b9824a88ba9e6fd5a0161e0fb536b13a5151c7c",
            "e30962321a690eca72a3dd970a361d09fa4fba5480c04dbc6f8b20f147ed59d4",
            "6a312885343a54f7bc426b7885cd4f2cac7e4aebdf17e481545e1c3fc3d361f6",
            "443aa9b99bfb43d27405663e2a59d94a7edf98b730bc2031dd99493b680c1168",
            "7e03f2506fd10c14990193c9027d72f39ef19d0f0e8ecbb353d820742bd394aa",
            "4550acf4f172f34770fe1da4e6000978c07ddb8f3515a6a6c9022b2e47b4015c",
            "9abdcc16db65f566e27597b68fcf3d905193763a34fb5fce51c296ed0c953a97",
            "b7dc0f2f101220cf686e6176afe958e6a253e65ae18ac1abcf08eace610e1070",
            "9dce4672b842a4ec1b09abd57c1b4549a77e2c06754a149dabeed75390d7a43d",
            "25841569be05aa5dfdeb8bcb40b7cf129bdb85f0b9839aa1a27c37560522748f",
            "1bf6ca5520a06253fbc4775cc2a46a6e86dab54b5f04c6a6fa54138719afe32c",
            "b389cbb989b4722cbe993f55f49d7d695010cd858060936a7438e018ea83755d",
            "d52a13492ae55c4887bd1496a302a9b2e619562d9108f9c8e7f1c51b108e1942",
            "9fc3b2ac40cccb3fa237840a3eaa72b010948c3237995fcd187d067fac5276e0",
            "54e15e85007b9c4ff6ab2572ab58d893ad8fb7ac48a42742ee42ecc6a42b71b1",
            "5d6c227ef797428ac9d737e799cd09cd0455d133ebe3597fcb5d9cd71abd2d78",
            "0a87b8556524ab070353e1ddb0a8f452c5746e4a94b17b14c9f3bc415107850a",
            "32b0e2d9da25797f5c691cd804008c221b149734cab8313b29883f2b4521d23c",
            "b8b04d58093b06088a4003e2274bf2dbf21687782f4ad828efcf85125190d23b",
            "c1029eafacc9e387d81f4f01e97173edfc09f547fca2dacf3bb566144a65b0cd",
            "3a2e7db11343f9824558eb552a334c342b4b03199bd27517440e9b1805099b4e",
            "069512a77986d5d0b63a483cb5b15e21e7a38190aebabc8c0646a7b899b4d5bc",
            "519cb77eb117ddd8c89c0a61747d317df326ba20c84dfe9f2eeb9e1a1fa0432a",
            "d6f1201502503f2a4141ee67185a5ca542f61123bc8fe0d8175320d93a1bb719",
            "2200db1254914466552c77b6d823002382603236207ad5f644933096eaa76ddc",
            "260cc3a76e928824e9bbb949ea4d490a1fc9fa7004561a34cb39af6c93fe2a6b",
            "70392a59d7b40d15464d4030765615f7b774f6b95cbd973a03301aa1c385bf9c",
            "01c88c8d1500a567ca65fdca9b5f1ec8d3ac3f197f32eef4b02e6581702a43a8",
            "c56cde0064aff95eb4f82f6679f7d9081d148573179f267369f17321cd5cb7a4",
            "0d39aec77f203720793035646c859bcfe1b695efa130772f60656adbe6e73e73",
            "f26af18450e14b7ceef4e09c955af47af68c9b5a8a13b65d3fff01b533aec4a9",
            "97cfde9d363a7b5f2b72b6eff332c7811bb819600542fe048bd413cb4444fc24",
            "bd28a0a755e8f62c9585c4da410056bf3a9569c568970f55d2b9edd84200a8a4",
            "2b0ffb8d959876e80e1be0e2e21edc96eaf7f60f0257502304f8c35266ccff8b",
            "4c1c3fdf6e3bd963c85960d13ff530850a97fa5d19407100fbf47f7c7025c285",
            "3755ca93d9aadd281ece28e99874984b3f010233b3b7ec20eb313eb5ea259a0a",
            "b0506b1de0aaec3eac8410dca086e84686f40ae9bede76b008c1636659ad414c",
            "f3309ef8520898be7889055e81b77e768d82462725218bb4582274c2e5a38b52",
            "fd9a443c3adece4bc4c1eea86bf6a21ac2173482a25a517b71771ccb7fb7e332",
            "190f2135b16f4dde29a250bba443d5049adb1f05ce3da2419db2385670f30106",
            "9a9ac4746bfcfd635ae195580b86afb47afd43cd433a4cdd53ce61ef2cc52246",
            "665584be20cfac13dc1f5be47505c7d50e812d7d6875aee0da0daae6183b7a92",
            "c04a67c8ec026cdc5541ea21ce54820954f7379ab914e03ea9d7283ca09951f8",
            "b22942587e67ab6a369362282e4087d2c11f99e467cca0c670e2762210aba347",
            "80bfa4e6c790583f536ce7e5bb5710f242fdb3c928435c84151f00f2a63b43fc",
            "beca1071dfa5baa9faa6d4dea23f292cac000d531ac1fe790bf5010e969ec0b4",
            "091f3103d817238c9d6cf6f2efad5833034c77367bcf9e533ef99aef84b760ae",
            "7287d3ea6dc154737f81be4846ffb227da6645390b2bb57d9d1cebb5fa530cec",
            "83144d651166e9b24897f3126ce080aefa3083a2eb1908953423d3c820ebc87d",
            "afe00e2bf45665a2e591ed50b6ab0b96aeacbe0a42e20b3351ffb5661870786a",
            "61e74facb5646ad1779e4102acece8c19d1527e3cc309678f020deddba6661fd",
            "78609fef7cd0dbc3e14b808e34adc9a50cd491a1d45344157b7f3f6c02e1db7d",
            "a45cb297652a7b86626941e4ac7f7de92f36425eef4e0bc6ff6dc09d121379cc",
            "7aa798bae183634a56e424c23cd92bb3d42cef3cf804c81d740c4641f3e0a1a7",
            "8021f28a12952212aceb4b0bfc869ccb29a1fc89b3daeb71024083532948436b",
            "fa83683c97f0d1a59f755620c3b079976abfc70cfd186e3100b7beea9dbcdfc5",
            "711bdfdabc5676ed45a117131cd23d301c70eabf49e82add532aadca0318cab9",
            "ad33318a6bc0c599cfecf973dbf166a6db88b0ce00549b331a585d870279c637",
            "16266cf0f2f82fda588991f8aaff2076231087556156b84ccc9bf20ca7efc4f3",
            "33c5a7986072d40c81fd809213f8c62ce4dbcd63366695ebf698a2347b220865",
            "0194c033d8b1f9131b54df715bbf8fbb77304e93086821f3bcfc7997cc9695ba",
            "bbf13dcba94635c98bdea8f57ca0a87f873a0ee8564bb7e37459d366ca00907e",
            "9236010101ca0dac70e22ae169ed595c06c8436f0f036b6f5965665f418ab2e1",
            "75cc5250dfdbaff703720bc88cd94cc91ed891f8fc51954ed8bda7a3bebd4e54",
            "aa6a5a972a36eacb8d6fdd3661083a774bd761fed81f0216b453c25b6988f273",
            "03d34da34441f88021c98a6815c901f7785367a312ac491a89002fed233e96b3",
            "4ee5593eddb25f319680cb3c352c8bf0d14cce1aeef42d1d7484b6bc3a1869cf",
            "c4397dff5249df336366b3306f424cbcc962b14a37b4b40334ad40e5979217d9",
            "1244ead37f74c5318c8c876ad8c1aea6cd976b1eb188a065f3092651bc0c519c",
            "fd7378c15be1c9a79b5e47c705f3f2d754d547975ffae2ff9c165bd132c08ff6",
            "a9f4ed045807805fc223f092f9ba23fac41fa5abe917edb30c2dd155c5757eb0",
            "0bc2a15b7dca66ccef872c2ebf7f53fc1d8fcb9258d0ea650f09908edc89579f",
            "e1063dfb0c730bc98eca4ee8d372e718cc343944df058366d1dfeaad4d753ae6",
            "2643c1fbb82a56256231aebcf830867e99a6bca626419af1537359effd2edaf8",
            "04a83563090c0895b40ca032b40aa95dce8ad9363d22ee54605665ccbd142d91",
            "e604e5079ff7324abb5b2f0a43f3e36beba5fa0f20bb7b67aa65c2d5e44fc46a",
            "a6e4b8342a9cc9a335334bca677c4173c6b34490bc2a79e17c1e898cb8758bd0",
            "e2ee9a848e2b829b855c76920edcfa9d5fa17206e6b419559a58e06727a9072c",
            "f8f2121684dac9e7a4e1b646c54ccdb0d8ab862fdf6d6777ee69c65122d9e746",
            "5ce992d19fb25d2b9eb71a8e573cdc7e3183a288a35a01afa7527e521c1d2185",
            "d1b0bfbea6e6a2029fcad02a21639d58ff75e369e1fcc6076027278bc1e0d1f5",
            "41cae3694ae2a809d04a95e8242e61784e422eb918d32eef16ba890a00e759e1",
            "826eadb079561f9527779b825bd593b6bd01f0ff78dbd88000f3e194968669fb",
            "b1a935a9e95353377d96686cfb4bff8c7d9f827e89919c1d42c6e3009b82800f",
            "0751a97b37b33dfe1b5ba78f95265f55708271c12c7021f2121d56084540dd55",
            "202602b0012e805ccd20bd6e6645b6b778f7a8bda48720af1b6b1584c5f22584",
            "f62e7fd959c5ad95cb528a3f5ffae28fe7d42c52e6c4ca8fa4ac69dbaae3ed13",
            "4aea33b8464fcd70e2c3eab334c7209ba4171e3b800df54686485af0446cbde9",
            "f4f2f01e2cf43a96405772c5aa41bac535f8bed1f75a476f129251a6ed43ee27",
            "3652ef6847f7b64167ce311090e06dd46679215567339e3e1df26ac74e2cfef6",
            "fc4f3211c21bd05f95d993696a9ec2049e83593face2c37d18a768416b5cf088",
            "11a7d253d90d8590d0a58d959691c53b60414eba4b85f3e2d1e8bc4aa3bdd1f8",
            "9e4e3765888b0019e155c04bd412486a9d57f89ab7295d99f15ef5c85f6d3b1f",
            "0b7b4eed6791b12df27129cfd337d38a9bba3a897bd7cfa2e753e69d643b3922",
            "95ce5d20f659c9536946812f21df5316a036f79058fe01c478bd9c8b296f198e",
            "88c0918f1309744e0a639a29ec558cd18e086fa63561ffe2f51de4ef59502715",
            "2175e661a7580f5c7fb20ee5a064f08672ca3762faa9aa4d75981076dc84ea2c",
            "6322477b7f70421cf18c5f99c76428f3641a6f86a8f6bd5d20c04cd04e8718dd",
            "f38c51a18d9336656f1a0221f916938ec2ab2529ed200dc1bd0825d51674db19",
            "385d2c1d39d1dcbbe4bbd98960a5b98fcd59cf477a5d6b4a3fffc10d3be33ddc",
            "42dc949c44079e264f7a495cfec862b32caed6c08ca71248b0fa9711e07354f9",
            "2a60f62dc7be8e563e8f1804cbbfac91c6e9f3b91352cfe30e568035bd6dd192",
            "e5b01e9951829836110e93aca04a723e3fc71a467a09cbf8f016d0c81007139e",
            "0f2e8eddbc8e4c8ce15d85febd0d0253bb241b97eae68805b3fd943380d0bfe6",
            "1e82ec2ae885292fba831d33eb58cbf68623363902fd748dc5a4cb53137c33e2",
            "8439606be4288814a60ebbd00c7052d53da9e401c381fce4b2adf1669b76c078",
            "80f4c41b2640297ffc520bf4163165bcf2fae74700c11412c4684cc0aee33bfb",
            "03db15196b5afaef8fe971977c7e99364cb1cd8be71d132bcb28437ac3f9bd05",
            "11d1b5b2af80ecd3519630f26a27a0b5a565257453b826665dcb87dd976ce96b",
            "2a6cbd23c4e3e299510fc03361987d3b69f43903250fa1dbf46c3b0370943eff",
            "0c2c674003f19306007188b595060768733ba18c7800021e92be382ee75c07e8",
            "1a1b002453cde553af11a346b105afb26020c0c1dfe52b880812a4d130c04f2a",
            "5f1bf5b82f318219c9dba6d305cd21b6f0cea95da3da662d4c1619aafc8a8c81",
            "3625f59e0814c2bb4520855967479ebbc5954f2d3f3f4f1e5d36a82a315008a2",
            "7af5028907905abb39f695b5bdf7f2bf77fc10fe8c444579a7a88f364aaac57d",
            "6d53943bd991e35eba7511c0f221962dfb15c95cb0bd43f660015ba89aae03e0",
            "3ccbfd1eaa9883b0f25dd09da6b66d1fab0260fcd75ee5a0994a584a40482641",
            "e7c5aab0ef7ab0d2640c4a49201fe7706991a970165c3564c65946ec95a9a865",
            "7b9956a89f18c87e78e1c1d193c2a760c92f82e87d29a026fa8e8b2ce6919d12",
            "29b68658656a11c744774f1d1f4ef69025465f48f019b22e28cac36f1acb2d87",
            "8bdbc0e48e7ada1b9f18a68792648d480d48faa0185c89be7f17744840f221da",
            "1dcfc7a8f7f25f7688bb75b789e1dc35f93de034c4dc07b3ba731b5f11808d09",
            "0500846c0dd7459bb9fd7bd9e542c6f3e5aef140901f6cac3b66c26a135b0c97",
            "4315cf99814e7f941ce881486d1e931254c62939a1c4a829acb674f52147316a",
            "569654cfe2daea8deb0d6de55ba1dbccfb6de2789f725dbea05ddb8ca225ec30",
            "5c3f99ddaaeda7d7388e122d66228daaef02477f085f1be1a889c48eb37c1530",
            "6963b05e2a54fe1e7654125afdd636df1ba542135bd4f3484e43f1578c60b613",
            "6c84c5a290c3a325fa9550c20526430c145f124d4e12c268a9cd6762f4e5d6d7",
            "4e8f90992e67fa24540f749fbee7bb6624e79ec236a08f4950d06847412147b6",
            "2278543f9d7fe962e6f83e609966ce784a001e73fd5ee95608103d45b916bbb5",
            "c9436ff22ab2048a4c683d4b0f1e37a0cec7cd0c83cc105d0d5ef30580c6f20d",
            "da3354299b2e60538bb5ae7769430bca8a7bd1875457f2e75628c48e342b7ae4",
            "8e2d86e8544ba23a5f339bec1f7eadafa9e15bd74c63508f159facef8672ef0a",
            "aefe0bc72cf8985d04788c5c13ffd67f705b419883fc521fae0d69284673cef9",
            "77f49ff8938fb0d742cd014b2d32556ba2ccebfce7e4df0cc7a69582a2ef7202",
            "0d8c631dfb3c0d70d51d69da902287f1fc3a7451d9e1ca4af26284c2f91cb0c1",
            "f8ddc49efe4a5452bbf1d13dd31ac02e8ff1a551f4f19ada0eb9d8a484510157",
            "3d612de151d10ff03c26badb1914dc5b99b5400265ee9994a887ab5110211155",
            "42a8bfb94b835ce648742eac5bcf1f9f7379be14cfd53950035eda64f9005612",
            "0d746c420ef51ea0df95b3b288066352958e22e5da8515bd866d3eb3d2b3bcc2",
            "e9e479f35e05ffad1159106e868794e27cbd4c7e997ae745c5ad1acaa9c57996",
            "9ff132c948763b8cf6383e09f73b6834f1e5e8c203150a992bc63eaff361a370",
            "ead162496b0d930921e189e307118f552ee46f8bf7ae8071dbf41c9219f19874",
            "943ea87759f548b538a666f5d17ffd4cfc0fa9d821647a29110c20ac5bf54fc2",
            "64a92ec5b8667c677903ace63b8019857ee07b11cd37cd2e55cacaf402a38cd3",
            "c7eba34dafbe346984b2c3f3e80df54f9a83aff485f122844fa049c300193a6b",
            "a834e83f997f2a8e958128027b12b4b8ee3c6e54c99672251cb0be953dd8651e",
            "e56cd48245278dbddd78ea0dadfcd3ff6e441750f53916218c7fd414ef165fce",
            "58817c6b2bf43fd06b88a3ff5468286610a624ad95cdf2a388f395b502af24e8",
            "78eee18ac3fac294fa8038f1b4e340e25a841ba594552dd05fae45b746a62ac1",
            "5bd51fc5d27e8d2171f7467a91ff5b568a664ea3210b89d67265c8cd8d61d911",
            "b8a412d77b13d4e18da4b47b9cdf5b82dfe23c6d4e64e8d7d894ca76206b0929",
            "cdd3e7f60d11b793acefbe37cdf8f3390cf34e0c45b997a9cd4a879ae7e2cff0",
            "fc08ebc9ba4dad52d63facddc4091c2ad4b9a2928aab26c42e0e01672490e5f1",
            "1b788ad67cd6b6ed40de4b16e97363c3820097c6080831536f072d731cc36d81",
            "cca02d8f645de774bddc45a252c9d6ac143a87df6c99812030f97ceeb593e983",
            "a376c25923d4feb3fac2e71356cda44430aaced5e104a153c12ef3da8341a1e2",
            "65aea2790335d244a2da264671d0a12e7888aa654e6c67b3b4568dd90f2ad46d",
            "657aead7913cd12cb639f5b4533fc9eeb51285ce6146438084ebb9c4eb030a81",
            "71958c7640211c25d02506fc3e58a73b18c7ec393ab08fbe29cbb04799b42911",
            "e3c32140cef949b8d17eb50caae592c26a3a7ae8c7e73dceaecce1b9f9dbdd34",
            "d2aa6d57f33431605c2d002d53610fce1d323d237af0105083020fe069c9a980",
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
    for (auto& level : xbn::merkle::as_list(tree))
    {
        for (auto& node : level)
        {
            tree_data.push_back(xbn::as_string(node.bytes, sizeof(xbn::merkle::Node)));
        }
    }

    // assert
    std::vector<std::string> tree_expected{
            "aef4a682805c9c89ad42022946461ffe370b489065a1ceb0200a0cb1c8e3aeeb",
            "3da69017312fafb193867764054c634c73bd6d97aaf5a13466556c74162b7ff0",
            "eac84c926667ea99d08842b1e7d48e94e9397e6cef4fe0b66875fa0a83738d2b",
            "e14c8fd13905b7bd74f99f982e48979f1ada025e04d9aca730df978c4b3b27a4",
            "c116158e903e2ef12606c11d8df28e337e31541628415b8aac8d1a7362192a4d",
            "28d16f0db526d26d1b2e9405716e7f71e903cbd651f2d39d32ff240152bdf9ce",
            "3d8594e871a09c2d51bacd4551cb543be8c5a7a1286e52fc8c2bec3a891bbde4",
            "fa3a9924950fc58ed2a8d001c179bc290b8cd22733d1ddbfb3e5fe14796ade2f",
            "5ff3408b471b43e1977c72148da217fd5d3a49ff3c3d275a0cce9ba23d7013af",
            "8c1c55cc23b02a8daae135243f7728361b216f1bd683566ba7cc7a6052261328",
            "71ed2679c48d5d483d92999023a84192d6b372ec901146d52fb7c7d430e581d6",
            "ef2432b9b1ccdf10d0a24b9d10706979c81e23858437b3934bf7cca780450dc2",
            "c01df6197d4ae50b7d82c9207b5b29a446c48a2eb250975f711b4d9e33887f49",
            "0d4bd8d585ef9e7cd3d477025320ed5aad7a7965f440ba0a81ea3a5d9d32f693",
            "8b2a18a9dd52a93ba4839b98265b6e3f20a01e470ddef88d566272293d2bc2e0",
            "0f1c1e065c6ee632dc2e9c9587a7fa2a0d992f8db34ba15065a03af3adc080ce",
            "cd9d155c99ff0c399c5d2a3fd8c97b11155c6976ef15eab176c0a20b6af1ecdd",
            "e75c816324b44a758ddda94784967f9d0f95f9729a77f929bff9e611dab47153",
            "7e92feaef7a6084267c0fa720908ce703204b58c420ec18da701d2bf918f3aee",
            "2149e576bf6f537b9a08e5c488d0e0faec29269dd7abc07dd2dd9b2e531ceb0b",
            "41c35985383693ac1d754850d0f369820267d40bff3b1ea01b36faf1fb1fc1c5",
            "a13ae2a40b74008bc2bef920e4a013d9ce2cf4e503f17151afc6505924b1461c",
            "490546fac2897bc279b93e039a5b36e90ee0120c5ec22eba889a69def786cd77",
            "aaf93de8bc026dd0c923ee6cd8b67137d5f1fd20b00e5504f8659ca4cfabf75c",
            "37ead40de99f56331d47b3601db7b829a41016eef4822f69966ab9aaa9fbf906",
            "eddd6cba39f1a365e8607f9e84f7988033e2511ec6390f2d71f4ce742a51f526",
            "afcc1834fce4dd92e20a1d13a1c21e2f84dfd7a5c6f567f6ebc1b683de296081",
            "c7081f11473ffca6d03a6062cdf2cfbe68ce11d0f3f9ae65e54165d72bb3f38b",
            "72487604ec1fb853737eb963bb65b79309c39c2230141248b5f03f7e2d747d5c",
            "8b7639600863a2f67ee4ea9e1ef29348a8204578542a226c37f35cdb58a9f09b",
            "5dc68f7d57382c3a238ed8c255ea8b93eef6075c7271883de6d85163604a77ed",
            "f3d72d9817dd77fce0a96aaf91ef126f2a90b0310c5b3ea40f6fa4df4e799f3b",
            "b67261a5a8762d8b550e1a0b0546c5a4ac4b1f19a46ae356c26d86538a54ee42",
            "e35a9ea28c03d0d8086d53aa7848e6563cb2edc588830d618aca450a2702c913",
            "ccc438360b1557657754171ef92baa1af395c6d6218131a8c8f686c72b929d52",
            "547754bf460005b9367e273e33eb051a328114a92535e6d8759316d8204aef13",
            "f225e41ae7e8dcbae3db67386fa7b85aabd5f38c451663ae87775d20614e7200",
            "905635796ddd9fa630da853bfefb3d6ab786f34653f3e7d78af6024715bdc30c",
            "61a1e698fc689222349c334dfae12c38af45b19076015aae4b9561ab8d6a02d9",
            "e2059f301fb014cbee8ec0f7208515ef5b508d4cd388c7eb749e24e9e855cecc",
            "c6970047d3f6c6dad9d3274db395ec5d26f766105a039f8480c8296f7ec88413",
            "e381a4c2eb6cac5da5bf77f274992c0c0a6526f2c7b022252ef2084fab95620a",
            "f62b8bb7bc3566c8f9e85e526800a4c04f5acd7bb17830dc850c10ec22baa451",
            "7b0e9fa2baeba855885c7fd558a3151daebb0f880d045e9fcbe4da670302aa41",
            "e86ca065b5278de44c47ee933f61f5027b40b25085116f87ac09c590188d5954",
            "417f0bff4bf03519690f9b67f1806bb6a14233af22fad0b1ba152275a5990578",
            "e3557bace14969cfd300a38ec6a1afa2d7863452447b3ec7d0c64b168a481f84",
            "6a43428741af630e92dc15be17bae1ad1457a343366f5f7a7cdc85424007ac21",
            "bb68b2e8cc5652b0b41c8479205dcac47d9e4ee101161fe2cbb5ea5ef435971f",
            "2d564eb171f366a6486e24270b9824a88ba9e6fd5a0161e0fb536b13a5151c7c",
            "e30962321a690eca72a3dd970a361d09fa4fba5480c04dbc6f8b20f147ed59d4",
            "6a312885343a54f7bc426b7885cd4f2cac7e4aebdf17e481545e1c3fc3d361f6",
            "443aa9b99bfb43d27405663e2a59d94a7edf98b730bc2031dd99493b680c1168",
            "7e03f2506fd10c14990193c9027d72f39ef19d0f0e8ecbb353d820742bd394aa",
            "4550acf4f172f34770fe1da4e6000978c07ddb8f3515a6a6c9022b2e47b4015c",
            "9abdcc16db65f566e27597b68fcf3d905193763a34fb5fce51c296ed0c953a97",
            "b7dc0f2f101220cf686e6176afe958e6a253e65ae18ac1abcf08eace610e1070",
            "9dce4672b842a4ec1b09abd57c1b4549a77e2c06754a149dabeed75390d7a43d",
            "25841569be05aa5dfdeb8bcb40b7cf129bdb85f0b9839aa1a27c37560522748f",
            "1bf6ca5520a06253fbc4775cc2a46a6e86dab54b5f04c6a6fa54138719afe32c",
            "b389cbb989b4722cbe993f55f49d7d695010cd858060936a7438e018ea83755d",
            "d52a13492ae55c4887bd1496a302a9b2e619562d9108f9c8e7f1c51b108e1942",
            "9fc3b2ac40cccb3fa237840a3eaa72b010948c3237995fcd187d067fac5276e0",
            "54e15e85007b9c4ff6ab2572ab58d893ad8fb7ac48a42742ee42ecc6a42b71b1",
            "5d6c227ef797428ac9d737e799cd09cd0455d133ebe3597fcb5d9cd71abd2d78",
            "0a87b8556524ab070353e1ddb0a8f452c5746e4a94b17b14c9f3bc415107850a",
            "32b0e2d9da25797f5c691cd804008c221b149734cab8313b29883f2b4521d23c",
            "b8b04d58093b06088a4003e2274bf2dbf21687782f4ad828efcf85125190d23b",
            "c1029eafacc9e387d81f4f01e97173edfc09f547fca2dacf3bb566144a65b0cd",
            "3a2e7db11343f9824558eb552a334c342b4b03199bd27517440e9b1805099b4e",
            "069512a77986d5d0b63a483cb5b15e21e7a38190aebabc8c0646a7b899b4d5bc",
            "519cb77eb117ddd8c89c0a61747d317df326ba20c84dfe9f2eeb9e1a1fa0432a",
            "d6f1201502503f2a4141ee67185a5ca542f61123bc8fe0d8175320d93a1bb719",
            "2200db1254914466552c77b6d823002382603236207ad5f644933096eaa76ddc",
            "260cc3a76e928824e9bbb949ea4d490a1fc9fa7004561a34cb39af6c93fe2a6b",
            "70392a59d7b40d15464d4030765615f7b774f6b95cbd973a03301aa1c385bf9c",
            "01c88c8d1500a567ca65fdca9b5f1ec8d3ac3f197f32eef4b02e6581702a43a8",
            "c56cde0064aff95eb4f82f6679f7d9081d148573179f267369f17321cd5cb7a4",
            "0d39aec77f203720793035646c859bcfe1b695efa130772f60656adbe6e73e73",
            "f26af18450e14b7ceef4e09c955af47af68c9b5a8a13b65d3fff01b533aec4a9",
            "97cfde9d363a7b5f2b72b6eff332c7811bb819600542fe048bd413cb4444fc24",
            "bd28a0a755e8f62c9585c4da410056bf3a9569c568970f55d2b9edd84200a8a4",
            "2b0ffb8d959876e80e1be0e2e21edc96eaf7f60f0257502304f8c35266ccff8b",
            "4c1c3fdf6e3bd963c85960d13ff530850a97fa5d19407100fbf47f7c7025c285",
            "3755ca93d9aadd281ece28e99874984b3f010233b3b7ec20eb313eb5ea259a0a",
            "b0506b1de0aaec3eac8410dca086e84686f40ae9bede76b008c1636659ad414c",
            "f3309ef8520898be7889055e81b77e768d82462725218bb4582274c2e5a38b52",
            "fd9a443c3adece4bc4c1eea86bf6a21ac2173482a25a517b71771ccb7fb7e332",
            "190f2135b16f4dde29a250bba443d5049adb1f05ce3da2419db2385670f30106",
            "9a9ac4746bfcfd635ae195580b86afb47afd43cd433a4cdd53ce61ef2cc52246",
            "665584be20cfac13dc1f5be47505c7d50e812d7d6875aee0da0daae6183b7a92",
            "c04a67c8ec026cdc5541ea21ce54820954f7379ab914e03ea9d7283ca09951f8",
            "b22942587e67ab6a369362282e4087d2c11f99e467cca0c670e2762210aba347",
            "80bfa4e6c790583f536ce7e5bb5710f242fdb3c928435c84151f00f2a63b43fc",
            "beca1071dfa5baa9faa6d4dea23f292cac000d531ac1fe790bf5010e969ec0b4",
            "091f3103d817238c9d6cf6f2efad5833034c77367bcf9e533ef99aef84b760ae",
            "7287d3ea6dc154737f81be4846ffb227da6645390b2bb57d9d1cebb5fa530cec",
            "83144d651166e9b24897f3126ce080aefa3083a2eb1908953423d3c820ebc87d",
            "afe00e2bf45665a2e591ed50b6ab0b96aeacbe0a42e20b3351ffb5661870786a",
            "61e74facb5646ad1779e4102acece8c19d1527e3cc309678f020deddba6661fd",
            "78609fef7cd0dbc3e14b808e34adc9a50cd491a1d45344157b7f3f6c02e1db7d",
            "a45cb297652a7b86626941e4ac7f7de92f36425eef4e0bc6ff6dc09d121379cc",
            "7aa798bae183634a56e424c23cd92bb3d42cef3cf804c81d740c4641f3e0a1a7",
            "8021f28a12952212aceb4b0bfc869ccb29a1fc89b3daeb71024083532948436b",
            "fa83683c97f0d1a59f755620c3b079976abfc70cfd186e3100b7beea9dbcdfc5",
            "711bdfdabc5676ed45a117131cd23d301c70eabf49e82add532aadca0318cab9",
            "ad33318a6bc0c599cfecf973dbf166a6db88b0ce00549b331a585d870279c637",
            "16266cf0f2f82fda588991f8aaff2076231087556156b84ccc9bf20ca7efc4f3",
            "33c5a7986072d40c81fd809213f8c62ce4dbcd63366695ebf698a2347b220865",
            "0194c033d8b1f9131b54df715bbf8fbb77304e93086821f3bcfc7997cc9695ba",
            "bbf13dcba94635c98bdea8f57ca0a87f873a0ee8564bb7e37459d366ca00907e",
            "9236010101ca0dac70e22ae169ed595c06c8436f0f036b6f5965665f418ab2e1",
            "75cc5250dfdbaff703720bc88cd94cc91ed891f8fc51954ed8bda7a3bebd4e54",
            "aa6a5a972a36eacb8d6fdd3661083a774bd761fed81f0216b453c25b6988f273",
            "03d34da34441f88021c98a6815c901f7785367a312ac491a89002fed233e96b3",
            "4ee5593eddb25f319680cb3c352c8bf0d14cce1aeef42d1d7484b6bc3a1869cf",
            "c4397dff5249df336366b3306f424cbcc962b14a37b4b40334ad40e5979217d9",
            "1244ead37f74c5318c8c876ad8c1aea6cd976b1eb188a065f3092651bc0c519c",
            "fd7378c15be1c9a79b5e47c705f3f2d754d547975ffae2ff9c165bd132c08ff6",
            "a9f4ed045807805fc223f092f9ba23fac41fa5abe917edb30c2dd155c5757eb0",
            "0bc2a15b7dca66ccef872c2ebf7f53fc1d8fcb9258d0ea650f09908edc89579f",
            "e1063dfb0c730bc98eca4ee8d372e718cc343944df058366d1dfeaad4d753ae6",
            "2643c1fbb82a56256231aebcf830867e99a6bca626419af1537359effd2edaf8",
            "04a83563090c0895b40ca032b40aa95dce8ad9363d22ee54605665ccbd142d91",
            "e604e5079ff7324abb5b2f0a43f3e36beba5fa0f20bb7b67aa65c2d5e44fc46a",
            "a6e4b8342a9cc9a335334bca677c4173c6b34490bc2a79e17c1e898cb8758bd0",
            "e2ee9a848e2b829b855c76920edcfa9d5fa17206e6b419559a58e06727a9072c",
            "f8f2121684dac9e7a4e1b646c54ccdb0d8ab862fdf6d6777ee69c65122d9e746",
            "5ce992d19fb25d2b9eb71a8e573cdc7e3183a288a35a01afa7527e521c1d2185",
            "d1b0bfbea6e6a2029fcad02a21639d58ff75e369e1fcc6076027278bc1e0d1f5",
            "41cae3694ae2a809d04a95e8242e61784e422eb918d32eef16ba890a00e759e1",
            "826eadb079561f9527779b825bd593b6bd01f0ff78dbd88000f3e194968669fb",
            "b1a935a9e95353377d96686cfb4bff8c7d9f827e89919c1d42c6e3009b82800f",
            "0751a97b37b33dfe1b5ba78f95265f55708271c12c7021f2121d56084540dd55",
            "202602b0012e805ccd20bd6e6645b6b778f7a8bda48720af1b6b1584c5f22584",
            "f62e7fd959c5ad95cb528a3f5ffae28fe7d42c52e6c4ca8fa4ac69dbaae3ed13",
            "4aea33b8464fcd70e2c3eab334c7209ba4171e3b800df54686485af0446cbde9",
            "f4f2f01e2cf43a96405772c5aa41bac535f8bed1f75a476f129251a6ed43ee27",
            "3652ef6847f7b64167ce311090e06dd46679215567339e3e1df26ac74e2cfef6",
            "fc4f3211c21bd05f95d993696a9ec2049e83593face2c37d18a768416b5cf088",
            "11a7d253d90d8590d0a58d959691c53b60414eba4b85f3e2d1e8bc4aa3bdd1f8",
            "9e4e3765888b0019e155c04bd412486a9d57f89ab7295d99f15ef5c85f6d3b1f",
            "0b7b4eed6791b12df27129cfd337d38a9bba3a897bd7cfa2e753e69d643b3922",
            "95ce5d20f659c9536946812f21df5316a036f79058fe01c478bd9c8b296f198e",
            "88c0918f1309744e0a639a29ec558cd18e086fa63561ffe2f51de4ef59502715",
            "2175e661a7580f5c7fb20ee5a064f08672ca3762faa9aa4d75981076dc84ea2c",
            "6322477b7f70421cf18c5f99c76428f3641a6f86a8f6bd5d20c04cd04e8718dd",
            "f38c51a18d9336656f1a0221f916938ec2ab2529ed200dc1bd0825d51674db19",
            "385d2c1d39d1dcbbe4bbd98960a5b98fcd59cf477a5d6b4a3fffc10d3be33ddc",
            "42dc949c44079e264f7a495cfec862b32caed6c08ca71248b0fa9711e07354f9",
            "2a60f62dc7be8e563e8f1804cbbfac91c6e9f3b91352cfe30e568035bd6dd192",
            "e5b01e9951829836110e93aca04a723e3fc71a467a09cbf8f016d0c81007139e",
            "0f2e8eddbc8e4c8ce15d85febd0d0253bb241b97eae68805b3fd943380d0bfe6",
            "1e82ec2ae885292fba831d33eb58cbf68623363902fd748dc5a4cb53137c33e2",
            "8439606be4288814a60ebbd00c7052d53da9e401c381fce4b2adf1669b76c078",
            "80f4c41b2640297ffc520bf4163165bcf2fae74700c11412c4684cc0aee33bfb",
            "03db15196b5afaef8fe971977c7e99364cb1cd8be71d132bcb28437ac3f9bd05",
            "11d1b5b2af80ecd3519630f26a27a0b5a565257453b826665dcb87dd976ce96b",
            "2a6cbd23c4e3e299510fc03361987d3b69f43903250fa1dbf46c3b0370943eff",
            "0c2c674003f19306007188b595060768733ba18c7800021e92be382ee75c07e8",
            "1a1b002453cde553af11a346b105afb26020c0c1dfe52b880812a4d130c04f2a",
            "5f1bf5b82f318219c9dba6d305cd21b6f0cea95da3da662d4c1619aafc8a8c81",
            "3625f59e0814c2bb4520855967479ebbc5954f2d3f3f4f1e5d36a82a315008a2",
            "7af5028907905abb39f695b5bdf7f2bf77fc10fe8c444579a7a88f364aaac57d",
            "6d53943bd991e35eba7511c0f221962dfb15c95cb0bd43f660015ba89aae03e0",
            "3ccbfd1eaa9883b0f25dd09da6b66d1fab0260fcd75ee5a0994a584a40482641",
            "e7c5aab0ef7ab0d2640c4a49201fe7706991a970165c3564c65946ec95a9a865",
            "7b9956a89f18c87e78e1c1d193c2a760c92f82e87d29a026fa8e8b2ce6919d12",
            "29b68658656a11c744774f1d1f4ef69025465f48f019b22e28cac36f1acb2d87",
            "8bdbc0e48e7ada1b9f18a68792648d480d48faa0185c89be7f17744840f221da",
            "1dcfc7a8f7f25f7688bb75b789e1dc35f93de034c4dc07b3ba731b5f11808d09",
            "0500846c0dd7459bb9fd7bd9e542c6f3e5aef140901f6cac3b66c26a135b0c97",
            "4315cf99814e7f941ce881486d1e931254c62939a1c4a829acb674f52147316a",
            "569654cfe2daea8deb0d6de55ba1dbccfb6de2789f725dbea05ddb8ca225ec30",
            "5c3f99ddaaeda7d7388e122d66228daaef02477f085f1be1a889c48eb37c1530",
            "6963b05e2a54fe1e7654125afdd636df1ba542135bd4f3484e43f1578c60b613",
            "6c84c5a290c3a325fa9550c20526430c145f124d4e12c268a9cd6762f4e5d6d7",
            "4e8f90992e67fa24540f749fbee7bb6624e79ec236a08f4950d06847412147b6",
            "2278543f9d7fe962e6f83e609966ce784a001e73fd5ee95608103d45b916bbb5",
            "c9436ff22ab2048a4c683d4b0f1e37a0cec7cd0c83cc105d0d5ef30580c6f20d",
            "da3354299b2e60538bb5ae7769430bca8a7bd1875457f2e75628c48e342b7ae4",
            "8e2d86e8544ba23a5f339bec1f7eadafa9e15bd74c63508f159facef8672ef0a",
            "aefe0bc72cf8985d04788c5c13ffd67f705b419883fc521fae0d69284673cef9",
            "77f49ff8938fb0d742cd014b2d32556ba2ccebfce7e4df0cc7a69582a2ef7202",
            "0d8c631dfb3c0d70d51d69da902287f1fc3a7451d9e1ca4af26284c2f91cb0c1",
            "f8ddc49efe4a5452bbf1d13dd31ac02e8ff1a551f4f19ada0eb9d8a484510157",
            "3d612de151d10ff03c26badb1914dc5b99b5400265ee9994a887ab5110211155",
            "42a8bfb94b835ce648742eac5bcf1f9f7379be14cfd53950035eda64f9005612",
            "0d746c420ef51ea0df95b3b288066352958e22e5da8515bd866d3eb3d2b3bcc2",
            "e9e479f35e05ffad1159106e868794e27cbd4c7e997ae745c5ad1acaa9c57996",
            "9ff132c948763b8cf6383e09f73b6834f1e5e8c203150a992bc63eaff361a370",
            "ead162496b0d930921e189e307118f552ee46f8bf7ae8071dbf41c9219f19874",
            "943ea87759f548b538a666f5d17ffd4cfc0fa9d821647a29110c20ac5bf54fc2",
            "64a92ec5b8667c677903ace63b8019857ee07b11cd37cd2e55cacaf402a38cd3",
            "c7eba34dafbe346984b2c3f3e80df54f9a83aff485f122844fa049c300193a6b",
            "a834e83f997f2a8e958128027b12b4b8ee3c6e54c99672251cb0be953dd8651e",
            "e56cd48245278dbddd78ea0dadfcd3ff6e441750f53916218c7fd414ef165fce",
            "58817c6b2bf43fd06b88a3ff5468286610a624ad95cdf2a388f395b502af24e8",
            "78eee18ac3fac294fa8038f1b4e340e25a841ba594552dd05fae45b746a62ac1",
            "5bd51fc5d27e8d2171f7467a91ff5b568a664ea3210b89d67265c8cd8d61d911",
            "b8a412d77b13d4e18da4b47b9cdf5b82dfe23c6d4e64e8d7d894ca76206b0929",
            "cdd3e7f60d11b793acefbe37cdf8f3390cf34e0c45b997a9cd4a879ae7e2cff0",
            "fc08ebc9ba4dad52d63facddc4091c2ad4b9a2928aab26c42e0e01672490e5f1",
            "1b788ad67cd6b6ed40de4b16e97363c3820097c6080831536f072d731cc36d81",
            "cca02d8f645de774bddc45a252c9d6ac143a87df6c99812030f97ceeb593e983",
            "a376c25923d4feb3fac2e71356cda44430aaced5e104a153c12ef3da8341a1e2",
            "65aea2790335d244a2da264671d0a12e7888aa654e6c67b3b4568dd90f2ad46d",
            "657aead7913cd12cb639f5b4533fc9eeb51285ce6146438084ebb9c4eb030a81",
            "71958c7640211c25d02506fc3e58a73b18c7ec393ab08fbe29cbb04799b42911",
            "e3c32140cef949b8d17eb50caae592c26a3a7ae8c7e73dceaecce1b9f9dbdd34",
            "d2aa6d57f33431605c2d002d53610fce1d323d237af0105083020fe069c9a980",
            "7481cadb5d1d592515a652c5286d692279b0cdec36f4949dfaa934ac346d76f9",
            "7b25f3db66a3612dcbb4480fce44cf0094b5e960cf9cb4d6205cca71ff947a69",
            "376f3bcaee4565c84b823cc2373a220d3cd7efb1ba9123fe5ad9fe083c83eab6",
            "c9430b0cd904308f306ddf45575ec1a73f283fd363815d123b3c3b69372b0de9",
            "c40715052e225c167abe905235fe712893253d658e52a920cd78fdd809922dcd",
            "9c5f6d3f25de51bd14e3a5d6b5054b5c46db8eee48ebd424e97677e16d444db5",
            "961471080abc9d7f46dd374153bdc5180f729bb43ec8c0726df2245ac6dc1594",
            "dd993983c6862bec9ec68c0d448bce94f3900cb3b25094e1b0dbef6444b6d19b",
            "030a2f35c6c938465a39cdbc9e64a6325d64cf567f26eb47f320729cb154d2e8",
            "a81fe00511c7f0e918fe3cdb0e2bf2a1bace4b1211826c22dbb9627a8c484412",
            "d2952306c11e1968a0017cabd92e5b4ab97dc5fa4ecbb59c5a2c37759db1bb1e",
            "7ea0afec296a9b95e2905bddee4ef8edd54ebb45cec3367c88dc870a07848ff7",
            "f1e3c2ea301687d6392360ef2a3cc210b3377d0785b5203acf1113531dbeb6a0",
            "1162befd10b3e5e754e61da046f4593ebf89e45a18c7f73067601d8915904c50",
            "5431fa8a9f156023af5e8ba2fa80cb2c0debb2b7e7f2d44b04b313b1b14a55b5",
            "f08741a9336eb0653bc76f2f92db261d234da1a9f194fa86953795f08dac911f",
            "90f0043499f00303390645e4e8577dea0c465ee602d65cfe5a2fce384c5c58c2",
            "1d3c88de842b12081eb7109bb7a866d8f7f608f7aef337ce18481cf26cb24344",
            "491f9e164072dc86fb941bf64cca9059af32b7c00ee4db14ffb0ada4c5602266",
            "43fe5c0a3dc844f059252890fc79d4718bb6db04f8c303853968273c399cd5fd",
            "49a36b4a5472564ecb2e8f654da80fabe2d516b8077af7a8a5b01e6f52af7cff",
            "09f8177801fdb181ceb7befaca2390779bd07fe4be0245e89c2b1120562eba9a",
            "0a18aff9fce6d48db981adf8c98c1b8d4e3eb4ac934df4edbec4206a4034ba32",
            "163e683dbf0a9ab7080f6938ce8c9b7ec9b5331614b6574d039bf7aa942088f5",
            "dfa265f532f0d14f7318cbc59c3f6050676d354e45c2ab4dd3cfd7614bb4c28c",
            "4140deaa386add5c680d72ec809b678ca51cd85abd503871224e994e401ec75d",
            "f57aa5e1a9d9e2658bf743e6fd4a206601157f64495047fd0dc2fcdfcd5d5d73",
            "a5ddd4c7445da26d85beadf6a46549aa0495055071e48c44572042b9536fb13a",
            "121e6c740ad66de73e53be3ea6030dd3384f9931fab6eec858a3b6798f1d9689",
            "3539989547e11d3c51ae8b4c190cdcd452d43460df39b37359d812eb8cdad887",
            "f661d4d507b248994015d810e9e6d497ce1f6ba9b8865b867fb3ecb8beb544cc",
            "cad29db719243406106446a4ed8b083c0bad319285864e3c8932692f7acd8926",
            "630fb44c4d6b6dc0f7eb2ed2b127a1e064d37060feeda1abd3387c213d088790",
            "43ba8812f5c74b506be95ec719e5f5e07bc1407ab30c8c03236e155d2693e983",
            "e3b020b21b9eeff1355cde5958e5453663ffd68f48ead5fdb9a4ecfbae5b6a69",
            "c91574bd4a09ae5aa7bd5b085931351adafbf8dcbd30c2813f599064b2a51ea3",
            "7bbac9971f9c165cf6e5e64ab2c1fc01aa0efd51ecd91f53e7ef537a169f925b",
            "5753d58ae25020cbe5ff5ef8724b58bcb68247d3fbb2839837ce2710b204f06f",
            "8f5e472111d1a51e810e0bfcb9835b6ff32442d20bc76cacc0af284795b2ce94",
            "2dcde1999084fd42787bb77ca6135e4f900d1c95502dc8d8ce9c904c022ed2cf",
            "57aa984e328a08bee04121ae9d567ba9a794ceb04e8f5fe490b3a469f994064a",
            "3e0a6efbf3f4898f92f14215874463f243ebaa333e3002ac65d431dd6c7e681c",
            "b60db58b655f37313808ab9c79c8b3dfd384d4128bf0cdf6572c9090bb2aa45f",
            "665e1e17a46e12ca5976c84dc4dc5ef95b1f8ed928f4467a0bf38f131d0aa013",
            "39e6ee7daaae95d05a1b7ec8f986f644f762d233de5297845be6052ae4784ea6",
            "0124d700e8a4867bd2a45c24751303c7d77c63e1b9a8b758c3352d7e912a0ce0",
            "e6026f015473484ab5291c43e558c6569b3378c6915b4435fff11241481bf942",
            "22f7150ce2bc8141551abb63c06664dc18bb542a4e039303b94aec1e52637687",
            "b944fa78ae6b00e800480a3eaf28a1690fcae295984b0cfbfa0e5dfb13763d7a",
            "9fd2e0073a6367832ab3b384d9db14635136d1ccd67c1b677edb37618ad0d318",
            "8eb7d3efffb42dc129ebc58d0a4673aadbe2400448adbd79bc2c8a144db223d4",
            "696ac3527741952713905cdcb51ee1b74450f547aeb38f87f84141e14fefbce4",
            "4ad9460a045847495cb23596e6b07f9f25f98715b217d61ffc6335bee9974846",
            "3d276e71e9a1d781db84e302691ba0fa44054f31bd0a4c09c27cf8f856668d35",
            "39cdade5795e3c32407a0cfeb0beb8cf79117f53661828636c102505d454f192",
            "26d2934b951038a3f32efba70163e217f68548ca3022a005ee70c3cdfd9079dc",
            "82c7e77d89fde7fe3991a46948ecd28670753d8fdfdb5a8f3d69fd06f5157703",
            "6e0b61db335f682a94832d6ce4f1dee2c0563ce7474892ae623be212e7611236",
            "e3c31e8b348ef6c831649a418d5b4c7226e8ca3af2cb3f61aa773e81e1a70165",
            "a8dd476a015ed8fb60bbcb1e9079693747a322ce0a7f33003e5b68b757b3a85c",
            "1766b4150e3b5f4a7dbd625d44d3045596bca50ee1e70ecfd7c004bc42dfaad4",
            "18872900109122a5da2f236f704bce80cb3772ad59d1bb21a85674db5ad4f128",
            "1989e783fa155d79cd2af30981934f6bd0711616f6caa67dcaae75145d821e39",
            "e1de9937863e860d56e7095fc09048c5b84753f7a17140caaa64550295cae23d",
            "3060ab69b1aef2917d356e596d542cd65bbafb56a80da485ce95fdde8874f7eb",
            "9c514a2965b9a7c131685ae3dfb61d059bdb54cdd1ec129b395ecdf45689c0ef",
            "798f8f550f34d040ed624b25389e47b758eb6ded1df017bd22191e29ecfe7d8f",
            "a06f9b0dde145737b488f980feb707f9722324bf59be66ccf66d2ba33a3f11c2",
            "bbec5036f9ac2a2b30d4da2ee54e47a913d98c51e710e13b7bbfde88c10d0c8e",
            "8dcb0b586c062a000888fc2d56bde3a190b00dc428974e28deff4034136485a1",
            "bae1203d22b5a618618157555d8c7b04ec274fa00acca070099d385f7793390b",
            "48857d1ea31ecad0a84055a48c1b5a121856cbaf8347e71f7d5a7e5ae61b01b7",
            "4bb87fc89576c8af460a88ea556186c9c19a65e7c230306f7de01bf5223ad3aa",
            "838f3db1da1786f6c038015ea311de024ba16333b583e2dfa941fcab1b7a380e",
            "c86bb31b1ff448395b7c6663b5a25d6c82b6aae4e5d78defac5cbea10377b766",
            "96849d0688620942011923d5d25130eb1c5675e4c6e1082e51b1922329038a85",
            "788d16583631698f11da7ede386a31db66ad8f18dc995c3efd25f1d708ff366d",
            "54b2f8895884563eb3680f4651a6e4887743d7b6c239c5562557aa44754e1e3d",
            "687f20fbe91689bf18b4764e0824a3cfa7923b054a7ab4822d511f05122e4002",
            "86e946eea0403643022daded8f5a1168e9518cf625018d48363ff761efc276ee",
            "2c0c239fbed7c124d0127eea38b189458a5ebbe6db2a944d06582afe5a57b8c0",
            "dfc8431109e331ce5242cfba8030dbd6394632d6d0d1269c50ae5067f0aaa6cb",
            "fc0a84f521adf1a576c2332139ba8258b1ecfc0c0292a5be09f75ca5d7949605",
            "9c68faa4aff4abe964a1d38b4eb36781c7f9a4017edd9d0e046690c34a7c80b1",
            "f41693509ba60139144092543d51f762b18bf744c8e506dcc6efc8ee04794061",
            "a52dcf45fa5dea13b6874f91c770462cfadc13ed9c948f7d27e5670285b5d9bd",
            "812449ee14ba76f5394bb8d09d0b57e066e7b3f2826a8e1139b7aa751dfaad5b",
            "e12a392c38fdfaa37e70f1825a0fd00040490856ff7736dc8a9fc707f78c1e84",
            "f0f2cc622ca2c724c1ae69290986b1c89c052decbf3384ca565c38e1b4d439af",
            "09df5b376b6aa02598aafdddf0ff0087ccd0eef2ba41b1dbb7396f7aa67a5098",
            "67f98c80ab40f558a0e318ff0d0ed079baf2c70e2e20d6e3a66d1fdce6366ae3",
            "2215b039a9f4cf2b31bdd5a57fceeb2d4fcfc0423372961ad720b372e947d8c1",
            "92e1aa7bbbdbf2a6c60e46e36ec77c9899723442a20f2d5e9b50fe578050d9a8",
            "a998721917a6c31ff679baf32ec957bc8dd94721818eb7652581f539e2dea630",
            "d3a1198dd41bbd333822f61898d3a03bf19d21a8671abacbe4a0868c7aa91771",
            "8324dd3b10febc7cbe1a5dcaf5f86d82ef5162526e9a4a6c7511db30d0472f84",
            "53a4eb734a8c543e3e4321b699b18abd6b453efa424c46857756c89e672f653e",
            "817e3385638504839b0665558f0d4b6ba6dedd2b00a545397d1f19e134798661",
            "c5a667f606e04c95d257f186fd1f0af36b85a79edb3c30d73cf0a245bb05bfec",
            "0fcfc334d17024eba62bc155101f21bcba0fa594104d529b8842a0de505c5864",
            "0dff780c2367b5f00e57bafb954d342d6c1bd6ff094ebc95ab2c31b17872c14b",
            "5d4eeacae1836d542b631bebe7fe7d107866aa348ecd9a3101dc7ec074c39be3",
            "5244f59a562398f037d6f19c8e27634eb7b3513482275ab1690684d7ee61323f",
            "9bb5801960391db287c82f194fa87ff5d075992ad272ae6bc91da90362c1d71d",
            "08f4c335855ed9cc7d555a569a64e58d592b9e27b090c0b90c2757ebe5d6b837",
            "a7031a548eaff8edf0d80cbc09058297ef43d9839925a140fd79b601e7162775",
            "e3224d4f6bd3252302497635e0e909b6076ae0b7f3f16e63cc1f8999549c07ec",
            "b99e3b4be7b1238d4ef113641e392a2b8be7d92ea688dc7d82ead84c7b8ebeb9",
            "31243edb74d68ad94307a97de2550357dd905f934a73ff96c4c50ed75d631908",
            "e0cb549b508661e03761520b814dc5f2942e554fec920f45b24279140dc87b6b",
            "68dce767fc221dcd038ca9a700a823ba5a978af55986eccd123ee834df448b96",
            "6dc302bc29265ee404787a0c9ea019c835fc15f063a9340a33ad2a713a6733ae",
            "4a9f66658b784890a5b4b5957af457d0ba5cabbd48ffe395443a1c4fa7b4a454",
            "d658c8d9a070db4d03bc9ed704439dd34056d1de0edb796f727ddb951246f79a",
            "0b24300a63303e5e1ce514b8ca38cfbc2b0151922196d6aa9d21f796efbd418c",
            "95224430f5de0cfc6aedeb3ece121881c7739bcc5842d3eaf7e5fc7b4cbd62c9",
            "a861da077873a6cc242daca7880c46454e0dcd82af474430b7e3096cf92346ed",
            "3dc30b1e9309fc48447b741f0e476a0b492b2cbb62badde99b9599817db45d8c",
            "1593a53ad4fb779278294ad1a80dd7813e56a7f08a200a57b15dbc41098ab346",
            "d499c8c2a90a120fea23d3d8a14fc26a263dfb55bb0c9f646365bf2979e485dc",
            "afa6d4d4555dd0c2fde598aa0f56b3ea6594ac45391dc0a37e5463f4dc7148b6",
            "377f1ce1a3f76d1e460dc85f57176247375d2e097e8a40f160d2dc3ba2e383b0",
            "a301216ffda4403d95c96217384ae9b098b41f312b2d7b279bfd847e29c90695",
            "8bf4655ee1ae6e148c3417bdfae7368f611dd67edd71d4f0d0f0822935f3f8fb",
            "6e3def93547145421ed89b68f0ae11c24047b368a75a9d988e98ada3a338bd78",
            "2d8d0c3da67b8acaa78280addf735f4a94e8dbfead521b629a1db9114a23caf8",
            "aae069f3d4725a27aa32fe85fb33865a8cace5324f30e480e44874745655b210",
            "e9a63be0e1257409b54e3ad16d50e8d9fcd4917820fafaa127924b6a42aa7a90",
            "cf76632c9c51f8d9d9845de87579501fef6e134be4cc4bc6fd61da97dfb92c0e",
            "4027834898597e2ac5f3a20c49857fac8cebd3d791aab0cdbb394bcd2131f0a4",
            "688754887ad541ea2b09577aa7dfbe52f9ae763804aa1fdea26ce0fed825953e",
            "9cfb7d86fdb35cdc9bf3d2d201f2b0b2b592edbe876974876aadc9cb4afc10fc",
            "f36af4b7a27c1dfac0dbf0eccdcffbb86f34388b188c34939d747e47d78de560",
            "7a12f21da246bb141cb9f2e0b20dc107ea066007a627b783f00ac6628601c26c",
            "ee2ab24fa2311c4cb7731883f984270b31a25bfecb2041fd36322384778b7b30",
            "fda1d8947d8795525dd4310975377a1a59f873bab6f2f28b94eb922e5431fdb1",
            "834b11b466cd74ebedf02c5419bb5ce63cf1b9ae620b758c6b84213ad6ec2d07",
            "a6e684067524089264cefa965019eb809c9fd9d1ac4d632d56185d7426d5c15c",
            "feb9d8fa8418a414caaab8db51077217dafe754e06b27288ef20795817fe7a6c",
            "5bdf5709ccc2429f1e65d5609e5abc1c2549843c4688cc92b7154e99aa273201",
            "3ce76652e290e02490f53c2415a88c7ae3fe1a8955fa9444ab845ed64bb04598",
            "92190e7b275a599a2e78f2359a9a2136e3619939827e89f6a08f6c9547724dad",
            "3855ade1d1b605697fcc61f01e5250af9abc7f707d87497834a1937ce6694fc9",
            "9e70d402619133f0611547a40a28e84143973c6066f88dcb0b4c5f3d089aa724",
            "45b2b772e62991f148e4bd8e8bd395f37aa3ead86e8e05faa1669e112a47d53b",
            "ab6d57ede3c63627dbca1bedc57e6a2474b1636705244f2f6a3b54df605c5d34",
            "3777fb4edb042dafc337a4e0075b2fe4ba34bf1694873b83b7ff27ee0fd77bfc",
            "df8ff9cce128028edb2ec23e5b3260ffee880fac14a33492b14bf48ab5680668",
            "e215870d39060bd7b9fd17e131c6931e0805957376b47a791305fe1b547b576c",
            "c19a47bf6f6a835fb73fa1da33aa9129f7c42bc15f329ac570808fd13ab17ec5",
            "6b54ff78452de68570d4fc18c1aa6277153ef68401e5c1cd0cadd3d3e783b1e8",
            "80def96ac2e9457c7f12fbb2f3aede7263831de1d5c232da2d35acbec89c4e72",
            "992673ec6f7ed40363fc683326167851a83d9aaa1b11658699746dc523690028",
            "d9cc237d67df4f8bac0b8ccad27c75e5296069ceb71ee9f8682a976fd76e1a87",
            "640ad6cac65250dd297a97279d0775c50f4690531d576219420eebe3edf3e081",
            "72744c6cab2af3d25388dca626f0049a02ceb042a86d0209fd8e849b119c2c8a",
            "dea30885434489e8c01628a6be134ba5ef93668b8d7a96cdf7253029d2ad8268",
            "8b63035ea6c1afcdd2fba83909f19a1c58c46aae9f5d096d94d18a86bc08d3a4",
            "d42632110f6827452d4bccffc59e8e87a5a2a61ca2d7e7fbb7f2888e2f765e35",
            "222afd1895aeb79c58fe761d576e12d40be63fedb956e604f3d7e5a74236ebd2",
            "e5a890e703336045c7d094c255b164c36e97f077b78189d3ceae55adcb7216e5",
            "ba55adf734eb32faf758694b1b80088018eaaacfeb985af2258827bdf55cbe82",
            "acb1a683123f4df425befc1502cfed7d03664808c559d0874d6e31b0819c1270",
            "547a5810f2aea18400ed602478a1783583c5e79d01c3799519428aea3b76c0d4",
            "1ba176613668f34063a34ecee8ebb923ae11b0503f47f19bca3a39bb828655d0",
            "d29ab8fb7b6933d62608fe8d2666fc311dc1eb93217685777bd7b464e84fad8f",
            "f79f86ffcff42a20a493e500697c38d7f190c76232ed4c5bfc32023c2f2bdcca",
            "5a5e60c0af3ba0e2bcdad361b1648d7e1210f3a8c906453608623b20c8ba5b32",
            "dac0f0c93b4971c17410875f53f8623baa0b82abaad1055d392b4df000f36480",
            "00c4026bbe62f75bd2f222b8949c01b98d25d71bde0c3213df2ec34dae59403c",
            "be26dd983f13ee45b50a022839be943e533a73c487110f522874cdacadff1590",
            "c5de2f2124fd72914efc188b67120fabb5c367d9e1b3bb8c3074ef3565c65ed3",
            "4ab3a504e68ee8a3e3ca37c986f4feeaccfeac3a2ac73f5f53d8f3804312b805",
            "f92f7dcff69de14068c5e10fa1109cfbb27cffa8e4300b47fbb98b015a3b933a",
            "4850fe504b019574ee110655b47ebd123be644d339fdb225c51a63b657b4be1d",
            "e8878ca00efaabd16ab10554ac1a5a19a42d9622acb30c38a76d322a8bbeb029",
            "9e4fcb8bb728599165111a757218e7071ad0783849d759d693b653c27fee3fb0",
            "aab1d632a08667b23ae74273efbdbf602c4a7d66d18c30e99edd8a3cf78a0806",
            "d25c11256c4d023a7144df3c38b6572819b437cddb97cad2406ce584df078608",
            "5d278cfe16f36502c5ec630bd97762dcfe092c7e71a19eae9f98789af3db3c4a",
            "ca35e68cde21fccb772dd3c7fa3bd329f765f41af63ed1268583d496641a1e78",
            "d902a531da505c238d125d41ae3655a4df18724114db71a33080ac8e3d072f38",
            "8bba6da0827377f88ed37d177f1fd0ccfb1b6e3b0ea41ac3cc5acc15697a937d",
            "7c0cfd6a7fa198d8dbd82d60fc9b53ce07c273f636fe591656b0e0e2daabb823",
            "f1fe5f774c16a58ab5ff7dab5eb04fa8f9e52a46cb4c3d95718c51bf375f25e0",
            "eea7b0c47163dcdb54a0f985406a008875519b011e6b318ec4ccc305799f13ae",
            "a54fd3bc20b2eb989bf2945577672725476690fea0236e1dba3c04854c25124e",
            "9254a598eef54ed3981adcdfec2f213c46f6f8c231ef2598f7a87da91035e465",
            "c513c4fe182da9321051648cb8d731e2be0a4dbc317a572122c362ed57babb21",
            "e39088253269e0b9fa2ccca164334b822eeb5750fa96d842433726108efe50b1",
            "444cb756108a34feb2dc1d8588d0e4155d570737245230f5d65d4c50e9668499",
            "ccee887ce67c111ca539b922c07299474faf70668729a2c3e41691980647d044",
            "97566b4576c52318c37b5adc77d0376443ebc29cb2d5ef2513ec85f1a77bb4ef",
            "40797829bb33cf63c94dcf08b0b6dc05eca376986d07bd70c434bf866fd3c7d4",
            "13cf3637fe61172d9b6fb451fbe78fd65c34f3c333779428254d7c850c66d5bc",
            "9c9601095ddee24bee71a3ef0aed1b9b14a6b3e28f15096835989c74f816a21f",
            "b5a468ef968d6db90f0cc48e6c2125d9bd42c067445b9a1cf799b93fd4b17256",
            "aeb1b43087d11dd78b3dbd400361fc90aee3478165b94469b89b93af33b8b421",
            "3dcba3a451a6b3af21123ad1ad20b5859543f807316433b6482cf7025707b1c3",
            "8dbeabda86649e0bf88bf05f4a4d75d8a3fd48677103cd3717e88a5dc462126d",
            "1ef984f806f9f45d241ea5088a84ec642732bb92ce8b6939a0a7d74350f4f051",
            "7353b4395c71d6e4766dfd1d1e8af0dba094577b9e08153b53286aa57e4aba1a",
            "c754feb10bbd503c972d766334fe07781c9e2f7cf88d2d20ec0cbbd60924d3ca",
            "a60eec56b392d1eec9e8582ec4cfb3347a12f53251f7f7a991091734fa12b947",
            "2eb97785bc52dacb533a66ab4e2bede3ac3453730b574faac6be24f635895a46",
            "fc4ef3f4f2577650a7c990ddd04dd38e1638a9644bb6e3c8231f7472574d0b65",
            "98cb3ffecee4ea526732604cfb5462000184a90e6f017806db27cb4a2f90ef19",
            "3bde86cefd131bd1b3394b20fcb56ce63ca7c49566005dab16bf9afebbe04dd1",
            "224a04f171a436258c075bf044816fbf7a0af80bb028fa6904f0a07a8990a752",
            "ad882a64c4d8a9a39dc88971ab3264b2271bef59f2351ce44729252c82d995a6",
            "fb75ad77215e2321e9bdb2923719138c237cfd495c61d25a4a0b32864375c88e",
            "bd543a58c17f10817c44ca4bd7c55e6428820c54b1f75face4334d7391909b84",
            "fdff7f81bcc76fae6a020e26073dfad68029ead992a4587c8c3551a06bac2d66",
            "c68139b6c8ef4494bc4850e17f15a1469920287598ab389f28dc7d97608f6dd3",
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

