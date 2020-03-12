#include <GL/glew.h>

#include "picker.h"

using namespace std;

Picker::Picker(const RigTForm& initialRbt, const ShaderState& curSS)
  : drawer_(initialRbt, curSS)
  , idCounter_(0)
  , srgbFrameBuffer_(true) {}

Picker::Picker(const RigTForm& initialRbt, const ShaderState& curSS,shared_ptr<SgRbtNode> selectedNode,shared_ptr<SgRootNode> worldNode,RigTForm eyeRbt,RigTForm motionRbt):drawer_(initialRbt, curSS)
, idCounter_(0)
, srgbFrameBuffer_(true)
, selectedRbtNode_(selectedNode)
, worldNode_(worldNode)
, eyeRbt_(eyeRbt)
, motionRbt_(motionRbt) {}


bool Picker::visit(SgTransformNode& node) {
  shared_ptr<SgNode> baseNode = node.shared_from_this();
  nodeStack_.push_back(baseNode);
    if(selectedRbtNode_ == baseNode){
        shared_ptr<SgRbtNode> currentNode = dynamic_pointer_cast<SgRbtNode>(baseNode);
        RigTForm nodeRbt = getPathAccumRbt(worldNode_, currentNode);
        //auxiliary frame wrt nodeRbt and eyeRbt
        RigTForm aRbt = makeMixedFrame(nodeRbt, eyeRbt_);
        RigTForm nodeParentRbt = getPathAccumRbt(worldNode_, currentNode,1);
        RigTForm asRbt = inv(nodeParentRbt) * aRbt;
        RigTForm nodeMotionRbt = doQtoOwrtA(motionRbt_, node.getRbt(), asRbt);
        currentNode->setRbt(nodeMotionRbt);
        return drawer_.visit(*currentNode);
    }
    
  return drawer_.visit(node);
}

bool Picker::postVisit(SgTransformNode& node) {
  nodeStack_.pop_back();
  return drawer_.postVisit(node);
}

bool Picker::visit(SgShapeNode& node) {
  // We will increment idcounter every time visit a shape node
    ++idCounter_;
    Cvec3 idColor = idToColor(idCounter_);

    shared_ptr<SgNode> baseNode = nodeStack_.back();
    shared_ptr<SgRbtNode> rbtNode = dynamic_pointer_cast<SgRbtNode>(baseNode);
    addToMap(idCounter_, rbtNode);
    //为选中的节点指定颜色
    if(selectedRbtNode_ == baseNode){
        safe_glUniform3f(drawer_.getCurSS().h_uIdColor, 0.3, 0.9, 0.5);
    }else{
    safe_glUniform3f(drawer_.getCurSS().h_uIdColor, idColor[0], idColor[1], idColor[2]);
    }
    
  return drawer_.visit(node);
}

bool Picker::postVisit(SgShapeNode& node) {
  // TODO
  return drawer_.postVisit(node);
}

//此处初始化raycaster，然后对几何体执行raycast动作
shared_ptr<SgRbtNode> Picker::getRbtNodeAtXY(int x, int y) {
    vector<char> image(3);
    PackedPixel pixel;
//    glReadBuffer(GL_BACK);
    glReadPixels(x,y, 1, 1, GL_RGB, GL_UNSIGNED_BYTE, &pixel);
    int colorId = colorToId(pixel);
    shared_ptr<SgRbtNode> jointNode = find(colorId);
  return jointNode;
}

//------------------
// Helper functions
//------------------
//
void Picker::addToMap(int id, shared_ptr<SgRbtNode> node) {
  idToRbtNode_[id] = node;
}

shared_ptr<SgRbtNode> Picker::find(int id) {
  IdToRbtNodeMap::iterator it = idToRbtNode_.find(id);
  if (it != idToRbtNode_.end())
    return it->second;
  else
    return shared_ptr<SgRbtNode>(); // set to null
}

// encode 2^4 = 16 IDs in each of R, G, B channel, for a total of 16^3 number of objects
static const int NBITS = 4, N = 1 << NBITS, MASK = N-1;

Cvec3 Picker::idToColor(int id) {
  assert(id > 0 && id < N * N * N);
  Cvec3 framebufferColor = Cvec3(id & MASK, (id >> NBITS) & MASK, (id >> (NBITS+NBITS)) & MASK);
  framebufferColor = framebufferColor / N + Cvec3(0.5/N);

  if (!srgbFrameBuffer_)
    return framebufferColor;
  else {
    // if GL3 is used, the framebuffer will be in SRGB format, and the color we supply needs to be in linear space
    Cvec3 linearColor;
    for (int i = 0; i < 3; ++i) {
      linearColor[i] = framebufferColor[i] <= 0.04045 ? framebufferColor[i]/12.92 : pow((framebufferColor[i] + 0.055)/1.055, 2.4);
    }
    return linearColor;
  }
}

int Picker::colorToId(const PackedPixel& p) {
  const int UNUSED_BITS = 8 - NBITS;
  int id = p.r >> UNUSED_BITS;
  id |= ((p.g >> UNUSED_BITS) << NBITS);
  id |= ((p.b >> UNUSED_BITS) << (NBITS+NBITS));
  return id;
}
