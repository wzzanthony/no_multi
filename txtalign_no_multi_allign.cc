/******************************************************************************

author: Zhizhi Wang

*******************************************************************************/
#include <iostream>
#include <fstream>
#include <vector>
#include <math.h>
#include <unordered_map>
#include <unordered_set>
#include <tuple>
#include <cassert>
#include <algorithm>
#include <dirent.h>
#include <sys/stat.h>
#include <chrono>
#include <cstring>

using namespace std;

#define EPS 0.00000001
int MAXLENGTH = 10000;//0x7fffffff; 

int K; 
int tau;
unordered_map<string, int> word2id; // map from word to id
vector<string> id2word;             // map from id to word
vector<int> id2maxfreq;
vector<vector<int>> id2mulId;

int hash_func = 0;
vector<pair<int, int>> ab;

// The hash value function
inline int hval(int word, int kth_hash, int multiplicity)
{
    assert(multiplicity >= 1);
    if (multiplicity == 1)
        return ab[kth_hash].first * word + ab[kth_hash].second;
    else
        return ab[kth_hash].first * id2mulId[word][multiplicity - 2] + ab[kth_hash].second;
}

// Generate random hash functions based on given seed
// ab: the output argument
void generateHashFunc(unsigned int seed, vector<pair<int, int>> &hf)
{
    // TODO: knuth shuffling?
    // TODO: random_seed
    srand(seed);
    int a = 0;
    while (a == 0)
        a = rand();
    int b = rand();
    hf.emplace_back(a, b);
}




class Range
{
public:
    int l;
    int r;
    Range(int a, int b)
        : l(a), r(b) {}
    Range()
    {
        l = 0;
        r = 0;
    }
};

class MultiWord
{
public:
    int hash;
    int pos;
    MultiWord(int h, int p)
        : hash(h), pos(p) {}
};
class CompactWindow
{
public:
    int hash_value;
    int c;
    int l;
    int r;
    CompactWindow(int hv, int pos, int left, int right){
        hash_value = hv;
        c = pos;
        l = left;
        r = right;
    }
};

class CompositeWindow
{
public:
    vector<int> sketch; // stores k hash values
    Range left;
    Range right;
    CompositeWindow() {}
    CompositeWindow(const vector<int> &hash){
        sketch = hash;
    }
    CompositeWindow(const vector<int> &hash, Range r1, Range r2)
    {
        sketch = hash;
        left.l = r1.l;
        left.r = r1.r;
        right.l = r2.l;
        right.r = r2.r;
    }
};
class TreeNode
{
public:
    int next; // for leaf node, the are pointers
    int prev; // for inner node, they are segments
    int word_pos = -1;
    bool has_visited = false;
    int hash_value;

    TreeNode() {}
    TreeNode(int l, int r)
    {
        next = l;
        prev = r;
    }
};

// This is a segement tree, root is 1, left child is *2 and right *2 + 1
// for each inner node, word_pos is -1 and [next, prev] is the segment
// for each leaf node, word_pos is the word position in the original text, and [next, prev] are pointers
// at begining the chain contains only header and tail.
class Tree
{
public:
    int root = 1;
    int leafNum; // the length of the original text
    int HEAD;
    int TAIL;

    vector<TreeNode> nodes; //use one vector to represent a tree and only the leaf nodes store the index value

    Tree(int size_data)
    {
        leafNum = size_data;
        nodes.clear();
        nodes.resize(4 * size_data);
        buildTree(leafNum);

        // initialize the chain
        HEAD = 0;
        TAIL = ((int)nodes.size()) - 1;
        nodes[HEAD].next = TAIL;
        nodes[HEAD].word_pos = -1;
        nodes[TAIL].prev = HEAD;
        nodes[TAIL].word_pos = leafNum;
    }

    void buildTree(int leafNum)
    {
        buildTreeHelper(root, 0, leafNum - 1);
    }

    void buildTreeHelper(const int node, const int segSt, const int segEn)
    {
        if (segSt == segEn)
        {
            // This is the leaf node
            nodes[node].prev = segSt;
            nodes[node].next = segSt;
            nodes[node].word_pos = segSt;
        }
        else
        {
            // Not a leaf node
            int mid = (segSt + segEn) >> 1;
            int leftChild = (node << 1);
            int rightChild = (node << 1) + 1;

            // First build left and then right
            buildTreeHelper(leftChild, segSt, mid);
            buildTreeHelper(rightChild, mid + 1, segEn);

            // Assertion !
            assert(nodes[leftChild].next + 1 == nodes[rightChild].prev);

            // Set left and right value
            nodes[node].prev = nodes[leftChild].prev;
            nodes[node].next = nodes[rightChild].next;
        }
    }

