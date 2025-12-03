// search_engine_full.cpp
// Compile: g++ -std=c++17 search_engine_full.cpp -O2 -o search
#include <bits/stdc++.h>
using namespace std;

// -------------------- Utilities --------------------
string toLower(const string &s) {
    string r = s;
    transform(r.begin(), r.end(), r.begin(), ::tolower);
    return r;
}

vector<string> tokenize(const string &text) {
    // basic tokenization: split on non-alphanumeric, keep words and numbers
    vector<string> tokens;
    string token;
    for (char c : text) {
        if (isalnum((unsigned char)c)) token.push_back(tolower(c));
        else {
            if (!token.empty()) { tokens.push_back(token); token.clear(); }
        }
    }
    if (!token.empty()) tokens.push_back(token);
    return tokens;
}

int levenshtein(const string &a, const string &b) {
    int n = a.size(), m = b.size();
    if (n == 0) return m;
    if (m == 0) return n;
    vector<int> prev(m+1), cur(m+1);
    iota(prev.begin(), prev.end(), 0);
    for (int i = 1; i <= n; ++i) {
        cur[0] = i;
        for (int j = 1; j <= m; ++j) {
            int cost = (a[i-1]==b[j-1])?0:1;
            cur[j] = min({ prev[j] + 1, cur[j-1] + 1, prev[j-1] + cost });
        }
        swap(prev, cur);
    }
    return prev[m];
}

// -------------------- Document & DB --------------------
struct Doc {
    int id;
    string title;
    string content;
    string link;
    vector<string> tokens;        // all tokens from title+content
    unordered_map<string,int> tf; // term frequency per doc
    string acronym;              // shortform created from title words
};

class SearchEngine {
private:
    vector<Doc> docs;
    unordered_map<string, unordered_map<int,int>> index; // token -> (docId -> freq)
    unordered_map<string, int> docFreq;                 // token -> doc frequency (df)
    int N = 0;
    set<string> stopwords;

    void buildStopwords() {
        const string arr[] = {"the","is","at","which","on","and","a","an","of","in","to","for","with","that","this","it","by","as","from"};
        for (auto &w : arr) stopwords.insert(w);
    }

    string buildAcronym(const string &text) {
        vector<string> words = tokenize(text);
        string a;
        for (auto &w : words) {
            if (!w.empty()) a.push_back(w[0]);
        }
        return a;
    }

public:
    SearchEngine() { buildStopwords(); }

    void addDoc(int id, const string &title, const string &content, const string &link) {
        Doc d;
        d.id = id;
        d.title = title;
        d.content = content;
        d.link = link;

        string combined = title + " " + content;
        d.tokens = tokenize(combined);
        for (auto &t : d.tokens) {
            if (stopwords.count(t)) continue;
            d.tf[t]++;
        }

        d.acronym = buildAcronym(title);
        docs.push_back(move(d));
        N = docs.size();
    }

    void buildIndex() {
        index.clear();
        docFreq.clear();
        for (auto &d : docs) {
            unordered_set<string> seen;
            for (auto &p : d.tf) {
                const string &token = p.first;
                int freq = p.second;
                index[token][d.id] = freq;
                if (!seen.count(token)) { docFreq[token]++; seen.insert(token); }
            }
        }
    }

    double idf(const string &token) {
        int df = 0;
        if (docFreq.find(token) != docFreq.end()) df = docFreq[token];
        if (df == 0) return 0.0;
        return log((double)N / (double)df);
    }

    // Compute TF-IDF vector for a document (map from token->weight)
    unordered_map<string,double> docVector(const Doc &d) {
        unordered_map<string,double> vec;
        double norm = 0.0;
        for (auto &p : d.tf) {
            double w = (1 + log(p.second)) * idf(p.first);
            vec[p.first] = w;
            norm += w*w;
        }
        norm = sqrt(norm);
        if (norm == 0) return vec;
        for (auto &x : vec) x.second /= norm; // normalize
        return vec;
    }

