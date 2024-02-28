
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

struct ConnectedFaceInfo{
    int faceIndex;
    VtIntArray face; // array with face vertex indicies
    VtIntArray faceVertexIndices; // array with indices of face vertex indicies
};

struct FaceInfo{
    VtIntArray face; // array with face vertex indicies
    VtIntArray faceVertexIndices; // array with indices of face vertex indicies
};

struct CompareDescend{
    bool operator()(int a, int b) const {
        return a>b;
    }
};

class UsdGeo : public UsdGeomMesh {
public:
    UsdGeo(const UsdPrim& prim) : UsdGeomMesh(prim) {
        // Initialize member attributes in the constructor
        pointsAttr.Get(&points);
        FVCAttr.Get(&fv_count);
        FVIAttr.Get(&fv_indices);
    }


    VtArray<FaceInfo> GetFaces(){
        VtArray<FaceInfo> faces;
        int fv = 0;

        for(int f=0; f<fv_count.size(); f++){
            int end_idx = fv + fv_count[f];

            VtIntArray face(fv_indices.begin() + fv, fv_indices.begin() + end_idx);
            VtIntArray faceIndices;
            while (end_idx>fv){
                faceIndices.push_back(fv);
                fv ++;
            }
            faces.push_back({face, faceIndices});
            fv = end_idx;
        }
        return faces;
    }


    VtArray<ConnectedFaceInfo> GetConnectedFacesToPoint(int pointNum) {
        // Get the faces that contain the same point number

        VtArray<ConnectedFaceInfo> connected_faces;
        VtArray<FaceInfo> facesInfo = GetFaces();

        for (int f = 0; f < facesInfo.size(); f++) {
            const VtIntArray& face = facesInfo[f].face;

            // Check if pointNum is present in the face
            auto it = std::find(face.begin(), face.end(), pointNum);

            if (it != face.end()) {
                // Calculate the index of pointNum in the face

                // Add connected face info
                // VtIntArray faceVertexIndices(facesInfo.faceVertexIndices.begin(), facesInfo.faceVertexIndices.end());
                connected_faces.push_back({f, face, facesInfo[f].faceVertexIndices});
            }
        }

        return connected_faces;
    }


    VtIntArray GetConnectedPoints(int pointNum) {

        VtArray<int> connectedPoints;

        std::set<int> uniquePoints;
        for (const auto& faceInfo: GetFaces()){
            for (int i=0; i <faceInfo.face.size(); i++){
                if (i == pointNum){
                    int previous_point = faceInfo.face[(i-1) % faceInfo.face.size()];
                    int next_point = faceInfo.face[(i+1) % faceInfo.face.size()];
                    uniquePoints.insert(previous_point);
                    uniquePoints.insert(next_point);
                }
            }
        }

        return VtIntArray(uniquePoints.begin(), uniquePoints.end());
    }


    int DeletePoint(int pointNum){
        points.erase(points.begin() + pointNum);

        // VtArray<int>  connected_faces = GetConnectedFacesToPoint(pointNum);

        VtArray<ConnectedFaceInfo> connectedFacedInfo = GetConnectedFacesToPoint(pointNum);
        std::set<int> pointsToDelete;
        for (const auto& info :  connectedFacedInfo){
            fv_count.erase(fv_count.begin() + info.faceIndex);
            for (const int fv : info.faceVertexIndices){
                fv_indices.erase(fv_indices.begin() + fv);
            }
            
            // Get unique points
            for (const int p : info.face){
                pointsToDelete.insert(p);
            }
        }
        
        // sort points descending and 
        std::set<int, CompareDescend> pointsToDeleteOrdered(pointsToDelete.begin(), pointsToDelete.end());
        for (const int pi : pointsToDeleteOrdered){
            
            for (int i=0; i<fv_indices.size(); i++){
                if (fv_indices[i] > pi){
                    fv_indices[i] = fv_indices[i] - 1;
                }
            }
        }

        pointsAttr.Set(points);
        FVCAttr.Set(fv_count);
        FVIAttr.Set(fv_indices);

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
    // fpreal		     radius;
    fpreal now = context.getTime();

    PRIMPATH(primpath, now);
    // radius = RADIUS(now);

    // Use editableDataHandle to get non-const access to our data handle, and
    // the lock it for writing. This makes sure that the USD stage is set up
    // to match the configuration defined in our data handle. Any edits made
    // to the stage at this point will be preserved in our data handle when
    // we unlock it.
    HUSD_AutoWriteLock	     writelock(editableDataHandle());
    HUSD_AutoLayerLock	     layerlock(writelock);


    // we make will be preserved by our data handle.
    UsdStageRefPtr	     stage = writelock.data()->stage();
    SdfPath		     sdfpath(HUSDgetSdfPath(primpath));
    UsdPrim		     prim = stage->GetPrimAtPath(sdfpath);

    if (! prim.IsValid()){
        addError(OP_ERR_ANYTHING, "Prim not found");
        return error();
    }


    VtArray<GfVec3f> points;
    VtArray<int> fv_count;
    VtArray<int> fv_indicies;

    UsdGeo mesh = UsdGeo(prim);


    for (int i=0; i<10; i++){
        mesh.DeletePoint(i);
    }
    

    return error();
}

