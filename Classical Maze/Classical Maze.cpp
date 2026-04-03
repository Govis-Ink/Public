#include <graphics.h>
#include <dshow.h>
#include <iostream>
#include <filesystem>
#include <fstream>
#include <string>
#include <map>
#include <vector>
#include <utility>
#include <unordered_set>
#include <unordered_map>
#include <queue>
#include <memory>
#include <random>
#include <thread>
#include <mutex>
#include <atomic>
#include <chrono>
#pragma comment( lib, "MSIMG32.LIB")
#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "strmiids.lib")

using namespace std;
#define PI 3.14159265358979323846
#define INF 0x3f3f3f3f

const int screenWidth = 1600, screenHeight = 900;
const int renderWidth = 16, renderHeight = 11;
const int areaSideLength = 64;
const int bufferAreaSize = 15;
const int terrainNum = 9;
const int FPS = 30;
mutex mtx;
long mapWidth = 63, mapHeight = 63;
long long centerX = areaSideLength * mapWidth / 2, centerY = areaSideLength * mapHeight / 2;
enum MapType { CITY = 0, COUNTY = 1 };
MapType mapType = CITY;
enum Direction { East = 0, South = 1, West = 2, North = 3 };
IMAGE checkImg, dontImg, fileImg;

class MusicPlayer {
private:
    std::atomic<bool> playing{ false };
    std::atomic<bool> stopRequested{ false };
    std::wstring currentFile;

    void PlayInternal(std::wstring file) {
        CoInitializeEx(NULL, COINIT_MULTITHREADED);

        IGraphBuilder* pGraph = nullptr;
        IMediaControl* pControl = nullptr;
        IMediaSeeking* pSeeking = nullptr;

        HRESULT hr = CoCreateInstance(CLSID_FilterGraph, NULL, CLSCTX_INPROC_SERVER,
            IID_IGraphBuilder, (void**)&pGraph);
        if (SUCCEEDED(hr)) {
            pGraph->QueryInterface(IID_IMediaControl, (void**)&pControl);
            pGraph->QueryInterface(IID_IMediaSeeking, (void**)&pSeeking);

            if (SUCCEEDED(pGraph->RenderFile(file.c_str(), NULL))) {
                pControl->Run();
                playing = true;

                while (!stopRequested) {
                    LONGLONG current = 0, duration = 0;
                    if (pSeeking) {
                        pSeeking->GetCurrentPosition(&current);
                        pSeeking->GetDuration(&duration);

                        if (duration > 0 && current >= duration) {
                            break;
                        }
                    }
                    std::this_thread::sleep_for(std::chrono::milliseconds(100));
                }
            }
        }

        playing = false;
        if (pSeeking) pSeeking->Release();
        if (pControl) {
            pControl->Stop();
            pControl->Release();
        }
        if (pGraph) pGraph->Release();

        CoUninitialize();
    }

public:
    MusicPlayer() = default;

    ~MusicPlayer() {
        Stop();
    }

    void Play(const std::wstring& file, bool blocking) {
        Stop();
        currentFile = file;
        stopRequested = false;

        if (blocking) {
            PlayInternal(file);
        }
        else {
            std::thread([this, file]() {
                PlayInternal(file);
                }).detach();
        }
    }

    void Stop() {
        stopRequested = true;
        playing = false;
    }

    bool IsPlaying() const { return playing; }
    std::wstring GetCurrentFile() const { return currentFile; }
};

template<typename T>
const T& random_element(const vector<T>& v) {
    random_device rd;
    mt19937 gen(rd());
    int num = v.size();
    uniform_int_distribution<int> dis(0, num - 1);
    return v[dis(gen)];
}

Direction Opposite(Direction dir) {
    return static_cast<Direction>((dir + 2) % 4);
}

double directionToRadian(Direction direction) {
    switch (direction) {
    case East: return 0;
    case South: return PI / 2 * 3;
    case West: return PI;
    case North: return PI / 2;
    default : return 0;
    }
}

void transparentimage(IMAGE* dstimg, int x, int y, IMAGE* srcimg, int srcx, int srcy, int srcwidth, int srcheight, UINT transparentcolor) {
    HDC dstDC = GetImageHDC(dstimg);
    HDC srcDC = GetImageHDC(srcimg);

    TransparentBlt(dstDC, x, y, srcwidth, srcheight, srcDC, srcx, srcy, srcwidth, srcheight, transparentcolor);
}

void drawTextAutoWrap(int x, int y, const std::wstring& text,
    int maxWidth = 500, int lineHeight = 20) {
    int startX = x;
    int currentY = y;
    std::wstring currentLine;

    setbkmode(TRANSPARENT);
    settextstyle(lineHeight, 0, _T("Times New Roman"));

    for (wchar_t ch : text) {
        if (ch == L'\n') {
            outtextxy(startX, currentY, currentLine.c_str());
            currentLine.clear();
            currentY += lineHeight;
            startX = x;
            continue;
        }

        currentLine += ch;

        if (textwidth(currentLine.c_str()) > maxWidth) {
            currentLine.pop_back();
            outtextxy(startX, currentY, currentLine.c_str());
            currentY += lineHeight;
            currentLine = ch;
        }
    }

    if (!currentLine.empty()) {
        outtextxy(startX, currentY, currentLine.c_str());
    }
}

void initUI() {
    loadimage(&fileImg, L"file.png");
    loadimage(&checkImg, L"check.png");
    loadimage(&dontImg, L"dont.png");
}

IMAGE Dirt, DirtRough, BlackMarble, BrickTiles, SquareTiles, RoundTiles, Grass, GrassTrim, WhiteMarble, CityCliff, NatureCliff;

