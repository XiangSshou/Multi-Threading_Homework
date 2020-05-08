// this application is an elevator simulation
#include <Windows.h>
#include <iostream>
#include <fstream>
#include <thread>
#include <chrono>
#include <mutex>
#include <condition_variable>
#include <ctime>
#include <vector> 
#include <queue>
#include <conio.h>
#include <stdio.h>
#include <comdef.h>

using namespace std;
using namespace std::chrono_literals;

/*** only these parameters should be changed ***/
#define capacity 15 // most passengers a cart can hold
#define generatePassenger 1500ms // time between generating passengers 
#define tOpenDoor 20ms // time for door to open and close
#define tMovePassenger 20ms // time for a passenger getting on or off the cart
#define tMoveCart 50ms // time for cart to move one floor
#define tMaxWait 10000ms // max time for cart to wait at the first floor
#define maxCome 8 // max people come to the building when generating passengers
#define maxMove 16 // max people want to move when generating passengers
#define tRunning 60s // the simulation running time
#define stayAtFirst true // whether the cart should go back to first floor while waiting
#define rFirst 50 // the rate of going to first floor
/***********************************************/

#define maxFloor 29 // Only for Print
#define minFloor 0  // Only for Print
#define firstFloor 0
#define screenW 90
#define screenH 40
#define nElevator 3 // this number should not be changed for now
const int totalFloor = maxFloor - minFloor + 1;
const SHORT cart_X[nElevator] = { 1,21,41 };
const SHORT analysis_X = 61;

wchar_t* screen = new wchar_t[screenH * screenW];
mutex lockForScreen;
mutex lockForFloor[totalFloor];
mutex lockForElevator[nElevator];
mutex lockForData;
mutex lockForStop;
condition_variable cvCart;
HANDLE console;

int totalServed = 0;
chrono::milliseconds totalWait;
int avgWait;
chrono::milliseconds maxWait = 0ms;
chrono::system_clock::time_point startTime;
bool Stop;

class Passenger {
private:
    int destFloor;
public:
    chrono::system_clock::time_point startWaitingTime;
    Passenger(int curFloor) {
        if (curFloor == firstFloor) {
            while ((destFloor = rand() % totalFloor) == curFloor);
        }
        else{
            if ((rand() % 100) < rFirst)
                destFloor = firstFloor;
            else 
                while ((destFloor = rand() % totalFloor) == curFloor);
        }
        startWaitingTime = chrono::system_clock::now();
    }
    ~Passenger() {}
    int getDest() {
        return destFloor;
    }
};


class Floor {
private:
    queue<Passenger*> UpPassengers;
    queue<Passenger*> DownPassengers;
    int floorNum;
    int printNum;
    int working;
public:
    bool pushedUp() {
        unique_lock<mutex> lock(lockForFloor[floorNum]);
        return !UpPassengers.empty();
    }
    bool pushedDown() {
        unique_lock<mutex> lock(lockForFloor[floorNum]);
        return !DownPassengers.empty();
    }
    Floor(): floorNum(0), printNum(0), working(0){}
    Floor(int num): floorNum(num), printNum(num + minFloor), working(0){
        if (printNum >= 0)printNum++;
    }
    ~Floor() {};
    int getUpNum() {
        unique_lock<mutex> lock(lockForFloor[floorNum]);
        return UpPassengers.size();
    }
    int getDownNum() {
        unique_lock<mutex> lock(lockForFloor[floorNum]);
        return DownPassengers.size();
    }
    int getWaitingNum() {
        unique_lock<mutex> lock(lockForFloor[floorNum]);
        return UpPassengers.size() + DownPassengers.size();
    }
    int getWorking() {
        unique_lock<mutex> lock(lockForFloor[floorNum]);
        return working;
    }
    void work(bool b) {
        unique_lock<mutex> lock(lockForFloor[floorNum]);
        working += b?1:-1;
    }
    Passenger* getUpPassenger() {
        unique_lock<mutex> lock(lockForFloor[floorNum]);
        if (UpPassengers.empty())return nullptr;
        Passenger* res = UpPassengers.front();
        UpPassengers.pop();
        return res;
    }
    Passenger* getDownPassenger() {
        unique_lock<mutex> lock(lockForFloor[floorNum]);
        if (DownPassengers.empty()) return nullptr;
        Passenger* res = DownPassengers.front();
        DownPassengers.pop();
        return res;
    }
    int getNum(){
        return floorNum;
    }
    int getPrintNum(){
        return printNum;
    }
    void addPassenger(Passenger* p) {
        if (p->getDest() > floorNum) {
            unique_lock<mutex> lock(lockForFloor[floorNum]);
            UpPassengers.push(p);
        }
        else if (p->getDest() < floorNum) {
            unique_lock<mutex> lock(lockForFloor[floorNum]);
            DownPassengers.push(p);
        }
        else {
            cout << "Passenger should not go to his current floor!";
            exit(1);
        }
    }
};

