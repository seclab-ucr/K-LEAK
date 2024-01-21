#pragma once

#include "llvm/Support/Casting.h"

#include "InstLoc.h"

#include <boost/multi_index_container.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/composite_key.hpp>

#include <map>

// Different kinds of memory object
enum Kind
{
    stack,
    heap,
    global,
    dummy,
};

typedef uint64_t ObjId;
typedef int64_t offset_t;

// This represents a location (within an object) that a pointer can point to
class ObjLoc
{
public:
    ObjId objId;
    offset_t offset;

    ObjLoc(ObjId objId, offset_t offset);
    bool operator<(const ObjLoc &rhs) const;
};

// This class represents a stack object
class StackObject
{
public:
    ObjId id;
    uint64_t size; // size of this object

    StackObject(ObjId id, uint64_t size);
    std::string toString() const;
};

typedef boost::multi_index::multi_index_container<
    StackObject,
    boost::multi_index::indexed_by<
        boost::multi_index::ordered_unique<
            boost::multi_index::member<StackObject, ObjId, &StackObject::id>>>>
    StackObjectSet;

// This class represents a heap object
class HeapObject
{
public:
    ObjId id;
    InstLoc allocationSite;

    HeapObject(ObjId id, InstLoc allocationSite);
    std::string toString() const;
};

typedef boost::multi_index::multi_index_container<
    HeapObject,
    boost::multi_index::indexed_by<
        boost::multi_index::ordered_unique<
            boost::multi_index::member<HeapObject, ObjId, &HeapObject::id>>>>
    HeapObjectSet;

// This class represents a global object
class GlbObject
{
public:
    ObjId id;
    llvm::GlobalVariable *glbVar;

    GlbObject(ObjId id, llvm::GlobalVariable *glbVar);
    std::string toString() const;
};

typedef boost::multi_index::multi_index_container<
    GlbObject,
    boost::multi_index::indexed_by<
        boost::multi_index::ordered_unique<
            boost::multi_index::member<GlbObject, ObjId, &GlbObject::id>>,
        boost::multi_index::ordered_unique<
            boost::multi_index::member<GlbObject, llvm::GlobalVariable *, &GlbObject::glbVar>>>>
    GlbObjectSet;

// This class represents a dummy object
class DummyObject
{
public:
    ObjId id;

    DummyObject(ObjId id);
    std::string toString() const;
};

typedef boost::multi_index::multi_index_container<
    DummyObject,
    boost::multi_index::indexed_by<
        boost::multi_index::ordered_unique<
            boost::multi_index::member<DummyObject, ObjId, &DummyObject::id>>>>
    DummyObjectSet;

class ObjectManager
{
public:
    static ObjId createStackObject(uint64_t size);
    static ObjId createHeapObject(InstLoc allocationSite);
    static ObjId getOrCreateGlobalObject(llvm::GlobalVariable *glbVar);
    static ObjId createDummyObject();
    static Kind getObjectKind(ObjId objId);
    static std::string toString(ObjId objId);
    static std::string toString();

private:
    static ObjId currObjId;
    static std::map<ObjId, Kind> objKindMap;
    static StackObjectSet stackObjSet;
    static HeapObjectSet heapObjSet;
    static GlbObjectSet glbObjSet;
    static DummyObjectSet dummyObjSet;

    static ObjId genObjId();
};