    inline int GetParent(int n) const
    {
        return n >> 1;
    }

    inline int GetLftChild(int n) const
    {
        return n << 1;
    }

    inline int GetRgtChild(int n) const
    {
        return (n << 1) + 1;
    }

    /**
    * given a word, update the chain using the tree by finding either its next or prev visited nodes
    * @param {Number} params 
    */

    void printChain()
    {
        int next = nodes[HEAD].next;
        cout << "chain: ";
        while (next != TAIL)
        {
            cout << nodes[next].word_pos << " ";
            next = nodes[next].next;
        }
        cout << endl;
    }

    int Word2Node(int node, int word_pos, int &subtree)
    {
        // lowest ancestor visited
        if (nodes[node].has_visited)
            subtree = node;

        if (nodes[node].word_pos != word_pos)
        {
            // inner node
            int leftChild = (node << 1);
            int rightChild = (node << 1) + 1;
            int mid = (nodes[node].prev + nodes[node].next) >> 1;
            //if (nodes[leftChild].prev <= word_pos && word_pos <= nodes[leftChild].next)
            if (word_pos <= mid)
                return Word2Node(leftChild, word_pos, subtree);
            else
                return Word2Node(rightChild, word_pos, subtree);
        }
        else
        {
            // leaf node
            return node;
        }
    }

    int UpdateChain(int word_pos)
    {
        int subtree = 0; // the root of the first visited subtree
        int tree_node = Word2Node(root, word_pos, subtree);

        // stop in advance because it has been visited before
        if (nodes[tree_node].has_visited)
        {
            return tree_node;
        }

        // get previous and next visited node
        int prev_node;
        int next_node;
        if (subtree == 0) // did not find any visited subtree
        {
            prev_node = HEAD;
            next_node = TAIL;
        }
        else if (nodes[GetLftChild(subtree)].has_visited)
        {
            // find a subtree whose left child has visited nodes
            prev_node = FindPrevVisited(GetLftChild(subtree));
            next_node = nodes[prev_node].next;
        }
        else if (nodes[GetRgtChild(subtree)].has_visited)
        {
            // find a subtree whose right child has visited nodes
            next_node = FindNextVisited(GetRgtChild(subtree));
            prev_node = nodes[next_node].prev;                         
        }

        // update chain
        nodes[prev_node].next = tree_node;
        nodes[next_node].prev = tree_node;
        nodes[tree_node].next = next_node;
        nodes[tree_node].prev = prev_node;

        // update the tree
        nodes[tree_node].has_visited = true;
        int parent = GetParent(tree_node);
        while (parent > 0 && !nodes[parent].has_visited)
        {
            nodes[parent].has_visited = true;
            parent = GetParent(parent);
        }
        return tree_node;
    }

    int FindNextVisited(int tree_node)
    {
        while (nodes[tree_node].word_pos == -1)
        {
            // inner node
            if (nodes[GetLftChild(tree_node)].has_visited)
                tree_node = GetLftChild(tree_node);
            else // use right child instead
                tree_node = GetRgtChild(tree_node);
        }
        return tree_node;
    }

