// https://github.com/sideeffects/HoudiniUsdBridge/blob/2cd8510dcb71049bf22d8d0b4ec3eab262ccc021/src/houdini/custom/USDOP/OBJ_LOPCamera.h

// Disable TBB warnings about atomics
#define __TBB_show_deprecation_message_atomic_H
#define __TBB_show_deprecation_message_task_H

#include <cmath>

#include "LOP_Blast.h"
#include <LOP/LOP_PRMShared.h>
#include <LOP/LOP_Error.h>
#include <HUSD/HUSD_CreatePrims.h>
#include <HUSD/HUSD_Constants.h>
#include <HUSD/HUSD_Utils.h>
#include <HUSD/XUSD_Data.h>
#include <HUSD/XUSD_Utils.h>
#include <OP/OP_OperatorTable.h>
#include <OP/OP_Operator.h>
#include <OP/OP_AutoLockInputs.h>
#include <PRM/PRM_Include.h>
#include <UT/UT_DSOVersion.h>
#include <UT/UT_Vector3.h>
#include <pxr/usd/usd/stage.h>
#include <pxr/usd/usd/attribute.h>
#include <pxr/usd/usdGeom/tokens.h>
#include <pxr/usd/usdGeom/mesh.h>
#include <pxr/usd/usdGeom/points.h>


using namespace HOU_HDK;

PXR_NAMESPACE_USING_DIRECTIVE

void newLopOperator(OP_OperatorTable *table){

    OP_Operator *op = new OP_Operator(
        "lop_Blast",               // Internal name
        "Blast",                   // UI name
        LOP_Blast::myConstructor,  // How to build the node
        LOP_Blast::myTemplateList, // parameters
        0,                        // Min # of sources
        1,                        // Max # of sources
        nullptr,                  // Custom local variables (none)
        OP_FLAG_GENERATOR         // Flag it as generator
        );
        op->setIconName("LOP_Blast.png");
    table->addOperator(op);            
}



// Parms
static PRM_Name     primpathName("primpath", "Prim Path");
static PRM_Name     attrnameParmName("attrname", "Attribute");
static PRM_Name     signParmName("sign", "");
static PRM_Name     attrvalParmName("attrval", "");
static PRM_Name     invertParmName("invert", "Invert");

static PRM_Name     signParmTypes[] =
{
    PRM_Name("==", "=="),
    PRM_Name("<", "<"),
    PRM_Name(">", ">"),
    PRM_Name(0)
};
static PRM_ChoiceList   signTypesMenu(PRM_CHOICELIST_SINGLE, signParmTypes);


static PRM_Default  attrnameParmDefault(0.0f, "");
static PRM_Default  attrvalParmDefault(0.0);
static PRM_Default  invertParmDefault = PRM_Default(false);

PRM_Template LOP_Blast::myTemplateList[] = {
    PRM_Template(PRM_STRING,                            1, &lopPrimPathName,         &lopEditPrimPathDefault, &lopPrimPathMenu,0, 0, &lopPrimPathSpareData),
    PRM_Template(PRM_STRING | PRM_TYPE_JOIN_NEXT,       1, &attrnameParmName,        &attrnameParmDefault),
    PRM_Template(PRM_ORD | PRM_TYPE_JOIN_NEXT,          1, &signParmName,           0,  &signTypesMenu),
    PRM_Template(PRM_FLT,                               1, &attrvalParmName,         &attrvalParmDefault),
    PRM_Template(PRM_TOGGLE,                            1, &invertParmName,          &invertParmDefault),

    PRM_Template(),
};

OP_Node* LOP_Blast::myConstructor(OP_Network *net, const char *name, OP_Operator *op){
    return new LOP_Blast(net, name, op);
}

LOP_Blast::LOP_Blast(OP_Network *net, const char *name, OP_Operator *op): LOP_Node(net, name, op){
}

LOP_Blast::~LOP_Blast(){}

void LOP_Blast::PRIMPATH(UT_String &str){
    evalString(str, lopPrimPathName.getToken(), 0, 0.0f);
    HUSDmakeValidUsdPath(str, true);
}

std::string LOP_Blast::evalAsString(const PRM_Name& parmName, fpreal t){
    UT_String name;
    evalString(name, parmName.getToken(), 0, t);
    return name.toStdString();
}


struct SortDescend{
    bool operator()(int a, int b) const {
        return a>b;
    }
};


