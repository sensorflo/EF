#include "Ast.h"

ElementsNode::ElementsNode() = default;

auto ElementsNode::Add(std::unique_ptr<Node> iNode) -> void {
    mElements.push_back(std::move(iNode));
}