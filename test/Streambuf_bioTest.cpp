/* 
 * File:   Streambuf_bioTest.cpp
 * Author: jwatson
 *
 * Created on May 11, 2011, 12:17:37 AM
 */

#include "testhelper.h"
#include "lj/Streambuf_bio.h"

#include <istream>
#include <ostream>
#include <fstream>

#include "test/Streambuf_bioTest_driver.h"

#define MEM_LENGTH (1024*1024)

char* random_stream()
{
    std::ifstream rand("/dev/urandom");
    char* ptr = new char[MEM_LENGTH];
    int len = 0;
    while (len < MEM_LENGTH)
    {
        rand.read(ptr + len, MEM_LENGTH - len);
        len += rand.gcount();
    }
    return ptr;
}

void testRead()
{
    char* rand_array = random_stream();
    BIO* mem = BIO_new_mem_buf(rand_array, MEM_LENGTH);
    lj::Streambuf_bio<char> buf(mem, 1024, 1);
    std::istream stream(&buf);
    for (int h = 0; h < MEM_LENGTH; ++h)
    {
        int source;
        int dest;
        try
        {
            source = rand_array[h];
            dest = lj::Streambuf_bio<char>::traits_type::to_char_type(stream.get());
            TEST_ASSERT(source == dest);
        }
        catch (const Test_failure& ex)
        {
            std::cout << "unmatched set at " << h
                    << " for " << source << " = "
                    << dest << std::endl;
            throw ex;
        }
    }
}

void testWrite()
{
    char* rand_array = random_stream();
    BIO* mem = BIO_new(BIO_s_mem());
    lj::Streambuf_bio<char> buf(mem, 1, 1024);
    std::ostream stream(&buf);
    stream.write(rand_array, MEM_LENGTH);
    stream.flush();

    BUF_MEM* output_array;
    BIO_get_mem_ptr(mem, &output_array);
    for (int h = 0; h < MEM_LENGTH; ++h)
    {
        int source;
        int dest;
        try
        {
            source = rand_array[h];
            dest = output_array->data[h];

            TEST_ASSERT(source == dest);
        }
        catch (const Test_failure& ex)
        {
            std::cout << "unmatched set at " << h
                    << " for " << source << " = "
                    << dest << std::endl;
            throw ex;
        }
    }
}

int main(int argc, char** argv)
{
    return Test_util::runner("lj::Base64", tests);
}

