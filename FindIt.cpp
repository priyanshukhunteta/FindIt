// ============================================================
//  FindIt - Smart Campus Lost & Found Management System
//  Single-file C++ OOP project (standard library only)
// ============================================================
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <cctype>
#include <cstdlib>
using namespace std;

// ---------------- Global settings ----------------
const string FILE_NAME = "reports.txt";   // data file
const string VERSION = "1.0";
const bool USE_COLOR = true;              // set to false if your terminal shows strange symbols
const int MIN_MATCH_SCORE = 40;           // lowest score shown as a "possible match"

// ---------------- Colour helpers (optional) ----------------
string paint(const string &text, const string &code) {
    if (!USE_COLOR) return text;
    return "\033[" + code + "m" + text + "\033[0m";
}
string green(const string &t)  { return paint(t, "32"); }
string red(const string &t)    { return paint(t, "31"); }
string yellow(const string &t) { return paint(t, "33"); }
string cyan(const string &t)   { return paint(t, "36"); }
string bold(const string &t)   { return paint(t, "1"); }

// ---------------- Small helper functions ----------------
string toLower(string text) {
    for (size_t i = 0; i < text.size(); i++)
        text[i] = tolower((unsigned char)text[i]);
    return text;
}

// removes spaces from both ends of a string
string trim(const string &text) {
    size_t start = text.find_first_not_of(" \t\r\n");
    if (start == string::npos) return "";
    size_t end = text.find_last_not_of(" \t\r\n");
    return text.substr(start, end - start + 1);
}

// the file uses '|' to separate fields, so we never allow '|' inside text
string cleanText(string text) {
    for (size_t i = 0; i < text.size(); i++)
        if (text[i] == '|') text[i] = '/';
    return text;
}

bool contains(const string &big, const string &small) {
    return big.find(small) != string::npos;
}

bool isNumber(const string &text) {
    if (text.empty() || text.size() > 9) return false;
    for (size_t i = 0; i < text.size(); i++)
        if (!isdigit((unsigned char)text[i])) return false;
    return true;
}

// reads one line from the user (exits safely if input ends)
string readLine(const string &prompt) {
    string line;
    cout << prompt;
    if (!getline(cin, line)) {
        cout << "\nInput closed. Exiting FindIt.\n";
        exit(0);
    }
    return trim(line);
}

// keeps asking until the user types something
string readRequired(const string &prompt) {
    while (true) {
        string text = readLine(prompt);
        if (!text.empty()) return cleanText(text);
        cout << red("[ERROR] This field cannot be empty. Please try again.") << endl;
    }
}

// reads a whole number; returns -1 if the input is not a valid number
int readNumber(const string &prompt) {
    string text = readLine(prompt);
    if (!isNumber(text)) return -1;
    return atoi(text.c_str());
}

