#include "AStar.hpp"
#include <algorithm>
#include <math.h>

using namespace std::placeholders;

bool AStar::CL_Vec2f::operator == (const CL_Vec2f& coordinates_)
{
    return (x == coordinates_.x && y == coordinates_.y);
}

AStar::CL_Vec2f operator + (const AStar::CL_Vec2f& left_, const AStar::CL_Vec2f& right_)
{
    return{ left_.x + right_.x, left_.y + right_.y };
}

AStar::Node::Node(CL_Vec2f coordinates_, Node* parent_)
{
    parent = parent_;
    coordinates = coordinates_;
    G = H = 0;
}

AStar::uint AStar::Node::getScore()
{
    return G + H;
}

AStar::Generator::Generator()
{
    setDiagonalMovement(false);
    setHeuristic(&Heuristic::manhattan);
    direction = {
        { 0, 1 }, { 1, 0 }, { 0, -1 }, { -1, 0 },
        { -1, -1 }, { 1, 1 }, { -1, 1 }, { 1, -1 },
        { 0, 2 }, { 2, 0 }, { 0, -2 }, { -2, 0 },
        { -2, -2 }, { 2, 2 }, { -2, 2 }, { 2, -2 }
    };
}

void AStar::Generator::setWorldSize(CL_Vec2f worldSize_)
{
    worldSize = worldSize_;
}

void AStar::Generator::setDiagonalMovement(bool enable_)
{
    directions = (enable_ ? 8 : 4);
}

void AStar::Generator::setHeuristic(HeuristicFunction heuristic_)
{
    heuristic = std::bind(heuristic_, _1, _2);
}

void AStar::Generator::addCollision(CL_Vec2f coordinates_)
{
    walls.push_back(coordinates_);
}

void AStar::Generator::removeCollision(CL_Vec2f coordinates_)
{
    auto it = std::find(walls.begin(), walls.end(), coordinates_);
    if (it != walls.end()) {
        walls.erase(it);
    }
}

void AStar::Generator::clearCollisions()
{
    walls.clear();
}

AStar::CoordinateList AStar::Generator::findPath(CL_Vec2f source_, CL_Vec2f target_)
{
    Node* current = nullptr;
    NodeSet openSet, closedSet;
    openSet.reserve(100);
    closedSet.reserve(100);
    openSet.push_back(new Node(source_));

    while (!openSet.empty())
    {

        auto current_it = openSet.begin();
        current = *current_it;

        if (current->coordinates == target_)
            break;

        closedSet.push_back(current);
        openSet.erase(current_it);

        for (uint i = 0; i < directions; ++i)
        {
            CL_Vec2f newCoordinates(current->coordinates + direction[i]);
            if (detectCollision(newCoordinates) || findNodeOnList(closedSet, newCoordinates))
                continue;


            uint totalCost = current->G + ((i < 4) ? 10 : 12);

            Node* successor = findNodeOnList(openSet, newCoordinates);
            if (successor == nullptr)
            {
                successor = new Node(newCoordinates, current);
                successor->G = totalCost;
                successor->H = heuristic(successor->coordinates, target_);
                openSet.push_back(successor);
            }
            else if (totalCost < successor->G)
            {
                successor->parent = current;
                successor->G = totalCost;
            }
        }
    }

    CoordinateList path;
    if (current->coordinates == target_)
    {
        while (current != nullptr)
        {
            path.push_back(current->coordinates);
            current = current->parent;
        }
    }

    releaseNodes(openSet);
    releaseNodes(closedSet);
    return path;
}

AStar::Node* AStar::Generator::findNodeOnList(NodeSet& nodes_, CL_Vec2f coordinates_)
{
    for (auto node : nodes_) {
        if (node->coordinates == coordinates_) {
            return node;
        }
    }
    return nullptr;
}

void AStar::Generator::releaseNodes(NodeSet& nodes_)
{
    for (auto it = nodes_.begin(); it != nodes_.end();) {
        delete* it;
        it = nodes_.erase(it);
    }
}

bool AStar::Generator::detectCollision(CL_Vec2f coordinates_)
{
    if (coordinates_.x < 0 || coordinates_.x >= worldSize.x ||
        coordinates_.y < 0 || coordinates_.y >= worldSize.y ||
        std::find(walls.begin(), walls.end(), coordinates_) != walls.end()) {
        return true;
    }
    return false;
}

AStar::CL_Vec2f AStar::Heuristic::getDelta(CL_Vec2f source_, CL_Vec2f target_)
{
    return{ abs(source_.x - target_.x),  abs(source_.y - target_.y) };
}

AStar::uint AStar::Heuristic::manhattan(CL_Vec2f source_, CL_Vec2f target_)
{
    auto delta = std::move(getDelta(source_, target_));
    return static_cast<uint>(10 * (delta.x + delta.y));
}

