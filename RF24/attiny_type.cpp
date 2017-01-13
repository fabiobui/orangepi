#include <iostream>

using namespace std;

typedef struct {
        uint8_t id      :3;
        uint8_t type    :7;                                       
        uint8_t status  :6;
        uint8_t value;
} childtype;

typedef struct {
    uint8_t NodeID;
    childtype child[7]; // max 7 child nodes per node
    char code[3]; // unused
} Payload;


Payload tx;

int main() {
	// your code goes here
	tx.NodeID = 95;
	tx.child[0].id      = 2;
	tx.child[0].type    = 34;
	tx.child[0].status  = 1;
	
	cout << "Size " << sizeof(tx);
	
	return 0;
}
