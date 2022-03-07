#include<queue>
#include<iostream>
#include<memory>
using namespace std;
struct cmp {
    bool operator() (const int x, const int y) {
        return x > y;
    }
};



int main() {
    /*    
    priority_queue<int, vector<int>, cmp> maxHeap;
    maxHeap.push(1);
    maxHeap.push(5);
    maxHeap.push(2);
    cout << maxHeap.top() << endl;
    maxHeap.pop();
    cout << maxHeap.top() << endl;
    maxHeap.pop();
    */
   shared_ptr<vector<int>> spVec;
   (*spVec)[0];
   return 0;
}