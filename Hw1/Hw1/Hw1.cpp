//CIS600/CSE691  HW1
//Due: 11:59PM, Friday(1/31)

/*
Implement the two member functions: merge_sort and merge, as defined below for a sequential merge sort.
Note that the merge will be called by merge_sort.

In implementing both functions, you are only allowed to modify "next" and "previous" of nodes, but not "values" of nodes.
You are not allowed to use any external structures such as array, linked list, etc.
You are not allowed to create any new node.
You are not allowed to create any new function.


After completing the above sequential version,  create a parallel version, by using two additional threads to speed up the merge sort.
You have to use the two functions you have implemented above.  You are not allowed to create new functions. Extra work will be needed in main function.

In your threaded implementation, you are allowed to introduce an extra node and a global pointer to the node.

It is alright if your implementation does not require the extra node or global pointer to node.

*/

#include <iostream>
#include <thread>

using namespace std;


class node {
public:
	int value;
	node* next;
	node* previous;
	node(int i) { value = i; next = previous = nullptr; }
	node() { next = previous = nullptr; }
};

class doubly_linked_list {
public:
	int num_nodes;
	node* head;
	node* tail;
	doubly_linked_list() { num_nodes = 0; head = tail = nullptr; }
	void make_random_list(int m, int n);
	void print_forward();
	void print_backward();


	//Recursively merge sort i numbers starting at node pointed by p
	void merge_sort(node* p, int i);//in-place recursive merge sort

	//Merge i1 numbers starting at node pointed by p1 with i2 numbers
	//starting at node pointed by p2
	void merge(node* p1, int i1, node* p2, int i2);
};

void doubly_linked_list::make_random_list(int m, int n) {

	for (int i = 0; i < m; i++) {
		node* p1 = new node(rand() % n);
		p1->previous = tail;
		if (tail != nullptr) tail->next = p1;
		tail = p1;
		if (head == nullptr) head = p1;
		num_nodes++;
	}
}

void doubly_linked_list::print_forward() {
	cout << endl;
	node* p1 = head;
	while (p1 != nullptr) {
		cout << p1->value << " ";
		p1 = p1->next;
	}
}

void doubly_linked_list::print_backward() {
	cout << endl;
	node* p1 = tail;
	while (p1 != nullptr) {
		cout << p1->value << " ";
		p1 = p1->previous;
	}
}

void doubly_linked_list::merge_sort(node* p, int i)
{
	if (i > 1) {
		int first = i / 2;
		int second = i - first;
		node* p2 = p, *p1 = p;
		for (int j = 0; j < first; j++) {
			p2 = p2->next;
		}
		merge_sort(p2, second);
		for (int j = 0; j < first; j++) {
			p2 = p2->next;
		}
		merge_sort(p, first);
		merge(p, first, p2, second);
	}
	return;
}

void doubly_linked_list::merge(node* p1, int i1, node* p2, int i2)
{
	int i = 0;
	node* n1 = p1, * n2 = p2;
	while (i < i1 + i2 && p1 != nullptr && p2 != nullptr) {
		if (p1->value > p2->value) {
			node *temp1, *temp2;
			temp1 = p1->next;
			temp2 = p2->next;
			if (p2->previous != nullptr)
				p2->previous->next = temp2;
			else
				head = temp2;
			if (temp2 != nullptr)
				temp2->previous = p2->previous;
			else
				tail = p2->previous;
			if (p1->previous != nullptr)
				p1->previous->next = p2;
			else
				head = p2;
			p2->previous = p1->previous;
			p2->next = p1;
			p1->previous = p2;
			p1 = temp1;
			p2 = temp2;
		}
		else {
			node *temp1, *temp2;
			temp1 = p1->next;
			temp2 = p2->next;
			// remove p2
			if (p2->previous != nullptr)
				p2->previous->next = temp2;
			else
				head = temp2;
			if (temp2 != nullptr)
				temp2->previous = p2->previous;
			else
				tail = p2->previous;
			// insert p2 after p1
			if (temp1 != nullptr)
				temp1->previous = p2;
			else
				tail = p2;
			p2->previous = p1->previous;
			p2->next = p1;
			p1->previous = p2;
			p1 = temp1;
			p2 = temp2;
		}
		i++;
	}
}


int main() {
	/*
	Implement the merge_sort and merge_functions defined above to complete a sequential version of
	merge sort.
	*/

	doubly_linked_list d1, d2;
	d1.make_random_list(30, 20);
	d1.print_forward();
	d1.print_backward();

	d1.merge_sort(d1.head, d1.num_nodes);
	d1.print_forward();
	d1.print_backward();


	d2.make_random_list(50, 40);
	d2.print_forward();
	d2.print_backward();

	/*
	Create two additional threads to speed up the merge sort.
	You have to still use the same merge_sort and merge functions implemented above.
	You will need to do some extra work within main funciton.
	*/
	d2.print_forward();
	d2.print_backward();
	return 0;
}