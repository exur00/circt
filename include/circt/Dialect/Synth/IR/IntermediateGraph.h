#include "circt/Dialect/Synth/IR/DependencySemiLattice.h"


namespace circt {
namespace synth {

class Node;

class Transition {
public:
    Transition(std::shared_ptr<Node> from, std::shared_ptr<Node> to, Dependencies deps);
    std::shared_ptr<Node> getOrigin() const;
    std::shared_ptr<Node> getDestination() const;
    Dependencies getDependencies() const;
    std::string toString() const;
private:
    //Node* origin;
    std::shared_ptr<Node> origin;
    //Node* destination;
    std::shared_ptr<Node> destination;
    Dependencies deps;
};

class Node {
public:
    Node(const std::string& name);

    const std::string& getName() const;

    void addIncomingTransition(std::shared_ptr<Transition> transition);
    void addOutgoingTransition(std::shared_ptr<Transition> transition);

    std::vector<std::shared_ptr<Transition>> getIncomingTransitions() const;
    std::vector<std::shared_ptr<Transition>> getOutgoingTransitions() const;

private:
    const std::string& name;
    std::set<std::shared_ptr<Transition>> incomingTransitions;
    std::set<std::shared_ptr<Transition>> outgoingTransitions;
};

class IntermediateGraph {
public:
    IntermediateGraph(const std::string& initialNode);

    void addTransition(const std::string& from, const std::string& to, Dependencies deps);

    std::shared_ptr<Node> getFirstNode() const;
    std::vector<std::shared_ptr<Transition>> getTransitions() const;
    std::shared_ptr<Node> getNode(const std::string& name) const;
    std::string toString() const;
    Dependencies lubTransitionDependencies();

private:
    std::shared_ptr<Node> getOrAddNode(const std::string& name);

    std::shared_ptr<Node> first;
    std::set<std::shared_ptr<Transition>> transitions;
    std::map<std::string, std::shared_ptr<Node>> nodes;
};

} // namespace synth
} // namespace circt