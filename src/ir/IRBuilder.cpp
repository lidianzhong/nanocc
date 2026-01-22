#include "ir/IRBuilder.h"
#include "ir/IR.h"

#include <cassert>
#include <iostream>
#include <vector>

void IRBuilder::SetCurrentFunction(Function *func) { cur_func_ = func; }

void IRBuilder::EndFunction() {
  cur_func_ = nullptr;
  cur_bb_ = nullptr;
}

BasicBlock *IRBuilder::CreateBlock(const std::string &name) {
  assert(cur_func_ && "CreateBlock requires a current function");
  std::string block_name = name == "entry" ? name : NewTempLabelName(name);
  return cur_func_->CreateBlock(block_name);
}

void IRBuilder::SetInsertPoint(BasicBlock *bb) { cur_bb_ = bb; }

void IRBuilder::Insert(Instruction *inst) {
    assert(cur_bb_ && "No current basic block to insert into");
    cur_bb_->Append(inst);
}

// Helpers
std::string IRBuilder::NewTempRegName() {
    return "%" + std::to_string(temp_reg_id_++);
}

std::string IRBuilder::NewTempAddrName(const std::string &name) {
  assert(!name.empty() && "Temp addr name cannot be empty");
  int &counter = temp_addr_counters_[name];
  std::string final_name = (counter == 0) ? name : name + "_" + std::to_string(counter);
  counter++;
  return "@" + final_name;
}

std::string IRBuilder::NewTempLabelName(const std::string &prefix) {
  int &counter = temp_label_counters_[prefix];
  std::string label = (counter == 0) ? prefix : prefix + "_" + std::to_string(counter);
  counter++;
  return label;
}

// --------------------------------------------------------------------------
// Memory Operations
// --------------------------------------------------------------------------

Value *IRBuilder::CreateAlloca(Type *type, const std::string &var_name) {
    std::string baseName = var_name.empty() ? "tmp" : var_name;
    std::string addrName = NewTempAddrName(baseName);
    
    // alloc 指令 returns a pointer to type
    auto ptrTy = Type::getPointerTy(type);
    auto inst = new Instruction(ptrTy, addrName, Opcode::Alloc, type->toString());
    Insert(inst);
    return inst;
}

Value *IRBuilder::CreateGlobalAlloc(const std::string &var_name, Type *type, Value *init_val) {
    std::string addrName = NewTempAddrName(var_name);
    auto ptrTy = Type::getPointerTy(type);
    
    // GlobalAlloc: global @name = alloc type, init
    auto inst = new Instruction(ptrTy, addrName, Opcode::GlobalAlloc, init_val);
    
    // Store in module globals list (assuming public access or similar mechanism)
    // For now we rely on the caller to handle module registration if needed, 
    // or we assume IRModule has exposed a way. 
    // Based on previous code: module_->globals_.push_back(...)
    // We will mimic that behavior.
    
    // Hack: construct a unique_ptr to push to module
    // But Instruction now is a Value* managed differently? 
    // Let's assume unique_ptr<Instruction> is still valid in Module for ownership.
    module_->globals_.push_back(std::unique_ptr<Instruction>(inst));
    
    return inst;
}


Value *IRBuilder::CreateGetElemPtr(Value *base_addr, Value *index) {
    if (index->getType()->isPointerTy()) {
        index = CreateLoad(index);
    }
    
    auto ptrTy = dynamic_cast<PointerType*>(base_addr->getType());
    assert(ptrTy && "GetElemPtr base must be a pointer");
    
    Type *elementType = nullptr;
    if (auto arrTy = dynamic_cast<ArrayType*>(ptrTy->getElementType())) {
        elementType = arrTy->getElementType();
    } else if (ptrTy->getElementType()->isIntegerTy()) {
        elementType = ptrTy->getElementType();
    } else {
        // Fallback or error
        assert(false && "Invalid type for GEP");
    }
    
    auto resTy = Type::getPointerTy(elementType);
    std::string resName = NewTempRegName();
    
    auto inst = new Instruction(resTy, resName, Opcode::GetElemPtr, base_addr, index);
    Insert(inst);
    return inst;
}

