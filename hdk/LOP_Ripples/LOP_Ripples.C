// Disable TBB warnings about atomics
#define __TBB_show_deprecation_message_atomic_H
#define __TBB_show_deprecation_message_task_H

#include <cmath>

#include "LOP_Ripples.h"
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
#include <UT/UT_Vector3.h>
#include <pxr/usd/usd/stage.h>
#include <pxr/usd/usd/attribute.h>
#include <pxr/usd/usdGeom/tokens.h>
#include <pxr/usd/usdGeom/mesh.h>
#include <pxr/usd/usdGeom/points.h>


using namespace HOU_HDK;
// using namespace std;
PXR_NAMESPACE_USING_DIRECTIVE

void newLopOperator(OP_OperatorTable *table){

    OP_Operator *op = new OP_Operator(
        "lop_Ripples",               // Internal name
        "Ripples",                   // UI name
        LOP_Ripples::myConstructor,  // How to build the node
        LOP_Ripples::myTemplateList, // parameters
        0,                        // Min # of sources
        1,                        // Max # of sources
        nullptr,                  // Custom local variables (none)
        OP_FLAG_GENERATOR         // Flag it as generator
        );
        op->setIconName("LOP_Ripples.png");
    table->addOperator(op);            
}



// Parms
static PRM_Name primpathName("primpath", "Prim Path");

static PRM_Name     propagationParmName("propagation", "Propagation");
static PRM_Default  propagationParamDefault = PRM_Default(0.05);
static PRM_Range    propagationParamRange(PRM_RANGE_UI , 0, PRM_RANGE_UI , 1);

PRM_Template LOP_Ripples::myTemplateList[] = {
    PRM_Template(PRM_STRING,       1, &lopPrimPathName,     &lopEditPrimPathDefault, &lopPrimPathMenu,0, 0, &lopPrimPathSpareData),
    PRM_Template(PRM_FLT,          1, &propagationParmName,     &propagationParamDefault,       0,  &propagationParamRange),
    PRM_Template(),
};

OP_Node* LOP_Ripples::myConstructor(OP_Network *net, const char *name, OP_Operator *op){
    return new LOP_Ripples(net, name, op);
}

LOP_Ripples::LOP_Ripples(OP_Network *net, const char *name, OP_Operator *op): LOP_Node(net, name, op){
}

LOP_Ripples::~LOP_Ripples(){}

void LOP_Ripples::PRIMPATH(UT_String &str, fpreal t){
    evalString(str, lopPrimPathName.getToken(), 0, t);
    HUSDmakeValidUsdPath(str, true);
}

UT_Vector3 LOP_Ripples::getXYZParameterValue(const PRM_Name& parmName, fpreal t)
{
    UT_Vector3 value(0.0, 0.0, 0.0);

    for (int i = 0; i < 3; ++i)
    {
        fpreal componentValue = evalFloat(parmName.getToken(), i, t);
        value[i] = componentValue;
    }

    return value;
}


OP_ERROR LOP_Ripples::cookMyLop(OP_Context &context){
    if (cookModifyInput(context) >= UT_ERROR_FATAL){return error();}

    fpreal now = context.getTime();

    UT_String yattrname = "primvars:y"; 
    UT_String laplaceattrname = "primvars:laplace"; 

    UT_String primpath;
    PRIMPATH(primpath, now);

    fpreal propagation = evalFloat(propagationParmName.getToken(), 0, now);

    // USD
    HUSD_AutoWriteLock writelock(editableDataHandle());
    HUSD_AutoLayerLock layerlock(writelock);

    // Create a helper class for creating USD primitives on the stage.
    HUSD_CreatePrims creator(layerlock);

    UsdStageRefPtr stage = writelock.data()->stage();
    UsdPrim prim = stage->GetPrimAtPath(SdfPath(HUSDgetSdfPath(primpath)));

    if (! prim.IsValid()){
        addError(LOP_PRIM_NOT_FOUND, primpath);
        return error();
    }

    VtArray<GfVec3f> points;
    VtArray<int> points_fv_count;
    VtArray<int> points_fv_indices;
    VtArray<float> yvalues;
    VtArray<float> laplacevalues;

    UsdGeomMesh mesh(prim);
    UsdAttribute pointsAttr = mesh.GetPointsAttr();
    UsdAttribute pointsFVAttr = mesh.GetFaceVertexCountsAttr();
    UsdAttribute pointsFVIAttr = mesh.GetFaceVertexIndicesAttr();
    UsdAttribute yAttr = prim.GetAttribute(TfToken(yattrname));
    UsdAttribute laplaceAttr = prim.GetAttribute(TfToken(laplaceattrname));


    pointsAttr.Get(&points, now);
    pointsFVAttr.Get(&points_fv_count, now);
    pointsFVIAttr.Get(&points_fv_indices, now);


    VtArray<float> zerovalues(points.size(), 0.0f);

    if ( ! yAttr.HasValue()){
        yAttr = prim.CreateAttribute(TfToken(yattrname), SdfValueTypeNames->FloatArray, true);
        yAttr.SetMetadata(TfToken("interpolation"), VtValue("vertex"));
        yAttr.Set(zerovalues);
    }

    if ( ! laplaceAttr.HasValue()){
        laplaceAttr = prim.CreateAttribute(TfToken(laplaceattrname), SdfValueTypeNames->FloatArray, true);
        laplaceAttr.SetMetadata(TfToken("interpolation"), VtValue("vertex"));
        laplaceAttr.Set(zerovalues);
    }
    

    // Get points indcies according to faces
    VtArray<VtIntArray> faces;
    int fv=0;
    for(int f=0; f<points_fv_count.size(); f++){
        int end_idx = fv + points_fv_count[f];

        VtIntArray face(points_fv_indices.begin() + fv, points_fv_indices.begin() + end_idx);
        faces.push_back(face);
        fv = end_idx;
    }

    // Get the faces that contain the same point
    VtArray<VtArray<VtIntArray>> connected_faces;
    for (int i=0; i<points.size(); i++){
        VtArray<VtIntArray> connected_face;
        for (const auto& face : faces) {
            if (std::any_of(face.begin(), face.end(), [i](int value) { return value == i; })) {
                connected_face.push_back(face);
            }
        }

        connected_faces.push_back(connected_face);

    }


    for (int t=0; t<100; t++){

        pointsAttr.Get(&points, t-1);
        laplaceAttr.Get(&laplacevalues, t-1);
        yAttr.Get(&yvalues, t-1);

        VtFloatArray laplace_values;
        for(int i=0; i<points.size(); i++){
            float laplace = 0.0;

            for(int f=0; f< connected_faces[i].size(); f++){
                for(int p=0; p<connected_faces[i][f].size(); p++){
                    if (connected_faces[i][f][p] == i){
                        continue;
                    }
                    int pt_idx = connected_faces[i][f][p];
                    laplace += points[pt_idx][1] - points[i][1];
                }
            }

            laplace_values.push_back(laplace);

        }


        // propagation
        for (int i=0; i<laplace_values.size(); i++){

            laplace_values[i] *= propagation;
            yvalues[i] += laplace_values[i];
            points[i][1] += yvalues[i];

        }
        
        laplaceAttr.Set(laplace_values, t);
        yAttr.Set(yvalues, t);
        pointsAttr.Set(points, t);

    }




    return error();

}