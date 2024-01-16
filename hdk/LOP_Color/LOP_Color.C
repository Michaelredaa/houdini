// Disable TBB warnings about atomics
#define __TBB_show_deprecation_message_atomic_H
#define __TBB_show_deprecation_message_task_H

#include <cmath>

#include "LOP_Color.h"
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
#include <PRM/PRM_RampUtils.h>
#include <PRM/PRM_Conditional.h>
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
        "lop_Color",               // Internal name
        "Color",                   // UI name
        LOP_Color::myConstructor,  // How to build the node
        LOP_Color::myTemplateList, // parameters
        0,                        // Min # of sources
        1,                        // Max # of sources
        nullptr,                  // Custom local variables (none)
        OP_FLAG_GENERATOR         // Flag it as generator
        );
        op->setIconName("LOP_Color.png");
    table->addOperator(op);            
}



// Parms
static PRM_Name primpathName("primpath", "Prim Path");

static PRM_Name     interpolationParmName("interpolation", "Interpolation");
static PRM_Name     interpolationTypes[] =
{
    PRM_Name("constant", "Constant"),
    PRM_Name("uniform", "Uniform"),
    PRM_Name("vertex", "Vertex"),
    PRM_Name("facevaring", "Face Variaing"),
    PRM_Name(0)
};
static PRM_ChoiceList   interpolationTypesMenu(PRM_CHOICELIST_SINGLE, interpolationTypes);

static PRM_Name     attributeParmName("attribute", "Attribute");
static PRM_Default attributeParmDefault(0.0f, "points");

static PRM_Name     chsParmName("channels", "Channels");
static PRM_Name     chxParmName("chx", "X");
static PRM_Default  chxParmDefault = PRM_Default(1);

static PRM_Name     chyParmName("chy", "Y");
static PRM_Default  chyParmDefault = PRM_Default(1);

static PRM_Name     chzParmName("chz", "Z");
static PRM_Default  chzParmDefault = PRM_Default(1);

static PRM_Name     normalizeParmName("normalize", "Normalize");
static PRM_Default  normalizeParmDefault = PRM_Default(true);

static PRM_Name     timeParmName("time", "Time");
static PRM_Default  timeParmDefault = PRM_Default(1.0);
static PRM_Range    timeParmRange(PRM_RANGE_UI , 1.0, PRM_RANGE_UI , 10.0);

static PRM_Name     colorTypeParmName("colortype", "Color Type");
static PRM_Name     colorTypeTypes[] =
{
    PRM_Name("constant", "Constant"),
    PRM_Name("ramp", "Ramp From Attribute"),
    PRM_Name(0)
};
static PRM_ChoiceList   colorTypeTypesMenu(PRM_CHOICELIST_SINGLE, colorTypeTypes);

static PRM_Name          colorParmName("color", "Color");
static PRM_Default       colorParmDefault[] = {PRM_Default(1.0), PRM_Default(1.0), PRM_Default(1.0)};
static PRM_Conditional   colorParmCondition("{ colortype != constant }", PRM_CONDTYPE_HIDE);

static PRM_Name          rampParmName("ramp", "Color Ramp");
static PRM_ConditionalGroup rampParmCondition;
static PRM_Conditional   rampParmConditionHide("{ colortype != ramp }", PRM_CONDTYPE_HIDE);
static PRM_Conditional   rampParmConditionDisable("{ normalize == 0 }", PRM_CONDTYPE_DISABLE);



void initializeConditions(void) {
    rampParmCondition.addConditional(rampParmConditionHide);
    rampParmCondition.addConditional(rampParmConditionDisable);
}