Floor floors[totalFloor];

class Cart {
private:
    int id;
    Passenger* passengers[capacity];
    bool pushedButton[totalFloor];
    int served;
public:
    int floor;
    int numOfPassengers;
    Cart() :id(-1), floor(firstFloor), served(0), passengers{nullptr}, pushedButton{ false }, numOfPassengers(0) {}
    Cart(int i) :id(i), floor(firstFloor), served(0), passengers{nullptr}, pushedButton{ false }, numOfPassengers(0) {}
    ~Cart() {}
    void run() {
        while (1) {
            if (floors[floor].getWorking()) {
                unique_lock<mutex> lock(lockForElevator[id]);
                cvCart.wait_for(lock, tMaxWait);
            }
            go(totalFloor - 1);
            go(0);
            if(stayAtFirst)go(firstFloor, true);
            {
                unique_lock<mutex> lock(lockForStop);
                if (Stop) break;
            }
        }
    }
    bool pushed(int n) {
        return pushedButton[n];
    }
    int addPassenger(Passenger* passenger) {
        this_thread::sleep_for(tMovePassenger);
        int i;
        for (i = 0; i < capacity; i++) {
            if (passengers[i] == nullptr) {
                passengers[i] = passenger;
                pushedButton[passenger->getDest()] = true;
                break;
            }
        }
        if (i == capacity) {
            cout << "No empty space. Please check \'numOfPassengers\' first." << endl;
            return 1;
        }
        numOfPassengers++;
        updateScreen();
        return 0;
    }
    void removePassenger() {
        pushedButton[floor] = false;
        for (int i = 0; i < capacity; i++) {
            if (passengers[i]!=nullptr && passengers[i]->getDest() == floor) {
                this_thread::sleep_for(tMovePassenger);
                {
                    unique_lock<mutex> lock(lockForData);
                    totalServed++;
                    chrono::milliseconds waitTime = chrono::duration_cast<chrono::milliseconds>(chrono::system_clock::now() - passengers[i]->startWaitingTime);
                    totalWait += waitTime;
                    avgWait = totalWait.count() / totalServed;
                    if (waitTime > maxWait)maxWait = waitTime;
                }
                delete passengers[i];
                passengers[i] = nullptr;
                numOfPassengers--;
                served++;
                updateScreen();
            }
        }
    }
    void go(int dest = firstFloor, bool toFirst = false) {
        bool up;
        if (dest > floor)up = true;
        else if (dest < floor)up = false;
        else return;
        while (true) {
            updateScreen();
            floors[floor].work(true); // start working on this cart;
            // remove passengers from cart
            if (pushedButton[floor]) {
                this_thread::sleep_for(tOpenDoor);
                removePassenger();
            }
            // add passengers to cart
            if ((up && floors[floor].pushedUp()) || (!up && floors[floor].pushedDown()) && numOfPassengers < capacity) {
                while (numOfPassengers < capacity) {
                    Passenger* p = up ? floors[floor].getUpPassenger() : floors[floor].getDownPassenger();
                    if (p == nullptr)break;
                    addPassenger(p);
                }
                if(numOfPassengers == capacity)cvCart.notify_one();
                this_thread::sleep_for(tOpenDoor);
            }
            floors[floor].work(false); // stop working on this cart;
            if (dest > floor) {
                if (!toFirst) {
                    int i;
                    for (i = floor + 1; i <= dest; i++) {
                        if (floors[i].pushedUp() || floors[i].pushedDown() || pushedButton[i])break;
                    }
                    if (i > dest) break;
                }
                this_thread::sleep_for(tMoveCart);
                floor++;
            }
            else if (dest < floor) {
                if (!toFirst) {
                    int i;
                    for (i = floor - 1; i >= dest; i--) {
                        if (floors[i].pushedUp() || floors[i].pushedDown() || pushedButton[i])break;
                    }
                    if (i < dest) break;
                }
                this_thread::sleep_for(tMoveCart);
                floor--;
            }
            else break;
        }
    }
    void updateScreen() {
        unique_lock<mutex> lock(lockForScreen);
        DWORD dwBytesWritten = 0;
        swprintf_s(&screen[0 * screenW + cart_X[id]], 14, L"Cart:     %3d", id);
        swprintf_s(&screen[1 * screenW + cart_X[id]], 14, L"Floor:    %3d", floor);
        swprintf_s(&screen[2 * screenW + cart_X[id]], 14, L"Passenger:%3d", numOfPassengers);
        swprintf_s(&screen[4 * screenW + cart_X[id] + 3], 8, L"Buttons");
        swprintf_s(&screen[5 * screenW + cart_X[id] + 3], 13, L" |  Cart Pos");
        swprintf_s(&screen[6 * screenW + cart_X[id] + 3], 7, L" |   |");
        for (int n = 0; n < totalFloor; n++) {
            int floorN = floors[n].getPrintNum();
            if (floor == n) {
                swprintf_s(&screen[(36 - n) * screenW + cart_X[id] + 1], 12, L"%2d %c [%3d ]", floorN, (pushed(n) ? L'#' : L'-'), numOfPassengers);
            }
            else {
                swprintf_s(&screen[(36 - n) * screenW + cart_X[id] + 1], 12, L"%2d %c       ", floorN, (pushed(n) ? L'#' : L'-'));
            }
        }
        for (int n = 0; n < totalFloor; n++) {
            int floorN = floors[n].getPrintNum();
            swprintf_s(&screen[(36 - n) * screenW + analysis_X + 1], 26, L"%2d %3d (Up:%3d, down:%3d)", floorN, floors[n].getWaitingNum(), floors[n].getUpNum(), floors[n].getDownNum());
        }
        WriteConsoleOutputCharacter(console, screen, screenH * screenW, { 0,0 }, &dwBytesWritten);
    }
};

