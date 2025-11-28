#ifndef PATHFINDER_2_H
#define PATHFINDER_2_H

#include <algorithm>
#include <climits>
#include <queue>
#include <utility>
#include <vector>

class PathFinder {
private:
    int ROW, COL;
    std::vector<int> dx, dy;
    std::vector<std::pair<int, int>> blockedCells;

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


    void setNeighbors(std::vector<int> dx, std::vector<int> dy) {
        this->dx = std::move(dx);
        this->dy = std::move(dy);
    }

    std::vector<std::pair<int, int>> getPath(int startX, int startY, int finishX, int finishY, const std::vector<std::vector<std::pair<int, int>>>& cameFrom) {
        std::vector<std::pair<int, int>> path;
        int x = finishX, y = finishY;
        while (x != startX || y != startY) {
            path.push_back({ x, y });
            int x_prev = cameFrom[x][y].first;
            int y_prev = cameFrom[x][y].second;
            x = x_prev;
            y = y_prev;
        }
        path.push_back({ startX, startY });
        std::reverse(path.begin(), path.end());
        return path;
    }

    std::vector<std::pair<int, int>> aStar(int startX, int startY, int finishX, int finishY) {
        std::vector<std::vector<int>> g(ROW, std::vector<int>(COL, INT_MAX));
        std::vector<std::vector<int>> h(ROW, std::vector<int>(COL, INT_MAX));
        std::vector<std::vector<std::pair<int, int>>> cameFrom(ROW, std::vector<std::pair<int, int>>(COL));
        std::priority_queue<Node> q;

        g[startX][startY] = 0;
        h[startX][startY] = std::max(std::abs(finishX - startX), std::abs(finishY - startY));
        q.push(Node(startX, startY, g[startX][startY], h[startX][startY]));

        while (!q.empty()) {
            Node curr = q.top();
            q.pop();

            if (curr.x == finishX && curr.y == finishY) {
                std::vector<std::pair<int, int>> path;
                while (curr.x != startX || curr.y != startY) {
                    path.push_back({ curr.x, curr.y });
                    curr = Node(cameFrom[curr.x][curr.y].first, cameFrom[curr.x][curr.y].second, g[curr.x][curr.y], h[curr.x][curr.y]);
                }
                path.push_back({ startX, startY });
                std::reverse(path.begin(), path.end());
                return path;
            }
            for (std::size_t i = 0; i < dx.size(); ++i) {
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
                h[x_][y_] = std::abs(finishX - x_) + std::abs(finishY - y_);
                q.push(Node(x_, y_, g[x_][y_], h[x_][y_]));
            }
        }
        return {};
    }
};

#endif // PATHFINDER_2_H