struct Terrain {
    IMAGE img;
    int terrainIndex;
    bool isExtended;

    Terrain() : terrainIndex(9), isExtended(false) {}
    Terrain(IMAGE img, int terrainIndex, bool isExtended = false) : img(img), terrainIndex(terrainIndex), isExtended(isExtended) {}
};

Terrain terrainTypes[11];

void initTerrain() {
    loadimage(&Dirt, L"TerrainArt/DRuins_Dirt.png");
    terrainTypes[0] = Terrain(Dirt, 0, true);
    loadimage(&DirtRough, L"TerrainArt/DRuins_DirtRough.png");
    terrainTypes[1] = Terrain(DirtRough, 1, true);
    loadimage(&BlackMarble, L"TerrainArt/DRuins_BlackMarble.png");
    terrainTypes[2] = Terrain(BlackMarble, 2, false);
    loadimage(&BrickTiles, L"TerrainArt/DRuins_BrickTiles.png");
    terrainTypes[3] = Terrain(BrickTiles, 3, false);
    loadimage(&SquareTiles, L"TerrainArt/DRuins_SquareTiles.png");
    terrainTypes[4] = Terrain(SquareTiles, 4, false);
    loadimage(&RoundTiles, L"TerrainArt/DRuins_RoundTiles.png");
    terrainTypes[5] = Terrain(RoundTiles, 5, false);
    loadimage(&Grass, L"TerrainArt/DRuins_Grass.png");
    terrainTypes[6] = Terrain(Grass, 6, true);
    loadimage(&GrassTrim, L"TerrainArt/DRuins_GrassTrim.png");
    terrainTypes[7] = Terrain(GrassTrim, 7, false);
    loadimage(&WhiteMarble, L"TerrainArt/DRuins_WhiteMarble.png");
    terrainTypes[8] = Terrain(WhiteMarble, 8, false);
    loadimage(&CityCliff, L"TerrainArt/DRuins_CityCliff.png");
    terrainTypes[9] = Terrain(CityCliff, 9, false);
    loadimage(&NatureCliff, L"TerrainArt/DRuins_NatureCliff.png");
    terrainTypes[10] = Terrain(NatureCliff, 10, false);
}

class Unit {
protected:
    int x, y;
public:
    Unit(int x, int y) : x(x), y(y) {}

    pair<int, int> getPosition() const {
        return { x, y };
    }

    void setPosition(int x, int y) {
        this->x = x;
        this->y = y;
    }

    ~Unit() {}
};

struct Point {
    bool isCliff = false;
    bool isCorridor = false;
    bool isStart = false;
    bool isEnd = false;
    bool isMarked = false;
    int x, y;
    int terrainIndex = 9;

    Point() : x(0), y(0), terrainIndex(9) {}

    Point(int x, int y, int terrainIndex = 9)
        : x(x), y(y), terrainIndex(terrainIndex) {
    }

    ~Point() {}
};

vector<vector<Point>> mapPoints;

pair<int, int> directionToVector(int direction) {
    switch (direction % 4) {
    case 0: return { 1, 0 };
    case 1: return { 0, 1 };
    case 2: return { -1, 0 };
    case 3: return { 0, -1 };
    default: return { 0, 0 };
    }
}

class Player : public Unit {
protected:
    Direction direction;
    bool isMoving = false;
    bool isAttacking = false;
    bool isVictory = false;
    int movement = 0;
    vector<Direction> way;
    IMAGE img;
    IMAGE wayCheck;
public:
    Player() : Unit(0, 0), direction(South) {
        loadimage(&img, L"Footman.png");
        loadimage(&wayCheck, L"check.png");
    }

    void setMoving(bool isMoving) {
        this->isMoving = isMoving;
    }

    void setAttacking(bool isAttacking) {
        this->isAttacking = isAttacking;
    }

    void setVictory(bool isVictory) {
        this->isVictory = isVictory;
    }

    vector<Direction> getWay() const {
        return way;
    }

    bool setDirection(char key) {
        if (key == 'D') {
            direction = East;
        }
        else if (key == 'S') {
            direction = South;
        }
        else if (key == 'A') {
            direction = West;
        }
        else if (key == 'W') {
            direction = North;
        }
        else return false;
        return true;
    }

    bool moveTo() {
        switch (direction) {
        case East:
            if (x < mapWidth - bufferAreaSize && !mapPoints[y][x + 1].isCliff) {
                x++;
                return true;
            }
            else return false;
        case South:
            if (y < mapHeight - bufferAreaSize && !mapPoints[y + 1][x].isCliff) {
                y++;
                return true;
            }
            else return false;
        case West:
            if (x > bufferAreaSize && !mapPoints[y][x - 1].isCliff) {
                x--;
                return true;
            }
            else return false;
        case North:
            if (y > bufferAreaSize && !mapPoints[y - 1][x].isCliff) {
                y--;
                return true;
            }
            else return false;
        default:
            return false;
        }
    }

    pair<vector<Direction>, bool> tryDirection(int nowX, int nowY, int targetX, int targetY, int iterations, int fromDir = -1) {
        if (iterations <= 0) return { {}, false };
        if (nowX == targetX && nowY == targetY) return { {}, true };

        vector<Direction> bestWay;

        for (int i = 0; i < 4; i++) {
            if (i == fromDir) continue;

            auto [dx, dy] = directionToVector(i);
            int newX = nowX + dx, newY = nowY + dy;

            if (!mapPoints[newY][newX].isCliff) {
                auto [way, found] = tryDirection(newX, newY, targetX, targetY, iterations - 1, (i + 2) % 4);
                if (found) {
                    way.push_back(static_cast<Direction>(i));
                    if (bestWay.empty() || way.size() < bestWay.size()) bestWay = way;
                }
            }
        }

        return { bestWay, !bestWay.empty() };
    }

