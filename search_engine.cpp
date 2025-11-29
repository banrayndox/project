#include <bits/stdc++.h>
using namespace std;

// -------------------------- Utility functions --------------------------

vector<string> tokenize(string s){
    for(char &c : s){
        if(ispunct(c)) c = ' ';
        else c = tolower(c);
    }
    stringstream ss(s);
    vector<string> tokens;
    string w;
    while(ss >> w) tokens.push_back(w);
    return tokens;
}

// -------------------------- Search Engine Class --------------------------

class SearchEngine{
public:
    vector<string> documents;                        // store full documents
    map<string, vector<int>> invertedIndex;          // word -> list of docs
    vector<map<string,int>> termFreq;                // per document word freq
    map<string,int> docFreq;                         // word -> count of docs containing it

    // Add a document/page
    void addDocument(string text){
        documents.push_back(text);
        int id = documents.size() - 1;

        vector<string> tokens = tokenize(text);
        
        map<string,int> tf;
        for(string &w : tokens){
            tf[w]++;
        }
        termFreq.push_back(tf);

        // update inverted index + doc freq
        set<string> uniqueWords(tokens.begin(), tokens.end());
        for(string w : uniqueWords){
            invertedIndex[w].push_back(id);
            docFreq[w]++;
        }
    }

    // Search using TF-IDF ranking
    vector<pair<int,double>> search(string query){
        vector<string> qTokens = tokenize(query);
        map<int,double> score;

        for(string &w : qTokens){
            if(invertedIndex.find(w) == invertedIndex.end()) continue;

            double idf = log((double)documents.size() / (1 + docFreq[w]));

            for(int docID : invertedIndex[w]){
                double tf = termFreq[docID][w];
                score[docID] += tf * idf;
            }
        }

        vector<pair<int,double>> result(score.begin(), score.end());
        sort(result.begin(), result.end(), [](auto &a, auto &b){
            return a.second > b.second;
        });

        return result;
    }
};

// -------------------------- MAIN PROGRAM --------------------------

int main(){
    SearchEngine engine;

    cout << "\n=== ADVANCED SEARCH ENGINE PROJECT ===\n\n";

    // Sample documents (you can replace with file input)
    engine.addDocument("C++ is a powerful general purpose programming language.");
    engine.addDocument("Search engines use data structures like inverted index and ranking.");
    engine.addDocument("Machine learning improves search ranking and user experience.");
    engine.addDocument("C++ allows fast algorithms for implementing search engines.");

    while(true){
        cout << "\nEnter query (or type EXIT): ";
        string query;
        getline(cin, query);

        if(query == "EXIT") break;

        auto results = engine.search(query);

        if(results.empty()){
            cout << "No results found.\n";
        } else {
            cout << "\nTop Results:\n";
            for(auto &p : results){
                cout << "Document #" << p.first 
                     << " | Score: " << p.second << "\n";
                cout << " → " << engine.documents[p.first] << "\n\n";
            }
        }
    }

    return 0;
}
