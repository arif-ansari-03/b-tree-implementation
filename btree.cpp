#include <bits/stdc++.h>

using namespace std;

/* For some reason I decided to call the vector of key, pointer pairs as nodes. This doesn't make sense as they are
** not nodes themselves. Might change the name at some point to keys or something else.
**
** Another note: I have given memory as a parameter in all the functions, but this is unnecessary. We can keep memory
** as global variable instead.
*/


/* Trying to implement btree
** In DBMS implementations, the root node's location is always fixed. Any overflow at the root page is accounted for,
** by swapping the pages. But in doing this, we have to update the parent of all the child nodes of the root. This
** is slower than just updating the root's location in the master table of dbms or the zeroth page in case of overflow in
** master table.
** So here, I am not worried about keeping the root at the same location in memory. However, it is very important
** not to forget this detail in the DBMS implementation.
**
** structure:
** In the dbms project, the structure contains parent, left most pointer, (key,pointer) pairs, and also data locations/data
** However, in this implementation, I only want to focus on operations on btree, not the storage part, so I will ignore data
** in this implementation
**
** So in the project, we have a vector of (key, pointer) pairs and parent pointer. The left most pointer is stored as node 0
** and its key will have minimum value.
** 
** In the DBMS, we have kept 0th page for separate use, so even here we will
** ignore the 0th index in memory vector, can keep it empty vector.
** 
** Since, this is a simple implementation, will assume that all keys take up
** about 4 bytes and that a page max size is say 40 bytes.
** Important thing to keep in mind is that we also have to store parent pointer,
** it also has to be added to current size of the page.
*/

// initially rootNode is 1 but we have to keep updating it if root is overflown.
int rootNode = 1;

struct Page
{
    vector<pair<int, int>> keys;
    int parent;
};

int size_of_page(Page page)
{
    int sz = 4;
    if (page.keys.size() == 0) return sz;

    sz += 4;
    sz += ((int)page.keys.size()-1) * 8;
    return sz;
}

/* search: 
** hmm so we want to check if a key exists and we also need to find the leaf node where it belongs?
** or maybe keep two separate functions, one to find the leaf node where the key should go
** and one that searches for key?
**
** what if we just find the best node for a given key. Then when we return the page/node, we can check in
** logn time if the key actually exists or maybe linear time. But since each page isn't very big, 
** even linear search might be fine.
**
** So let's try to implement a function that searches in a btree, the best page for a key, i.e, if the key
** exists, then return its page. If it doesn't exist then find the leaf node where it must belong.
*/

int search(int key, vector<Page> &memory, int cur_node)
{
    vector<pair<int,int>>& keys = memory[cur_node].keys; 
    int left = 0, right = keys.size();
    if (right == 0) return cur_node;
    right--;

    // pos is the largest index of the key in the current node
    // such that this key is less than or equal to the key to be inserted.
    int pos = 0;
    while (left <= right)
    {
        int mid = left + (right-left)/2;

        if (keys[mid].first <= key)
        {
            pos = mid;
            left = mid+1;
        }
        else right = mid-1;
    }

    if (key == keys[pos].first || keys[pos].second == 0) return cur_node;
    return search(key, memory, keys[pos].second);
}

/*
** insertion:
** So before insertion, we have to find the leaf node where the new key should go 
** This is done using the search function. Either we can overload insert function
** to call search function and then call another insert function or we can
** just call search and insert. Here I will not overload insert function, just
** to keep it easy to understand.
**
** Overflow:
** Hardest concept of btrees alongside underflow in deletion of key.
** Overflow involves finding median, taking left subarray, nodes[0 : median-1], parent key, nodes[median:] and redistributing in some way.
** 
*/

pair<int,int> overflow(vector<Page> &memory, int cur_node)
{
    int median = 0;
    int sz = size_of_page(memory[cur_node]);

    int cur_sz = 0;
    while (cur_sz < (sz+1)/2)
    {
        median++;
        cur_sz += 8;
    }

    int new_node = memory.size();
    memory.emplace_back(Page());

    memory[new_node].parent = memory[cur_node].parent;

    vector<pair<int,int>>& cur_keys = memory[cur_node].keys;
    vector<pair<int,int>>& new_keys = memory[new_node].keys; 

    new_keys.emplace_back(-1e9, cur_keys[median].second);

    for (int i = median+1; i<cur_keys.size(); i++)
        new_keys.emplace_back(cur_keys[i]);

    // this step is important because, we give the child of all elements >= median to a new node.
    // meaning these children have a new parent now, so we update them ALL.
    for (auto &[k, p] : new_keys)
        if (p) memory[p].parent = new_node;

    pair<int, int> new_key = {cur_keys[median].first, new_node};

    while (cur_keys.size()>median) cur_keys.pop_back();

    return new_key;
}