// checks the format DD-MM-YYYY and that the day fits the month
bool isValidDate(const string &d) {
    if (d.size() != 10 || d[2] != '-' || d[5] != '-') return false;
    for (int i = 0; i < 10; i++) {
        if (i == 2 || i == 5) continue;
        if (!isdigit((unsigned char)d[i])) return false;
    }
    int day = atoi(d.substr(0, 2).c_str());
    int month = atoi(d.substr(3, 2).c_str());
    int year = atoi(d.substr(6, 4).c_str());
    int daysInMonth[12] = {31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    if (month < 1 || month > 12) return false;
    if (year < 2000 || year > 2100) return false;
    return day >= 1 && day <= daysInMonth[month - 1];
}

void printHeading(const string &title) {
    cout << "\n==============================================\n";
    cout << "  " << bold(title) << "\n";
    cout << "==============================================\n";
}

// ============================================================
//  Class ItemReport : stores ONE lost or found report
// ============================================================
class ItemReport {
private:
    int id;
    string itemName;
    string category;
    string location;
    string date;
    string description;
    string reportType;   // "Lost" or "Found"
    bool resolved;       // false = Active, true = Resolved

public:
    // constructor: sets all details of a report
    ItemReport(int reportId, string name, string cat, string loc, string dt,
               string desc, string type, bool isResolved = false) {
        id = reportId;
        itemName = name;
        category = cat;
        location = loc;
        date = dt;
        description = desc;
        reportType = type;
        resolved = isResolved;
    }

    // getters (public functions to read private data)
    int getId() const { return id; }
    string getItemName() const { return itemName; }
    string getCategory() const { return category; }
    string getLocation() const { return location; }
    string getDate() const { return date; }
    string getDescription() const { return description; }
    string getReportType() const { return reportType; }
    bool isResolved() const { return resolved; }
    bool isLost() const { return reportType == "Lost"; }

    void markResolved() { resolved = true; }

    // shows the report on the screen
    void display() const {
        cout << "----------------------------------------------\n";
        cout << "Report ID   : " << id << "\n";
        cout << "Item Name   : " << itemName << "\n";
        cout << "Category    : " << category << "\n";
        cout << "Location    : " << location << "\n";
        cout << "Date        : " << date << "\n";
        cout << "Description : " << description << "\n";
        cout << "Type        : " << (isLost() ? yellow("LOST") : cyan("FOUND")) << "\n";
        cout << "Status      : " << (resolved ? green("RESOLVED") : yellow("ACTIVE")) << "\n";
    }

    // one line of text for reports.txt
    string toFileLine() const {
        return to_string(id) + "|" + itemName + "|" + category + "|" + location + "|" +
               date + "|" + description + "|" + reportType + "|" +
               (resolved ? "Resolved" : "Active");
    }
};

// ============================================================
//  Class MatchEngine : compares Lost and Found reports
// ============================================================
struct MatchResult {
    int lostIndex;    // position of the lost report in the vector
    int foundIndex;   // position of the found report in the vector
    int score;        // 0 to 100
    string details;   // explanation of the score
};

class MatchEngine {
private:
    // very common words that should not count as a match
    bool isStopWord(const string &w) const {
        string stops[] = {"the", "and", "with", "near", "for", "was", "has", "have",
                          "this", "that", "from", "are", "lost", "found", "item", "its"};
        for (int i = 0; i < 16; i++)
            if (w == stops[i]) return true;
        return false;
    }

    // splits text into unique lowercase keywords (3+ letters)
    vector<string> getKeywords(const string &text) const {
        vector<string> words;
        string lower = toLower(text);
        string current = "";
        for (size_t i = 0; i <= lower.size(); i++) {
            if (i < lower.size() && isalnum((unsigned char)lower[i])) {
                current += lower[i];
            } else {
                if (current.size() >= 3 && !isStopWord(current)) {
                    bool alreadyAdded = false;
                    for (size_t k = 0; k < words.size(); k++)
                        if (words[k] == current) alreadyAdded = true;
                    if (!alreadyAdded) words.push_back(current);
                }
                current = "";
            }
        }
        return words;
    }

    // up to 20 points: 5 points for each shared keyword
    int keywordScore(const ItemReport &lost, const ItemReport &found, string &sharedWords) const {
        vector<string> a = getKeywords(lost.getItemName() + " " + lost.getDescription());
        vector<string> b = getKeywords(found.getItemName() + " " + found.getDescription());
        string lostCat = toLower(lost.getCategory());
        string foundCat = toLower(found.getCategory());
        string lostLoc = toLower(lost.getLocation());
        string foundLoc = toLower(found.getLocation());
        int common = 0;
        sharedWords = "";
        for (size_t i = 0; i < a.size(); i++) {
            // skip words already rewarded by the category or location points
            if (contains(lostCat, a[i]) || contains(foundCat, a[i]) ||
                contains(lostLoc, a[i]) || contains(foundLoc, a[i]))
                continue;
            for (size_t j = 0; j < b.size(); j++) {
                if (a[i] == b[j]) {
                    common++;
                    if (sharedWords != "") sharedWords += ", ";
                    sharedWords += a[i];
                    break;
                }
            }
        }
        int points = common * 5;
        if (points > 20) points = 20;
        return points;
    }

public:
    // score out of 100 using simple rules
    int calculateScore(const ItemReport &lost, const ItemReport &found, string &details) const {
        int score = 0;
        details = "";

        if (toLower(lost.getCategory()) == toLower(found.getCategory())) {
            score += 35;
            details += "Same category (+35). ";
        }

        string loc1 = toLower(lost.getLocation());
        string loc2 = toLower(found.getLocation());
        if (loc1 == loc2 || contains(loc1, loc2) || contains(loc2, loc1)) {
            score += 25;
            details += "Similar location (+25). ";
        }

        if (lost.getDate() == found.getDate()) {
            score += 20;
            details += "Same date (+20). ";
        }

        string shared;
        int points = keywordScore(lost, found, shared);
        if (points > 0) {
            score += points;
            details += "Shared keywords: " + shared + " (+" + to_string(points) + "). ";
        }

        if (details == "") details = "No matching details.";
        return score;
    }

    // compares every ACTIVE Lost report with every ACTIVE Found report
    vector<MatchResult> findMatches(const vector<ItemReport> &reports) const {
        vector<MatchResult> results;
        for (size_t i = 0; i < reports.size(); i++) {
            if (!reports[i].isLost() || reports[i].isResolved()) continue;
            for (size_t j = 0; j < reports.size(); j++) {
                if (reports[j].isLost() || reports[j].isResolved()) continue;
                MatchResult m;
                m.lostIndex = (int)i;
                m.foundIndex = (int)j;
                m.score = calculateScore(reports[i], reports[j], m.details);
                if (m.score >= MIN_MATCH_SCORE) results.push_back(m);
            }
        }
        sortMatches(results);
        return results;
    }

    // bubble sort: highest score first
    void sortMatches(vector<MatchResult> &list) const {
        for (size_t i = 0; i + 1 < list.size(); i++) {
            for (size_t j = 0; j + 1 < list.size() - i; j++) {
                if (list[j].score < list[j + 1].score) {
                    MatchResult temp = list[j];
                    list[j] = list[j + 1];
                    list[j + 1] = temp;
                }
            }
        }
    }

    int countMatches(const vector<ItemReport> &reports) const {
        return (int)findMatches(reports).size();
    }

    void displayMatches(const vector<ItemReport> &reports) const {
        printHeading("SMART MATCHES");
        vector<MatchResult> results = findMatches(reports);
        if (results.empty()) {
            cout << yellow("[INFO] No possible matches found right now.") << endl;
        } else {
            for (size_t i = 0; i < results.size(); i++) {
                const ItemReport &lost = reports[results[i].lostIndex];
                const ItemReport &found = reports[results[i].foundIndex];
                cout << "----------------------------------------------\n";
                cout << "Match #" << (i + 1) << "\n";
                cout << "Lost Report  : ID " << lost.getId() << " - " << lost.getItemName() << "\n";
                cout << "Found Report : ID " << found.getId() << " - " << found.getItemName() << "\n";
                cout << "Match Score  : " << results[i].score << " / 100\n";
                cout << "Details      : " << results[i].details << "\n";
                cout << "Status       : " << yellow("Pending verification") << "\n";
            }
            cout << "----------------------------------------------\n";
            cout << "Possible matches shown: " << results.size() << "\n";
        }
        cout << "\n" << yellow("NOTE: ") << "The score is only a rule-based similarity value. "
             << "It is NOT the probability that the items are the same.\n"
             << "Always verify the owner before resolving a match.\n";
    }
};

// ============================================================
//  Class FindItSystem : stores reports, files, search, resolve
// ============================================================
class FindItSystem {
private:
    vector<ItemReport> reports;
    int nextId;

    // returns the position of a report ID in the vector, or -1
    int findIndexById(int id) const {
        for (size_t i = 0; i < reports.size(); i++)
            if (reports[i].getId() == id) return (int)i;
        return -1;
    }

    string chooseCategory() const {
        string names[5] = {"Wallet", "ID Card", "Electronics", "Books", "Accessories"};
        while (true) {
            cout << "Category:\n";
            for (int i = 0; i < 5; i++) cout << "  " << (i + 1) << ". " << names[i] << "\n";
            cout << "  6. Other (type your own)\n";
            int choice = readNumber("Choose category (1-6): ");
            if (choice >= 1 && choice <= 5) return names[choice - 1];
            if (choice == 6) return readRequired("Enter category: ");
            cout << red("[ERROR] Please enter a number from 1 to 6.") << endl;
        }
    }

public:
    FindItSystem() { nextId = 1; }

    const vector<ItemReport> &getReports() const { return reports; }

    // ---------- file handling ----------
    void loadFromFile() {
        ifstream file(FILE_NAME.c_str());
        if (!file) {
            cout << yellow("[INFO] No saved data found. Starting with an empty list.") << endl;
            return;
        }
        string line;
        int skipped = 0;
        while (getline(file, line)) {
            line = trim(line);
            if (line.empty()) continue;

            // split the line at every '|'
            vector<string> parts;
            string part;
            stringstream ss(line);
            while (getline(ss, part, '|')) parts.push_back(part);

            // check that the line is valid
            bool ok = (parts.size() == 8) && isNumber(parts[0]) &&
                      !parts[1].empty() && !parts[2].empty() && !parts[3].empty() &&
                      (parts[6] == "Lost" || parts[6] == "Found") &&
                      (parts[7] == "Active" || parts[7] == "Resolved");
            if (ok) {
                int id = atoi(parts[0].c_str());
                if (id < 1 || findIndexById(id) != -1) ok = false;   // bad or duplicate ID
            }
            if (!ok) {
                skipped++;
                continue;
            }

            int id = atoi(parts[0].c_str());
            reports.push_back(ItemReport(id, parts[1], parts[2], parts[3], parts[4],
                                         parts[5], parts[6], parts[7] == "Resolved"));
            if (id >= nextId) nextId = id + 1;
        }
        file.close();
        cout << green("[OK] Loaded " + to_string(reports.size()) + " report(s) from " + FILE_NAME) << endl;
        if (skipped > 0)
            cout << yellow("[WARNING] Skipped " + to_string(skipped) + " damaged line(s) in the file.") << endl;
    }

    void saveToFile() const {
        ofstream file(FILE_NAME.c_str());
        if (!file) {
            cout << red("[ERROR] Could not save data to " + FILE_NAME) << endl;
            return;
        }
        for (size_t i = 0; i < reports.size(); i++)
            file << reports[i].toFileLine() << "\n";
        file.close();
    }

    // ---------- Features 1 & 2: add a report ----------
    void addReport(const string &type) {
        printHeading("REPORT A " + string(type == "Lost" ? "LOST" : "FOUND") + " ITEM");
        string name = readRequired("Item name: ");
        string category = chooseCategory();
        string location = readRequired("Location: ");
        string date;
        while (true) {
            date = readLine("Date (DD-MM-YYYY): ");
            if (isValidDate(date)) break;
            cout << red("[ERROR] Invalid date. Use the format DD-MM-YYYY, for example 05-10-2026.") << endl;
        }
        string description = readRequired("Short description (colour, brand, marks): ");

        reports.push_back(ItemReport(nextId, name, category, location, date, description, type));
        cout << green("[SUCCESS] Report saved. Your Report ID is " + to_string(nextId)) << endl;
        nextId++;
        saveToFile();
    }

    // ---------- Feature 3: view all ----------
    void viewAll() const {
        printHeading("ALL REPORTS");
        if (reports.empty()) {
            cout << yellow("[INFO] No reports have been added yet.") << endl;
            return;
        }
        for (size_t i = 0; i < reports.size(); i++) reports[i].display();
        cout << "----------------------------------------------\n";
        cout << "Total reports: " << reports.size() << "\n";
    }

    // ---------- Feature 4: search ----------
    void searchReports() const {
        printHeading("SEARCH REPORTS");
        if (reports.empty()) {
            cout << yellow("[INFO] No reports to search.") << endl;
            return;
        }
        cout << "Search by:\n  1. Report ID\n  2. Item name\n  3. Category\n  4. Location\n";
        int choice = readNumber("Choose (1-4): ");
        if (choice < 1 || choice > 4) {
            cout << red("[ERROR] Invalid search option.") << endl;
            return;
        }

        int idToFind = -1;
        string word = "";
        if (choice == 1) {
            idToFind = readNumber("Enter Report ID: ");
            if (idToFind < 1) {
                cout << red("[ERROR] Please enter a valid positive number.") << endl;
                return;
            }
        } else {
            word = toLower(readRequired("Enter search text: "));
        }

        int count = 0;
        for (size_t i = 0; i < reports.size(); i++) {
            bool match = false;
            if (choice == 1) match = (reports[i].getId() == idToFind);
            else if (choice == 2) match = contains(toLower(reports[i].getItemName()), word);
            else if (choice == 3) match = contains(toLower(reports[i].getCategory()), word);
            else match = contains(toLower(reports[i].getLocation()), word);

            if (match) {
                reports[i].display();
                count++;
            }
        }
        if (count == 0) cout << yellow("[INFO] No matching reports found.") << endl;
        else cout << "----------------------------------------------\nRecords found: " << count << "\n";
    }

    // ---------- Feature 6: resolve ----------
    void resolveMatch() {
        printHeading("RESOLVE A VERIFIED MATCH");
        cout << yellow("NOTE: ") << "This is a basic educational simulation, not a secure admin login.\n"
             << "Resolve only after checking proof of ownership.\n\n";
        if (reports.size() < 2) {
            cout << yellow("[INFO] At least one Lost and one Found report are needed.") << endl;
            return;
        }

        int lostId = readNumber("Enter the LOST report ID: ");
        int foundId = readNumber("Enter the FOUND report ID: ");
        if (lostId < 1 || foundId < 1) {
            cout << red("[ERROR] Report IDs must be positive numbers.") << endl;
            return;
        }

        int a = findIndexById(lostId);
        int b = findIndexById(foundId);
        if (a == -1 || b == -1) {
            cout << red("[ERROR] One or both report IDs do not exist.") << endl;
            return;
        }
        if (!reports[a].isLost() || reports[b].isLost()) {
            cout << red("[ERROR] The first ID must be a Lost report and the second a Found report.") << endl;
            return;
        }
        if (reports[a].isResolved() || reports[b].isResolved()) {
            cout << red("[ERROR] One or both reports are already resolved.") << endl;
            return;
        }

        reports[a].display();
        reports[b].display();
        string answer = toLower(readLine("\nHas the owner been verified? Mark both as resolved? (y/n): "));
        if (answer == "y" || answer == "yes") {
            reports[a].markResolved();
            reports[b].markResolved();
            saveToFile();
            cout << green("[SUCCESS] Both reports are now marked as RESOLVED.") << endl;
        } else {
            cout << yellow("[CANCELLED] No changes were made.") << endl;
        }
    }

    // ---------- Feature 7: dashboard ----------
    void showDashboard(const MatchEngine &engine) const {
        printHeading("DASHBOARD AND STATISTICS");
        int lost = 0, found = 0, active = 0, resolved = 0;
        for (size_t i = 0; i < reports.size(); i++) {
            if (reports[i].isLost()) lost++;
            else found++;
            if (reports[i].isResolved()) resolved++;
            else active++;
        }
        cout << "Total reports        : " << reports.size() << "\n";
        cout << "Total Lost reports   : " << lost << "\n";
        cout << "Total Found reports  : " << found << "\n";
        cout << "Active reports       : " << active << "\n";
        cout << "Resolved reports     : " << resolved << "\n";
        cout << "Possible match pairs : " << engine.countMatches(reports)
             << "  (score " << MIN_MATCH_SCORE << " or more, active reports only)\n";
    }
};

// ============================================================
//  Menu and main()
// ============================================================
void showMenu() {
    cout << "\n==============================================\n";
    cout << "   " << bold("FindIt - Smart Campus Lost & Found") << "  v" << VERSION << "\n";
    cout << "==============================================\n";
    cout << "  1. Report a Lost Item\n";
    cout << "  2. Report a Found Item\n";
    cout << "  3. View All Reports\n";
    cout << "  4. Search Reports\n";
    cout << "  5. Find Smart Matches\n";
    cout << "  6. Resolve a Verified Match\n";
    cout << "  7. Dashboard and Statistics\n";
    cout << "  0. Exit\n";
    cout << "----------------------------------------------\n";
}

int main() {
    FindItSystem system;
    MatchEngine engine;

    system.loadFromFile();

    int choice = -1;
    while (choice != 0) {
        showMenu();
        choice = readNumber("Enter your choice: ");

        if (choice == 1) system.addReport("Lost");
        else if (choice == 2) system.addReport("Found");
        else if (choice == 3) system.viewAll();
        else if (choice == 4) system.searchReports();
        else if (choice == 5) engine.displayMatches(system.getReports());
        else if (choice == 6) system.resolveMatch();
        else if (choice == 7) system.showDashboard(engine);
        else if (choice == 0) cout << green("\nThank you for using FindIt. Goodbye!") << endl;
        else cout << red("[ERROR] Invalid choice. Please enter a number from 0 to 7.") << endl;

        if (choice != 0) readLine("\nPress Enter to continue...");
    }
    return 0;
}