    int FindPrevVisited(int tree_node)
    {
        while (nodes[tree_node].word_pos == -1)
        {
            // inner node
            if (nodes[GetRgtChild(tree_node)].has_visited)
                tree_node = GetRgtChild(tree_node);
            else // use left child instead
                tree_node = GetLftChild(tree_node);
        }
        return tree_node;
    }
    void findCWSAllign(const MultiWord &multi_word, vector<CompactWindow> &res_cws, const vector<int> &doc)
    {
        int hv = multi_word.hash;
        int position = multi_word.pos;
        int tree_node = UpdateChain(position);
        nodes[tree_node].hash_value = hv;
        //find from its left
        int node = nodes[tree_node].prev;
        while (nodes[node].word_pos != -1){
            int temp_hv = nodes[node].hash_value;
            if (temp_hv < hv){
                break;
            }
            node = nodes[node].prev;
        }
        int left = nodes[node].word_pos + 1;
        node = nodes[tree_node].next;
        while (nodes[node].word_pos != leafNum){
            int temp_hv = nodes[node].hash_value;
            if (temp_hv < hv){
                break;
            }
            node = nodes[node].next;
        }
        int right = nodes[node].word_pos - 1;
        res_cws.emplace_back(hv, position, left, right);
    } 
    void findCWSKmins(const MultiWord &multi_word, vector<CompositeWindow> &res_cws, const vector<int> &doc){
        int left_l = -1;
        int left_r = -1;
        unordered_set<int> hash_values;
        int hv = multi_word.hash;
        int position = multi_word.pos;
        int tree_node = UpdateChain(position);
        nodes[tree_node].hash_value = hv;
        //x is the number of selected items from the left part
        for (int x = 0; x < K; x++){
            //cout << "-------------------------------" << endl;
            hash_values.clear();
            //find from its left part
            hash_values.insert(hv);
            //cout << "current hash value: " << hv << endl;
            int node = nodes[tree_node].prev;
            int left_count = 0;
            //cout << "find from left part" << endl;
            while (nodes[node].word_pos != -1 ){
                int temp_hv = nodes[node].hash_value;
                if (hash_values.find(temp_hv) == hash_values.end()){
                    left_count += 1;
                    if (left_count > x){
                        break;
                    }
                    hash_values.insert(temp_hv);
                }
                node = nodes[node].prev;
            }
            if (left_count >= x){
                left_l = nodes[node].word_pos+1;
                node = nodes[node].next;
                left_r = nodes[node].word_pos;
            }else{
                //x will turn into x+1 but it fails at x
                break;
            }
            //cout << "find from the right part" << endl;
            //find from its right part
            node = nodes[tree_node].next;
            int right_count = 0;
            while (nodes[node].word_pos != leafNum){
                int temp_hv = nodes[node].hash_value;
                //cout <<"current hash value from right: " << temp_hv << endl;
                if (hash_values.find(temp_hv) == hash_values.end()){
                    right_count += 1;
                    if (right_count > K - x - 1){
                        break;
                    }
                    hash_values.insert(temp_hv);
                }
                node = nodes[node].next;
            }
            if (right_count >= K - x - 1){
                int right_r = nodes[node].word_pos - 1;
                node = nodes[node].prev;
                int right_l = nodes[node].word_pos;
                //add it to the composite window
                vector<int> sketch;
                for (auto it = hash_values.begin(); it != hash_values.end(); it++){
                    sketch.push_back(*it);
                }
                res_cws.emplace_back(sketch);
                res_cws.back().left.l = left_l;
                res_cws.back().left.r = left_r;
                res_cws.back().right.l = right_l;
                res_cws.back().right.r = right_r;
            }
            //cout << "-------------------------------_________" << endl;
        }
    }
};
void preProcess(const vector<int> &doc, vector<MultiWord> &multi_doc)
{
    for (int pos = 0; pos < doc.size(); pos++)
    {
        int word = doc[pos];
        int hash_value = hval(word, hash_func, 1);
        multi_doc.emplace_back(hash_value, pos);
    }
    sort(multi_doc.begin(), multi_doc.end(), [](const MultiWord &p1, const MultiWord &p2)
        {
             if (p1.hash < p2.hash)
                 return true;
             else if (p1.hash > p2.hash)
                 return false;
             else
                 return p1.pos < p2.pos;
        });
}

/*********** IO ************/

// Make string to tokens based on delimiters
// Output parameter: res
void strToTokens(string &str, const string &delimiter, vector<string> &res, vector<pair<int, int>> &offsets) {
    #ifdef NORMAL
    // Replace illegal chars
    for (int i = 0; i < str.length(); ++i) {
      str[i] = str[i] <= 0 || str[i] == '\n'? ' ': str[i];
    }
    #endif
    char *inputStr = strdup(str.c_str());
    int wordNum = 0;
    char *key = strtok(inputStr, delimiter.c_str());

    // Iterate each word
    while(key){
      // Calculate start position
      int startPos = key - inputStr;
      int endPos = startPos + strlen(key);
      wordNum ++;
      for(int p = 0; p < strlen(key); p ++)
        key[p] = tolower(key[p]);
      res.push_back(key);
      offsets.push_back({startPos, endPos});
      key = strtok(0, delimiter.c_str());
    }

    delete []inputStr;
}

void getFiles(string path, vector<string> &files)
{
    DIR *dr;
    struct dirent *en;
    string file_path;
    dr = opendir(path.c_str()); //open all directory
    if (dr)
    {
        while ((en = readdir(dr)) != NULL)
        {
            //ignore hidden files and folders
            if (en->d_name[0] != '.')
            {
                file_path = path + en->d_name;
                files.push_back(file_path);
            }
        }
        closedir(dr); //close all directory
    }
    cout << endl;
}

