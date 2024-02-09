#include<iostream>
#include<random>
#include<set>
#include<chrono>

using namespace std;
using namespace std::chrono;

bool check_byte(int byte_num){
    if (byte_num == 2) return true;
    if (byte_num == 4) return true;
    if (byte_num == 8) return true;
    return false;
}

int main(int argc, char* argv[]){
    srand(1601);

    int byte_num = stoi(argv[1]);
    int bit_num = byte_num * 8;
    int sender_size = stoi(argv[2]);
    int receiver_size = stoi(argv[3]);
    int intersection_size = stoi(argv[4]);

    if (check_byte(byte_num) ^ 1){
        throw "Currently do not support numbers different from 16, 32, and 64 bits.";
    }

    cout << "Number of bytes: " << byte_num << "\n";

    auto start = high_resolution_clock::now();
    auto stop = high_resolution_clock::now();
    auto duration = duration_cast<microseconds>(stop - start);

    start = high_resolution_clock::now();

    set<string> intersection_set;
    while (intersection_set.size() < intersection_size){
        string s;
        for (int i = 0; i < bit_num; i++){
            s.push_back('0' + rand() % 2);
        }
        intersection_set.insert(s);
    }

    stop = high_resolution_clock::now();
    duration = duration_cast<microseconds>(stop - start);
    cout << "Time for preparing intersection: " << duration.count() / 1000000 << "s" << endl;


    start = high_resolution_clock::now();

    set<string> sender_set = intersection_set;
    while (sender_set.size() < sender_size){
        string s;
        for (int i = 0; i < bit_num; i++){
            s.push_back('0' + rand() % 2);
        }
        sender_set.insert(s);
    }

    stop = high_resolution_clock::now();
    duration = duration_cast<microseconds>(stop - start);
    cout << "Time for preparing sender: " << duration.count() / 1000000 << "s" << endl;

    start = high_resolution_clock::now();

    set<string> receiver_set = intersection_set;
    while (sender_set.size() < receiver_size){
        string s;
        for (int i = 0; i < bit_num; i++){
            s.push_back('0' + rand() % 2);
        }
        if (sender_set.find(s) != sender_set.end() && intersection_set.find(s) == intersection_set.end()) continue;
        receiver_set.insert(s);
    }

    stop = high_resolution_clock::now();
    duration = duration_cast<microseconds>(stop - start);
    cout << "Time for preparing receiver: " << duration.count() / 1000000 << "s" << endl;
}