    bool setWay(int targetX, int targetY) {
        int iterations = (mapWidth - 2 * bufferAreaSize) * (mapHeight - 2 * bufferAreaSize);

        auto result = tryDirection(x, y, targetX, targetY, iterations);
        if (result.second) {
            way = result.first;
            return true;
        }

        return false;
    }

    bool followWay() {
        lock_guard<mutex> lock(mtx);
        if (way.empty()) return false;
        direction = way.back();
        moveTo();
        way.pop_back();
        return true;
    }

    void clearWay() {
        way.clear();
    }

    void draw(IMAGE& canvas) {
        int screenX = x * areaSideLength + screenWidth / 2 - centerX - areaSideLength / 2;
        int screenY = y * areaSideLength + screenHeight / 2 - centerY - areaSideLength / 2;
        IMAGE temporaryImg = IMAGE(areaSideLength, areaSideLength);
        if (isVictory) {
            if (movement == 0) {
                transparentimage(&temporaryImg, 0, 0, &img, 3 * areaSideLength, 0, areaSideLength, areaSideLength, RGB(0, 0, 0));
                movement++;
            }
            else {
                transparentimage(&temporaryImg, 0, 0, &img, 0, 0, areaSideLength, areaSideLength, RGB(0, 0, 0));
                movement = 0;
            }
        }
        else {
            if (isMoving) {
                if (movement == 0) {
                    transparentimage(&temporaryImg, 0, 0, &img, 1 * areaSideLength, 0, areaSideLength, areaSideLength, RGB(0, 0, 0));
                    movement++;
                }
                else {
                    transparentimage(&temporaryImg, 0, 0, &img, 2 * areaSideLength, 0, areaSideLength, areaSideLength, RGB(0, 0, 0));
                    movement = 0;
                    isMoving = false;
                }
            }
            else {
                transparentimage(&temporaryImg, 0, 0, &img, 0, 0, areaSideLength, areaSideLength, RGB(0, 0, 0));
            }
        }
        rotateimage(&temporaryImg, &temporaryImg, directionToRadian(direction));
        transparentimage(&canvas, screenX, screenY, &temporaryImg, 0, 0, areaSideLength, areaSideLength, RGB(0, 0, 0));
    }

    void drawTips(IMAGE& canvas) {
        lock_guard<mutex> lock(mtx);
        int nowX = x, nowY = y;
        for (auto it = way.rbegin(); it != way.rend(); ++it) {
            auto [dx, dy] = directionToVector(*it);
            nowX += dx;
            nowY += dy;
            int screenX = nowX * areaSideLength + screenWidth / 2 - centerX - areaSideLength / 2 - 2;
            int screenY = nowY * areaSideLength + screenHeight / 2 - centerY - areaSideLength / 2 - 2;
            transparentimage(&canvas, screenX, screenY, &checkImg, 0, 0, areaSideLength + 4, areaSideLength + 4, RGB(0, 0, 0));
        }
    }

    ~Player() {}
};

template<typename T>
int spirit_index_transform(const T& s) {
    int y = 0;
    for (int x : s) {
        y += 1 << x;
    }
    return y;
}

vector<pair<int, int>> unExtendedIndex = { {0, 0}, {3, 3} };
vector<pair<int, int>> extendedIndex = {
    {0, 0}, {3, 3}, {4, 0}, {5, 0}, {6, 0}, {7 , 0},
    {4, 1}, {5, 1}, {6, 1}, {7, 1}, {4, 2}, {5, 2},
    {6, 2}, {7, 2}, {4, 3}, {5, 3}, {6, 3}, {7, 3}
};

pair<int, int> spirit_index_extend(bool isExtended) {
    if (isExtended) {
        return random_element(extendedIndex);
    }
    else {
        return random_element(unExtendedIndex);
    }
}

pair<int, int> point_index_transform(int n) {
    if (n >= 0 && n < 4) {
        int first = (n & 2) ? 0 : 1;
        int second = (n & 1) ? 0 : 1;
        return { first, second };
    }
    return { 0, 0 };
}

class Area {
private:
    map<int, vector<int>> terrains = {};
    int maxIndex = 0;
    pair<int, int> roundIndex;
    long long x, y;
public:
    Area(int x = 0, int y = 0) {
        this->x = x;
        this->y = y;
        initDraw(x, y);
    }

    void initDraw(int x, int y) {
        int terrainIndex = 9;

        for (int i = 0; i < 4; i++) {
            pair<int, int> positionShift = point_index_transform(i);
            int shiftY = y + positionShift.first, shiftX = x + positionShift.second;

            terrainIndex = mapPoints[shiftY][shiftX].terrainIndex;
            if (terrains.find(terrainIndex) == terrains.end()) {
                terrains[terrainIndex] = {};
            }
            terrains[terrainIndex].push_back(i);
            if (terrainIndex > maxIndex && terrainIndex < terrainNum) {
                maxIndex = terrainIndex;
            }
        }

        roundIndex = spirit_index_extend(terrainTypes[maxIndex].isExtended);
    }

