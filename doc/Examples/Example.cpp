#include <Zipper/Zipper.hpp>
#include <Zipper/Unzipper.hpp>
#include <fstream>

using namespace zipper;

// g++ -W -Wall --std=c++11 Example.cpp -o example `pkg-config zipper --cflags --libs`
int main()
{
    try
    {
        std::ifstream input1("Test1.txt");
        if (input1.fail())
        {
            throw std::runtime_error("Cannot open input file 1");
        }
        std::ifstream input2("Test2.txt");
        if (input2.fail())
        {
            throw std::runtime_error("Cannot open input file 2");
        }

        //
        std::cout << "Zipper example" << std::endl;
        Zipper zipper("ziptest2.zip", "123456", Zipper::Overwrite);
        std::cout << "Input1" << std::endl;
        zipper.add(input1, "Test1", Zipper::Medium);
        std::cout << "Input2" << std::endl;
        zipper.add(input2, "Test2");
        std::cout << "Zipper Finished" << std::endl;
        zipper.close();

        //
        Unzipper unzipper("ziptest2.zip", "123456");
        if (!unzipper.extract("foo"))
        {
            throw std::runtime_error("Failed extracting");
        }
        unzipper.close();
        std::cout << "Unzipper Finished" << std::endl;
    }
    catch (std::exception &exception)
    {
        std::cerr << exception.what() << std::endl;
    }
}
