#include <iostream>
#include <string>
#include "dynamic_array.hpp"

int main()
{
	dynamic_array<std::string> A(250'000'000ull);

	for (int i = 0; i < 250'000'000; ++i)
		A.push_back("word");

	std::cout << "Capacity: " << A.capacity() << "\nSize: " << A.size() << "\nLast element: " << A.last();

	std::cout << "\n\n\n";
	return 0;
}
