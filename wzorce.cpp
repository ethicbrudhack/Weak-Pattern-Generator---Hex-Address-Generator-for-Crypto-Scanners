#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <chrono>
#include <iomanip>
#include <thread>
#include <mutex>
#include <atomic>
#include <functional>
#include <cmath>
#include <map>
#include <random>
#include <algorithm>

using namespace std;
using namespace chrono;

// Klasa do wyświetlania paska postępu
class ProgressBar {
private:
    int width;
    atomic<bool> running;
    thread update_thread;
    mutex mtx;
    long long total;
    atomic<long long> current;
    steady_clock::time_point start_time;
    string description;
    
public:
    ProgressBar(long long total_bytes, string desc = "Generowanie") 
        : width(50), running(true), total(total_bytes), current(0), description(desc) {
        start_time = steady_clock::now();
        update_thread = thread(&ProgressBar::update, this);
    }
    
    ~ProgressBar() {
        running = false;
        if (update_thread.joinable()) {
            update_thread.join();
        }
    }
    
    void update_progress(long long bytes) {
        current += bytes;
    }
    
    void update() {
        while (running) {
            this_thread::sleep_for(milliseconds(100));
            
            long long curr = current.load();
            if (curr == 0) continue;
            
            double progress = (double)curr / total;
            int pos = (int)(width * progress);
            
            auto now = steady_clock::now();
            double elapsed = duration_cast<duration<double>>(now - start_time).count();
            double speed = curr / elapsed / (1024 * 1024);
            double eta = 0;
            if (curr > 0 && elapsed > 0) {
                eta = (total - curr) / (curr / elapsed);
            }
            
            string curr_str = format_size(curr);
            string total_str = format_size(total);
            
            cout << "\r\033[K";
            cout << description << ": [";
            for (int i = 0; i < width; ++i) {
                if (i < pos) cout << "=";
                else if (i == pos) cout << ">";
                else cout << " ";
            }
            cout << "] " << fixed << setprecision(1) << (progress * 100) << "% ";
            cout << curr_str << "/" << total_str;
            cout << " @ " << fixed << setprecision(2) << speed << " MB/s";
            if (eta > 0) {
                cout << " ETA: " << format_time(eta);
            }
            cout << flush;
        }
        cout << endl;
    }
    
    string format_size(long long bytes) {
        if (bytes < 1024) return to_string(bytes) + " B";
        if (bytes < 1024 * 1024) return to_string(bytes / 1024) + " KB";
        if (bytes < 1024 * 1024 * 1024) {
            return to_string(bytes / (1024 * 1024)) + " MB";
        }
        return to_string(bytes / (1024 * 1024 * 1024)) + " GB";
    }
    
    string format_time(double seconds) {
        if (seconds < 60) return to_string((int)seconds) + "s";
        if (seconds < 3600) {
            int mins = (int)(seconds / 60);
            int secs = (int)(seconds) % 60;
            return to_string(mins) + "m " + to_string(secs) + "s";
        }
        int hours = (int)(seconds / 3600);
        int mins = ((int)(seconds) % 3600) / 60;
        return to_string(hours) + "h " + to_string(mins) + "m";
    }
};

// Główna klasa generatora
class HexPatternGenerator {
private:
    int base;
    int step;
    int length;
    string prefix;
    string suffix;
    function<int(int)> transform;
    bool reverse_pattern;
    int buffer_size;
    int line_length;
    int pattern_index;
    vector<string> pattern_cycle_strings;
    
    vector<int> generate_values(int b, int s) {
        vector<int> values;
        int i = b;
        while (i <= 15) {
            values.push_back(i);
            i += s;
        }
        if (values.size() > 1) {
            i = values.back() - s;
            while (i >= b) {
                values.push_back(i);
                i -= s;
            }
        }
        return values;
    }
    
    string val_to_hex(int val, string pref, string suff, function<int(int)> trans) {
        if (trans) {
            val = trans(val);
        }
        string result;
        if (!pref.empty() && !suff.empty()) {
            result = pref + to_hex_char(val) + suff;
        } else if (!pref.empty()) {
            result = pref + to_hex_char(val);
        } else if (!suff.empty()) {
            result = to_hex_char(val) + suff;
        } else {
            result = to_hex_byte(val);
        }
        return result;
    }
    
    string to_hex_char(int val) {
        val = val & 0xF;
        if (val < 10) return string(1, '0' + val);
        return string(1, 'a' + val - 10);
    }
    
    string to_hex_byte(int val) {
        return to_hex_char(val >> 4) + to_hex_char(val & 0xF);
    }
    