    void draw(IMAGE& canvas) {
        IMAGE img = IMAGE(areaSideLength, areaSideLength);
        SetWorkingImage(&img);
        cleardevice();

        putimage(0, 0, areaSideLength, areaSideLength, &terrainTypes[maxIndex].img,
            roundIndex.first * areaSideLength, roundIndex.second * areaSideLength);

        for (auto& t : terrains) {
            if (t.first != maxIndex) {
                int spiritIndex = spirit_index_transform(t.second);
                transparentimage(&img, 0, 0, &terrainTypes[t.first].img, (spiritIndex % 4) * areaSideLength, (spiritIndex / 4) * areaSideLength, areaSideLength, areaSideLength, RGB(0, 0, 0));
            }
        }

        SetWorkingImage(&canvas);
        int screenX = x * areaSideLength + screenWidth / 2 - centerX, screenY = y * areaSideLength + screenHeight / 2 - centerY;
        putimage(screenX, screenY, &img);
    }
};

vector<Area> mapAreas = {};

struct TPoint {
    int x, y;
    TPoint(int x = 0, int y = 0) : x(x), y(y) {}

    bool operator==(const TPoint& other) const {
        return x == other.x && y == other.y;
    }

    bool operator!=(const TPoint& other) const {
        return !(*this == other);
    }

    bool operator<(const TPoint& other) const {
        return (x < other.x) || (x == other.x && y < other.y);
    }

    TPoint Move(Direction dir, int step = 1) const {
        switch (dir) {
        case East: return TPoint(x + step, y);
        case South: return TPoint(x, y + step);
        case West: return TPoint(x - step, y);
        case North: return TPoint(x, y - step);
        }
        return *this;
    }

    struct Hash {
        size_t operator()(const TPoint& p) const {
            return hash<int>()(p.x) ^ (hash<int>()(p.y) << 1);
        }
    };
};

class MazeGenerator {
private:
    int width_, height_;
    vector<vector<int>> grid_;
    TPoint startPoint_;
    TPoint endPoint_;

    random_device rd_;
    mt19937 gen_;

public:
    MazeGenerator(int width, int height)
        : width_(width), height_(height), gen_(rd_()) {
        grid_.resize(height_, vector<int>(width_, 0));
    }

    void Generate() {
        for (int y = 0; y < height_; y++) {
            for (int x = 0; x < width_; x++) {
                grid_[y][x] = 0;
            }
        }

        GenerateMazePrim();

        EnsureConnectivity();

        SetStartAndEndPoints();

        grid_[startPoint_.y][startPoint_.x] = 2;
        grid_[endPoint_.y][endPoint_.x] = 3;
    }

    const vector<vector<int>> GetGrid() const { return grid_; }

    const TPoint& GetStartPoint() const { return startPoint_; }

    const TPoint& GetEndPoint() const { return endPoint_; }

private:
    void GenerateMazePrim() {
        uniform_int_distribution<> xDist(1, width_ - 2);
        uniform_int_distribution<> yDist(1, height_ - 2);
        TPoint start = TPoint(xDist(gen_), yDist(gen_));

        grid_[start.y][start.x] = 1;

        vector<pair<TPoint, Direction>> walls;
        AddWalls(start, walls);

        while (!walls.empty()) {
            uniform_int_distribution<> dist(0, walls.size() - 1);
            int index = dist(gen_);
            auto [wallPos, dir] = walls[index];
            walls.erase(walls.begin() + index);

            TPoint cell = wallPos.Move(dir);

            if (cell.x > 0 && cell.x < width_ - 1 &&
                cell.y > 0 && cell.y < height_ - 1) {
                if (grid_[cell.y][cell.x] == 0) {
                    grid_[wallPos.y][wallPos.x] = 1;
                    grid_[cell.y][cell.x] = 1;

                    AddWalls(cell, walls);
                }
            }
        }
    }

    void AddWalls(const TPoint& cell, vector<pair<TPoint, Direction>>& walls) {
        for (int dir = 0; dir < 4; dir++) {
            TPoint wall = cell.Move(static_cast<Direction>(dir));

            if (wall.x > 0 && wall.x < width_ - 1 &&
                wall.y > 0 && wall.y < height_ - 1 &&
                grid_[wall.y][wall.x] == 0) {
                walls.emplace_back(wall, static_cast<Direction>(dir));
            }
        }
    }

    void EnsureConnectivity() {
        vector<unordered_set<TPoint, TPoint::Hash>> regions;
        unordered_set<TPoint, TPoint::Hash> allVisited;

        for (int y = 1; y < height_ - 1; y++) {
            for (int x = 1; x < width_ - 1; x++) {
                TPoint p(x, y);

                if (grid_[y][x] == 1 && allVisited.find(p) == allVisited.end()) {
                    unordered_set<TPoint, TPoint::Hash> region;
                    FloodFill(p, region);

                    regions.push_back(region);
                    allVisited.insert(region.begin(), region.end());
                }
            }
        }

        if (regions.size() > 1) {
            for (size_t i = 1; i < regions.size(); i++) {
                ConnectRegions(regions[0], regions[i]);
            }
        }
    }

    void FloodFill(const TPoint& start, unordered_set<TPoint, TPoint::Hash>& region) {
        queue<TPoint> q;
        q.push(start);
        region.insert(start);

        while (!q.empty()) {
            TPoint current = q.front();
            q.pop();

            for (int dir = 0; dir < 4; dir++) {
                TPoint neighbor = current.Move(static_cast<Direction>(dir));

                if (neighbor.x <= 0 || neighbor.x >= width_ - 1 ||
                    neighbor.y <= 0 || neighbor.y >= height_ - 1) continue;

                if (grid_[neighbor.y][neighbor.x] == 1 &&
                    region.find(neighbor) == region.end()) {
                    q.push(neighbor);
                    region.insert(neighbor);
                }
            }
        }
    }