// Read stopwords from given file name
void readStopWords(const string &fileName, unordered_set<string> &stopWords)
{
    // one per line
    string stopWord;
    ifstream file(fileName, ios::in);
    while (getline(file, stopWord))
        stopWords.insert(stopWord);
}

// Read from file and do preprocessing
//
// Input parameter:
//  1) filename: the file name
//  2) stopwords: stopwords that don't need to be considered
//
// Output parameter:
//  1) doc: contains the word id (from word string => word id)
//  2) ppos: contains peroid positions
//  3) word2id: the mapping from word to id
//  4) id2word: the mapping from id to word
void word2int(const string &filename, vector<int> &doc, unordered_map<string, int> &word2id, vector<string> &id2word, vector<int> &id2maxFreq, vector<vector<int>> &id2mulId, const unordered_set<string> &stopwords)
{
    static int mulWordId = -1;
#ifdef NORMAL
    const string delim = "\t\n\r\x0b\x0c !\"#$%&\'()*+,-./:;<=>?@[\\]^_`{|}~";
    const string period_str = ",.!?;";
//cout << "execute NORMAL here line 102" <<endl;
#endif

#ifndef NORMAL
    const string delim = " ";
    const string period_str = "\n";
//cout << "execute NORMAL here line 108" <<endl;
#endif

    // Read from file ...
    ifstream datafile(filename, ios::binary);
    datafile.seekg(0, std::ios_base::end);
    int length = datafile.tellg();
    string docstr(length + 1, '\0');
    datafile.seekg(0);
    datafile.read(&docstr[0], length);

    // Make the docstr to sentences ..
    vector<string> tokens;
    vector<pair<int, int>> tmp;
    strToTokens(docstr, delim, tokens, tmp);

    unordered_map<int, int> id2freq;
    int wordcnt = 0;
    for (int i = 0; i < tokens.size(); i++)
    {

        // Skip stop words
        if (stopwords.find(tokens[i]) != stopwords.end())
            continue;
        
        if (wordcnt++ >= MAXLENGTH)
          break;

        // If a new word, add to word2id, offsets, doc
        if (word2id.find(tokens[i]) == word2id.end())
        {
            word2id[tokens[i]] = id2word.size();
            id2word.emplace_back(tokens[i]);
        }

        int wid = word2id[tokens[i]];
        doc.emplace_back(wid);
        id2freq[wid] += 1;
    }

    // Now calculate multi-id for each word that appears multiple times .. 
    id2maxFreq.resize(id2word.size(), 1);
    id2mulId.resize(id2word.size(), vector<int>());

    for (auto &entry : id2freq)
    {
        int id = entry.first;
        int freq = entry.second;
        if (id2maxFreq[id] >= freq)
            continue;

        for (int i = id2maxFreq[id] + 1; i <= freq; i++)
        {
            id2mulId[id].emplace_back(mulWordId--);
        }
        id2maxFreq[id] = freq;
    }
}

void bf(vector<int> &doc, unordered_map<uint64_t, pair<int, int>> &um)
{
    vector<vector<int>> sketches;
    vector<pair<int, int>> passage;
    for (int i = 0; i < doc.size(); i++)
    {
        for (int j = i + K - 1; j < doc.size(); j++)
        {
            unordered_map<int, int> word_pos; // the positions of the key (a word)
            vector<int> hashes;
            for (int pos = i; pos <= j; pos++)
            {
                int word = doc[pos];
                int hash_value = hval(word, hash_func, 1);
                hashes.push_back(hash_value);
            }
            nth_element(hashes.begin(), hashes.begin() + K, hashes.end());
            sketches.emplace_back(hashes.begin(), hashes.begin() + K);   
            sort(sketches.back().begin(), sketches.back().end());
            passage.emplace_back(i, j);
            
            /*
            cout << i << " to --- to " << j << endl;
            for (auto &e : sketches.back())
                cout << e << " ";
            cout << endl;
            */
        }
    }

    cout << "before unique: " << sketches.size() << endl;

    int pid = 0;
    for (auto &entry : sketches)
    {
        uint64_t hv = 0;
        int x = 7919;
        assert(entry.size() == K);
        for (int i = 0; i < K; i++)
        {
            hv *= x;
            hv += entry[i];
        }
        um[hv] = passage[pid];
        pid++;
    }
    cout << "after unique: " << um.size() << endl;
}

/*********** IO ************/

