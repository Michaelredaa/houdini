// Disable TBB warnings about atomics
#define __TBB_show_deprecation_message_atomic_H
#define __TBB_show_deprecation_message_task_H

#include <cmath>

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
#include <UT/UT_Vector3.h>
#include <pxr/usd/usd/stage.h>
#include <pxr/usd/usd/attribute.h>
#include <pxr/usd/usdGeom/tokens.h>
#include <pxr/usd/usdGeom/mesh.h>
#include <pxr/usd/usdGeom/points.h>
#include <pxr/usd/usdGeom/bboxCache.h>
#include <pxr/usd/usdGeom/xformCache.h>
#include <pxr/usd/usdGeom/primvarsAPI.h>
#include <pxr/base/gf/bbox3d.h>


using namespace HOU_HDK;
using namespace std;
PXR_NAMESPACE_USING_DIRECTIVE


void newLopOperator(OP_OperatorTable *table){

    OP_Operator *op = new OP_Operator(
        "lop_grid",               // Internal name
        "grid",                   // UI name
        LOP_Grid::myConstructor,  // How to build the node
        LOP_Grid::myTemplateList, // parameters
        0,                        // Min # of sources
        1,                        // Max # of sources
        nullptr,                  // Custom local variables (none)
        OP_FLAG_GENERATOR         // Flag it as generator
        );
        op->setIconName("LOP_Grid.png");
    table->addOperator(op);            
}


GfVec3f rotatePoints(GfVec3f point, UT_Vector3 angle){

    fpreal s, c, rad;
    fpreal x, y, z;
    fpreal rx, ry, rz;

    x = rx = point[0];
    y = ry = point[1];
    z = rz = point[2];



    // @X
    if (angle.x() != 0){
        rad = M_PI / 180.0 * angle.x();

        s = sin(rad);
        c = cos(rad);

        rz = z*c - y*s;
        ry = z*s + y*c;
        z=rz; y=ry;

    }

    // @Y
    if (angle.y() != 0){
        rad = M_PI / 180.0 * angle.y();

        s = sin(rad);
        c = cos(rad);

        rx = x*c + z*s;
        rz = -x*s + z*c;
        x=rx; z=rz; 
    }

    // @Z
    if (angle.z() != 0){
        rad = M_PI / 180.0 * angle.z();

        s = sin(rad);
        c = cos(rad);

        rx = x*c - y*s;
        ry = x*s + y*c;
        x=rx; y=ry; 
    }
    return GfVec3f(rx, ry, rz);
}


// Parms
static PRM_Name primpathName("primpath", "Prim Path");

static PRM_Name     orientaionParmName("orientaion", "Orientation");
static PRM_Name orienationStrChoices[] =
{
    PRM_Name("zx", "ZX Plan"),
    PRM_Name("xy", "XY Plan"),
    PRM_Name("yz", "YZ Plan"),
    PRM_Name(0)
};

static PRM_ChoiceList   orienationStringMenu(PRM_CHOICELIST_SINGLE, orienationStrChoices);
static PRM_Name     sizeParmName("size", "Size");
static PRM_Default  sizeParamDefault[] = {PRM_Default(5.0), PRM_Default(5.0)};
static PRM_Range    sizeParamRange(PRM_RANGE_UI, 0, PRM_RANGE_UI, 100);

static PRM_Name     centerParmName("center", "Center");
static PRM_Default  centerParamDefault[] = {PRM_Default(0.0), PRM_Default(0.0), PRM_Default(0.0)};
static PRM_Range    centerParamRange(PRM_RANGE_UI, 0, PRM_RANGE_UI, 10);

static PRM_Name     rotateParmName("rotate", "Roate");
static PRM_Default  rotateNameParamDefault[] = {PRM_Default(0.0), PRM_Default(0.0), PRM_Default(0.0)};
static PRM_Range    rotateParamRange(PRM_RANGE_UI, 0, PRM_RANGE_UI, 360);

static PRM_Name     rowParmName("row", "Row");
static PRM_Default  rowParamDefault = PRM_Default(5);
static PRM_Range    rowParamRange(PRM_RANGE_UI , 0, PRM_RANGE_UI , 10);

static PRM_Name     columnParmName("column", "Column");
static PRM_Default  columnParamDefault = PRM_Default(5);
static PRM_Range    columnParamRange(PRM_RANGE_UI, 0, PRM_RANGE_UI, 10);