    // Compute TF-IDF vector for query tokens
    unordered_map<string,double> queryVector(const vector<string> &qtokens) {
        unordered_map<string,int> qtf;
        for (auto &t : qtokens) if (!stopwords.count(t)) qtf[t]++;
        unordered_map<string,double> vec;
        double norm = 0.0;
        for (auto &p : qtf) {
            double w = (1 + log(p.second)) * idf(p.first);
            vec[p.first] = w;
            norm += w*w;
        }
        norm = sqrt(norm);
        if (norm == 0) return vec;
        for (auto &x : vec) x.second /= norm;
        return vec;
    }

    // Cosine similarity between doc vector and query vector
    double cosineSimilarity(const unordered_map<string,double> &dvec, const unordered_map<string,double> &qvec) {
        double s = 0.0;
        // iterate over smaller map for efficiency
        if (qvec.size() < dvec.size()) {
            for (auto &q : qvec) {
                auto it = dvec.find(q.first);
                if (it != dvec.end()) s += it->second * q.second;
            }
        } else {
            for (auto &d : dvec) {
                auto it = qvec.find(d.first);
                if (it != qvec.end()) s += d.second * it->second;
            }
        }
        return s;
    }

    // Search interface
    vector<pair<int,double>> search(const string &rawQuery, int topK = 10) {
        string q = toLower(rawQuery);
        vector<pair<int,double>> ranked; // {docId, score}

        // Detect phrase search (if query inside double quotes)
        bool phraseSearch = false;
        string phrase;
        size_t firstQ = rawQuery.find('"');
        size_t lastQ = rawQuery.rfind('"');
        if (firstQ != string::npos && lastQ != string::npos && firstQ != lastQ) {
            phraseSearch = true;
            phrase = rawQuery.substr(firstQ+1, lastQ-firstQ-1);
            phrase = toLower(phrase);
        }

        // tokenize raw query for vector/keyword search
        vector<string> qtokens = tokenize(q);
        // build query vector (TF-IDF) if long enough
        bool longQuery = (rawQuery.size() > 30 || qtokens.size() > 3);

        unordered_map<string,double> qvec;
        if (longQuery) qvec = queryVector(qtokens);

        // Acronym / shortform match
        string qAcr = "";
        // if user wrote uppercase letters no quotes and length <= 6 we assume shortform
        bool maybeAcr = false;
        bool allUpper = true;
        for (char c : rawQuery) {
            if (isalpha((unsigned char)c) && islower((unsigned char)c)) { allUpper = false; break; }
        }
        if (rawQuery.size() <= 6 && allUpper && rawQuery.find(' ') == string::npos) { // SHORT heuristic
            maybeAcr = true;
            qAcr = toLower(rawQuery);
        } else {
            // also build acronym of query tokens (first letters)
            string acr="";
            for (auto &t : qtokens) if (!stopwords.count(t) && !t.empty()) acr.push_back(t[0]);
            if (!acr.empty()) { maybeAcr = true; qAcr = acr; }
        }

        // Prepare candidate doc set: union of docs containing any token OR substring OR acronym OR fuzzy
        unordered_set<int> candidates;
        for (auto &t : qtokens) {
            if (index.count(t)) {
                for (auto &p : index[t]) candidates.insert(p.first);
            }
        }
        // substring / phrase candidates (cheap full text scan)
        for (auto &d : docs) {
            string hay = toLower(d.title + " " + d.content);
            if (hay.find(q) != string::npos) candidates.insert(d.id);
            if (phraseSearch) {
                if (hay.find(phrase) != string::npos) candidates.insert(d.id);
            }
        }
        // acronym candidates
        if (maybeAcr && !qAcr.empty()) {
            for (auto &d : docs) {
                if (d.acronym.size() && toLower(d.acronym).find(qAcr) != string::npos) candidates.insert(d.id);
            }
        }
        // fuzzy candidates: if token doesn't exist in index, look for tokens within edit distance 1
        for (auto &t : qtokens) {
            if (index.count(t)) continue;
            for (auto &entry : docFreq) {
                const string &tok = entry.first;
                if (abs((int)tok.size() - (int)t.size()) > 1) continue;
                if (levenshtein(tok, t) <= 1) {
                    // add docs which have tok
                    for (auto &p : index[tok]) candidates.insert(p.first);
                }
            }
        }

        // If no candidates found but there are docs, fallback to all docs (so we can compute similarity)
        if (candidates.empty()) {
            for (auto &d : docs) candidates.insert(d.id);
        }

        // Score each candidate
        unordered_map<int, double> scoreMap;
        for (int docId : candidates) {
            // find doc record
            const Doc *docPtr = nullptr;
            for (auto &d : docs) if (d.id == docId) { docPtr = &d; break; }
            if (!docPtr) continue;
            const Doc &d = *docPtr;

            double score = 0.0;

            // 1) phrase / substring exact match gives big boost
            string hay = toLower(d.title + " " + d.content);
            if (phraseSearch) {
                if (hay.find(phrase) != string::npos) score += 3.0;
            }
            if (hay.find(q) != string::npos) score += 2.0;

            // 2) acronym match boost
            if (maybeAcr && !qAcr.empty()) {
                if (!d.acronym.empty() && toLower(d.acronym).find(qAcr) != string::npos) score += 2.0;
            }

            // 3) token overlap / tf-idf for keywords or longQuery (cosine similarity)
            if (longQuery) {
                auto dvec = docVector(d);
                double sim = cosineSimilarity(dvec, qvec);
                score += sim * 5.0; // scale up similarity
            } else {
                // simpler token-based scoring for short queries
                double tokenScore = 0.0;
                for (auto &t : qtokens) {
                    if (d.tf.find(t) != d.tf.end()) tokenScore += (1 + log(d.tf.at(t))) * idf(t);
                    else {
                        // reward fuzzy tokens that are close
                        for (auto &p : d.tf) {
                            if (levenshtein(p.first, t) <= 1) { tokenScore += 0.3; break; }
                        }
                    }
                }
                score += tokenScore;
            }

            // 4) small bonus if title contains query tokens (title is more important)
            for (auto &t : qtokens) {
                vector<string> titleTokens = tokenize(toLower(d.title));
                for (auto &tt : titleTokens) if (tt == t) { score += 0.6; break; }
            }

            // 5) small length normalization: shorter doc that matches exactly might be more relevant
            double lenFactor = 1.0;
            int totalTok = d.tokens.size() ? (int)d.tokens.size() : 1;
            lenFactor = 1.0 / sqrt(totalTok / 50.0 + 1.0); // heuristic
            score *= lenFactor;

            scoreMap[docId] = score;
        }

        // Move to vector and sort by score desc
        for (auto &p : scoreMap) {
            ranked.push_back({p.first, p.second});
        }
        sort(ranked.begin(), ranked.end(), [](const pair<int,double> &a, const pair<int,double> &b){
            if (fabs(a.second - b.second) > 1e-9) return a.second > b.second;
            return a.first < b.first;
        });

        // trim zeros and keep topK
        vector<pair<int,double>> res;
        for (auto &r : ranked) {
            if (r.second <= 0 && (int)res.size() >= topK) break;
            if ((int)res.size() < topK) res.push_back(r);
            else break;
        }
        return res;
    }

