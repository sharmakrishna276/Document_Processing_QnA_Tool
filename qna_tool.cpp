#include <assert.h>
#include <sstream>
#include "qna_tool.h"
#include <cmath>

using namespace std;

void readfile (Dict& d) {
    ifstream file("unigram_freq.csv");
    if (!file.is_open()) {
        cout << "Error opening the file!" << endl;
        return;
    }
    string line;
    getline(file,line);
    while (getline(file, line)) {
        istringstream ss(line);
        vector<string> row;
        string cell;
        while (getline(ss, cell, ',')) {
            row.push_back(cell);
        }
        long long temp_count=stoll(row[1]);
        d.insert_word(row[0],temp_count);
    }
    file.close();
}

class my_para {
    public:
    long double para_score = 0.0;
    Dict my_dict;
    int bk;
    int pgn;
    int prg;
    my_para()
    {
        para_score = 0.0;
        my_dict = Dict();
        int bk = -1;
        int pgn = -1;
        int prg = -1;
    }
};

QNA_tool::QNA_tool(){
    // Implement your function here
    book=-1;
    para_count=0;
    para_temp=-1;
    page_temp=-1;
    readfile(gen_corpus);
    for(int i = 0; i < 35;i++)
    {
        pref[i] = -1;
    }
    for(int i = 35;i < 128;i++)
    {
        pref[i] = 0;
    }
    pref[39] = -1;
    pref[40] = -1;
    pref[41] = -1;
    pref[44] = -1;
    pref[45] = -1;
    pref[46] = -1;
    pref[58] = -1;
    pref[59] = -1;
    pref[63] = -1;
    pref[64] = -1;
    pref[91] = -1;
    pref[93] = -1;
    pref[127] = -1;
}

QNA_tool::~QNA_tool(){
    // Implement your function here
    for(int i = 0; i < para_count;i++)
    {
        delete para[i];

    }
}

void QNA_tool::insert_sentence(int book_code, int page, int paragraph, int sentence_no, string sentence){
    // Implement your function here
    corpus.insert_sentence(book_code, page, paragraph, sentence_no, sentence);
    if (book==book_code && page_temp==page && para_temp==paragraph) {
        para[para_count-1]->my_dict.insert_sentence(book_code, page, paragraph, sentence_no, sentence);
    }
    else {
        my_para* new_para = new my_para();
        new_para->my_dict.insert_sentence(book_code, page, paragraph, sentence_no, sentence);
        new_para->bk=book_code;
        new_para->pgn=page;
        new_para->prg=paragraph;
        para.push_back(new_para);
        para_count++;
        book=book_code;
        page_temp=page;
        para_temp=paragraph;
    }
    return;
}

void splitsentence(vector<string> &words,string sentence,int* pref) {
    string word="";
    int len = sentence.size();
    for (int i=0;i<len;i++) {
        char x = sentence[i];
        if (pref[x] == -1) {
            int num = word.size();
            if (num>0) {
                words.push_back(word);
            }
            word="";
        }
        else {
            word+=x;
        }
    }
    int num = word.size();
    if (num>0) {
        words.push_back(word);
    }
}

int parent(int idx)
{
    return (idx-1)/2;
}

void hup(vector<my_para*>& heap, int shize)
{
    int child = shize-1;
    int par = parent(child);
    while(par != child && heap[par]->para_score > heap[child]->para_score)
    {
        my_para* tempu = heap[par];
        heap[par] = heap[child];
        heap[child] = tempu;
        child = par;
        par = parent(child);
    }
}

void hdown(vector<my_para*>& heap, int shize)
{
    int par = 0;
    while(2*par + 1 < shize)
    {
        int l = 2*par+1;
        int r = 2*par + 2;
        my_para* mini = heap[l];
        if(r < shize)
        {
            if(heap[r]->para_score < mini->para_score)
            {
                mini = heap[r];
            }
        }
        if(mini->para_score < heap[par]->para_score)
        {
            if(mini->para_score == heap[l]->para_score)
            {
                heap[l] = heap[par];
                heap[par] = mini;
                par = l;
            }
            else
            {
                heap[r] = heap[par];
                heap[par] = mini;
                par = r;
            }
        }
        else
        {
            break;
        }
    }
}

