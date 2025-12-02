#include <bits/stdc++.h>
using namespace std;
#include <chrono>

// WebPage class
class WebPage {
private:
    int id;
    string title;
    string content;
    string link;

public:
    WebPage(int _id, const string &_title, const string &_content, const string &_link)
        : id(_id), title(_title), content(_content), link(_link) {}

    int getId() const { return id; }
    string getTitle() const { return title; }
    string getContent() const { return content; }
    string getLink() const { return link; }

    void displaySummary() const {
        cout << "ID: " << id
             << " | Title: " << title
             << " | Link: " << link
             << "\nDescription: " 
             << (content.length() > 150 ? content.substr(0,150) + "..." : content)
             << "\n\n";
    }

    void displayFull() const {
        cout << "\n----- " << title << " -----\n";
        cout << "Link: " << link << "\n";
        cout << "Content:\n" << content << "\n---------------------\n";
    }
};

// Node for linked list
struct Node {
    WebPage page;
    Node* next;
    Node(const WebPage &p) : page(p), next(nullptr) {}
};

// Linked List class to store database
class LinkedList {
private:
    Node* head;
    Node* tail;
    int size;

public:
    LinkedList() : head(nullptr), tail(nullptr), size(0) {}

    void addPage(const WebPage &page) {
        Node* node = new Node(page);
        if (!head) head = tail = node;
        else {
            tail->next = node;
            tail = node;
        }
        size++;
    }

    Node* getHead() const { return head; }
    int getSize() const { return size; }

    WebPage* findById(int id) const {
        Node* curr = head;
        while (curr) {
            if (curr->page.getId() == id) return &curr->page;
            curr = curr->next;
        }
        return nullptr;
    }

    vector<WebPage*> getAllPages() const {
        vector<WebPage*> pages;
        Node* curr = head;
        while (curr) {
            pages.push_back(&curr->page);
            curr = curr->next;
        }
        return pages;
    }
};

// Search Engine class
class SearchEngine {
private:
    LinkedList database;
    unordered_map<string, set<int>> invertedIndex;

    string toLower(const string& str) {
        string res = str;
        transform(res.begin(), res.end(), res.begin(), ::tolower);
        return res;
    }

public:
    SearchEngine(const LinkedList &db) : database(db) {}

    void buildIndex() {
        Node* curr = database.getHead();
        while (curr) {
            stringstream ss(curr->page.getTitle() + " " + curr->page.getContent());
            string word;
            while (ss >> word) {
                invertedIndex[toLower(word)].insert(curr->page.getId());
            }
            curr = curr->next;
        }
    }

    vector<int> searchKeyword(const string &keyword) {
        string kw = toLower(keyword);
        if (invertedIndex.find(kw) != invertedIndex.end()) {
            return vector<int>(invertedIndex[kw].begin(), invertedIndex[kw].end());
        }
        return {};
    }

    void displayResults(const vector<int> &results, int pageSize = 2) {
        if (results.empty()) {
            cout << "No results found.\n";
            return;
        }

        int totalPages = (results.size() + pageSize - 1) / pageSize;
        int page = 0;

        while (true) {
            cout << "\nPage " << page + 1 << "/" << totalPages << ":\n";
            int start = page * pageSize;
            int end = min(start + pageSize, (int)results.size());

            for (int i = start; i < end; i++) {
                WebPage* p = database.findById(results[i]);
                if (p) p->displaySummary();
            }

            cout << "Options: [N]ext page | [P]revious page | [O]pen <ID> | [Q]uit\n";
            string choice;
            cin >> choice;

            if (choice == "N" || choice == "n") {
                if (page + 1 < totalPages) page++;
                else cout << "Already last page.\n";
            } else if (choice == "P" || choice == "p") {
                if (page > 0) page--;
                else cout << "Already first page.\n";
            } else if (choice == "O" || choice == "o") {
                int id;
                cin >> id;
                WebPage* p = database.findById(id);
                if (p) p->displayFull();
            } else if (choice == "Q" || choice == "q") break;
            else cout << "Invalid option! Try again.\n";
        }
    }
};

int main() {
    LinkedList pages;
    pages.addPage(WebPage(1, "C++ Basics", "Learn C++ programming from scratch. This tutorial covers variables, loops, functions, classes, and more to help you get started quickly.", "https://example.com/cpp-basics"));
    pages.addPage(WebPage(2, "Qt Tutorial", "GUI development with Qt framework. Create windows, buttons, input forms, layouts, and handle events in C++ using Qt.", "https://example.com/qt-tutorial"));
    pages.addPage(WebPage(3, "Advanced Search", "Building search engines in C++ using data structures like vectors, maps, and sets. Learn inverted index, keyword search, and ranking.", "https://example.com/advanced-search"));
    pages.addPage(WebPage(4, "Data Structures", "Learn arrays, linked list, stack, queue, trees, and graphs in C++. Understand their implementation and use in algorithms.", "https://example.com/ds"));
    pages.addPage(WebPage(5, "Algorithms", "Sorting, searching, graph traversal, dynamic programming, and more. Master algorithmic problem-solving with C++ examples.", "https://example.com/algo"));

    SearchEngine engine(pages);
    engine.buildIndex();

    while (true) {
        string keyword;
        cout << "\nEnter search keyword (or 'exit' to quit): ";
        cin >> keyword;
        if (keyword == "exit") break;

        auto start = chrono::high_resolution_clock::now();
        vector<int> results = engine.searchKeyword(keyword);
        auto end = chrono::high_resolution_clock::now();

        auto duration = chrono::duration_cast<chrono::milliseconds>(end - start);
        cout << "Search took " << duration.count() << " ms.\n";

        engine.displayResults(results);
    }

    cout << "Goodbye!\n";
    return 0;
}