VtArray<GfVec3f> normalize_array_all_channels(const VtArray<GfVec3f>& arr) {
    if (arr.empty()) {
        return VtArray<GfVec3f>();
    }

    // Get min and max values
    GfVec3f range = arr[0];
    for (const GfVec3f& vec : arr) {
        for (int a = 0; a < 3; ++a) {
            range[0] = min(range[0], vec[a]);
            range[1] = max(range[1], vec[a]);
        }

    }

    // Normalize the array
    VtArray<GfVec3f> normalized_arr;
    normalized_arr.reserve(arr.size());

    for (const GfVec3f& vec : arr) {
        GfVec3f normalize_vector;
        for (int a = 0; a < 3; ++a) {
            normalize_vector[a] = (range[0] - range[1] != 0) ? (vec[a] - range[0]) / (range[1] - range[0]) : 1.0;
        }
        normalized_arr.push_back(normalize_vector);
    }

    return normalized_arr;
}

VtArray<GfVec3f> normalize_array(const VtArray<GfVec3f>& arr) {
    if (arr.empty()) {
        return VtArray<GfVec3f>();
    }

    // Get min and max values
    GfVec3f min_range = arr[0];
    GfVec3f max_range = arr[0];

    for (const GfVec3f& vec : arr) {
        for (int a = 0; a < 3; ++a) {
            min_range[a] = min(min_range[a], vec[a]);
            max_range[a] = max(max_range[a], vec[a]);
        }
    }
    // Normalize the array
    VtArray<GfVec3f> normalized_arr;
    normalized_arr.reserve(arr.size());

    for (const GfVec3f& vec : arr) {
        GfVec3f normalize_vector;
        for (int a = 0; a < 3; ++a) {
            normalize_vector[a] = (max_range[a] - min_range[a] != 0) ? (vec[a] - min_range[a]) / (max_range[a] - min_range[a]) : 0.0;
        }
        normalized_arr.push_back(normalize_vector);
    }

    return normalized_arr;
}


PRM_Template LOP_Color::myTemplateList[] = {
    PRM_Template(PRM_STRING,             1, &lopPrimPathName,        &lopEditPrimPathDefault, &lopPrimPathMenu, 0, 0, &lopPrimPathSpareData),
    PRM_Template(PRM_ORD,                1, &interpolationParmName,                                             0,  &interpolationTypesMenu),
    PRM_Template(PRM_STRING,             1, &attributeParmName,      &attributeParmDefault),
    PRM_Template(PRM_FLT,                1, &timeParmName,           &timeParmDefault,                          0,  &timeParmRange),
    PRM_Template(PRM_ORD,                1, &colorTypeParmName,                                                 0,  &colorTypeTypesMenu),


    PRM_Template(PRM_TOGGLE,             1, &normalizeParmName,      &normalizeParmDefault),
    PRM_Template(PRM_LABEL  | PRM_TYPE_JOIN_NEXT,             1, &chsParmName),
    PRM_Template(PRM_TOGGLE | PRM_TYPE_JOIN_NEXT,             1, &chxParmName, &chxParmDefault),
    PRM_Template(PRM_TOGGLE | PRM_TYPE_JOIN_NEXT,             1, &chyParmName, &chyParmDefault),
    PRM_Template(PRM_TOGGLE                     ,             1, &chzParmName, &chzParmDefault),

    PRM_Template(PRM_RGB,                3, &colorParmName,          colorParmDefault, 0, 0, 0, 0, 1, 0, &colorParmCondition),
    PRM_Template(PRM_MULTITYPE_RAMP_RGB, NULL, 1, &rampParmName,     PRMtwoDefaults,   0, 0, 0, &rampParmConditionHide),
    PRM_Template(),
};

OP_Node* LOP_Color::myConstructor(OP_Network *net, const char *name, OP_Operator *op){
    return new LOP_Color(net, name, op);
}

LOP_Color::LOP_Color(OP_Network *net, const char *name, OP_Operator *op): LOP_Node(net, name, op){
}

LOP_Color::~LOP_Color(){}

void LOP_Color::PRIMPATH(UT_String &str, fpreal t){
    evalString(str, lopPrimPathName.getToken(), 0, t);
    HUSDmakeValidUsdPath(str, true);
}

string LOP_Color::evalOrdAsString(const PRM_Name& parmName, fpreal t){
    UT_String name;
    evalString(name, parmName.getToken(), 0, t);
    return name.toStdString();
}

