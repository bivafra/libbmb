#pragma once
/**
 * @file llist_nodes.h
 * Definitions of LinkedList nodes.
 * @authors bivafra
 */

namespace bmb {
namespace llist {

struct BaseNode {
    BaseNode* next = nullptr;
};

template <typename T>
struct Node : BaseNode {
    T val;
};

}  // namespace llist
}  // namespace bmb
