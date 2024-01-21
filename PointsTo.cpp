#include "PointsTo.h"
#include "Utils.h"

#include <boost/range.hpp>

#define MEM_SSA true

using namespace llvm;
using namespace std;

ObjPto::ObjPto(ObjId srcObjId, offset_t srcOffset, ObjId dstObjId, offset_t dstOffset, InstLoc instLoc) : srcObjId(srcObjId), srcOffset(srcOffset), dstObjId(dstObjId), dstOffset(dstOffset), instLoc(instLoc)
{
}

bool ObjPto::operator<(const ObjPto &rhs) const
{
    if (this->srcObjId != rhs.srcObjId)
    {
        return this->srcObjId < rhs.srcObjId;
    }
    else if (this->srcOffset != rhs.srcOffset)
    {
        return this->srcOffset < rhs.srcOffset;
    }
    else if (this->dstObjId != rhs.dstObjId)
    {
        return this->dstObjId < rhs.dstObjId;
    }
    else if (this->dstOffset != rhs.dstOffset)
    {
        return this->dstOffset < rhs.dstOffset;
    }
    return false;
}

set<ObjLoc> PointsToRecords::getPteesOfValPtr(Context context, Value *pointer)
{
    auto &tmp = this->valueData[context];
    if (tmp.find(pointer) != tmp.end())
    {
        return tmp.at(pointer);
    }
    else
    {
        return set<ObjLoc>();
    }
}

void PointsToRecords::addPteesForValPtr(Context context, Value *pointer, set<ObjLoc> pointees)
{
    if (pointees.empty())
    {
        return;
    }
    this->valueData[context][pointer].insert(pointees.begin(), pointees.end());
}

string PointsToRecords::toString()
{
    string res;

    res += "StackPtoRecords(\n";
    for (auto &itCtx : this->valueData)
    {
        res += "context(";
        res += itCtx.first.toString();
        res += "\n";
        for (auto &itVal : itCtx.second)
        {
            res += getName(itVal.first);
            res += " -> [";

            for (auto &itObjLoc : itVal.second)
            {
                res += "(";
                res += ObjectManager::toString(itObjLoc.objId);
                res += ", ";
                res += to_string(itObjLoc.offset);
                res += "), ";
            }
            res += "]\n";
        }
        res += ")\n";
    }
    res += ")\n";

    res += "ObjPtoRecords(\n";
    for (auto &objPto : this->objPtos)
    {
        res += "(";
        res += ObjectManager::toString(objPto.srcObjId);
        res += ", ";
        res += to_string(objPto.srcOffset);
        res += ") -> (";
        res += "(";
        res += ObjectManager::toString(objPto.dstObjId);
        res += ", ";
        res += to_string(objPto.dstOffset);
        res += ")\n";
    }
    res += ")";

    return res;
}

set<ObjLoc> PointsToRecords::getPteesOfObjPtr(ObjLoc pointer, InstLoc currLoc)
{
    set<ObjLoc> res;
    auto &srcView = this->objPtos.get<0>();
    for (auto &objPto : boost::make_iterator_range(srcView.equal_range(boost::make_tuple(pointer.objId, pointer.offset))))
    {
#if MEM_SSA
        if (isReachable(objPto.instLoc, currLoc))
        {
#endif
            res.insert(ObjLoc(objPto.dstObjId, objPto.dstOffset));
#if MEM_SSA
        }
#endif
    }
    return res;
}

void PointsToRecords::addPteesForObjPtr(ObjLoc pointer, set<ObjLoc> pointees, InstLoc updateLoc)
{
    if (pointees.empty())
    {
        return;
    }
    for (auto &pointee : pointees)
    {
        this->objPtos.insert(ObjPto(pointer.objId, pointer.offset, pointee.objId, pointee.offset, updateLoc));
    }
}

set<ObjPto> PointsToRecords::getRecordsOfObj(ObjId objId)
{
    set<ObjPto> res;
    auto &srcView = this->objPtos.get<0>();
    for (auto &objPto : boost::make_iterator_range(srcView.equal_range(objId)))
    {
        res.insert(objPto);
    }
    return res;
}
