
#include <iostream>
#include <set>

#include "message.hpp"
#include "fakeRPC.hpp"
#include "quorum.hpp"
#include "slot.hpp"
#include "node.hpp"
#include <chrono>
#include <stdio.h>
using namespace DISTPROJ;

Node::Node(NodeID _id, RPCLayer& _rpc) 
  : id(_id), rpc(_rpc), t(nullptr) {
  rpc.AddNode(id);
}

Node::Node(NodeID _id, RPCLayer& _rpc, Quorum _quorumSet) 
  : id(_id), rpc(_rpc), quorumSet(_quorumSet), t(nullptr) {
  rpc.AddNode(id);
}

NodeID Node::GetNodeID() { 
  return id; 
}

Quorum Node::GetQuorumSet() {
  return quorumSet;
}

void Node::PrintQuorumSet() {
  printf("Printing quorum set for node %llu \n", id);
  printf("Threshold: %i ", quorumSet.threshold);
  printf("Quorum members : \n");
  std::set<NodeID>::iterator iter;
  for (iter=quorumSet.members.begin(); iter != quorumSet.members.end(); ++iter) {
    std::cout << (*iter) << "\n";
  }
}

LocalNode::LocalNode(NodeID _id, RPCLayer& _rpc)
  : Node(_id, _rpc) {
  mc = _rpc.GetClient(_id);
};
LocalNode::LocalNode(NodeID _id, RPCLayer& _rpc, Quorum _quorumSet) 
  : Node(_id, _rpc, _quorumSet) {
  mc = _rpc.GetClient(_id);
}; 

void LocalNode::Tick() {
  std::shared_ptr<Message> m;
  while (true){
	mtx.lock();
    if (ReceiveMessage(&m)) {
      ProcessMessage(m);


    }
	mtx.unlock();
    std::this_thread::sleep_for(std::chrono::seconds(1));
  }
}

void LocalNode::Start() {
  printf(" Start\n");
  if (t == nullptr) {
    t = new std::thread(&LocalNode::Tick, this);
  }
}

void LocalNode::AddKnownNode(NodeID v) {
  knownNodes.insert(v);
}

void LocalNode::UpdateQurorum(Quorum _quorumSet) {
  quorumSet = _quorumSet;
}

void LocalNode::AddNodeToQuorum(NodeID v) {
  quorumSet.members.insert(v);
}

SlotNum LocalNode::Propose(std::string value){
  mtx.lock();
  auto i = NewSlot();
  maxSlot++;
  auto b = Ballot{1, value};
  printf("Finding Nonce\n");
  auto nonce = generateNonce(&b, i);
  printf("Nonce Found %i\n", nonce);
  auto m = std::make_shared<PrepareMessage>(id, i, b, Ballot{}, Ballot{}, Ballot{}, quorumSet, 0); /* TODO; resending etc */
  SendMessage(m);
  mtx.unlock();
  return i;
}
SlotNum LocalNode::NewSlot(){
  return maxSlot+1;
}

void LocalNode::SendMessage(std::shared_ptr<Message> msg) {
  // TODO : interface with FakeRPC.
  mc->Broadcast(msg);
}

bool LocalNode::ReceiveMessage(std::shared_ptr<Message>* msg) {
  bool received = mc->Receive(msg);
  if (received && msg) {

    // PRINT here just to show we got it 
    printf("Got a message\n");
    return true;
  }
  return false;
}

void LocalNode::ProcessMessage(std::shared_ptr<Message> msg) {
  auto slot = msg -> getSlot();
  if (log.find(slot) == log.end()) {
    log[slot] = new Slot(slot, this);
	if (slot > maxSlot) {
	  maxSlot = slot;
	}
  }
  log[msg->getSlot()]->handle(msg);
}

void LocalNode::DumpLog() {
  for (auto slot : log) {
    slot.second->Dump();
  }
}



std::pair<std::string, bool> LocalNode::View(SlotNum s){
  std::lock_guard<std::mutex> lock(mtx);
  try{
    if (log.at(s)->GetPhase() == EXTERNALIZE){
      return std::pair<std::string, bool>(log.at(s)->GetValue(), true);
    }
  } catch (std::out_of_range){
  }
  return std::pair<std::string, bool>("", false);
}
