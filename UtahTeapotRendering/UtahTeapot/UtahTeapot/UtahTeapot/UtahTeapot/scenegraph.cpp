#include <algorithm>

#include "scenegraph.h"

using namespace std;

//accept的实现逻辑，只要有visit权限，就可以一直向深层授权
bool SgTransformNode::accept(SgNodeVisitor& visitor) {
  if (!visitor.visit(*this))
    return false;
  for (long i = 0, n = children_.size(); i < n; ++i) {
    if (!children_[i]->accept(visitor))
      return false;
  }
  return visitor.postVisit(*this);
}

void SgTransformNode::addChild(shared_ptr<SgNode> child) {
  children_.push_back(child);
}

void SgTransformNode::removeChild(shared_ptr<SgNode> child) {
  children_.erase(find(children_.begin(), children_.end(), child));
}

bool SgShapeNode::accept(SgNodeVisitor& visitor) {
  if (!visitor.visit(*this))
    return false;
  return visitor.postVisit(*this);
}

class RbtAccumVisitor : public SgNodeVisitor {
protected:
  vector<RigTForm> rbtStack_;
  SgTransformNode& target_;
  bool found_;
public:
  RbtAccumVisitor(SgTransformNode& target)
    : target_(target)
    , found_(false) {}

  const RigTForm getAccumulatedRbt(int offsetFromStackTop = 0,int offsetFromStackBottom = 0) {
      RigTForm accumulatedRbt;
      long stackSize = rbtStack_.size();
      //g_world is just a root node and it is not a rbtnode,so we dont need to explicitly exclude g_world(source) node
      if((stackSize - offsetFromStackBottom - offsetFromStackTop)>=1 ){
          for(auto start=begin(rbtStack_) + offsetFromStackBottom , last = end(rbtStack_) - offsetFromStackTop ; start!=last; ++start ){
              accumulatedRbt = accumulatedRbt * (*start) ;
          }
      }
      
      return accumulatedRbt;
  }

  virtual bool visit(SgTransformNode& node) {
//      shared_ptr<SgRbtNode> rbtNode = dynamic_pointer_cast<SgRbtNode>(node.shared_from_this());
//      if(rbtNode!=NULL){
//          rbtStack_.push_back(rbtNode->getRbt());
//      }
      rbtStack_.push_back(node.getRbt());
      if(node == target_)
          return false;
      else
          return true;
  }

  virtual bool postVisit(SgTransformNode& node) {
      //rbtStack_.pop_back();
      if(node == target_){
          return false;
      }else{
          rbtStack_.pop_back();
          return true;
      }
  }
};

RigTForm getPathAccumRbt(
  shared_ptr<SgTransformNode> source,
  shared_ptr<SgTransformNode> destination,
  int offsetFromDestination,int offsetFromStackBottom) {

  RbtAccumVisitor accum(*destination);
  source->accept(accum);
  return accum.getAccumulatedRbt(offsetFromDestination,offsetFromStackBottom);
}
