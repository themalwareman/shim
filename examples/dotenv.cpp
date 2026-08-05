#include <shim/dotenv.h>

#include <iostream>


int main(int argc, char* argv[]) {
    auto dotenv = shm::dotenv::load("..\\..\\tests\\.env");

    int j;
}