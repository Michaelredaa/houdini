
// Disable TBB warnings about atomics
#define __TBB_show_deprecation_message_atomic_H
#define __TBB_show_deprecation_message_task_H

#include "LOP_UsdGeom.h"
#include <LOP/LOP_PRMShared.h>
#include <LOP/LOP_Error.h>
#include <HUSD/HUSD_CreatePrims.h>
#include <HUSD/HUSD_Constants.h>
#include <HUSD/HUSD_Utils.h>
#include <HUSD/XUSD_Data.h>
#include <HUSD/XUSD_Utils.h>
#include <OP/OP_OperatorTable.h>
#include <OP/OP_Operator.h>
#include <PRM/PRM_Include.h>
#include <UT/UT_DSOVersion.h>
#include <pxr/usd/usd/stage.h>
#include <pxr/usd/usd/attribute.h>
#include <pxr/usd/usdGeom/tokens.h>
#include <pxr/usd/usdGeom/mesh.h>
#include <pxr/usd/usdGeom/points.h>

using namespace HDK_Sample;
PXR_NAMESPACE_USING_DIRECTIVE

void
newLopOperator(OP_OperatorTable *table)
{
    table->addOperator(new OP_Operator(
        "UsdGeom",
        "UsdGeom",
        LOP_UsdGeom::myConstructor,
        LOP_UsdGeom::myTemplateList,
        (unsigned)0,
        (unsigned)1));
}

static PRM_Name		 theRadiusName("radius", "Radius");

PRM_Template
LOP_UsdGeom::myTemplateList[] = {
    PRM_Template(PRM_STRING, 1, &lopPrimPathName,
		 &lopAddPrimPathDefault, &lopPrimPathMenu,
		 0, 0, &lopPrimPathSpareData),
    PRM_Template(PRM_FLT,    1, &theRadiusName, PRMoneDefaults),
    PRM_Template(),
};

OP_Node *
LOP_UsdGeom::myConstructor(OP_Network *net, const char *name, 
OP_Operator *op)
{
    return new LOP_UsdGeom(net, name, op);
}

LOP_UsdGeom::LOP_UsdGeom(OP_Network *net, const char *name, OP_Operator *op)
    : LOP_Node(net, name, op)
{
}

LOP_UsdGeom::~LOP_UsdGeom()
{
}

void
LOP_UsdGeom::PRIMPATH(UT_String &str, fpreal t)
{
    evalString(str, lopPrimPathName.getToken(), 0, t);
    HUSDmakeValidUsdPath(str, true);
}

fpreal
LOP_UsdGeom::RADIUS(fpreal t)
{
    return evalFloat(theRadiusName.getToken(), 0, t);
}



class UsdGeo : public UsdGeomMesh {
public:
    UsdGeo(const UsdPrim& prim) : UsdGeomMesh(prim) {
        // Initialize member attributes in the constructor
        pointsAttr.Get(&points);
        FVCAttr.Get(&fv_count);
        FVIAttr.Get(&fv_indices);
    }


    VtArray<VtIntArray> GetFaces(){
        VtArray<VtIntArray>  faces;
        int fv = 0;

        for(int f=0; f<fv_count.size(); f++){
            int end_idx = fv + fv_count[f];

            VtIntArray face(fv_indices.begin() + fv, fv_indices.begin() + end_idx);
            faces.push_back(face);
            fv = end_idx;
        }
        return faces;
    }


    VtArray<VtIntArray> GetConnectedFacesToPoint(int pointNum){
        // Get the faces that contains the same point number

        VtArray<VtIntArray>  connected_faces;
        for (const auto& face: GetFaces()){
            if (std::any_of(face.begin(), face.end(), [pointNum](int value) {return value == pointNum; })){
                connected_faces.push_back(face);
            }
        }

        return connected_faces;
    }


    VtIntArray GetConnectedPoints(int pointNum) {

        VtArray<int> connectedPoints;

        std::set<int> uniquePoints;
        for (const auto& face: GetFaces()){
            for (int i=0; i <face.size(); i++){
                if (i == pointNum){
                    int previous_point = face[(i-1) % face.size()];
                    int next_point = face[(i+1) % face.size()];
                    uniquePoints.insert(previous_point);
                    uniquePoints.insert(next_point);
                }
            }
        }

        return VtIntArray(uniquePoints.begin(), uniquePoints.end());
    }


    void DeletePoint(int pointNum){
        points
        return 0;
    }



private:
    VtArray<GfVec3f> points;
    VtArray<int> fv_count;
    VtArray<int> fv_indices;

    UsdAttribute pointsAttr = GetPointsAttr();
    UsdAttribute FVCAttr = GetFaceVertexCountsAttr();
    UsdAttribute FVIAttr = GetFaceVertexIndicesAttr();
};












OP_ERROR
LOP_UsdGeom::cookMyLop(OP_Context &context)
{
    // Cook the node connected to our input, and make a "soft copy" of the
    // result into our own HUSD_DataHandle.
    if (cookModifyInput(context) >= UT_ERROR_FATAL)
	return error();

    UT_String		     primpath;
    fpreal		     radius;
    fpreal now = context.getTime();

    PRIMPATH(primpath, now);
    radius = RADIUS(now);

    // Use editableDataHandle to get non-const access to our data handle, and
    // the lock it for writing. This makes sure that the USD stage is set up
    // to match the configuration defined in our data handle. Any edits made
    // to the stage at this point will be preserved in our data handle when
    // we unlock it.
    HUSD_AutoWriteLock	     writelock(editableDataHandle());
    HUSD_AutoLayerLock	     layerlock(writelock);

    // Create a helper class for creating USD primitives on the stage.
    HUSD_CreatePrims	     creator(layerlock);

    // Use the helper class to create a new "UsdGeom" primitive, with a
    // specifier type of "def" to define this primitive even if it doesn't
    // exist on the stage yet.
    //
    // The "empty string" parameter to createPrim tells it to not author a
    // "kind" setting for this primiitve.
    if (!creator.createPrim(primpath, "UsdGeomUsdGeom",
	    UT_StringHolder::theEmptyString,
	    HUSD_Constants::getPrimSpecifierDefine(),
	    HUSD_Constants::getXformPrimType()))
    {
	addError(LOP_PRIM_NOT_CREATED, primpath);
        return error();
    }

    // Use direct editing of the USD stage to modify the radius attribute
    // of the new UsdGeom primitive. We could use the HUSD_SetAttributes
    // helper class here, but this demonstrates that we are allowed to
    // access the stage directly. The edit target will already be set to
    // the "active layer", and as long as we leave it there, any changes
    // we make will be preserved by our data handle.
    UsdStageRefPtr	     stage = writelock.data()->stage();
    SdfPath		     sdfpath(HUSDgetSdfPath(primpath));
    UsdPrim		     prim = stage->GetPrimAtPath(sdfpath);


    VtArray<GfVec3f> points;
    VtArray<int> fv_count;
    VtArray<int> fv_indicies;

    UsdGeo mesh = UsdGeo(prim);
    UsdAttribute pointsAttr = mesh.GetPointsAttr();
    UsdAttribute FVCAttr = mesh.GetFaceVertexCountsAttr();
    UsdAttribute FVIAttr = mesh.GetFaceVertexIndicesAttr();

    pointsAttr.Get(&points, now);
    FVCAttr.Get(&fv_count, now);
    FVIAttr.Get(&fv_indicies, now);



    if (prim)
	prim.GetAttribute(UsdGeomTokens->radius).Set(radius);

    setLastModifiedPrims(primpath);

    return error();
}