UT_Vector3 LOP_Color::getXYZParameterValue(const PRM_Name& parmName, fpreal t)
{
    UT_Vector3 value(0.0, 0.0, 0.0);

    for (int i = 0; i < 3; ++i)
    {
        fpreal componentValue = evalFloat(parmName.getToken(), i, t);
        value[i] = componentValue;
    }

    return value;
}


OP_ERROR LOP_Color::cookMyLop(OP_Context &context){
    if (cookModifyInput(context) >= UT_ERROR_FATAL){return error();}

    fpreal now = context.getTime();


    UT_String primpath;
    PRIMPATH(primpath, now);

    string attributename = evalOrdAsString(attributeParmName, now);
    string colortype = evalOrdAsString(colorTypeParmName, now);
    string interpolation = evalOrdAsString(interpolationParmName, now);
    UT_Vector3 constantcolor = getXYZParameterValue(colorParmName, now);

    int chx = bool(evalInt(chxParmName.getToken(), 0, now));
    int chy = bool(evalInt(chyParmName.getToken(), 0, now));
    int chz = bool(evalInt(chzParmName.getToken(), 0, now));
    int normalize = evalInt(normalizeParmName.getToken(), 0, now);

    fpreal time = evalFloat(timeParmName.getToken(), 0, now);

    UT_Ramp ramp;
    updateRampFromMultiParm(now, getParm(string(rampParmName.getToken())), ramp);


    VtArray<fpreal> positions;
    VtArray<GfVec3f> colors;
    for (int i=0; i< ramp.getNodeCount(); i++){
        const UT_ColorNode *colornode = ramp.getNode(i);
        fpreal pos = colornode->t;
        GfVec3f color = GfVec3f(colornode->rgba.r, colornode->rgba.g, colornode->rgba.b);
        positions.push_back(pos);
        colors.push_back(color);

    }

    // USD
    HUSD_AutoWriteLock writelock(editableDataHandle());
    HUSD_AutoLayerLock layerlock(writelock);

    UsdStageRefPtr stage = writelock.data()->stage();
    UsdPrim prim = stage->GetPrimAtPath(SdfPath(HUSDgetSdfPath(primpath)));

    if (! prim.IsValid()){
        addError(OP_ERR_ANYTHING, "Prim Not Found");
        return error();
    }


    UsdGeomMesh mesh(prim);
    UsdAttribute pointsAttr = mesh.GetPointsAttr();
    UsdAttribute pointsFVAttr = mesh.GetFaceVertexCountsAttr();
    UsdAttribute pointsFVIAttr = mesh.GetFaceVertexIndicesAttr();
    UsdAttribute displayColorAttr = prim.GetAttribute(TfToken("primvars:displayColor"));

    VtArray<GfVec3f> display_color;
    VtArray<GfVec3f> points;
    VtArray<int> points_fv_count;
    VtArray<int> points_fv_indices;

    pointsAttr.Get(&points, now);
    pointsFVAttr.Get(&points_fv_count, now);
    pointsFVIAttr.Get(&points_fv_indices, now);


    GfVec3f colorvector = GfVec3f(constantcolor.x(), constantcolor.y(), constantcolor.z());

    if (colortype == "constant"){
        if (interpolation == "constant" ){
            VtArray<GfVec3f> colorarray(1, colorvector);
            displayColorAttr.SetMetadata(TfToken("interpolation"), VtValue("constant"));
            displayColorAttr.Set(colorarray);

        }else if (interpolation == "uniform" ){
            VtArray<GfVec3f> colorarray(points_fv_count.size(), colorvector);
            displayColorAttr.SetMetadata(TfToken("interpolation"), VtValue("uniform"));
            displayColorAttr.Set(colorarray);

        }else if (interpolation == "facevaring" ){
            VtArray<GfVec3f> colorarray(points_fv_indices.size(), colorvector);
            displayColorAttr.SetMetadata(TfToken("interpolation"), VtValue("facevaring"));
            displayColorAttr.Set(colorarray);

        }else if (interpolation == "vertex" ){
            VtArray<GfVec3f> colorarray(points.size(), colorvector);
            displayColorAttr.SetMetadata(TfToken("interpolation"), VtValue("vertex"));
            displayColorAttr.Set(colorarray);
        }


    }else if (colortype == "ramp"){
        UsdAttribute attr = prim.GetAttribute(TfToken(attributename));

        if (! attr.HasValue()){
            return error();
        }
        VtValue interpolation;
        if (! attr.GetMetadata(TfToken("interpolation"), &interpolation)){
            interpolation = VtValue("vertex");
        }
        displayColorAttr.SetMetadata(TfToken("interpolation"), interpolation);

        SdfValueTypeName attrType = attr.GetTypeName();
        SdfTupleDimensions attrsize = attrType.GetDimensions();

        bool usdramp = false;
        VtValue value;
        VtArray<GfVec3f> attrValue;
        VtArray<GfVec3f> attrValueNormalized;
        if (attr.Get(&value)){
            if (value.IsHolding<VtArray<GfVec3f>>()){
                int c = -1;
                if (chx && !chy && !chz && c==-1){ // single value
                    c = 0;
                }else if (chy && !chx && !chz){
                    c = 1;
                } else if (chz && !chy && !chx){
                    c = 2;
                }

                for (const GfVec3f val: value.UncheckedGet<VtArray<GfVec3f>>()){
                    if (chx && chy && chz){
                        attrValue.push_back(GfVec3f(val[0], val[1], val[2]));

                    }else if (chx && chy && !chz){
                        attrValue.push_back(GfVec3f(val[0], val[1], 0.0));

                    }else if (chx && !chy && chz){
                        attrValue.push_back(GfVec3f(val[0], 0.0, val[2]));
                    }else if (!chx && chy && chz){
                        attrValue.push_back(GfVec3f(0.0, val[1], val[2]));
                    }
                    else{
                        attrValue.push_back(GfVec3f(val[c], val[c], val[c]));
                        usdramp = true;
                    }
                }


            }else if (value.IsHolding<VtArray<GfVec2f>>()) {
                for (const GfVec2f& vec: value.UncheckedGet<VtArray<GfVec2f>>()){
                    attrValue.push_back(GfVec3f(vec[0], vec[1], 0.0));
                    }

            }else if (value.IsHolding<VtArray<float>>()) {
                for (const float val: value.UncheckedGet<VtArray<float>>()){
                    attrValue.push_back(GfVec3f(val, val, val));
                    usdramp = true;
                }

            }else if (value.IsHolding<VtArray<int>>()) {
                for (const int val: value.UncheckedGet<VtArray<int>>()){
                        float casted_val = static_cast<float>(val);
                        attrValue.push_back(GfVec3f(casted_val, casted_val, casted_val));
                        usdramp = true;
            }
        }else{
            addError(OP_ERR_ANYTHING, "Bad Attribue");
            return error();
            }

        attrValueNormalized = normalize_array(attrValue);

        VtArray<GfVec3f>  attrValueColor;

        if (positions.size() > 1){
            // Remap input colors to positions
            for (int i = 0; i < attrValueNormalized.size(); i++) {
                // Find the two positions that surround the current input color
                
                double value = attrValueNormalized[i][1];
                size_t p;
                for (p = 0; p < positions.size() - 1; p++) {
                    if (value < positions[p + 1]) {
                        break;
                    }
                }

                // Perform linear interpolation
                double t = (attrValueNormalized[i][1] - positions[p]) / (positions[p + 1] - positions[p]);

                GfVec3f remappedColor;
                for (int a = 0; a < 3; ++a) {
                    remappedColor[a] = colors[p][a] + t*(colors[p + 1][a] - colors[p][a]);
                    }
                attrValueColor.push_back(remappedColor);

            }
        }

        if (normalize){
            if (usdramp){
                displayColorAttr.Set(attrValueColor);
            }else{
                displayColorAttr.Set(attrValueNormalized);
            }
            
        }else{
            displayColorAttr.Set(attrValue);
        }
        }

    }

    return error();

}