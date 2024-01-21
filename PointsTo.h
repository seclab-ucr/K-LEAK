#pragma once

#include "llvm/IR/Value.h"

#include "InstLoc.h"
#include "Object.h"

#include <boost/multi_index_container.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/composite_key.hpp>

#include <set>

// This class represents the relation of one memory object pointing to another memory object
class ObjPto
{
public:
    ObjId srcObjId;
    offset_t srcOffset;
    ObjId dstObjId;
    offset_t dstOffset;
    InstLoc instLoc; // for memory SSA

    ObjPto(ObjId srcObjId, offset_t srcOffset, ObjId dstObjId, offset_t dstOffset, InstLoc instLoc);
    bool operator<(const ObjPto &rhs) const; // for std::set
};

typedef boost::multi_index::multi_index_container<
    ObjPto,
    boost::multi_index::indexed_by<
        boost::multi_index::ordered_non_unique<
            boost::multi_index::composite_key<
                ObjPto,
                boost::multi_index::member<ObjPto, ObjId, &ObjPto::srcObjId>,
                boost::multi_index::member<ObjPto, offset_t, &ObjPto::srcOffset>>>>>
    ObjPtos;

class PointsToRecords
{
private:
    std::map<llvm::GlobalVariable *, std::set<ObjLoc>> globalData;
    std::map<Context, std::map<llvm::Value *, std::set<ObjLoc>>> valueData;
    ObjPtos objPtos;

public:
    // value
    std::set<ObjLoc> getPteesOfValPtr(Context context, llvm::Value *pointer);
    void addPteesForValPtr(Context context, llvm::Value *pointer, std::set<ObjLoc> pointees);

    // object
    std::set<ObjLoc> getPteesOfObjPtr(ObjLoc pointer, InstLoc currLoc);
    void addPteesForObjPtr(ObjLoc pointer, std::set<ObjLoc> pointees, InstLoc updateLoc);
    std::set<ObjPto> getRecordsOfObj(ObjId objId);

    std::string toString();
};