class Elevator {
private:
    DWORD dwBytesWritten;
    Cart cart[nElevator];
public:
    Elevator() : dwBytesWritten(0){
        for (int i = 0; i < nElevator; i++) {
            cart[i] = Cart(i);
        }
        for (int i = 0; i < totalFloor; i++) {
            floors[i] = Floor(i);
        }
    }
    void run() {
        startTime = chrono::system_clock::now();
        for (int i = 0; i < screenH * screenW; i++) screen[i] = L' ';
        for (int i = 0; i < screenH - 2; i++) screen[i * screenW] = screen[i * screenW + 20] = screen[i * screenW + 40] = screen[i * screenW + 60] = L'|';
        console = GetStdHandle(STD_OUTPUT_HANDLE);
        dwBytesWritten = 0;
        WriteConsoleOutputCharacter(console, screen, screenH * screenW, { 0,0 }, &dwBytesWritten);
        updateScreen();
        thread cartThread[nElevator];
        for (int i = 0; i < nElevator; i++) {
            cartThread[i] = thread(&Cart::run, ref(cart[i]));
        }
        while (1) {
            this_thread::sleep_for(generatePassenger);
            int nCome = rand() % (maxCome + 1), nMove = rand() % (maxMove + 1);
            // generate incomming passengers
            for (int i = 0; i < nCome; i++) {
                Passenger* passenger = new Passenger(firstFloor);
                floors[firstFloor].addPassenger(passenger);
            }
            // generate moving passengers
            for (int i = 0; i < nMove; i++) {
                int n = rand() % totalFloor;
                Passenger* passenger = new Passenger(n);
                floors[n].addPassenger(passenger);
            }
            cvCart.notify_one();
            updateData();
            if (chrono::duration_cast<chrono::seconds>(chrono::system_clock::now() - startTime) > tRunning) {
                unique_lock<mutex> lock(lockForStop);  
                Stop = true;
                break;
            }
        }
        for (int i = 0; i < nElevator; i++) {
            cartThread[i].join();
        }
    }
    void updateScreen() {
        {
            unique_lock<mutex> lock(lockForScreen);
            for (int i = 0; i < nElevator; i++) {
                swprintf_s(&screen[0 * screenW + cart_X[i]], 14, L"Cart:     %3d", i);
                swprintf_s(&screen[1 * screenW + cart_X[i]], 14, L"Floor:    %3d", cart[i].floor);
                swprintf_s(&screen[2 * screenW + cart_X[i]], 14, L"Passenger:%3d", cart[i].numOfPassengers);
                swprintf_s(&screen[4 * screenW + cart_X[i] + 3], 8, L"Buttons");
                swprintf_s(&screen[5 * screenW + cart_X[i] + 3], 13, L" |  Cart Pos");
                swprintf_s(&screen[6 * screenW + cart_X[i] + 3], 7, L" |   |");
                for (int n = 0; n < totalFloor; n++) {
                    int floorN = floors[n].getPrintNum();
                    if (cart[i].floor == n) {
                        swprintf_s(&screen[(36 - n) * screenW + cart_X[i] + 1], 12, L"%2d %c [%3d ]", floorN, (cart[i].pushed(n) ? L'#' : L'-'), cart[i].numOfPassengers);
                    }
                    else {
                        swprintf_s(&screen[(36 - n) * screenW + cart_X[i] + 1], 12, L"%2d %c       ", floorN, (cart[i].pushed(n) ? L'#' : L'-'));
                    }
                }
            }
            WriteConsoleOutputCharacter(console, screen, screenH * screenW, { 0,0 }, &dwBytesWritten);
        }
        updateData();
    }
    void updateData(){
        unique_lock<mutex> lock(lockForScreen);
        swprintf_s(&screen[0 * screenW + analysis_X], 19, L"Total Served: %4d", totalServed);
        swprintf_s(&screen[1 * screenW + analysis_X], 21, L"Avg Waiting: %5dms", avgWait);
        swprintf_s(&screen[2 * screenW + analysis_X], 21, L"Max Waiting: %5dms", (int)maxWait.count());
        swprintf_s(&screen[5 * screenW + analysis_X + 3], 19, L"Waiting Passengers");
        swprintf_s(&screen[6 * screenW + analysis_X + 3], 5, L"   |");
        for (int n = 0; n < totalFloor; n++) {
            int floorN = floors[n].getPrintNum();
            swprintf_s(&screen[(36 - n) * screenW + analysis_X + 1], 26, L"%2d %3d (Up:%3d, down:%3d)", floorN, floors[n].getWaitingNum(), floors[n].getUpNum(), floors[n].getDownNum());
        }
        WriteConsoleOutputCharacter(console, screen, screenH * screenW, { 0,0 }, &dwBytesWritten);
    }
    void print() {
        WriteConsoleOutputCharacter(console, L"This is a print test", 20, { 0,0 }, &dwBytesWritten);
    }
};