// PRM_Template(int type, int vector_size, const PRM_Name *name, const PRM_Default *def, unsigned int export_flag = 0, const PRM_SpareData *spare = 0, const PRM_Callback *cb = 0);
PRM_Template
LOP_Grid::myTemplateList[] = {
    PRM_Template(PRM_STRING,       1, &lopPrimPathName, &lopAddPrimPathDefault, &lopPrimPathMenu,0, 0, &lopPrimPathSpareData),
    PRM_Template(PRM_ORD,          1, &orientaionParmName,                          0,  &orienationStringMenu),
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


UT_Vector3 LOP_Grid::getXYZParameterValue(const PRM_Name& parmName, fpreal t)
{
    UT_Vector3 value(0.0, 0.0, 0.0);

    for (int i = 0; i < 3; ++i)
    {
        fpreal componentValue = evalFloat(parmName.getToken(), i, t);
        value[i] = componentValue;
    }

    return value;
}


string LOP_Grid::evalOrdAsString(const PRM_Name& parmName, fpreal t){
    UT_String name;
    evalString(name, parmName.getToken(), 0, t);
    return name.toStdString();
}


int createUV(UsdGeomMesh mesh){

    UsdPrim prim = mesh.GetPrim();
    UsdAttribute pointAttr = mesh.GetPointsAttr();
    UsdAttribute FCAttr = mesh.GetFaceVertexCountsAttr();
    UsdAttribute FVIAttr = mesh.GetFaceVertexIndicesAttr();

    VtArray<GfVec3f> points;
    VtIntArray face_count;
    VtIntArray fv_indices;

    pointAttr.Get(&points);
    FCAttr.Get(&face_count);
    FVIAttr.Get(&fv_indices);

    TfTokenVector includePurpose{UsdGeomTokens->default_};
    UsdGeomBBoxCache bboxCache(UsdTimeCode::Default(), includePurpose);
    GfBBox3d worldBBox = bboxCache.ComputeWorldBound(prim);

    double min_x = worldBBox.GetRange().GetMin()[0];
    double min_z = worldBBox.GetRange().GetMin()[2];

    double size_x = worldBBox.GetRange().GetSize()[0];
    double size_z = worldBBox.GetRange().GetSize()[2];


    int fv=0;
    VtArray<GfVec2f>  st_array;
    for (int fc=0; fc<face_count.size(); fc++){
        int end_idx = fv + face_count[fc];
        for (int i=fv; i<end_idx; i++){
            int idx = fv_indices[i];
            st_array.emplace_back(GfVec2f((points[idx][0]- min_x)/size_x, (points[idx][2]- min_z)/size_z));
        }
        fv = end_idx;
    }


    UsdGeomPrimvar stPrimvar =  UsdGeomPrimvarsAPI(mesh).CreatePrimvar(TfToken("st"), SdfValueTypeNames->TexCoord2fArray, UsdGeomTokens->faceVarying);
    stPrimvar.Set(st_array);
    // stPrimvar.SetIndices(fv_indices);

    return 0;
}




OP_ERROR LOP_Grid::cookMyLop(OP_Context &context){
    // Cook the node connected to our input, and make a "soft copy" of the
    // result into our own HUSD_DataHandle.
    if (cookModifyInput(context) >= UT_ERROR_FATAL)
	return error();

    fpreal now = context.getTime();

    UT_String primpath;
    int rows = evalInt(rowParmName.getToken(), 0, now);
    int columns = evalInt(columnParmName.getToken(), 0, now);
    UT_Vector3 grid_size = getXYZParameterValue(sizeParmName, now);
    UT_Vector3 grid_center = getXYZParameterValue(centerParmName, now);
    UT_Vector3 rot_angle = getXYZParameterValue(rotateParmName, now);

    string orientation = evalOrdAsString(orientaionParmName, now);


    if(rows < 2){rows = 2;};
    if(columns < 2){columns = 2;};

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


    // Use direct editing of the USD stage to modify the radius attribute
    // of the new sphere primitive. We could use the HUSD_SetAttributes
    // helper class here, but this demonstrates that we are allowed to
    // access the stage directly. The edit target will already be set to
    // the "active layer", and as long as we leave it there, any changes
    // we make will be preserved by our data handle.
    UsdStageRefPtr stage = writelock.data()->stage();
    SdfPath sdfpath(HUSDgetSdfPath(primpath));

    

    UsdGeomMesh mesh = UsdGeomMesh::Define(stage, sdfpath);
    
    VtArray<GfVec3f> points = {};
    for (int r=0; r<rows; r++){
        for (int c=0; c<columns; c++){

            fpreal x = (static_cast<fpreal>(r) / (rows-1) - 0.5) * grid_size.x();
            fpreal y = 0;
            fpreal z = (static_cast<fpreal>(c)  / (columns-1) - 0.5) * grid_size.y();

            GfVec3f point = GfVec3f(x, y, z);

            if (orientation == "xy"){
                point = GfVec3f(y, z, x);
            }
            if (orientation == "yz"){
                point = GfVec3f(z, x, y);
            }
            point = rotatePoints(point, rot_angle);

            point += GfVec3f(grid_center.data());
            points.emplace_back(point);
        }
        
    }

    // Set vertix count 
    VtArray<int> face_count;
    VtArray<int> fv_indices;
    for (int r=0; r<rows-1; r++){
        for (int c=0; c<columns-1; c++){
            face_count.emplace_back(4);

            int idx = r*columns + c;

            fv_indices.emplace_back(idx);
            fv_indices.emplace_back(idx + 1);
            fv_indices.emplace_back(idx + 1 + columns);
            fv_indices.emplace_back(idx + columns);

        }
    }
    mesh.CreateFaceVertexCountsAttr().Set(face_count);
    mesh.CreateFaceVertexIndicesAttr().Set(fv_indices);
    mesh.CreatePointsAttr().Set(points);

    createUV(mesh);

    setLastModifiedPrims(primpath);

    return error();
}