int main(int argc, char **argv)
{
    auto start = chrono::high_resolution_clock::now();

    string src_path = "src/";       //path of the source folder
    string query_file = "test_long.txt"; //quert test's filename
    double theta = 0.8;             // threshold
    K = 16;
    int doc_number = -1;
    tau = 16;
    for (int i = 0; i < argc; i++)
    {
        string arg = argv[i];
        if (arg == "-src_path")
            src_path = string(argv[i + 1]);
        if (arg == "-query_file")
            query_file = string(argv[i + 1]);
        if (arg == "-theta")
            theta = atof(argv[i + 1]);
        if (arg == "-k")
            K = atoi(argv[i + 1]);
        if (arg == "-n")
            MAXLENGTH = atoi(argv[i + 1]);
        if (arg == "-doc_number")
            doc_number = atoi(argv[i + 1]);
        if  (arg == "-tau")
            tau = atoi(argv[i+1]);
    }
    //0. generate hash functions
    generateHashFunc(1111, ab);

    //1. read stopwords from stopwords.txt
    unordered_set<string> stopWords;       // store stopwords
    readStopWords("stopwords.txt", stopWords);

    auto stop = chrono::high_resolution_clock::now();
    auto duration = chrono::duration_cast<chrono::microseconds>(stop - start);
    //cout << "time for loading stopwords 1: " << duration.count() / 1000000.0 << " seconds" << endl;
    start = chrono::high_resolution_clock::now();

    //2. read words from source folder
    vector<string> files;           //store file_path of each document in the given folder
    getFiles(src_path, files);
    int doc_num = files.size();
    if (doc_number != -1) doc_num = doc_number;
    vector<vector<int>> docs(doc_num);
    int did = 0;
    for (auto &file : files)
    {
        //cout << "current dealing with file: "<< did << endl;
        word2int(file, docs[did], word2id, id2word, id2maxfreq, id2mulId, stopWords);
        did++;
        if (did >= doc_num) break;
    }
    stop = chrono::high_resolution_clock::now();
    duration = chrono::duration_cast<chrono::microseconds>(stop - start);
    cout << "readfile time: " << duration.count() / 1000000.0 << "s" << endl;

    // indexing
    start = chrono::high_resolution_clock::now();
    //vector<vector<CompactWindow>> docs_cws(doc_num); //store compact windows of each document in the given folder    
    vector<vector<CompactWindow>> docs_cws(doc_num);
    for (int docid = 0; docid < doc_num; docid++)
    {
        cout << "the doc size: " << docs[docid].size() << endl;
        vector<MultiWord> multi_doc; //(hash_value, pos)
	    preProcess(docs[docid], multi_doc);
        cout <<"finish preProcess" << endl;
        Tree tree(docs[docid].size());
        for (const auto &multi_word : multi_doc){
            tree.findCWSAllign(multi_word, docs_cws[docid], docs[docid]);
            //tree.findCWSAllign(multi_word, docs_cws[docid], docs[docid]);
        }
    }

    stop = chrono::high_resolution_clock::now();
    duration = chrono::duration_cast<chrono::microseconds>(stop - start);
    cout << "source window generation time: " << K * duration.count() / 1000000.0 << "s" << endl;
    
    // build the prefix filter index
    start = chrono::high_resolution_clock::now(); 
    
    vector<vector<int>> bottomks;
    vector<vector<pair<int, int>>> bottomk_of_cws;
    unordered_map<uint64_t, int> hash2vec;
    unordered_map<int, vector<pair<int,int>>> prefixes; //just to finish the compile 
   
    vector<int> query;
    //vector<CompactWindow> query_cws;       //the compact window of the query text
    vector<CompactWindow> query_cws;
    word2int(query_file, query, word2id, id2word, id2maxfreq, id2mulId, stopWords);
    
    start = chrono::high_resolution_clock::now();  

    vector<MultiWord> query_multi_doc; //(hash_value, pos, left, right)
	preProcess(query, query_multi_doc);
    Tree query_tree(query.size());
    for (const auto &query_multi_word : query_multi_doc)
        query_tree.findCWSAllign(query_multi_word, query_cws, query);
        //query_tree.findCWSAllign(query_multi_word, query_cws, query);

    stop = chrono::high_resolution_clock::now();
    duration = chrono::duration_cast<chrono::microseconds>(stop - start);
    cout << "query window generation time: " << K * duration.count() / 1000000.0 << "s" << endl;
    cout << "query cws size: " << K * query_cws.size() << endl;
    cout << endl;

    unordered_map<uint64_t, pair<int, int>> gt;
    return 0;
   
}