bool SetConsole() {
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_SCREEN_BUFFER_INFO ConsoleInfo; 
    GetConsoleScreenBufferInfo(hOut, &ConsoleInfo);
    if (ConsoleInfo.dwMaximumWindowSize.X < screenW || ConsoleInfo.dwMaximumWindowSize.Y < screenH) {
        cout << "The console window is too small. Please set the console window to at least " << screenW << "*" << screenH << endl;
    }
    SMALL_RECT rc = { 0,0, screenW-1, screenH-1 };
    if (!SetConsoleWindowInfo(hOut, true, &rc)) {
        cout << "Set Window Size Error" << endl;
        return false;
    }
    COORD size = { screenW, screenH };
    if (!SetConsoleScreenBufferSize(hOut, size)) {
        cout << "Set Buffer Size Error" << endl;
        return false;
    }
    wchar_t title[20] = L"Elevator Simulation";
    SetConsoleTitle(title);
    //CONSOLE_CURSOR_INFO cursorInfo = { 1, FALSE };
    //SetConsoleCursorInfo(hOut, &cursorInfo);
    return true;
}


int main()
{
    Elevator elevator;
    if (!SetConsole())return 1;
    elevator.run();
    DWORD dwBytesWritten;
    WriteConsoleOutputCharacter(console, L"Simulation Finished! Press any key to exit.", 44, { 0,38 }, &dwBytesWritten);
    COORD w = { 0, 39 };
    SetConsoleCursorPosition(console, w);
    system("pause");
    CloseHandle(console);
}