    void ConnectRegions(const unordered_set<TPoint, TPoint::Hash>& region1,
        const unordered_set<TPoint, TPoint::Hash>& region2) {
        TPoint point1, point2;
        int minDistance = INT_MAX;

        for (const auto& p1 : region1) {
            for (const auto& p2 : region2) {
                int distance = abs(p1.x - p2.x) + abs(p1.y - p2.y);
                if (distance < minDistance) {
                    minDistance = distance;
                    point1 = p1;
                    point2 = p2;
                }
            }
        }

        int dx = point2.x - point1.x;
        int dy = point2.y - point1.y;

        TPoint current = point1;

        int stepX = (dx > 0) ? 1 : -1;
        for (int i = 0; i < abs(dx); i++) {
            current.x += stepX;
            if (current.x <= 0 || current.x >= width_ - 1 ||
                current.y <= 0 || current.y >= height_ - 1) break;

            grid_[current.y][current.x] = 1;
        }

        int stepY = (dy > 0) ? 1 : -1;
        for (int i = 0; i < abs(dy); i++) {
            current.y += stepY;
            if (current.x <= 0 || current.x >= width_ - 1 ||
                current.y <= 0 || current.y >= height_ - 1) break;

            grid_[current.y][current.x] = 1;
        }
    }

    void SetStartAndEndPoints() {
        int startX = 1 + gen_() % (width_ / 4);
        int startY = 1 + gen_() % (height_ / 4);

        startPoint_ = FindNearestPathPoint(TPoint(startX, startY));

        int endX = width_ - 2 - gen_() % (width_ / 4);
        int endY = height_ - 2 - gen_() % (height_ / 4);

        endPoint_ = FindNearestPathPoint(TPoint(endX, endY));

        if (startPoint_ == endPoint_) {
            do {
                endX = width_ - 2 - gen_() % (width_ / 4);
                endY = height_ - 2 - gen_() % (height_ / 4);
                endPoint_ = FindNearestPathPoint(TPoint(endX, endY));
            } while (startPoint_ == endPoint_);
        }

        EnsurePathBetweenStartAndEnd();
    }

    TPoint FindNearestPathPoint(const TPoint& start) {
        queue<TPoint> q;
        unordered_set<TPoint, TPoint::Hash> visited;

        q.push(start);
        visited.insert(start);

        while (!q.empty()) {
            TPoint current = q.front();
            q.pop();

            if (grid_[current.y][current.x] == 1) {
                return current;
            }

            for (int dir = 0; dir < 4; dir++) {
                TPoint neighbor = current.Move(static_cast<Direction>(dir));

                if (neighbor.x < 0 || neighbor.x >= width_ ||
                    neighbor.y < 0 || neighbor.y >= height_) continue;

                if (visited.find(neighbor) != visited.end()) continue;

                q.push(neighbor);
                visited.insert(neighbor);
            }
        }

        return start;
    }

    void EnsurePathBetweenStartAndEnd() {
        vector<TPoint> path = FindAStarPath(startPoint_, endPoint_);

        if (path.empty()) {
            CreateDirectPath(startPoint_, endPoint_);
        }
    }

    vector<TPoint> FindAStarPath(const TPoint& start, const TPoint& end) {
        struct Node {
            TPoint pos;
            int g, h, f;
            shared_ptr<Node> parent;

            Node(TPoint p, int g = 0, int h = 0, shared_ptr<Node> parent = nullptr)
                : pos(p), g(g), h(h), f(g + h), parent(parent) {
            }
        };

        auto heuristic = [](const TPoint& a, const TPoint& b) {
            return abs(a.x - b.x) + abs(a.y - b.y);
            };

        auto cmp = [](const shared_ptr<Node>& a, const shared_ptr<Node>& b) {
            return a->f > b->f;
            };

        priority_queue<shared_ptr<Node>,
            vector<shared_ptr<Node>>,
            decltype(cmp)> openList(cmp);
        unordered_map<TPoint, int, TPoint::Hash> gValues;

        openList.push(make_shared<Node>(start, 0, heuristic(start, end)));
        gValues[start] = 0;

        while (!openList.empty()) {
            auto current = openList.top();
            openList.pop();

            if (current->g > gValues[current->pos]) continue;

            if (current->pos == end) {
                vector<TPoint> path;
                auto node = current;
                while (node) {
                    path.push_back(node->pos);
                    node = node->parent;
                }
                reverse(path.begin(), path.end());
                return path;
            }

            for (int dir = 0; dir < 4; dir++) {
                TPoint neighbor = current->pos.Move(static_cast<Direction>(dir));

                if (neighbor.x < 0 || neighbor.x >= width_ ||
                    neighbor.y < 0 || neighbor.y >= height_) continue;

                int moveCost = (grid_[neighbor.y][neighbor.x] == 0) ? 10 : 1;
                int newG = current->g + moveCost;

                if (gValues.find(neighbor) == gValues.end() || newG < gValues[neighbor]) {
                    gValues[neighbor] = newG;
                    int h = heuristic(neighbor, end);
                    openList.push(make_shared<Node>(neighbor, newG, h, current));
                }
            }
        }

        return {};
    }

    void CreateDirectPath(const TPoint& start, const TPoint& end) {
        TPoint current = start;

        int stepX = (end.x > start.x) ? 1 : -1;
        for (int x = start.x; x != end.x; x += stepX) {
            current.x = x;
            if (current.x <= 0 || current.x >= width_ - 1) break;
            if (current.y <= 0 || current.y >= height_ - 1) break;

            grid_[current.y][current.x] = 1;
        }
        current.x = end.x;

        int stepY = (end.y > start.y) ? 1 : -1;
        for (int y = start.y; y != end.y; y += stepY) {
            current.y = y;
            if (current.x <= 0 || current.x >= width_ - 1) break;
            if (current.y <= 0 || current.y >= height_ - 1) break;

            grid_[current.y][current.x] = 1;
        }
    }
};