    const Doc* getDocById(int id) const {
        for (auto &d : docs) if (d.id == id) return &d;
        return nullptr;
    }

    void printDocSummary(const Doc &d) const {
        cout << "ID: " << d.id << " | Title: " << d.title << " | Link: " << d.link << "\n";
        string snippet = d.content.substr(0, 160);
        if (d.content.size() > 160) snippet += "...";
        cout << snippet << "\n";
    }

    void printDocFull(const Doc &d) const {
        cout << "\n----- " << d.title << " -----\n";
        cout << "Link: " << d.link << "\n";
        cout << d.content << "\n------------------------\n";
    }
};

// -------------------- Demo main --------------------
int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    SearchEngine engine;
    // Add sample docs (you can add many more or load from files)
    engine.addDoc(1, "C++ Basics", "Learn C++ programming from scratch. This tutorial covers variables, loops, functions, classes, and more to help you get started quickly. Great for beginners.", "https://example.com/cpp-basics");
    engine.addDoc(2, "Qt Tutorial", "GUI development with Qt framework. Create windows, buttons, input forms, layouts, and handle events in C++ using Qt.", "https://example.com/qt-tutorial");
    engine.addDoc(3, "Advanced Search", "Building search engines in C++ using data structures like vectors, maps, and sets. Learn inverted index, keyword search, ranking, and tf-idf.", "https://example.com/advanced-search");
    engine.addDoc(4, "Data Structures", "Learn arrays, linked list, stack, queue, trees, and graphs in C++. Understand their implementation and use in algorithms.", "https://example.com/ds");
    engine.addDoc(5, "Algorithms", "Sorting, searching, graph traversal, dynamic programming, and more. Master algorithmic problem-solving with C++ examples.", "https://example.com/algo");
    engine.addDoc(6, "DS & Algo (Short: DSA)", "Complete notes and examples for Data Structures and Algorithms (DSA). Perfect for placement and coding interviews.", "https://example.com/dsa");
    engine.addDoc(7, "Binary Tree Top View", "This article explains top view of binary tree and other tree traversals including level-order and inorder, with examples in C++.", "https://example.com/topview");
    engine.buildIndex();

    cout << "Mini Full-Text Search Engine (local)\n";
    cout << "Commands: type a query and press enter. For phrase search, use double quotes: \"top view\".\n";
    cout << "Type ':quit' to exit, ':open <ID>' to open a doc, ':page <n>' to change results per page.\n";

    int pageSize = 3;
    while (true) {
        cout << "\nEnter search (or command): ";
        string line;
        if (!getline(cin, line)) break;
        if (line.empty()) continue;
        if (line == ":quit") break;

        // handle commands
        if (line.rfind(":open", 0) == 0) {
            stringstream ss(line);
            string cmd; int id;
            ss >> cmd >> id;
            const Doc* d = engine.getDocById(id);
            if (d) engine.printDocFull(*d);
            else cout << "Doc not found.\n";
            continue;
        } else if (line.rfind(":page", 0) == 0) {
            stringstream ss(line);
            string cmd; ss >> cmd >> pageSize;
            cout << "Page size set to " << pageSize << "\n";
            continue;
        }

        auto start = chrono::high_resolution_clock::now();
        auto results = engine.search(line, 50); // get top 50 then paginate
        auto stop = chrono::high_resolution_clock::now();
        auto ms = chrono::duration_cast<chrono::milliseconds>(stop - start).count();
        cout << "Search took " << ms << " ms. " << results.size() << " results (showing top " << min((int)results.size(), pageSize) << ").\n";

        if (results.empty()) {
            cout << "No results.\n";
            continue;
        }

        int page = 0;
        int totalPages = (results.size() + pageSize - 1) / pageSize;
        while (true) {
            int startIdx = page * pageSize;
            int endIdx = min(startIdx + pageSize, (int)results.size());
            cout << "\n--- Page " << page+1 << " / " << totalPages << " ---\n";
            for (int i = startIdx; i < endIdx; ++i) {
                int id = results[i].first;
                double score = results[i].second;
                const Doc* d = engine.getDocById(id);
                if (!d) continue;
                cout << "[" << id << "] (score: " << fixed << setprecision(3) << score << ") ";
                cout << d->title << "  - " << d->link << "\n";
                string snippet = d->content.substr(0,140);
                if (d->content.size() > 140) snippet += "...";
                cout << "   " << snippet << "\n";
            }
            cout << "Options: [N]ext | [P]rev | [O]pen <id> | [Q]uit results\n";
            string opt;
            if (!getline(cin, opt)) { opt = "q"; }
            if (opt.empty()) opt = "q";
            if (opt == "N" || opt == "n") {
                if (page + 1 < totalPages) page++; else cout << "Already last page.\n";
            } else if (opt == "P" || opt == "p") {
                if (page > 0) page--; else cout << "Already first page.\n";
            } else if (opt.rfind("O",0) == 0 || opt.rfind("o",0) == 0) {
                stringstream ss(opt);
                string c; int id; ss >> c >> id;
                const Doc* d = engine.getDocById(id);
                if (d) engine.printDocFull(*d);
                else cout << "Invalid id.\n";
            } else {
                break; // quit results loop
            }
        }
    }

    cout << "Goodbye.\n";
    return 0;
}
