// Disable TBB warnings about atomics
#define __TBB_show_deprecation_message_atomic_H
#define __TBB_show_deprecation_message_task_H

#include "LOP_Grid.h"
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


using namespace HOU_HDK;
PXR_NAMESPACE_USING_DIRECTIVE



void newLopOperator(OP_OperatorTable *table){
    table->addOperator(new OP_Operator(
        "Grid",
        "Grid",
        LOP_Grid::myConstructor,
        LOP_Grid::myTemplateList,
        (unsigned)0,
        (unsigned)1));
}


// Parms
static PRM_Name primpathName("primpath", "Prim Path");

static PRM_Name     orientaionName("orientaion", "Orientation");
static PRM_Default  orienationParamDefault = PRM_Default(30.0);

static PRM_Name     sizeParmName("size", "Size");
static PRM_Default  sizeParamDefault[] = {PRM_Default(5.0), PRM_Default(5.0)};
static PRM_Range    sizeParamRange(PRM_RANGE_RESTRICTED, 0, PRM_RANGE_RESTRICTED, 100);

static PRM_Name     centerParmName("center", "Center");
static PRM_Default  centerParamDefault[] = {PRM_Default(0.0), PRM_Default(0.0), PRM_Default(0.0)};
static PRM_Range    centerParamRange(PRM_RANGE_RESTRICTED, 0, PRM_RANGE_RESTRICTED, 10);

static PRM_Name     rotateParmName("rotate", "Roate");
static PRM_Default  rotateNameParamDefault[] = {PRM_Default(0.0), PRM_Default(0.0), PRM_Default(0.0)};
static PRM_Range    rotateParamRange(PRM_RANGE_RESTRICTED, 0, PRM_RANGE_RESTRICTED, 360);

static PRM_Name     rowParmName("row", "Row");
static PRM_Default  rowParamDefault = PRM_Default(5);
static PRM_Range    rowParamRange(PRM_RANGE_RESTRICTED, 0, PRM_RANGE_RESTRICTED, 10);

static PRM_Name     columnParmName("column", "Column");
static PRM_Default  columnParamDefault = PRM_Default(5);
static PRM_Range    columnParamRange(PRM_RANGE_RESTRICTED, 0, PRM_RANGE_RESTRICTED, 10);



// PRM_Template(int type, int vector_size, const PRM_Name *name, const PRM_Default *def, unsigned int export_flag = 0, const PRM_SpareData *spare = 0, const PRM_Callback *cb = 0);
PRM_Template
LOP_Grid::myTemplateList[] = {
    PRM_Template(PRM_STRING,       1, &lopPrimPathName, &lopAddPrimPathDefault, &lopPrimPathMenu,0, 0, &lopPrimPathSpareData),
    // PRM_Template(PRM_FLT,       1, &orientaionName,  &orienationParamDefault ),
    PRM_Template(PRM_XYZ_J,        2, &sizeParmName,    sizeParamDefault,       0,  &sizeParamRange   ),
    PRM_Template(PRM_XYZ_J,        3, &centerParmName,  centerParamDefault,     0,  &centerParamRange ),
    PRM_Template(PRM_XYZ_J,        3, &rotateParmName,  rotateNameParamDefault, 0,  &rotateParamRange ),
    PRM_Template(PRM_INT,          1, &rowParmName,     &rowParamDefault,       0,  &rowParamRange    ),
    PRM_Template(PRM_INT,          1, &columnParmName,  &columnParamDefault,    0,  &columnParamRange ),
    PRM_Template(),
};

OP_Node* LOP_Grid::myConstructor(OP_Network *net, const char *name, OP_Operator *op){
    return new LOP_Grid(net, name, op);
}

LOP_Grid::LOP_Grid(OP_Network *net, const char *name, OP_Operator *op): LOP_Node(net, name, op){
}

LOP_Grid::~LOP_Grid(){
}

void LOP_Grid::PRIMPATH(UT_String &str, fpreal t){
    evalString(str, lopPrimPathName.getToken(), 0, t);
    HUSDmakeValidUsdPath(str, true);
}

OP_ERROR LOP_Grid::cookMyLop(OP_Context &context){
    // Cook the node connected to our input, and make a "soft copy" of the
    // result into our own HUSD_DataHandle.
    if (cookModifyInput(context) >= UT_ERROR_FATAL)
	return error();

    float now = context.getTime();

    UT_String primpath;
    fpreal points;



    PRIMPATH(primpath, now);

    // Use editableDataHandle to get non-const access to our data handle, and
    // the lock it for writing. This makes sure that the USD stage is set up
    // to match the configuration defined in our data handle. Any edits made
    // to the stage at this point will be preserved in our data handle when
    // we unlock it.
    HUSD_AutoWriteLock writelock(editableDataHandle());
    HUSD_AutoLayerLock layerlock(writelock);

    // Create a helper class for creating USD primitives on the stage.
    HUSD_CreatePrims creator(layerlock);

    // Use the helper class to create a new "Sphere" primitive, with a
    // specifier type of "def" to define this primitive even if it doesn't
    // exist on the stage yet.
    //
    // The "empty string" parameter to createPrim tells it to not author a
    // "kind" setting for this primiitve.
    // if (!creator.createPrim(primpath, "UsdGeomSphere",
	//     UT_StringHolder::theEmptyString,
	//     HUSD_Constants::getPrimSpecifierDefine(),
	//     HUSD_Constants::getXformPrimType()))
    // {
	// addError(LOP_PRIM_NOT_CREATED, primpath);
    //     return error();
    // }

    

    // Use direct editing of the USD stage to modify the radius attribute
    // of the new sphere primitive. We could use the HUSD_SetAttributes
    // helper class here, but this demonstrates that we are allowed to
    // access the stage directly. The edit target will already be set to
    // the "active layer", and as long as we leave it there, any changes
    // we make will be preserved by our data handle.
    // UsdStageRefPtr stage = writelock.data()->stage();
    // SdfPath sdfpath(HUSDgetSdfPath(primpath));
    // // UsdPrim prim = stage->GetPrimAtPath(sdfpath);

    // UsdGeomMesh geom = UsdGeomMesh::Define(stage, sdfpath);

    // if (prim)
	// prim.GetAttribute(UsdGeomTokens->radius).Set(points);

    // setLastModifiedPrims(primpath);

    return error();
}