void insert(pair<int, int> key, vector<Page> &memory, int cur_node)
{
    vector<pair<int,int>>& keys = memory[cur_node].keys;

    if (keys.size() == 0)
    {
        keys.emplace_back(-1e9, 0);
        keys.emplace_back(key);
        return;
    }

    int i = keys.size();
    keys.emplace_back(key);
    // i holds the current index of key

    while (1)
    {
        if (keys[i].first < keys[i-1].first) swap(keys[i], keys[i-1]);
        else break;
        i--;
    }

    int sz = size_of_page(memory[cur_node]);
    if (sz <= 32) return;

    int parent = memory[cur_node].parent;

    if (parent == 0)
    {
        // making new page/node
        parent = memory.size();
        memory.emplace_back(Page());

        memory[parent].parent = 0;
        memory[parent].keys.emplace_back(-1e9, cur_node);

        memory[cur_node].parent = parent;
        rootNode = parent;
    }

    pair<int,int> new_key = overflow(memory, cur_node);

    insert(new_key, memory, parent);
}


/* Now time for deletion:
** Deletion is similar to insertion, we have underflow in deletion. There are two ways to solving an underflow, unlike overflow which usually only has 1 way to
** deal with it. Before coming to the 2 ways, let's define the underflow state. 
**
** A node is in underflow state if:
** 1. It is not the root node.
** 2. It has occupied less than some amount of memory. 
**
** Choosing the "some amount of memory":
** By "some amount of memory", I mean a fixed number, but in other implementations, where key size is fixed one may use number of keys as deciding factor. In this
** implementation, a page has 32 bytes of memory, 4 bytes for parent, 4 bytes for left most pointer, remaining 24 bytes for 3 key, pointer pairs.
** Defining lower bound for underflow is harder when in terms of memory. However, even in the extreme case, the B-tree should still be logarithmic, like a balanced
** binary tree. The extreme case being only 1 key, pointer pair in each node.
** It might be ok to keep underflow condition as less than 25% memory used rather than 50%. Because memory being less than 50% can easily happen when we split a node
** in overflow state. Memory per page is 4KB and most of the time, entries are not bigger than 500 bytes (12.5% of memory). The parent pointer and the left most
** pointer will take very little space, less than 5% ( about 1% to be more precise ) so we will have at least 7 keys in case of overflow, in this case when we
** split, clearly at least 2 keys remain which is 25% (2 * 12.5%).
** 
** So 25% is a good underflow limit.
** In this implementation however, the key size is fixed. In each node there are at most 3 keys, we can expect 1 key to remain in some node after splitting, so 
** let's keep 1 key as the minimum size. That is 4 bytes for left most pointer, 4 for parent, 8 for 1 key,pointer pair. so 16 bytes is minimum.
**
*/

void inOrder(vector<Page> &memory, int page_num)
{
    if (!page_num) return;
    inOrder(memory, memory[page_num].keys[0].second);
    for (int i = 1; i < memory[page_num].keys.size(); i++)
    {
        cout << memory[page_num].keys[i].first << '\n';
        inOrder(memory, memory[page_num].keys[i].second);
    }
}

void prtPage(Page &page)
{
    cout << "Parent: " << page.parent << " size: " << size_of_page(page) << '\n';
    for (auto &[k, p] : page.keys)
        cout << "{ " << k << ", " << p << " }, ";
    cout << '\n';
}

int maxH(vector<Page> &memory, int cur_node, int d = 0)
{
    if (cur_node == 0) return d-1;
    vector<pair<int,int>> keys = memory[cur_node].keys;

    if (keys.size()==0) return d-1;

    int D = d;
    for (int i = 0; i < keys.size(); i++)
        D = max(D, maxH(memory, keys[i].second, d+1));

    return D;
}

void prtMem(vector<Page> &memory)
{
    for (int i = 0; i < memory.size(); i++)
    {
        cout << i << ": ";
        prtPage(memory[i]);
    }
}


// pg = search(i, memory, rootNode);
// insert({i,0}, memory, pg);

int main()
{
    vector<Page> memory(2);
    int pg;

    for (int i = 1; i <= 10; i++)
    {
        pg = search(i, memory, rootNode);
        insert({i,0}, memory, pg);
        prtMem(memory);
    }

    
}