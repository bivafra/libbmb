#pragma once
/**
 * @file llist_nodes.h
 * Definitions of LinkedList nodes.
 * @authors bivafra
 */

#include "utils/move.h"

namespace bmb {
namespace llist {

struct BaseNode {
    BaseNode* next = nullptr;
};

template <typename T>
struct Node : BaseNode {
    // Enales construction of `val` from
    // given args
    template <typename... Args>
    Node(Args&&... args)
        : val(forward<Args>(args)...) {}

    T val;
};

}  // namespace llist
}  // namespace bmb
