#include "Object.h"

#include <string>

using namespace llvm;
using namespace std;

ObjLoc::ObjLoc(ObjId objId, offset_t offset) : objId(objId), offset(offset)
{
}

bool ObjLoc::operator<(const ObjLoc &rhs) const
{
    if (this->objId == rhs.objId)
    {
        return this->offset < rhs.offset;
    }
    else
    {
        return this->objId < rhs.objId;
    }
}

StackObject::StackObject(ObjId id, uint64_t size) : id(id), size(size)
{
}

string StackObject::toString() const
{
    string res;
    res += "Stack(";

    res += "id=";
    res += to_string(this->id);
    res += ", ";

    res += "size=";
    res += to_string(this->size);

    res += ")";

    return res;
}

HeapObject::HeapObject(ObjId id, InstLoc allocationSite) : id(id), allocationSite(allocationSite)
{
}

string HeapObject::toString() const
{
    string res;
    res += "Heap(";
    res += this->allocationSite.toString();
    res += ")";
    return res;
}

GlbObject::GlbObject(ObjId id, GlobalVariable *glbVar) : id(id), glbVar(glbVar)
{
}

string GlbObject::toString() const
{
    string res;
    res += "Global(id=";
    res += to_string(this->id);
    res += ")";
    return res;
}

DummyObject::DummyObject(ObjId id) : id(id)
{
}

string DummyObject::toString() const
{
    string res;
    res += "Dummy(id=";
    res += to_string(this->id);
    res += ")";
    return res;
}

ObjId ObjectManager::currObjId = 0;
map<ObjId, Kind> ObjectManager::objKindMap;
StackObjectSet ObjectManager::stackObjSet;
HeapObjectSet ObjectManager::heapObjSet;
GlbObjectSet ObjectManager::glbObjSet;
DummyObjectSet ObjectManager::dummyObjSet;

ObjId ObjectManager::createStackObject(uint64_t size)
{
    ObjId id = ObjectManager::genObjId();
    ObjectManager::objKindMap[id] = stack;
    ObjectManager::stackObjSet.insert(StackObject(id, size));
    return id;
}

ObjId ObjectManager::createHeapObject(InstLoc allocationSite)
{
    ObjId id = ObjectManager::genObjId();
    ObjectManager::objKindMap[id] = heap;
    ObjectManager::heapObjSet.insert(HeapObject(id, allocationSite));
    return id;
}

ObjId ObjectManager::getOrCreateGlobalObject(llvm::GlobalVariable *glbVar)
{
    auto &glbVarView = ObjectManager::glbObjSet.get<1>();
    auto it = glbVarView.find(glbVar);
    if (it != glbVarView.end()) // get
    {
        return it->id;
    }
    else // create
    {
        ObjId id = ObjectManager::genObjId();
        ObjectManager::objKindMap[id] = global;
        ObjectManager::glbObjSet.insert(GlbObject(id, glbVar));
        return id;
    }
}

ObjId ObjectManager::createDummyObject()
{
    ObjId id = ObjectManager::genObjId();
    ObjectManager::objKindMap[id] = dummy;
    ObjectManager::dummyObjSet.insert(DummyObject(id));
    return id;
}

Kind ObjectManager::getObjectKind(ObjId objId)
{
    return ObjectManager::objKindMap.at(objId);
}

string ObjectManager::toString(ObjId objId)
{
    switch (ObjectManager::objKindMap.at(objId))
    {
    case stack:
        return (*ObjectManager::stackObjSet.find(objId)).toString();
    case heap:
        return (*ObjectManager::heapObjSet.find(objId)).toString();
    case global:
        return (*ObjectManager::glbObjSet.find(objId)).toString();
    case dummy:
        return (*ObjectManager::dummyObjSet.find(objId)).toString();
    default:
        return "error";
    }
}

string ObjectManager::toString()
{
    string res;
    res += "ObjectManager(\n";

    res += "StackObjects(\n";
    for (auto &obj : ObjectManager::stackObjSet)
    {
        res += obj.toString() + "\n";
    }
    res += ")\n";

    res += "HeapObjects(\n";
    for (auto &obj : ObjectManager::heapObjSet)
    {
        res += obj.toString() + "\n";
    }
    res += ")\n";

    res += "GlobalObjects(\n";
    for (auto &obj : ObjectManager::glbObjSet)
    {
        res += obj.toString() + "\n";
    }
    res += ")\n";

    res += "DummyObjects(\n";
    for (auto &obj : ObjectManager::dummyObjSet)
    {
        res += obj.toString() + "\n";
    }
    res += ")\n";

    res += ")\n";
    return res;
}

ObjId ObjectManager::genObjId()
{
    return ObjectManager::currObjId++;
}
