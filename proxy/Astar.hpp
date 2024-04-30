/*
    Copyright (c) 2015, Damian Barczynski <daan.net@wp.eu>
    Following tool is licensed under the terms and conditions of the ISC license.
    For more information visit https://opensource.org/licenses/ISC.
*/
#ifndef __ASTAR_HPP_8F637DB91972F6C878D41D63F7E7214F__
#define __ASTAR_HPP_8F637DB91972F6C878D41D63F7E7214F__

#include <vector>
#include <functional>
#include <set>

namespace AStar
{
    struct CL_Vec2f
    {
        int x, y;

        bool operator == (const CL_Vec2f& coordinates_);
    };

    using uint = unsigned int;
    using HeuristicFunction = std::function<uint(CL_Vec2f, CL_Vec2f)>;
    using CoordinateList = std::vector<CL_Vec2f>;

    struct Node
    {
        uint G, H;
        CL_Vec2f coordinates;
        Node* parent;

        Node(CL_Vec2f coord_, Node* parent_ = nullptr);
        uint getScore();
    };

    using NodeSet = std::vector<Node*>;

    class Generator
    {
        bool detectCollision(CL_Vec2f coordinates_);
        Node* findNodeOnList(NodeSet& nodes_, CL_Vec2f coordinates_);
        void releaseNodes(NodeSet& nodes_);

    public:
        Generator();
        void setWorldSize(CL_Vec2f worldSize_);
        void setDiagonalMovement(bool enable_);
        void setHeuristic(HeuristicFunction heuristic_);
        CoordinateList findPath(CL_Vec2f source_, CL_Vec2f target_);
        void addCollision(CL_Vec2f coordinates_);
        void removeCollision(CL_Vec2f coordinates_);
        void clearCollisions();

    private:
        HeuristicFunction heuristic;
        CoordinateList direction, walls;
        CL_Vec2f worldSize;
        uint directions;
    };

    class Heuristic
    {
        static CL_Vec2f getDelta(CL_Vec2f source_, CL_Vec2f target_);

    public:
        static uint manhattan(CL_Vec2f source_, CL_Vec2f target_);
    };
}

#endif // __ASTAR_HPP_8F637DB91972F6C878D41D63F7E7214F__