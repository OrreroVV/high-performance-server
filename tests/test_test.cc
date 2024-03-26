#include <bits/stdc++.h>

using namespace std;

static uint32_t EncodeZigzag32(const int32_t& v) {
    if(v < 0) {
        return ((uint32_t)(-v)) * 2 - 1;
    } 
    else {
        return v * 2;
    }
}

static int32_t DecodeZigzag32(const uint32_t& v) {
    return (v >> 1) ^ -(v & 1);
}

struct Node
{
    /*
    8
    8
    24
    */
    
     int8_t v;
     int64_t v1;
     char x[9];

};

int main(int argc, char *argv[]) {
    char *t = new char[100];
    
    cout << sizeof(Node) << endl;
    return 0;
}