OP_ERROR LOP_Blast::cookMyLop(OP_Context &context){
    if (cookModifyInput(context) >= UT_ERROR_ABORT){return error();}


    UT_String		     primpath;
    OP_AutoLockInputs	 auto_lock_inputs(this);
    HUSD_TimeSampling	 time_sampling = HUSD_TimeSampling::NONE;
    fpreal               now = context.getTime();


    PRIMPATH(primpath);
    std::string attrname = evalAsString(attrnameParmName, now);
    std::string sign = evalAsString(signParmName, now);
    fpreal attrval = evalFloat(attrvalParmName.getToken(), 0, now);
    int invert = bool(evalInt(invertParmName.getToken(), 0, now));

    if (attrname.empty()){
        return error();
    }

    // USD
    HUSD_AutoWriteLock writelock(editableDataHandle());
    HUSD_AutoLayerLock layerlock(writelock);

    UsdStageRefPtr stage = writelock.data()->stage();
    UsdPrim prim = stage->GetPrimAtPath(SdfPath(HUSDgetSdfPath(primpath)));

    if (! prim.IsValid()){
        // appendError("LOP", LOP_PRIM_NOT_FOUND, "Prim is not found", UT_ERROR_ABORT);
        return error();
    }
    if (! prim.IsA<UsdGeomMesh>()){
        appendError("LOP", LOP_MESSAGE, "Prim is not Mesh", UT_ERROR_ABORT);
        return UT_ERROR_ABORT;
    }

    UsdGeomMesh mesh(prim);
    UsdAttribute pointsAttr = mesh.GetPointsAttr();
    UsdAttribute pointsFVCAttr = mesh.GetFaceVertexCountsAttr();
    UsdAttribute pointsFVIAttr = mesh.GetFaceVertexIndicesAttr();
    UsdAttribute maskAttr = prim.GetAttribute(TfToken(attrname));


    if (! maskAttr.IsValid()){
        appendError("LOP", LOP_MESSAGE, "Invalid Attribute", UT_ERROR_ABORT);
        return UT_ERROR_ABORT; 
    }

    VtArray<GfVec3f> points;
    VtArray<int> points_fv_count;
    VtArray<int> points_fv_indices;
    VtArray<float> mask_array;

    pointsAttr.Get(&points, now);
    pointsFVCAttr.Get(&points_fv_count, now);
    pointsFVIAttr.Get(&points_fv_indices, now);
    maskAttr.Get(&mask_array, now);


    // Get Mask Indicies
    VtIntArray mask;
    for (int i=0; i<mask_array.size(); i++){

        if (mask_array[i] > 0.5){
            mask.push_back(i);
        }
    }

    if (mask.size() <= 0){
        return error();
    }

    std::sort(mask.begin(), mask.end(), SortDescend());

    // Faces 
    VtArray<int> fvc_mask;
    VtArray<int> fvi_mask;
    int fv=0;
    for(int f=0; f<points_fv_count.size(); f++){
        int end_idx = fv + points_fv_count[f];

        VtIntArray face(points_fv_indices.begin() + fv, points_fv_indices.begin() + end_idx);
        for (int i=0; i<face.size(); i++){
            bool jumb=false;
            for (int p=0; p<mask.size(); p++){
                if (face[i] == mask[p]){
                    fvc_mask.push_back(f);
                    for (int fi=0; fi<face.size(); fi++){
                        fvi_mask.push_back(fv + fi);
                    }
                    jumb = true;
                    break;
                }
            }
            if (jumb) break;
        }

        fv = end_idx;
    }

    std::sort(fvc_mask.begin(), fvc_mask.end(), SortDescend());
    std::sort(fvi_mask.begin(), fvi_mask.end(), SortDescend());


    for (int i=0; i<fvc_mask.size(); i++){
        points_fv_count.erase(points_fv_count.begin()+fvc_mask[i]);
    }


    for (int i=0; i<fvi_mask.size(); i++){
        points_fv_indices.erase(points_fv_indices.begin()+fvi_mask[i]);
    }

    for (int i=0; i<mask.size(); i++){
        points.erase(points.begin()+mask[i]);

        for (int ii=0; ii<points_fv_indices.size(); ii++){
            if (points_fv_indices[ii] >  mask[i]){
                points_fv_indices[ii] = points_fv_indices[ii]-1;
            }
        }
    }


    pointsAttr.Set(points, now);
    pointsFVCAttr.Set(points_fv_count, now);
    pointsFVIAttr.Set(points_fv_indices, now);

        // VtValue interpolation;
        // if (! attr.GetMetadata(TfToken("interpolation"), &interpolation)){
        //     interpolation = VtValue("vertex");
        // }

    for (UsdAttribute attr : prim.GetAttributes()){

        if (attr.GetName() == TfToken("points") ||
            attr.GetName() == TfToken("faceVertexIndices") || 
            attr.GetName() == TfToken("faceVertexCounts")
            ){ continue; }

        VtValue interpolation;
        attr.GetMetadata(TfToken("interpolation"), &interpolation);

        if (interpolation.IsEmpty()){ continue; }

        VtValue attrVtValue;
        attr.Get(&attrVtValue);

        if ( attrVtValue.IsEmpty()){ continue; }
        TfType arrayType = attrVtValue.GetType(); //.GetTypeName().c_str();

        
        if (interpolation == TfToken("vertex")){
            VtValue attrValues;

            if (attrVtValue.IsHolding<VtArray<GfVec2f>>()) {
                attrValues = attrVtValue.UncheckedGet<VtArray<GfVec2f>>();
            }
            else if (attrVtValue.IsHolding<VtArray<GfVec3f>>()) {
                attrValues = attrVtValue.UncheckedGet<VtArray<GfVec3f>>();
            }
            else if (attrVtValue.IsHolding<VtArray<float>>()) {
                attrValues = attrVtValue.UncheckedGet<VtArray<float>>();
            }
            else if (attrVtValue.IsHolding<VtArray<int>>()) {
                attrValues = attrVtValue.UncheckedGet<VtArray<int>>();
            }
            else {
                continue;
            }

            for (int i=0; i<mask.size(); i++){
                attrValues.erase(attrValues.begin()+mask[i]);
            }
        }
        if (interpolation == TfToken("uniform")){
            
        }
        if (interpolation == TfToken("faceVarying")){
            if (! attrVtValue.IsHolding<VtArray<GfVec2f>>()) {continue;}

            VtArray<GfVec2f> attrValues = attrVtValue.UncheckedGet<VtArray<GfVec2f>>();
            
            for (int i=0; i<fvi_mask.size(); i++){
                attrValues.erase(attrValues.begin()+fvi_mask[i]);
            }
            attr.Set(attrValues, UsdTimeCode::Default());
        }


        // UsdTimeCode::Default()

        // printf("Attr: %s Type: %s\n", attr.GetName().GetString().c_str(), attrVtValue.GetType().GetTypeName().c_str());
    }



    return error();

}