    string generate_pattern_cycle(int b, int s, string pref, string suff, 
                                  function<int(int)> trans) {
        vector<int> values = generate_values(b, s);
        string result;
        for (int val : values) {
            result += val_to_hex(val, pref, suff, trans);
        }
        return result;
    }
    
    void init_patterns(vector<map<string, string>> pattern_configs) {
        pattern_cycle_strings.clear();
        
        for (auto& config : pattern_configs) {
            int b = stoi(config.count("base") ? config.at("base") : "0");
            int s = stoi(config.count("step") ? config.at("step") : "1");
            string pref = config.count("prefix") ? config.at("prefix") : "";
            string suff = config.count("suffix") ? config.at("suffix") : "";
            
            string cycle = generate_pattern_cycle(b, s, pref, suff, nullptr);
            pattern_cycle_strings.push_back(cycle);
        }
        
        if (pattern_cycle_strings.empty()) {
            pattern_cycle_strings.push_back(
                generate_pattern_cycle(0, 1, "", "", nullptr)
            );
        }
        
        pattern_index = 0;
    }
    
    string generate_line_with_pattern(int pattern_idx) {
        string pattern = pattern_cycle_strings[pattern_idx % pattern_cycle_strings.size()];
        string line = "";
        while (line.length() < line_length) {
            line += pattern;
        }
        return line.substr(0, line_length);
    }
    
public:
    HexPatternGenerator() 
        : base(0), step(1), length(32), prefix(""), suffix(""), 
          transform(nullptr), reverse_pattern(false), 
          buffer_size(1024 * 1024), line_length(64), pattern_index(0) {}
    
    void set_line_length(int len) {
        line_length = len;
    }
    
    bool generate_txt_file(const string& filename, long long size_mb = 10,
                          vector<map<string, string>> pattern_configs = {},
                          bool random_patterns = true) {
        
        if (pattern_configs.empty()) {
            pattern_configs = {
                {{"base", "0"}, {"step", "1"}},
                {{"base", "0"}, {"step", "2"}},
                {{"base", "0"}, {"step", "1"}, {"prefix", "f"}},
                {{"base", "0"}, {"step", "1"}, {"suffix", "a"}},
                {{"base", "0"}, {"step", "1"}, {"prefix", "a"}},
                {{"base", "0"}, {"step", "3"}},
                {{"base", "0"}, {"step", "1"}, {"prefix", "ff"}, {"suffix", "00"}},
                {{"base", "0"}, {"step", "1"}, {"suffix", "f"}},
                {{"base", "1"}, {"step", "2"}},
                {{"base", "3"}, {"step", "1"}},
                {{"base", "0"}, {"step", "1"}, {"prefix", "1"}},
                {{"base", "2"}, {"step", "2"}, {"prefix", "aa"}, {"suffix", "bb"}}
            };
        }
        
        init_patterns(pattern_configs);
        
        long long target_size = size_mb * 1024 * 1024;
        
        cout << "Rozmiar docelowy: " << size_mb << " MB (" << target_size << " znakow)" << endl;
        cout << "Dlugosc linii: " << line_length << " znakow" << endl;
        cout << "Liczba roznych wzorcow: " << pattern_cycle_strings.size() << endl;
        cout << "Tryb: " << (random_patterns ? "LOSOWY" : "SEKWENCYJNY") << endl;
        
        for (size_t i = 0; i < min((size_t)5, pattern_cycle_strings.size()); ++i) {
            cout << "  Wzorzec " << (i+1) << ": " 
                 << pattern_cycle_strings[i].substr(0, 30) << "..." << endl;
        }
        cout << endl;
        
        ofstream file(filename);
        if (!file.is_open()) {
            cerr << "Blad: Nie mozna otworzyc pliku " << filename << endl;
            return false;
        }
        
        string buffer;
        buffer.reserve(buffer_size);
        
        long long written = 0;
        auto start_time = chrono::steady_clock::now();
        long long lines_written = 0;
        
        ProgressBar progress(target_size, "Generowanie " + to_string(size_mb) + " MB");
        
        random_device rd;
        mt19937 gen(rd());
        uniform_int_distribution<> dis(0, pattern_cycle_strings.size() - 1);
        
        int pattern_idx = 0;
        
        while (written < target_size) {
            if (random_patterns) {
                pattern_idx = dis(gen);
            } else {
                pattern_idx = (pattern_idx + 1) % pattern_cycle_strings.size();
            }
            
            string line = generate_line_with_pattern(pattern_idx) + "\n";
            size_t line_size = line.length();
            
            size_t to_write = min(line_size, (size_t)(target_size - written));
            buffer += line.substr(0, to_write);
            
            written += to_write;
            lines_written++;
            progress.update_progress(to_write);
            
            if (buffer.length() >= (size_t)buffer_size) {
                file << buffer;
                buffer.clear();
            }
            
            if (lines_written % 10000 == 0) {
                auto now = chrono::steady_clock::now();
                auto elapsed = chrono::duration_cast<chrono::seconds>(now - start_time).count();
                if (elapsed > 0) {
                    double speed = (double)written / elapsed / (1024 * 1024);
                    cout << "\n📝 Linii: " << lines_written 
                         << " | " << written / (1024*1024) << " MB / " << size_mb << " MB"
                         << " @ " << fixed << setprecision(2) << speed << " MB/s" << flush;
                }
            }
        }
        
        if (!buffer.empty()) {
            file << buffer;
        }
        
        file.close();
        
        auto end_time = chrono::steady_clock::now();
        auto elapsed = chrono::duration_cast<chrono::seconds>(end_time - start_time).count();
        double speed = (double)written / elapsed / (1024 * 1024);
        
        cout << "\n\n" << string(60, '=') << endl;
        cout << "✅ GENEROWANIE ZAKONCZONE!" << endl;
        cout << string(60, '=') << endl;
        cout << "📁 Plik: " << filename << endl;
        cout << "📊 Rozmiar: " << written / (1024*1024) << " MB (" << written << " znakow)" << endl;
        cout << "📝 Liczba linii: " << lines_written << endl;
        cout << "📏 Dlugosc linii: " << line_length << " znakow" << endl;
        cout << "🔄 Liczba roznych wzorcow: " << pattern_cycle_strings.size() << endl;
        cout << "⏱️  Czas: " << elapsed / 3600 << "h " << (elapsed % 3600) / 60 << "m " << elapsed % 60 << "s" << endl;
        cout << "⚡ Srednia predkosc: " << fixed << setprecision(2) << speed << " MB/s" << endl;
        cout << string(60, '=') << endl;
        
        return true;
    }
};