void putMaze(const vector<vector<int>>& maze, vector<vector<Point>>& map) {
    for (int y = 0; y < maze.size(); y++) {
        for (int x = 0; x < maze[y].size(); x++) {
            if (maze[y][x] == 0) {
                map[y + bufferAreaSize][x + bufferAreaSize].isCliff = true;
                if (mapType == 0) {
                    map[y + bufferAreaSize][x + bufferAreaSize].terrainIndex = 9;
                }
                else if (mapType == 1) {
                    map[y + bufferAreaSize][x + bufferAreaSize].terrainIndex = 10;
                }
            }
            else if (maze[y][x] == 1) {
                map[y + bufferAreaSize][x + bufferAreaSize].isCorridor = true;
                if (mapType == 0) {
                    map[y + bufferAreaSize][x + bufferAreaSize].terrainIndex = 2;
                }
                else if (mapType == 1) {
                    map[y + bufferAreaSize][x + bufferAreaSize].terrainIndex = 6;
                }
            }
            else if (maze[y][x] == 2) {
                map[y + bufferAreaSize][x + bufferAreaSize].isStart = true;
                map[y + bufferAreaSize][x + bufferAreaSize].isStart = true;
                if (mapType == 0) {
                    map[y + bufferAreaSize][x + bufferAreaSize].terrainIndex = 3;
                }
                else if (mapType == 1) {
                    map[y + bufferAreaSize][x + bufferAreaSize].terrainIndex = 5;
                }
            }
            else if (maze[y][x] == 3) {
                map[y + bufferAreaSize][x + bufferAreaSize].isCorridor = true;
                map[y + bufferAreaSize][x + bufferAreaSize].isEnd = true;
                if (mapType == 0) {
                    map[y + bufferAreaSize][x + bufferAreaSize].terrainIndex = 7;
                }
                else if (mapType == 1) {
                    map[y + bufferAreaSize][x + bufferAreaSize].terrainIndex = 8;
                }
            }

        }
    }
}

TPoint startPoint, endPoint;

void generateMap() {
    MazeGenerator generator(mapWidth - 2 * bufferAreaSize, mapHeight - 2 * bufferAreaSize);
    generator.Generate();

    mapPoints = vector<vector<Point>>(mapHeight, vector<Point>(mapWidth));
    mapAreas.clear();

    int terrainIndex = mapType == 0 ? 0 : 1;

    for (int y = 0; y < mapHeight; y++) {
        for (int x = 0; x < mapWidth; x++) {
            mapPoints[y][x] = Point(x, y, terrainIndex);
        }
    }

    putMaze(generator.GetGrid(), mapPoints);

    startPoint = generator.GetStartPoint();
    startPoint.x += bufferAreaSize;
    startPoint.y += bufferAreaSize;

    endPoint = generator.GetEndPoint();
    endPoint.x += bufferAreaSize;
    endPoint.y += bufferAreaSize;

    for (int y = 0; y < mapHeight - 1; y++) {
        for (int x = 0; x < mapWidth - 1; x++) {
            mapAreas.emplace_back(Area(x, y));
        }
    }
}

class Game {
private:
    Player* player;
    bool gameOver = false;
    bool levelOver = false;
    bool reloadNow = false;
    bool tip = true;
    int level = 0;
    int heighestLevel = 0;
    wstring text = L"";
    bool fileBox = false;
    int cursorPos = 0;
    thread drawThread, updateThread, bkSoundThread;
    IMAGE canvas;
    MusicPlayer soundPlayer;
public:
    Game() {
        ifstream score("record.txt", ios::in);
        if (score) {
            int recordedScore;
            score >> recordedScore;
            heighestLevel = recordedScore;
            score.close();
        }
        srand(time(0));
        player = new Player();
        initgraph(screenWidth, screenHeight);
        bkSoundThread = thread(&Game::bkSound, this);
        bkSoundThread.detach();
        start();
    }

    void saveGame() {
        ofstream file(text, ios::out);
        if (file.is_open()) {
            file << mapWidth << " " << mapHeight << endl;
            for (int y = 0; y < mapHeight; y++) {
                for (int x = 0; x < mapWidth; x++) {
                    file << mapPoints[y][x].terrainIndex << " ";
                    file << mapPoints[y][x].isCliff << " ";
                    file << mapPoints[y][x].isCorridor << " ";
                    file << mapPoints[y][x].isMarked << " ";
                }
                file << endl;
            }
            file << startPoint.x << " " << startPoint.y << " " << endPoint.x << " " << endPoint.y << endl;
            auto position = player->getPosition();
            file << position.first << " " << position.second << endl;
            file.close();
        }
        text = L"";
        cursorPos = 0;
    }

    void reloadGame() {
        ifstream file(text, ios::in);
        if (file.is_open()) {
            file >> mapWidth >> mapHeight;
            mapPoints = vector<vector<Point>>(mapHeight, vector<Point>(mapWidth));
            mapAreas.clear();

            for (int y = 0; y < mapHeight; y++) {
                for (int x = 0; x < mapWidth; x++) {
                    int terrainIndex, isCliff, isCorridor, isMarked;
                    file >> terrainIndex >> isCliff >> isCorridor >> isMarked;
                    mapPoints[y][x] = Point(x, y, terrainIndex);
                    mapPoints[y][x].isCliff = isCliff;
                    mapPoints[y][x].isCorridor = isCorridor;
                    mapPoints[y][x].isMarked = isMarked;
                }
            }

            int startX, startY, endX, endY;
            file >> startX >> startY >> endX >> endY;
            startPoint = TPoint(startX, startY);
            mapPoints[startX][startY].isStart = true;
            endPoint = TPoint(endX, endY);
            mapPoints[endX][endY].isEnd = true;

            for (int y = 0; y < mapHeight - 1; y++) {
                for (int x = 0; x < mapWidth - 1; x++) {
                    mapAreas.emplace_back(Area(x, y));
                }
            }

            int positionX, positionY;
            file >> positionX >> positionY;
            player->setPosition(positionX, positionY);

            file.close();
        }
        text = L"";
        cursorPos = 0;
        levelOver = false;
        reloadNow = false;
        drawThread = thread(&Game::draw, this);
        updateThread = thread(&Game::update, this);
        drawThread.join();
        updateThread.join();
        levelEnd();
    }

