#include "circt/Dialect/Synth/IR/IntermediateGraph.h"

namespace circt {
namespace synth {

Transition::Transition(std::shared_ptr<Node> from, std::shared_ptr<Node> to, Dependencies deps): origin(from), destination(to), deps(deps) {}
std::shared_ptr<Node> Transition::getOrigin() const {return origin;}
std::shared_ptr<Node> Transition::getDestination() const {return destination;}
Dependencies Transition::getDependencies() const {return deps;}
std::string Transition::toString() const {
    return "from " + origin.get()->getName() + " to " + destination.get()->getName() + " depending on " + deps.toString();
}

Node::Node(const std::string name) : name(name) {};
const std::string Node::getName() const {return name;}
void Node::addIncomingTransition(std::shared_ptr<Transition> transition) {
    incomingTransitions.insert(transition);
}
void Node::addOutgoingTransition(std::shared_ptr<Transition> transition) {
    outgoingTransitions.insert(transition);
}

std::vector<std::shared_ptr<Transition>> Node::getIncomingTransitions() const {
    std::vector<std::shared_ptr<Transition>> result(incomingTransitions.begin(), incomingTransitions.end());
    return result;
}
std::vector<std::shared_ptr<Transition>> Node::getOutgoingTransitions() const {
    std::vector<std::shared_ptr<Transition>> result(outgoingTransitions.begin(), outgoingTransitions.end());
    return result;
}

//IntermediateGraph::IntermediateGraph(const std::string& initialNode) : first(this->getOrAddNode(initialNode)) {};
 IntermediateGraph::IntermediateGraph(const std::string initialNode) {
     std::shared_ptr<Node> n = std::make_shared<Node>(Node(initialNode));
     nodes.insert({initialNode, n});
     first = n;
 };
void IntermediateGraph::addTransition(const std::string from, const std::string to, Dependencies deps) {
    std::shared_ptr<Node> fromNode = this->getOrAddNode(from);
    std::shared_ptr<Node> toNode = this->getOrAddNode(to);
    std::shared_ptr<Transition> t = std::make_shared<Transition>(Transition(fromNode, toNode, deps));
    transitions.insert(t);
}

std::shared_ptr<Node> IntermediateGraph::getFirstNode() const {return first;}
std::vector<std::shared_ptr<Transition>> IntermediateGraph::getTransitions() const {
    std::vector<std::shared_ptr<Transition>> result(transitions.begin(), transitions.end());
    return result;
}
std::shared_ptr<Node> IntermediateGraph::getNode(const std::string name) const {
    return nodes.at(name);
}
std::string IntermediateGraph::toString() const {
    std::string result = "";
    for (auto t : transitions) {
        result += t.get()->toString() + "\n";
    }
    return result;
    //return "TODO: IMPLEMENT";//TODO: implement
}
Dependencies IntermediateGraph::lubTransitionDependencies() {
    Dependencies result = Dependencies();
    for (auto t : transitions) {
        result = Dependencies::leastUpperBound(result, t.get()->getDependencies());
    }
    return result;
}

std::shared_ptr<Node> IntermediateGraph::getOrAddNode(const std::string name) {
    auto n = nodes.find(name);
    if (n != nodes.end()) {
        return n->second;
    } else {
        std::shared_ptr<Node> newNode = std::make_shared<Node>(name);
        nodes.insert({name, newNode});
        return newNode;
    }
}



} // namespace synth
} // namespace circt