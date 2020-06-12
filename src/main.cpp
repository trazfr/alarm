#include <iostream>

#include "app.hpp"
#include "error.hpp"

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        std::cout << "Usage:\n\t" << argv[0] << " configuration_file.json" << std::endl;
        std::cout << "The configuration_file.json is created with default values if it does not exist" << std::endl;
        return 0;
    }

    try
    {
        App{argv[1]}.run();
        return 0;
    }
    catch (Error &e)
    {
        std::cerr << "ERROR from " << e.function << "() in " << e.file << '@' << e.line << ": " << e.what() << std::endl;
    }
    catch (std::exception &e)
    {
        std::cerr << "std::exception: " << e.what() << std::endl;
    }
    return -1;
}