    void buffer() {
        BeginBatchDraw();

        pair<int, int> position = player->getPosition();
        centerX = position.first * areaSideLength + areaSideLength / 2;
        centerY = position.second * areaSideLength + areaSideLength / 2;
        canvas = IMAGE(screenWidth, screenHeight);
        SetWorkingImage(&canvas);

        for (int y = position.second - renderHeight; y <= position.second + renderHeight; y++) {
            for (int x = position.first - renderWidth; x <= position.first + renderWidth; x++) {
                if (x >= 0 && x < mapWidth && y >= 0 && y < mapHeight) {
                    if (x < mapWidth - 1 && y < mapHeight - 1) {
                        mapAreas[y * (mapWidth - 1) + x].draw(canvas);
                    }
                    if (mapPoints[y][x].isMarked) {
                        int screenX = x * areaSideLength + screenWidth / 2 - centerX - areaSideLength / 2 - 2;
                        int screenY = y * areaSideLength + screenHeight / 2 - centerY - areaSideLength / 2 - 2;
                        transparentimage(&canvas, screenX, screenY, &dontImg, 0, 0, areaSideLength + 4, areaSideLength + 4, RGB(0, 0, 0));
                    }
                }
            }
        }
        player->draw(canvas);

        if (tip) {
            player->drawTips(canvas);
        }

        drawTextAutoWrap(20, 20,
            L"Press 'AWSD' to move by one step.\n"
            L"Click the left mouse button to set target point.\n"
            L"Click the right mouse button to move to a position immediately.\n"
            L"Click the middle mouse buttonto set mark.\n"
            L"Press 'E' to set the target point to the end point.\n"
            L"Press 'R' to move to start point immediately.\n"
            L"Press 'T' to switch display path toggle.\n"
            L"Press 'F' to automatically follow the path you choose.\n"
            L"Press 'G' to skip this level.\n"
            L"Press 'H' to move to random position.\n"
            L"Press 'C' to delete game save file.\n"
            L"Press 'V' to save game.\n"
            L"Press 'B' to reload game.\n"
            L"Press 'Escape' to quit the game.\n"
            L"You have passed:  " + to_wstring(level) + L"\n"
            L"The recorded score:  " + to_wstring(heighestLevel)
        , 1000, 20);

        if (fileBox) {
            transparentimage(&canvas, 500, 345, &fileImg, 0, 0, 600, 210, RGB(0, 0, 0));
            drawTextAutoWrap(580, 425, text, 600, 50);
            setfillcolor(WHITE);
            solidrectangle(580 + cursorPos * 28, 470, 610 + cursorPos * 28, 480);
        }

        SetWorkingImage(NULL);
        cleardevice();
        putimage(0, 0, &canvas);
        FlushBatchDraw();
    }

    bool fileBoxAction() {
        flushmessage();
        ExMessage msg;
        fileBox = true;
        while (text.size() <= 15) {
            getmessage(&msg, EX_MOUSE | EX_KEY);
            if (msg.message == WM_RBUTTONDOWN || msg.message == WM_LBUTTONDOWN || msg.message == WM_MBUTTONDOWN) {
                fileBox = false;
                text = L"";
                return false;
            }
            else if (msg.message == WM_KEYDOWN) {
                if (msg.vkcode == VK_RETURN) {
                    fileBox = false;
                    text += L".txt";
                    return true;
                }
                else if (msg.vkcode == VK_BACK) {
                    if (!text.empty() && cursorPos > 0) {
                        text.erase(cursorPos - 1, 1);
                        cursorPos--;
                    }
                }
                else if (msg.vkcode == VK_LEFT) {
                    if (cursorPos > 0) cursorPos--;
                }
                else if (msg.vkcode == VK_RIGHT) {
                    if (cursorPos < text.length()) cursorPos++;
                }
                else if (msg.vkcode == VK_DELETE) {
                    if (cursorPos < text.length()) {
                        text.erase(cursorPos, 1);
                    }
                }
                else if (msg.vkcode == VK_HOME) {
                    cursorPos = 0;
                }
                else if (msg.vkcode == VK_END) {
                    cursorPos = text.length();
                }
                else {
                    if (msg.vkcode >= 'A' && msg.vkcode <= 'Z') {
                        text.insert(cursorPos, 1, (wchar_t)msg.vkcode);
                        cursorPos++;
                    }
                }
            }
        }
        return false;
    }

