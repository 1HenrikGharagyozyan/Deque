#include <iostream>
#include <deque>

#include "Deque.h"

int main()
{
	Deque<int> deq;

	deq.push_back(3);
	deq.push_back(5);
	deq.push_back(4);
	deq.push_back(6);
	deq.push_back(9);

	for (const auto& elem : deq)
		std::cout << elem << " ";

}