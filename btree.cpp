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
    vector<pair<int, int>> nodes;
    int parent;
};

int size_of_page(Page page)
{
    int sz = 4;
    if (page.nodes.size() == 0) return sz;

    sz += 4;
    sz += ((int)page.nodes.size()-1) * 8;
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
    vector<pair<int,int>>& nodes = memory[cur_node].nodes; 
    int left = 0, right = nodes.size();
    if (right == 0) return cur_node;
    right--;

    // pos is the largest index of the key in the current node
    // such that this key is less than or equal to the key to be inserted.
    int pos = 0;
    while (left <= right)
    {
        int mid = left + (right-left)/2;

        if (nodes[mid].first <= key)
        {
            pos = mid;
            left = mid+1;
        }
        else right = mid-1;
    }

    if (key == nodes[pos].first || nodes[pos].second == 0) return cur_node;
    return search(key, memory, nodes[pos].second);
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

    vector<pair<int,int>>& cur_nodes = memory[cur_node].nodes;
    vector<pair<int,int>>& new_nodes = memory[new_node].nodes; 

    new_nodes.emplace_back(-1e9, cur_nodes[median].second);

    for (int i = median+1; i<cur_nodes.size(); i++)
        new_nodes.emplace_back(cur_nodes[i]);

    pair<int, int> new_key = {cur_nodes[median].first, new_node};

    while (cur_nodes.size()>median) cur_nodes.pop_back();

    return new_key;
}

void insert(pair<int, int> key, vector<Page> &memory, int cur_node)
{
    vector<pair<int,int>>& nodes = memory[cur_node].nodes;

    if (nodes.size() == 0)
    {
        nodes.emplace_back(-1e9, 0);
        nodes.emplace_back(key);
        return;
    }

    int i = nodes.size();
    nodes.emplace_back(key);
    // i holds the current index of key

    while (1)
    {
        if (nodes[i].first < nodes[i-1].first) swap(nodes[i], nodes[i-1]);
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
        memory[parent].nodes.emplace_back(-1e9, cur_node);

        memory[cur_node].parent = parent;
        rootNode = parent;
    }

    pair<int,int> new_key = overflow(memory, cur_node);

    insert(new_key, memory, parent);
}

void inOrder(vector<Page> &memory, int page_num)
{
    if (!page_num) return;
    inOrder(memory, memory[page_num].nodes[0].second);
    for (int i = 1; i < memory[page_num].nodes.size(); i++)
    {
        cout << memory[page_num].nodes[i].first << ' ' << memory[page_num].nodes[i].second << '\n';
        inOrder(memory, memory[page_num].nodes[i].second);
    }
}

void prtPage(Page &page)
{
    cout << "Parent: " << page.parent << '\n';
    for (auto &[k, p] : page.nodes)
        cout << "{ " << k << ", " << p << " }, ";
    cout << '\n';
}

int main()
{
    vector<Page> memory(2);

    int pg;
    vector<int> A = {6, 5, 7, 4, 8, 3, 9, 2, 10, 1, 11};

    for (auto &a : A)
    {
        pg = search(a, memory, rootNode);
        insert({a,0}, memory, pg);
        cout << a << ' ' << pg << '\n';

        for (int i = 0; i < memory.size(); i++)
        {
            cout << i << ": ";
            prtPage(memory[i]);
        }
    }

    // cout << memory[1].nodes.size() << '\n';
    // for (auto &[k, p] : memory[1].nodes)
    //     cout << k << ' ' << p << '\n';

    // cout << size_of_page(memory[1]) << ' ';

    inOrder(memory, rootNode);

    
}