    void action() {
        ExMessage msg;
        if (peekmessage(&msg, EX_MOUSE | EX_KEY, false)) {
            if (msg.message == WM_KEYDOWN) {
                if (msg.vkcode == 'E') {
                    if (player->setWay(endPoint.x, endPoint.y)) {
                        soundPlayer.Play(L"FootmanYes3.wav", false);
                    }
                    return;
                }
                if (msg.vkcode == 'R') {
                    player->setPosition(startPoint.x, startPoint.y);
                    player->clearWay();
                    return;
                }
                if (msg.vkcode == 'T') {
                    tip = !tip;
                    return;
                }
                if (msg.vkcode == 'F') {
                    if (player->followWay()) {
                        player->setMoving(true);
                        soundPlayer.Play(L"Step.wav", false);
                        this_thread::sleep_for(chrono::milliseconds(150));
                    }
                    return;
                }
                if (msg.vkcode == 'G') {
                    levelOver = true;
                    player->clearWay();
                    return;
                }
                if (msg.vkcode == 'H') {
                    static random_device rd;
                    static mt19937 gen(rd());
                    uniform_int_distribution<int> dis(bufferAreaSize, mapWidth - bufferAreaSize - 1);
                    int x = dis(gen);
                    int y = dis(gen);
                    for (int i = x - 2; i <= x + 2; i++) {
                        for (int j = y - 2; j <= y + 2; j++) {
                            if (mapPoints[j][i].isCorridor) {
                                player->setPosition(i, j);
                                player->clearWay();
                                return;
                            }
                        }
                    }
                    return;
                }
                if (msg.vkcode == 'C') {
                    if (fileBoxAction()) {
                        if (filesystem::exists(text)) {
                            filesystem::remove(text);
                        }
                        text = L"";
                        cursorPos = 0;
                    }
                    return;
                }
                if (msg.vkcode == 'V') {
                    if (fileBoxAction()) {
                        saveGame();
                    }
                    return;
                }
                if (msg.vkcode == 'B') {
                    if (fileBoxAction()) {
                        if (filesystem::exists(text)) {
                            levelOver = true;
                            reloadNow = true;
                            player->clearWay();
                        }
                        else {
                            text = L"";
                            cursorPos = 0;
                        }
                    }
                    return;
                }
                if (player->setDirection(msg.vkcode)) {
                    if (player->moveTo()) {
                        player->clearWay();
                        player->setMoving(true);
                        soundPlayer.Play(L"Step.wav", false);
                        this_thread::sleep_for(chrono::milliseconds(100));
                    }
                    return;
                }
                if (msg.vkcode == 'Q') {
                    if (filesystem::exists("record.txt")) {
                        filesystem::remove("record.txt");
                    }
                }
                if (msg.vkcode == VK_ESCAPE) {
                    gameOver = true;
                    levelOver = true;
                    return;
                }
            }
            if (msg.message == WM_LBUTTONDOWN) {
                int x = (centerX - screenWidth / 2 + msg.x + areaSideLength / 2) / areaSideLength;
                int y = (centerY - screenHeight / 2 + msg.y + areaSideLength / 2) / areaSideLength;
                if (mapPoints[y][x].isCorridor) {
                    soundPlayer.Play(L"FootmanYes2.wav", false);
                    player->setWay(x, y);
                }
                return;
            }
            if (msg.message == WM_RBUTTONDOWN) {
                int x = (centerX - screenWidth / 2 + msg.x + areaSideLength / 2) / areaSideLength;
                int y = (centerY - screenHeight / 2 + msg.y + areaSideLength / 2) / areaSideLength;
                if (mapPoints[y][x].isCorridor) {
                    soundPlayer.Play(L"FootmanYes1.wav", false);
                    player->clearWay();
                    player->setPosition(x, y);
                }
                return;
            }
            if (msg.message == WM_MBUTTONDOWN) {
                int x = (centerX - screenWidth / 2 + msg.x + areaSideLength / 2) / areaSideLength;
                int y = (centerY - screenHeight / 2 + msg.y + areaSideLength / 2) / areaSideLength;
                if (mapPoints[y][x].isCorridor) {
                    soundPlayer.Play(L"FootmanYes4.wav", false);
                    mapPoints[y][x].isMarked = !mapPoints[y][x].isMarked;
                }
                return;
            }
        }
    }

    void update() {
        while (!levelOver) {
            action();
            auto position = player->getPosition();
            if (position.first == endPoint.x && position.second == endPoint.y) {
                level++;
                if (level > heighestLevel) {
                    heighestLevel = level;
                }
                player->setVictory(true);
                soundPlayer.Play(L"FootmanWarcry.wav", false);
                this_thread::sleep_for(chrono::milliseconds(1000));
                levelOver = true;
            }
            flushmessage();
            this_thread::sleep_for(chrono::milliseconds(5));
        }
    }

    void draw() {
        while (!levelOver) {
            buffer();
            this_thread::sleep_for(chrono::milliseconds(1000 / FPS));
        }
    }

    void bkSound() {
        while (!gameOver) {
            soundPlayer.Play(L"Human1.mp3", true);
            soundPlayer.Play(L"Human2.mp3", true);
            soundPlayer.Play(L"Human3.mp3", true);
        }
    }

    void start() {
        mapType = static_cast<MapType>(rand() % 2);
        generateMap();
        player->setPosition(startPoint.x, startPoint.y);
        soundPlayer.Play(L"FootmanReady.wav", false);
        drawThread = thread(&Game::draw, this);
        updateThread = thread(&Game::update, this);
        drawThread.join();
        updateThread.join();
        levelEnd();
    }

    void levelEnd() {
        if (gameOver) {
            closegraph();
        }
        else {
            restart();
        }
    }

    void restart() {
        if (reloadNow) {
            reloadGame();
            return;
        }
        if (level < 15) {
            mapWidth += bufferAreaSize;
            mapHeight += bufferAreaSize;
        }
        levelOver = false;
        player->setVictory(false);
        start();
    }

    ~Game() {
        ofstream score("record.txt", ios::out);
        if (score.is_open()) {
            score << heighestLevel << endl;
            score.close();
        }
        delete player;
    }
};

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);
    cout.tie(nullptr);

    initTerrain();
    initUI();

    Game game;

    return 0;
}