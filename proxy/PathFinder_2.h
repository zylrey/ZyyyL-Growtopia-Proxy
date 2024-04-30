#include <iostream>
#include <queue>
#include <vector>
#include <algorithm>
#include <climits>

using namespace std;

class PathFinder {
private:
    int ROW, COL;
    vector<int> dx, dy;
    vector<pair<int, int>> blockedCells;

public:
    PathFinder(int ROW, int COL) : ROW(ROW), COL(COL) {}

    struct Node {
        int x, y;
        int g, h;
        Node(int x, int y, int g, int h) : x(x), y(y), g(g), h(h) {}
        bool operator<(const Node& n) const { return g + h > n.g + n.h; }
    };

    void setBlocked(int x, int y) {
        blockedCells.push_back({ x, y });
    }


    bool isBlocked(int x, int y) {
        for (auto& p : blockedCells) {
            if (p.first == x && p.second == y) {
                return true;
            }
        }
        return false;
    }


    void setNeighbors(vector<int> dx, vector<int> dy) {
        this->dx = dx;
        this->dy = dy;
    }

    vector<pair<int, int>> getPath(int startX, int startY, int finishX, int finishY, const vector<vector<pair<int, int>>>& cameFrom) {
        vector<pair<int, int>> path;
        int x = finishX, y = finishY;
        while (x != startX || y != startY) {
            path.push_back({ x, y });
            int x_prev = cameFrom[x][y].first;
            int y_prev = cameFrom[x][y].second;
            x = x_prev;
            y = y_prev;
        }
        path.push_back({ startX, startY });
        reverse(path.begin(), path.end());
        return path;
    }

    vector<pair<int, int>> aStar(int startX, int startY, int finishX, int finishY) {
        vector<vector<int>> g(ROW, vector<int>(COL, INT_MAX));
        vector<vector<int>> h(ROW, vector<int>(COL, INT_MAX));
        vector<vector<pair<int, int>>> cameFrom(ROW, vector<pair<int, int>>(COL));
        priority_queue<Node> q;

        g[startX][startY] = 0;
        h[startX][startY] = max(abs(finishX - startX), abs(finishY - startY));
        q.push(Node(startX, startY, g[startX][startY], h[startX][startY]));

        while (!q.empty()) {
            Node curr = q.top();
            q.pop();

            if (curr.x == finishX && curr.y == finishY) {
                vector<pair<int, int>> path;
                while (curr.x != startX || curr.y != startY) {
                    path.push_back({ curr.x, curr.y });
                    curr = Node(cameFrom[curr.x][curr.y].first, cameFrom[curr.x][curr.y].second, g[curr.x][curr.y], h[curr.x][curr.y]);
                }
                path.push_back({ startX, startY });
                reverse(path.begin(), path.end());
                return path;
            }
            for (int i = 0; i < dx.size(); ++i) {
                int x_ = curr.x + dx[i], y_ = curr.y + dy[i];
                if (x_ < 0 || x_ >= ROW || y_ < 0 || y_ >= COL) {
                    continue;
                }

                if (isBlocked(x_, y_)) {
                    continue;
                }

                int gScore = g[curr.x][curr.y] + 1;
                if (gScore >= g[x_][y_]) {
                    continue;
                }

                cameFrom[x_][y_] = { curr.x, curr.y };
                g[x_][y_] = gScore;
                h[x_][y_] = abs(finishX - x_) + abs(finishY - y_);
                q.push(Node(x_, y_, g[x_][y_], h[x_][y_]));
            }
        }
        return {};
    }
};
