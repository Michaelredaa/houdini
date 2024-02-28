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
using namespace std;
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

string LOP_Blast::evalAsString(const PRM_Name& parmName, fpreal t){
    UT_String name;
    evalString(name, parmName.getToken(), 0, t);
    return name.toStdString();
}

OP_ERROR LOP_Blast::cookMyLop(OP_Context &context){
    if (cookModifyInput(context) >= UT_ERROR_ABORT){return error();}


    UT_String		     primpath;
    OP_AutoLockInputs	 auto_lock_inputs(this);
    HUSD_TimeSampling	 time_sampling = HUSD_TimeSampling::NONE;
    fpreal               now = context.getTime();


    PRIMPATH(primpath);
    string attrname = evalAsString(attrnameParmName, now);
    string sign = evalAsString(signParmName, now);
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
    UsdAttribute pointsFVAttr = mesh.GetFaceVertexCountsAttr();
    UsdAttribute pointsFVIAttr = mesh.GetFaceVertexIndicesAttr();
    UsdAttribute maskAttr = prim.GetAttribute(TfToken(attrname));


    if (! maskAttr.IsValid()){
        appendError("LOP", LOP_MESSAGE, "Invalid Attribute", UT_ERROR_ABORT);
        return UT_ERROR_ABORT; 
    }

    VtArray<GfVec3f> points;
    VtArray<int> points_fv_count;
    VtArray<int> points_fv_indices;
    VtArray<float> mask_values;

    pointsAttr.Get(&points, now);
    pointsFVAttr.Get(&points_fv_count, now);
    pointsFVIAttr.Get(&points_fv_indices, now);
    maskAttr.Get(&mask_values, now);


    // Get Mask Indicies
    VtArray<int> mask;
    for (int i=0; i<mask_values.size(); i++){

        if (mask_values[i] > 0.5){
            mask.push_back(i);
        }
    }

    if (mask.size() <= 0){
        return error();
    }


    // Faces 
    VtArray<int> fv_mask;
    VtArray<int> fvi_mask;
    VtArray<VtIntArray> faces;
    int fv=0;
    for(int f=0; f<points_fv_count.size(); f++){
        int end_idx = fv + points_fv_count[f];

        VtIntArray face(points_fv_indices.begin() + fv, points_fv_indices.begin() + end_idx);
        faces.push_back(face);
        for (int i=0; i<face.size(); i++){
            bool jumb=false;
            for (int p=0; p<mask.size(); p++){
                if (face[i] == mask[p]){
                    fv_mask.push_back(f);
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
    
    int sf = fv_mask.size()-1;
    while (true){
        points_fv_count.erase(points_fv_count.begin()+fv_mask[sf]);
        sf--;
        if (sf <0){
            break;
        }
    }

    int sfi = fvi_mask.size()-1;
    while (true){
        points_fv_indices.erase(points_fv_indices.begin()+fvi_mask[sfi]);
        sfi--;
        if (sfi <0){
            break;
        }
    }


    // Points
    int s = mask.size()-1;
    while (true){
        points.erase(points.begin()+mask[s]);

        for (int i=0; i<points_fv_indices.size(); i++){
            if (points_fv_indices[i] >  mask[s]){
                points_fv_indices[i] = points_fv_indices[i]-1;
            }
        }



        s--;
        if (s <0){
            break;
        }
    }






    pointsAttr.Set(points, now);
    pointsFVAttr.Set(points_fv_count, now);
    pointsFVIAttr.Set(points_fv_indices, now);

    return error();

}