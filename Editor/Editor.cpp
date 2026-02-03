#include <iostream>
#include "Genesis.h"

int main()
{
    std::cout << "Hello World!\n";
    gns::core::Engine engine;
    engine.Initialize();
    engine.Run();
    engine.ShutDown();
}