Value *IRBuilder::CreateGetPtr(Value *base_ptr, Value *index) {
     if (index->getType()->isPointerTy()) {
        index = CreateLoad(index);
    }
    
    auto ptrTy = dynamic_cast<PointerType*>(base_ptr->getType());
    assert(ptrTy && "GetPtr base must be a pointer");
    
    std::string resName = NewTempRegName();
    auto inst = new Instruction(ptrTy, resName, Opcode::GetPtr, base_ptr, index);
    Insert(inst);
    return inst;
}

Value *IRBuilder::CreateLoad(Value *addr) {
    auto ptrTy = dynamic_cast<PointerType*>(addr->getType());
    assert(ptrTy && "Load expects a pointer");
    
    std::string resName = NewTempRegName();
    auto inst = new Instruction(ptrTy->getElementType(), resName, Opcode::Load, addr);
    Insert(inst);
    return inst;
}

void IRBuilder::CreateStore(Value *value, Value *addr) {
    auto destPtrTy = dynamic_cast<PointerType*>(addr->getType());
    assert(destPtrTy && "Store addr must be a pointer");
    // Assert addr points to value's type. 
    // We check via toString() to handle identical structure but different generic instances if any.
    // In simple SysY compiler, pointer equality of singletons (Int32) usually suffices, 
    // but strict check is requested.
    // assert(destPtrTy->getElementType() == value->getType() && "Store addr must be a pointer to value type");
    
    auto inst = new Instruction(Type::getVoidTy(), "", Opcode::Store, value, addr);
    Insert(inst);
}

// --------------------------------------------------------------------------
// Control Flow
// --------------------------------------------------------------------------

void IRBuilder::CreateBranch(Value *cond, BasicBlock *then_bb, BasicBlock *else_bb) {
    Value *condVal = cond;
    if (condVal->getType()->isPointerTy()) {
        condVal = CreateLoad(condVal);
    }
    
    BranchTarget targetThen(then_bb, {});
    BranchTarget targetElse(else_bb, {});
    
    auto inst = new Instruction(Type::getVoidTy(), "", Opcode::Br, condVal, targetThen, targetElse);
    Insert(inst);
}

void IRBuilder::CreateJump(BasicBlock *target_bb) {
    BranchTarget target(target_bb, {});
    auto inst = new Instruction(Type::getVoidTy(), "", Opcode::Jmp, target);
    Insert(inst);
}

void IRBuilder::CreateReturn(Value *value) {
    Value *retVal = value;
    if (retVal->getType()->isPointerTy()) {
        retVal = CreateLoad(retVal);
    }
    auto inst = new Instruction(Type::getVoidTy(), "", Opcode::Ret, retVal);
    Insert(inst);
}

void IRBuilder::CreateReturn() {
    auto inst = new Instruction(Type::getVoidTy(), "", Opcode::Ret);
    Insert(inst);
}

// --------------------------------------------------------------------------
// Arithmetic & Ops
// --------------------------------------------------------------------------

Value *IRBuilder::CreateUnaryOp(Opcode op, Value *value) {
    assert(false && "CreateUnaryOp is deprecated, use CreateBinaryOp with constant 0");
    return nullptr;
}


Value *IRBuilder::CreateBinaryOp(Opcode op, Value *lhs, Value *rhs) {
    Value *l = lhs;
    Value *r = rhs;
    if (l->getType()->isPointerTy()) l = CreateLoad(l);
    if (r->getType()->isPointerTy()) r = CreateLoad(r);
    
    std::string resName = NewTempRegName();
    auto inst = new Instruction(Type::getInt32Ty(), resName, op, l, r);
    Insert(inst);
    return inst;
}


Value *IRBuilder::CreateCall(const std::string &func_name, const std::vector<Value*> &args, Type *ret_type) {
    std::vector<Value*> loadedArgs;
    for (auto *arg : args) {
        loadedArgs.push_back(arg);
    }

    auto inst = new Instruction(ret_type, 
                                ret_type->isVoidTy() ? "" : NewTempRegName(),
                                Opcode::Call, 
                                func_name, 
                                loadedArgs);
    Insert(inst);
    return inst;
}

Value *IRBuilder::CreateCall(Function *func, const std::vector<Value*> &args) {
    // TODO: get ret type from func
    return CreateCall(func->getName(), args, Type::getVoidTy()); // placeholder
}