Node* QNA_tool::get_top_k_para(string question, int k) {
    // Implement your function here
    vector<string> words;
    splitsentence(words,question,pref);
    int sz=words.size();
    long double qry[sz];
    for(int i = 0; i < sz;i++)
    {
        long long a = gen_corpus.get_word_count(words[i]);
        long long b = corpus.get_word_count(words[i]);
        long double one = 1;
        qry[i] = (one+b)/(one+a);
    }
    for (long long j=0;j<para_count;j++) {
        for (int i=0;i<sz;i++) {
            para[j]->para_score+= qry[i]*para[j]->my_dict.get_word_count(words[i]);
        }
        if(j < k)
        {
            hup(para,j+1);
        }
        else if(para[j]->para_score > para[0]->para_score)
        {
            my_para* tp = para[j];
            para[j] = para[0];
            para[0] = tp;
            hdown(para,k);
        }
    }
    //sort(para.rbegin(),para.rend(),my_compare);
    Node* head = new Node(para[0]->bk,para[0]->pgn,para[0]->prg,-5,-5);
    int shize = k;
    my_para* tomp = para[0];
    para[0] = para[shize-1];
    para[shize-1] = tomp;
    shize--;
    hdown(para,shize);
    head->left=nullptr;
    head->right=nullptr;
    for (int i=1;i<k;i++) {
        Node* temp = new Node(para[0]->bk,para[0]->pgn,para[0]->prg,-5,-5);
        my_para* tomp = para[0];
        para[0] = para[shize-1];
        para[shize-1] = tomp;
        shize--;
        hdown(para,shize);
        temp->left=nullptr;
        temp->right=head;
        head->left=temp;
        head=temp;
    }
    return head;
}

void QNA_tool::query(string question, string filename){
    // Implement your function here
    Node* t = QNA_tool::get_top_k_para(question, 5);
    query_llm(filename, t, 5, "OpenAI API key", question);
    //std::cout << "Q: " << question << std::endl;
    //std::cout << "A: " << "Studying COL106 :)" << std::endl;
    return;
}

std::string QNA_tool::get_paragraph(int book_code, int page, int paragraph){

    cout << "Book_code: " << book_code << " Page: " << page << " Paragraph: " << paragraph << endl;

    std::string filename = "mahatma-gandhi-collected-works-volume-";
    filename += to_string(book_code);
    filename += ".txt";

    std::ifstream inputFile(filename);

    std::string tuple;
    std::string sentence;

    if (!inputFile.is_open()) {
        std::cerr << "Error: Unable to open the input file " << filename << "." << std::endl;
        exit(1);
    }

    std::string res = "";

    while (std::getline(inputFile, tuple, ')') && std::getline(inputFile, sentence)) {
        // Get a line in the sentence
        tuple += ')';

        int metadata[5];
        std::istringstream iss(tuple);

        // Temporary variables for parsing
        std::string token;

        // Ignore the first character (the opening parenthesis)
        iss.ignore(1);

        // Parse and convert the elements to integers
        int idx = 0;
        while (std::getline(iss, token, ',')) {
            // Trim leading and trailing white spaces
            size_t start = token.find_first_not_of(" ");
            size_t end = token.find_last_not_of(" ");
            if (start != std::string::npos && end != std::string::npos) {
                token = token.substr(start, end - start + 1);
            }

            // Check if the element is a number or a string
            if (token[0] == '\'') {
                // Remove the single quotes and convert to integer
                int num = std::stoi(token.substr(1, token.length() - 2));
                metadata[idx] = num;
            } else {
                // Convert the element to integer
                int num = std::stoi(token);
                metadata[idx] = num;
            }
            idx++;
        }

        if(
            (metadata[0] == book_code) &&
            (metadata[1] == page) &&
            (metadata[2] == paragraph)
        ){
            res += sentence;
        }
    }

    inputFile.close();
    return res;
}

void QNA_tool::query_llm(string filename, Node* root, int k, string API_KEY, string question){

    // first write the k paragraphs into different files

    Node* traverse = root;
    int num_paragraph = 0;

    while(num_paragraph < k){
        assert(traverse != nullptr);
        string p_file = "paragraph_";
        p_file += to_string(num_paragraph);
        p_file += ".txt";
        // delete the file if it exists
        remove(p_file.c_str());
        ofstream outfile(p_file);
        string paragraph = get_paragraph(traverse->book_code, traverse->page, traverse->paragraph);
        assert(paragraph != "$I$N$V$A$L$I$D$");
        outfile << paragraph;
        outfile.close();
        traverse = traverse->right;
        num_paragraph++;
    }

    // write the query to query.txt
    ofstream outfile("query.txt");
    outfile << "On the basis of this, ";
    outfile << question;
    outfile << "\nI want answers strictly based on the excerpts provided.\nRead each excerpt line by line and tell me the relevant information from excerpts based on the question that I provided!";
    //outfile << "\nAlso state where exactly in the excerpt did the line that you stated occurs";
    // You can add anything here - show all your creativity and skills of using ChatGPT
    outfile.close();

    // you do not need to necessarily provide k paragraphs - can configure yourself

    // python3 <filename> API_KEY num_paragraphs query.txt
    string command = "python3 ";
    command += filename;
    command += " ";
    command += API_KEY;
    command += " ";
    command += to_string(k);
    command += " ";
    command += "query.txt";

    system(command.c_str());
    return;
}