// Funkcja do tworzenia wzorców
vector<map<string, string>> create_patterns() {
    vector<map<string, string>> patterns;
    
    // ===== PODSTAWOWE SEKWENCJE =====
    patterns.push_back({{"base", "0"}, {"step", "1"}});
    patterns.push_back({{"base", "0"}, {"step", "2"}});
    patterns.push_back({{"base", "0"}, {"step", "3"}});
    patterns.push_back({{"base", "0"}, {"step", "4"}});
    patterns.push_back({{"base", "0"}, {"step", "5"}});
    
    // ===== RÓŻNE BAZY =====
    for (int b = 1; b <= 5; b++) {
        patterns.push_back({{"base", to_string(b)}, {"step", "1"}});
    }
    
    // ===== Z PREFIXEM =====
    vector<string> prefixes = {"f", "a", "1", "b", "c", "d", "e"};
    for (const auto& p : prefixes) {
        patterns.push_back({{"base", "0"}, {"step", "1"}, {"prefix", p}});
    }
    
    // ===== Z SUFIXEM =====
    vector<string> suffixes = {"f", "a", "1", "b", "c"};
    for (const auto& s : suffixes) {
        patterns.push_back({{"base", "0"}, {"step", "1"}, {"suffix", s}});
    }
    
    // ===== Z PREFIXEM I SUFIXEM =====
    patterns.push_back({{"base", "0"}, {"step", "1"}, {"prefix", "ff"}, {"suffix", "00"}});
    patterns.push_back({{"base", "0"}, {"step", "1"}, {"prefix", "aa"}, {"suffix", "bb"}});
    patterns.push_back({{"base", "0"}, {"step", "1"}, {"prefix", "11"}, {"suffix", "22"}});
    patterns.push_back({{"base", "0"}, {"step", "1"}, {"prefix", "f"}, {"suffix", "a"}});
    patterns.push_back({{"base", "0"}, {"step", "1"}, {"prefix", "a"}, {"suffix", "f"}});
    
    // ===== KOMBINACJE Z KROKIEM =====
    patterns.push_back({{"base", "0"}, {"step", "2"}, {"prefix", "f"}});
    patterns.push_back({{"base", "0"}, {"step", "2"}, {"suffix", "a"}});
    patterns.push_back({{"base", "0"}, {"step", "3"}, {"prefix", "a"}});
    patterns.push_back({{"base", "1"}, {"step", "2"}, {"prefix", "f"}});
    patterns.push_back({{"base", "2"}, {"step", "3"}, {"suffix", "b"}});
    
    // ===== SPECJALNE =====
    patterns.push_back({{"base", "2"}, {"step", "2"}, {"prefix", "aa"}, {"suffix", "bb"}});
    patterns.push_back({{"base", "3"}, {"step", "2"}, {"prefix", "f"}});
    patterns.push_back({{"base", "5"}, {"step", "2"}, {"suffix", "a"}});
    
    // ===== ODWROTNE KROKI =====
    patterns.push_back({{"base", "15"}, {"step", "1"}});
    patterns.push_back({{"base", "15"}, {"step", "2"}});
    patterns.push_back({{"base", "14"}, {"step", "2"}});
    
    // ===== DŁUGIE PREFIXY/SUFIXY =====
    patterns.push_back({{"base", "0"}, {"step", "1"}, {"prefix", "ffff"}});
    patterns.push_back({{"base", "0"}, {"step", "1"}, {"suffix", "ffff"}});
    patterns.push_back({{"base", "0"}, {"step", "1"}, {"prefix", "0000"}});
    patterns.push_back({{"base", "0"}, {"step", "1"}, {"suffix", "0000"}});
    patterns.push_back({{"base", "0"}, {"step", "1"}, {"prefix", "1234"}, {"suffix", "5678"}});
    
    // ===== SPECJALNE WZORCE =====
    patterns.push_back({{"base", "0"}, {"step", "1"}, {"prefix", "dead"}});
    patterns.push_back({{"base", "0"}, {"step", "1"}, {"suffix", "beef"}});
    patterns.push_back({{"base", "0"}, {"step", "1"}, {"prefix", "cafe"}, {"suffix", "babe"}});
    
    return patterns;
}

int main() {
    cout << string(70, '=') << endl;
    cout << "GENERATOR PLIKOW TXT - WSZYSTKIE WZORCE" << endl;
    cout << string(70, '=') << endl;
    cout << endl;
    
    cout << "Dostepne opcje:" << endl;
    cout << "  1. Generuj plik 100 MB TXT (rozne wzorce)" << endl;
    cout << "  2. Generuj plik 1 GB TXT (rozne wzorce)" << endl;
    cout << "  3. Generuj plik 10 GB TXT (rozne wzorce)" << endl;
    cout << "  4. Generuj plik 100 MB TXT (sekwencyjnie)" << endl;
    cout << "  5. Generuj plik 1 GB TXT (sekwencyjnie)" << endl;
    cout << "  6. Wyjdz" << endl;
    
    cout << "\nWybierz opcje (1-6): ";
    int choice;
    cin >> choice;
    
    HexPatternGenerator gen;
    gen.set_line_length(64);
    
    vector<map<string, string>> patterns = create_patterns();
    
    cout << "\n📋 Liczba wzorcow: " << patterns.size() << endl;
    
    switch (choice) {
        case 1: {
            cout << "\n🚀 Generowanie pliku 100 MB TXT (losowo)..." << endl;
            gen.generate_txt_file("wzorce_100mb.txt", 100, patterns, true);
            break;
        }
        case 2: {
            cout << "\n🚀 Generowanie pliku 1 GB TXT (losowo)..." << endl;
            cout << "⚠️  Uwaga: To moze zajac kilka minut!" << endl;
            gen.generate_txt_file("wzorce_1gb.txt", 1024, patterns, true);
            break;
        }
        case 3: {
            cout << "\n🚀 Generowanie pliku 10 GB TXT (losowo)..." << endl;
            cout << "⚠️  Uwaga: To moze zajac kilkadziesiat minut!" << endl;
            gen.generate_txt_file("wzorce_10gb.txt", 10240, patterns, true);
            break;
        }
        case 4: {
            cout << "\n🚀 Generowanie pliku 100 MB TXT (sekwencyjnie)..." << endl;
            gen.generate_txt_file("wzorce_100mb_seq.txt", 100, patterns, false);
            break;
        }
        case 5: {
            cout << "\n🚀 Generowanie pliku 1 GB TXT (sekwencyjnie)..." << endl;
            cout << "⚠️  Uwaga: To moze zajac kilka minut!" << endl;
            gen.generate_txt_file("wzorce_1gb_seq.txt", 1024, patterns, false);
            break;
        }
        case 6:
            cout << "Do widzenia!" << endl;
            break;
        default:
            cout << "Nieprawidlowa opcja!" << endl;
    }
    
    return 0;
}